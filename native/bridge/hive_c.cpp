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
#include "terrain_brush.h"

namespace fs = std::filesystem;

namespace {

struct HiveHandle {
	Map* map = nullptr;
	TerrainBrush* brush = nullptr;
	fs::path unpack_dir;
	std::string unpack_dir_string;
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

void copy_directory(const fs::path& from, const fs::path& to) {
	for (const auto& entry : fs::recursive_directory_iterator(from)) {
		if (!entry.is_regular_file()) {
			continue;
		}
		const fs::path target = to / fs::relative(entry.path(), from);
		fs::create_directories(target.parent_path());
		fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
	}
}

bool unpack_map(const fs::path& map_path, fs::path& unpack_dir, std::string& error) {	mpq::MPQ archive;
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
	if (fs::is_directory(map_path)) {
		handle->unpack_dir = map_path;
	} else if (!unpack_map(map_path, handle->unpack_dir, message)) {
		set_error(error, error_size, message);
		return nullptr;
	}

	handle->map = new Map();
	try {
		handle->map->load(handle->unpack_dir);
	} catch (const std::exception& e) {
		set_error(error, error_size, std::string("加载中断：") + e.what());
	}

	handle->brush = new TerrainBrush(
		handle->map->terrain, handle->map->units, handle->map->tilesets, handle->map->world_undo);
	handle->map->brush = handle->brush;

	return handle.release();
}

void hive_close(void* handle) {
	if (handle == nullptr) {
		return;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	delete hive->brush;
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
		fs::create_directories(target_dir);
		const fs::path source_dir = hive->map->filesystem_path;
		const fs::path target = fs::absolute(target_dir) / "";
		if (source_dir != target) {
			copy_directory(source_dir, target);
			hive->map->filesystem_path = target;
			hierarchy.map_directory = target;
			hive->unpack_dir = target;
		}
		if (!hive->map->save(target)) {
			set_error(error, error_size, "保存失败");
			return -1;
		}
	} catch (const std::exception& e) {
		set_error(error, error_size, e.what());
		return -1;
	}
	return 0;
}

int32_t hive_save_map(void* handle, const char* map_path, char* error, const int32_t error_size) {
	if (handle == nullptr || map_path == nullptr) {
		set_error(error, error_size, "参数为空");
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	const fs::path staging = fs::temp_directory_path() / ("hivewe_stage_" + std::to_string(reinterpret_cast<std::uintptr_t>(handle)));
	std::error_code ec;
	fs::remove_all(staging, ec);

	const fs::path original = hive->map->filesystem_path;

	try {
		fs::create_directories(staging);
		copy_directory(hive->unpack_dir, staging);

		hive->map->filesystem_path = staging / "";
		hierarchy.map_directory = staging;
		if (!hive->map->save(staging)) {
			set_error(error, error_size, "保存失败");
			return -1;
		}

		std::string message;
		if (!mpq::pack_directory(staging, map_path, message)) {
			set_error(error, error_size, message);
			return -1;
		}
	} catch (const std::exception& e) {
		set_error(error, error_size, e.what());
		return -1;
	}

	hive->map->filesystem_path = original;
	hierarchy.map_directory = original;
	fs::remove_all(staging, ec);
	return 0;
}

int32_t hive_map_info(void* handle, HiveMapInfo* out) {	if (handle == nullptr || out == nullptr) {
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

int32_t hive_copy_water_heights(void* handle, float* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = HiveBridge::water_heights(static_cast<HiveHandle*>(handle)->map->terrain);
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, source.data(), count * sizeof(float));
	return static_cast<int32_t>(count);
}

float hive_water_offset(void* handle) {
	if (handle == nullptr) {
		return 0.f;
	}
	return HiveBridge::water_offset(static_cast<HiveHandle*>(handle)->map->terrain);
}

int32_t hive_texture_info(void* handle, const int32_t index, int32_t* width, int32_t* height, int32_t* channels) {	if (handle == nullptr || index < 0) {
		return -1;
	}
	const auto& textures = HiveBridge::ground_textures(static_cast<HiveHandle*>(handle)->map->terrain);
	if (static_cast<std::size_t>(index) >= textures.size()) {
		return -1;
	}
	const auto& texture = *textures[static_cast<std::size_t>(index)];
	if (width != nullptr) {
		*width = texture.tile_size;
	}
	if (height != nullptr) {
		*height = texture.layer_count;
	}
	if (channels != nullptr) {
		*channels = 4;
	}
	return 0;
}

int32_t hive_copy_texture(void* handle, const int32_t index, uint8_t* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || index < 0) {
		return -1;
	}
	const auto& textures = HiveBridge::ground_textures(static_cast<HiveHandle*>(handle)->map->terrain);
	if (static_cast<std::size_t>(index) >= textures.size()) {
		return -1;
	}
	const auto& data = textures[static_cast<std::size_t>(index)]->stored_tiles;
	const auto count = std::min<std::size_t>(data.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, data.data(), count);
	return static_cast<int32_t>(count);
}

int32_t hive_minimap_info(void* handle, int32_t* width, int32_t* height) {
	if (handle == nullptr) {
		return -1;
	}
	const Texture texture = static_cast<HiveHandle*>(handle)->map->terrain.minimap_image();
	if (width != nullptr) {
		*width = texture.width;
	}
	if (height != nullptr) {
		*height = texture.height;
	}
	return 0;
}

int32_t hive_copy_minimap(void* handle, uint8_t* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const Texture texture = static_cast<HiveHandle*>(handle)->map->terrain.minimap_image();
	const auto count = std::min<std::size_t>(texture.data.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, texture.data.data(), count);
	return static_cast<int32_t>(count);
}

const char* hive_map_directory(void* handle) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->unpack_dir_string = hive->unpack_dir.string();
	return hive->unpack_dir_string.c_str();
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

void hive_brush_move(void* handle, const float x, const float y) {
	if (handle == nullptr) {
		return;
	}
	input_handler.mouse_world = glm::vec3(x, y, 0.f);
}

void hive_brush_size(void* handle, const int32_t size) {
	if (handle == nullptr) {
		return;
	}
	static_cast<HiveHandle*>(handle)->brush->set_size(glm::ivec2(size));
}

void hive_brush_shape(void* handle, const int32_t shape) {
	if (handle == nullptr) {
		return;
	}
	static Brush::Shape target = Brush::Shape::square;
	switch (shape) {
		case 0:
			target = Brush::Shape::square;
			break;
		case 1:
			target = Brush::Shape::circle;
			break;
		case 2:
			target = Brush::Shape::diamond;
			break;
		default:
			return;
	}
	static_cast<HiveHandle*>(handle)->brush->set_shape(target);
}

int32_t hive_set_texture_tool(void* handle, const char* tile_id) {
	if (handle == nullptr || tile_id == nullptr) {
		return -1;
	}
	auto* brush = static_cast<HiveHandle*>(handle)->brush;
	brush->texture_operator.tile_id = tile_id;
	brush->activate_operator(brush->texture_operator);
	return 0;
}

int32_t hive_set_height_tool(void* handle, const int32_t deformation) {
	if (handle == nullptr || deformation < 0 || deformation > 4) {
		return -1;
	}
	auto* brush = static_cast<HiveHandle*>(handle)->brush;
	brush->height_operator.deformation_type = static_cast<HeightOperator::deformation>(deformation);
	brush->activate_operator(brush->height_operator);
	return 0;
}

int32_t hive_set_cliff_tool(void* handle, const int32_t operation, const char* cliff_id) {
	if (handle == nullptr || operation < 0 || operation > 7) {
		return -1;
	}
	auto* brush = static_cast<HiveHandle*>(handle)->brush;
	brush->cliff_operator.cliff_operation_type = static_cast<CliffOperator::cliff_operation>(operation);
	if (cliff_id != nullptr) {
		brush->cliff_operator.cliff_id = cliff_id;
	}
	brush->activate_operator(brush->cliff_operator);
	return 0;
}

int32_t hive_set_cell_tool(void* handle, const int32_t operation) {
	if (handle == nullptr || operation < 0 || operation > 3) {
		return -1;
	}
	auto* brush = static_cast<HiveHandle*>(handle)->brush;
	brush->cell_operator.set_operation_type(static_cast<CellOperator::cell_operation>(operation));
	brush->activate_operator(brush->cell_operator);
	return 0;
}

void hive_brush_begin(void* handle) {
	if (handle == nullptr) {
		return;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto ctx = hive->map->edit_context();
	hive->brush->apply_begin(ctx);
}

void hive_brush_apply(void* handle, const double frame_delta) {
	if (handle == nullptr) {
		return;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto ctx = hive->map->edit_context();
	hive->brush->apply(ctx, frame_delta);
}

void hive_brush_end(void* handle) {
	if (handle == nullptr) {
		return;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto ctx = hive->map->edit_context();
	hive->brush->apply_end(ctx);
}

int32_t hive_undo(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto ctx = hive->map->edit_context();
	hive->map->world_undo.undo(ctx);
	return 1;
}

int32_t hive_redo(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto ctx = hive->map->edit_context();
	hive->map->world_undo.redo(ctx);
	return 1;
}

}
