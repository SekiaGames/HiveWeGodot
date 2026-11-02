#include "hive_c.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

#include "globals.hpp"
#include "hive_bridge.hpp"
#include "map.hpp"
#include "mpq.hpp"

namespace fs = std::filesystem;

namespace {

struct HiveHandle {
	Map* map = nullptr;
	fs::path unpack_dir;
};

bool data_source_ready = false;

void set_error(char* error, const int32_t size, const std::string& message) {
	if (error == nullptr || size <= 0) {
		return;
	}
	const std::size_t length = std::min<std::size_t>(message.size(), static_cast<std::size_t>(size) - 1);
	std::memcpy(error, message.data(), length);
	error[length] = '\0';
}

bool prepare_data_source(const fs::path& data_dir, std::string& error) {
	if (data_source_ready) {
		return true;
	}

	if (fs::exists(data_dir / "WAR3.MPQ")) {
		if (!hierarchy.game_data.open(data_dir / "WAR3.MPQ")) {
			error = "打不开 " + (data_dir / "WAR3.MPQ").string();
			return false;
		}
		for (const char* patch : {"War3x.mpq", "War3xlocal.mpq", "War3Patch.mpq", "Patch127.mpq"}) {
			hierarchy.game_data.add_patch(data_dir / patch);
		}
	}

	hierarchy.root_directory = data_dir;
	hierarchy.remastered = false;
	hierarchy.allow_local_files = true;

	thread_pool.init();

	try {
		world_edit_strings.load("UI/WorldEditStrings.txt");
		world_edit_game_strings.load("UI/WorldEditGameStrings.txt");
		world_edit_data.load("UI/WorldEditData.txt");
		world_edit_data.substitute(world_edit_game_strings, "WorldEditStrings");
		world_edit_data.substitute(world_edit_strings, "WorldEditStrings");
	} catch (const std::exception& e) {
		error = std::string("读游戏字符串失败：") + e.what();
		return false;
	}

	data_source_ready = true;
	return true;
}

bool unpack_map(const fs::path& map_path, fs::path& unpack_dir, std::string& error) {
	mpq::MPQ archive;
	if (!archive.open(map_path)) {
		error = "打不开地图归档：" + map_path.string();
		return false;
	}

	unpack_dir = fs::temp_directory_path() / ("hivewe_" + map_path.stem().string());
	std::error_code ec;
	fs::remove_all(unpack_dir, ec);

	if (!archive.unpack(unpack_dir)) {
		error = "解包失败：" + map_path.string();
		return false;
	}
	return true;
}

}

extern "C" {

void* hive_open(const char* map_path, const char* data_dir, char* error, const int32_t error_size) {
	if (map_path == nullptr || data_dir == nullptr) {
		set_error(error, error_size, "参数为空");
		return nullptr;
	}

	std::string message;
	if (!prepare_data_source(data_dir, message)) {
		set_error(error, error_size, message);
		return nullptr;
	}

	auto handle = std::make_unique<HiveHandle>();
	if (!unpack_map(map_path, handle->unpack_dir, message)) {
		set_error(error, error_size, message);
		return nullptr;
	}

	handle->map = new Map();
	try {
		handle->map->load(handle->unpack_dir);
	} catch (const std::exception& e) {
		set_error(error, error_size, std::string("加载中断：") + e.what());
	}

	return handle.release();
}

void hive_close(void* handle) {
	if (handle == nullptr) {
		return;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	delete hive->map;
	delete hive;
}

int32_t hive_save(void* handle, const char* target_dir, char* error, const int32_t error_size) {
	if (handle == nullptr || target_dir == nullptr) {
		set_error(error, error_size, "参数为空");
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	try {
		if (!hive->map->save(target_dir)) {
			set_error(error, error_size, "保存失败");
			return -1;
		}
	} catch (const std::exception& e) {
		set_error(error, error_size, e.what());
		return -1;
	}
	return 0;
}

int32_t hive_map_info(void* handle, HiveMapInfo* out) {
	if (handle == nullptr || out == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const Terrain& terrain = hive->map->terrain;

	out->width = terrain.width;
	out->height = terrain.height;
	out->tiles_x = terrain.width - 1;
	out->tiles_y = terrain.height - 1;
	out->texture_count = static_cast<int32_t>(HiveBridge::ground_textures(terrain).size());
	out->doodad_count = static_cast<int32_t>(hive->map->doodads.doodads.size());
	out->unit_count = static_cast<int32_t>(hive->map->units.units.size());
	out->region_count = static_cast<int32_t>(hive->map->regions.regions.size());
	out->tileset_id = terrain.tileset_id;
	return 0;
}

int32_t hive_copy_heights(void* handle, float* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = HiveBridge::heights(static_cast<HiveHandle*>(handle)->map->terrain);
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, source.data(), count * sizeof(float));
	return static_cast<int32_t>(count);
}

int32_t hive_copy_tiles(void* handle, HiveTile* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = HiveBridge::tiles(static_cast<HiveHandle*>(handle)->map->terrain);
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	for (std::size_t i = 0; i < count; ++i) {
		out[i].texture[0] = source[i].x;
		out[i].texture[1] = source[i].y;
		out[i].texture[2] = source[i].z;
		out[i].texture[3] = source[i].w;
		out[i].variation = source[i].x >> 16;
	}
	return static_cast<int32_t>(count);
}

int32_t hive_copy_ground_exists(void* handle, uint8_t* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = HiveBridge::ground_exists(static_cast<HiveHandle*>(handle)->map->terrain);
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, source.data(), count);
	return static_cast<int32_t>(count);
}

int32_t hive_copy_water_exists(void* handle, uint8_t* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = HiveBridge::water_exists(static_cast<HiveHandle*>(handle)->map->terrain);
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, source.data(), count);
	return static_cast<int32_t>(count);
}

const char* hive_texture_id(void* handle, const int32_t index) {
	if (handle == nullptr) {
		return nullptr;
	}
	const auto& ids = HiveBridge::tileset_ids(static_cast<HiveHandle*>(handle)->map->terrain);
	if (index < 0) {
		return nullptr;
	}
	if (static_cast<std::size_t>(index) == ids.size()) {
		return "blight";
	}
	if (static_cast<std::size_t>(index) > ids.size()) {
		return nullptr;
	}
	return ids[static_cast<std::size_t>(index)].c_str();
}

}
