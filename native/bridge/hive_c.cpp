#include "hive_c.h"

#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

#include "blp.hpp"
#include "globals.hpp"
#include "hive_bridge.hpp"
#include "map.hpp"
#include "mpq.hpp"
#include "doodads_undo.hpp"
#include "skinned_mesh.hpp"
#include "units_undo.hpp"
#include "terrain_brush.h"

namespace fs = std::filesystem;

namespace {

struct CachedModel {
	HiveModelInfo info {};
	std::vector<HiveGeosetInfo> geosets;
	std::vector<std::string> texture_paths;
	std::vector<float> positions;
	std::vector<float> normals;
	std::vector<float> uvs;
	std::vector<uint32_t> indices;
};

struct HiveHandle {
	Map* map = nullptr;
	TerrainBrush* brush = nullptr;
	fs::path unpack_dir;
	std::string unpack_dir_string;
	std::vector<std::shared_ptr<CachedModel>> models;
	std::unordered_map<std::string, int32_t> model_keys;
	std::vector<std::string> doodad_types;
	std::unordered_map<std::string, blp::Image> images;
	std::string transient_string;
	std::vector<Doodad> clipboard;
	glm::vec2 clipboard_anchor {0.f};
	bool clipboard_grid_aligned = false;
	std::vector<std::string> unit_types;
	std::vector<Unit> unit_clipboard;
	glm::vec2 unit_clipboard_anchor {0.f};

	struct GroupEdit {
		bool active = false;
		bool unit = false;
		bool grid_aligned = false;
		glm::vec2 cursor {0.f};
		std::vector<int32_t> indices;
		std::vector<Doodad> doodads;
		std::vector<Unit> units;
	};
	GroupEdit group;

	struct ObjectField {
		std::string key;
		std::string column;
		int level = 0;
		std::string label;
		std::string type;
		std::string description;
	};
	struct ObjectFields {
		int32_t category = -1;
		std::string id;
		std::vector<ObjectField> fields;
	};
	ObjectFields object_fields;
	int32_t doodad_random = 0;
	std::mt19937 random_engine {std::random_device {}()};
};

std::shared_ptr<CachedModel> build_model(const std::shared_ptr<mdx::MDX>& mdx) {
	auto model = std::make_shared<CachedModel>();

	float min_x = 1e30f;
	float min_y = 1e30f;
	float min_z = 1e30f;
	float max_x = -1e30f;
	float max_y = -1e30f;
	float max_z = -1e30f;

	for (const auto& geoset : mdx->geosets) {
		HiveGeosetInfo info {};
		info.vertex_offset = static_cast<int32_t>(model->positions.size() / 3);
		info.vertex_count = static_cast<int32_t>(geoset.vertices.size());
		info.index_offset = static_cast<int32_t>(model->indices.size());
		info.index_count = static_cast<int32_t>(geoset.faces.size());
		info.alpha = 1.f;

		for (std::size_t i = 0; i < geoset.vertices.size(); ++i) {
			const glm::vec3& vertex = geoset.vertices[i];
			model->positions.push_back(vertex.x);
			model->positions.push_back(vertex.y);
			model->positions.push_back(vertex.z);

			min_x = std::min(min_x, vertex.x);
			min_y = std::min(min_y, vertex.y);
			min_z = std::min(min_z, vertex.z);
			max_x = std::max(max_x, vertex.x);
			max_y = std::max(max_y, vertex.y);
			max_z = std::max(max_z, vertex.z);

			const glm::vec3 normal = i < geoset.normals.size() ? geoset.normals[i] : glm::vec3(0.f, 0.f, 1.f);
			model->normals.push_back(normal.x);
			model->normals.push_back(normal.y);
			model->normals.push_back(normal.z);

			glm::vec2 uv(0.f);
			if (!geoset.uv_sets.empty() && i < geoset.uv_sets[0].size()) {
				uv = geoset.uv_sets[0][i];
			}
			model->uvs.push_back(uv.x);
			model->uvs.push_back(uv.y);
		}

		for (const uint16_t face : geoset.faces) {
			model->indices.push_back(face);
		}

		std::string texture_path;
		if (geoset.material_id < mdx->materials.size()) {
			const mdx::Material& material = mdx->materials[geoset.material_id];
			info.two_sided = (material.flags & mdx::Material::two_sided) ? 1 : 0;

			if (!material.layers.empty()) {
				const mdx::Layer& layer = material.layers[0];
				info.alpha = layer.alpha;
				info.unshaded = (layer.shading_flags & mdx::Layer::unshaded) ? 1 : 0;
				if ((layer.shading_flags & mdx::Layer::two_sided) != 0) {
					info.two_sided = 1;
				}

				if (!layer.textures.empty()) {
					const uint32_t texture_id = layer.textures[0].id;
					if (texture_id < mdx->textures.size()) {
						const mdx::Texture& texture = mdx->textures[texture_id];
						if (texture.replaceable_id == 0) {
							texture_path = texture.file_name.generic_string();
						} else if (const auto found = mdx::replaceable_id_to_texture.find(static_cast<int>(texture.replaceable_id));
								   found != mdx::replaceable_id_to_texture.end()) {
							texture_path = found->second + ".blp";
						}
					}
				}
			}
		}

		model->texture_paths.push_back(texture_path);
		model->geosets.push_back(info);
	}

	model->info.geoset_count = static_cast<int32_t>(model->geosets.size());
	model->info.vertex_count = static_cast<int32_t>(model->positions.size() / 3);
	model->info.index_count = static_cast<int32_t>(model->indices.size());
	model->info.min_x = min_x == 1e30f ? 0.f : min_x;
	model->info.min_y = min_y == 1e30f ? 0.f : min_y;
	model->info.min_z = min_z == 1e30f ? 0.f : min_z;
	model->info.max_x = max_x == -1e30f ? 0.f : max_x;
	model->info.max_y = max_y == -1e30f ? 0.f : max_y;
	model->info.max_z = max_z == -1e30f ? 0.f : max_z;
	return model;
}

const blp::Image* load_image(HiveHandle* hive, const char* path, bool& ok) {
	ok = false;
	if (path == nullptr) {
		return nullptr;
	}

	if (const auto found = hive->images.find(path); found != hive->images.end()) {
		ok = true;
		return &found->second;
	}

	auto opened = hierarchy.open_file(path, Hierarchy::FileSource::all, {".blp", ".tga", ".dds"});
	if (!opened) {
		return nullptr;
	}

	if (!blp::is_blp(*opened)) {
		return nullptr;
	}

	auto image = blp::load(*opened);
	if (!image) {
		return nullptr;
	}

	auto [slot, inserted] = hive->images.emplace(path, std::move(*image));
	ok = true;
	return &slot->second;
}

/// 地图 tileset 单字符 → 替换贴图文件名里用的季节词
const char* tileset_season(const uint32_t tileset_id) {
	switch (static_cast<char>(tileset_id & 0xFF)) {
		case 'L':
			return "Summer";
		case 'F':
			return "Fall";
		case 'W':
			return "Winter";
		case 'I':
		case 'N':
			return "Snow";
		case 'A':
			return "Ashen";
		case 'B':
			return "Barrens";
		case 'Z':
			return "Ruins";
		case 'O':
			return "Outland";
		case 'V':
			return "Village";
		default:
			return "";
	}
}

/// 替换贴图按 tileset 分文件。replaceable 表里只给了秋天那份
/// （ReplaceableTextures/LordaeronTree/LordaeronFallTree），换张地图就对不上。
/// 先按当前 tileset 的季节词在同目录找，再退回第一个能解码的。
const blp::Image* load_replacement_image(HiveHandle* hive, const std::string& path, bool& ok) {
	const blp::Image* direct = load_image(hive, path.c_str(), ok);
	if (ok) {
		return direct;
	}
	if (path.find("ReplaceableTextures") == std::string::npos) {
		return nullptr;
	}

	const fs::path parent = hierarchy.root_directory / fs::path(path).parent_path();
	const std::string season = tileset_season(static_cast<uint32_t>(hive->map->terrain.tileset_id));
	std::error_code ec;

	if (!season.empty()) {
		for (const auto& entry : fs::directory_iterator(parent, ec)) {
			if (entry.path().extension() != ".blp") {
				continue;
			}
			if (entry.path().filename().string().find(season) == std::string::npos) {
				continue;
			}
			const std::string relative = (fs::path(path).parent_path() / entry.path().filename()).generic_string();
			const blp::Image* candidate = load_image(hive, relative.c_str(), ok);
			if (ok) {
				return candidate;
			}
		}
	}

	for (const auto& entry : fs::directory_iterator(parent, ec)) {
		if (entry.path().extension() != ".blp") {
			continue;
		}
		const std::string relative = (fs::path(path).parent_path() / entry.path().filename()).generic_string();
		const blp::Image* candidate = load_image(hive, relative.c_str(), ok);
		if (ok) {
			return candidate;
		}
	}
	return nullptr;
}

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

int32_t hive_copy_regions(void* handle, HiveRegion* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->map->regions.regions;
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	for (std::size_t i = 0; i < count; ++i) {
		out[i].left = std::min(source[i].left, source[i].right);
		out[i].right = std::max(source[i].left, source[i].right);
		out[i].bottom = std::min(source[i].bottom, source[i].top);
		out[i].top = std::max(source[i].bottom, source[i].top);
		out[i].red = source[i].color.r;
		out[i].green = source[i].color.g;
		out[i].blue = source[i].color.b;
	}
	return static_cast<int32_t>(count);
}

const char* hive_region_name(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->map->regions.regions;
	if (static_cast<std::size_t>(index) >= source.size()) {
		return nullptr;
	}
	return source[static_cast<std::size_t>(index)].name.c_str();
}

int32_t hive_pathing_info(void* handle, int32_t* width, int32_t* height) {
	if (handle == nullptr) {
		return -1;
	}
	const PathingMap& pathing = static_cast<HiveHandle*>(handle)->map->pathing_map;
	if (width != nullptr) {
		*width = pathing.width;
	}
	if (height != nullptr) {
		*height = pathing.height;
	}
	return 0;
}

int32_t hive_copy_pathing(void* handle, uint8_t* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->map->pathing_map.pathing_cells_static;
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, source.data(), count);
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


int32_t hive_copy_doodads(void* handle, HiveDoodad* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->map->doodads.doodads;
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	for (std::size_t i = 0; i < count; ++i) {
		out[i].x = source[i].position.x;
		out[i].y = source[i].position.y;
		out[i].z = source[i].position.z;
		out[i].angle = source[i].angle;
		out[i].scale_x = source[i].scale.x;
		out[i].scale_y = source[i].scale.y;
		out[i].scale_z = source[i].scale.z;
		out[i].variation = source[i].variation;
		out[i].solid = source[i].state == Doodad::State::visible_solid ? 1 : 0;
	}
	return static_cast<int32_t>(count);
}

const char* hive_doodad_id(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->map->doodads.doodads;
	if (static_cast<std::size_t>(index) >= source.size()) {
		return nullptr;
	}
	return source[static_cast<std::size_t>(index)].id.c_str();
}

int32_t model_for_doodad(HiveHandle* hive, const std::string& id, const int32_t variation) {
	const std::string key = id + "#" + std::to_string(variation);

	if (const auto found = hive->model_keys.find(key); found != hive->model_keys.end()) {
		return found->second;
	}

	auto mesh = hive->map->doodads.get_mesh(id, variation);
	if (!mesh || !mesh->mdx) {
		hive->model_keys[key] = -1;
		return -1;
	}

	hive->models.push_back(build_model(mesh->mdx));
	const auto index = static_cast<int32_t>(hive->models.size() - 1);
	hive->model_keys[key] = index;
	return index;
}

int32_t hive_model_from_doodad(void* handle, const int32_t doodad_index) {
	if (handle == nullptr || doodad_index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->map->doodads.doodads;
	if (static_cast<std::size_t>(doodad_index) >= source.size()) {
		return -1;
	}

	const Doodad& doodad = source[static_cast<std::size_t>(doodad_index)];
	return model_for_doodad(hive, doodad.id, doodad.variation);
}


/// 命中距离，没打中返回负数。direction 必须归一化。
float aabb_hit_distance(const glm::vec3& minimum, const glm::vec3& maximum, const glm::vec3& origin,
						const glm::vec3& direction, const float max_t) {
	float t_min = 0.f;
	float t_max = max_t;
	for (int axis = 0; axis < 3; ++axis) {
		if (std::abs(direction[axis]) < 1e-8f) {
			if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis]) {
				return -1.f;
			}
			continue;
		}
		float near = (minimum[axis] - origin[axis]) / direction[axis];
		float far = (maximum[axis] - origin[axis]) / direction[axis];
		if (near > far) {
			std::swap(near, far);
		}
		t_min = std::max(t_min, near);
		t_max = std::min(t_max, far);
		if (t_min > t_max) {
			return -1.f;
		}
	}
	return t_min;
}

/// War3 模型以 1/128 tile 为单位，位置和射线都先换到模型单位再比。
constexpr float model_scale = 128.f;

glm::mat4 placement_matrix(const glm::vec3& position, const float angle, const glm::vec3& scale) {
	glm::mat4 matrix = glm::translate(glm::mat4(1.f), position * model_scale);
	matrix = glm::rotate(matrix, -angle, glm::vec3(0.f, 0.f, 1.f));
	return glm::scale(matrix, scale);
}

std::pair<glm::vec3, glm::vec3> mesh_bounds(const std::shared_ptr<mdx::MDX>& mdx) {
	glm::vec3 minimum(1e30f);
	glm::vec3 maximum(-1e30f);
	for (const auto& geoset : mdx->geosets) {
		for (const auto& vertex : geoset.vertices) {
			minimum = glm::min(minimum, vertex);
			maximum = glm::max(maximum, vertex);
		}
	}
	if (minimum.x > maximum.x) {
		return {glm::vec3(0.f), glm::vec3(0.f)};
	}
	return {minimum, maximum};
}

int32_t pick_from_ray(HiveHandle* hive, const float* origin, const float* direction, const float max_distance,
					  const bool units) {
	if (origin == nullptr || direction == nullptr) {
		return -1;
	}

	const glm::vec3 ray_origin(origin[0] * model_scale, origin[2] * model_scale, origin[1] * model_scale);
	glm::vec3 ray_direction(direction[0], direction[2], direction[1]);
	if (glm::length(ray_direction) < 1e-6f) {
		return -1;
	}
	ray_direction = glm::normalize(ray_direction);

	const float max_t = max_distance * model_scale;
	int32_t best = -1;
	float best_t = max_t;

	auto test = [&](const std::string& id, const float variation, const glm::vec3& position, const float angle,
					const glm::vec3& scale, const int32_t index) {
		if (scale.x == 0.f || scale.y == 0.f || scale.z == 0.f) {
			return;
		}

		auto mesh = units ? hive->map->units.get_mesh(id) : hive->map->doodads.get_mesh(id, static_cast<int>(variation));
		if (!mesh || !mesh->mdx) {
			return;
		}

		const auto bounds = mesh_bounds(mesh->mdx);
		glm::vec3 minimum;
		glm::vec3 maximum;
		transform_aabb_non_uniform(bounds.first, bounds.second, minimum, maximum,
								   placement_matrix(position, angle, scale));

		const float hit = aabb_hit_distance(minimum, maximum, ray_origin, ray_direction, max_t);
		if (hit >= 0.f && hit < best_t) {
			best_t = hit;
			best = index;
		}
	};

	if (units) {
		const auto& list = hive->map->units.units;
		for (std::size_t i = 0; i < list.size(); ++i) {
			test(list[i].id, 0.f, list[i].position, list[i].angle, list[i].scale, static_cast<int32_t>(i));
		}
	} else {
		const auto& list = hive->map->doodads.doodads;
		for (std::size_t i = 0; i < list.size(); ++i) {
			test(list[i].id, static_cast<float>(list[i].variation), list[i].position, list[i].angle, list[i].scale,
				 static_cast<int32_t>(i));
		}
	}

	return best;
}

int32_t hive_pick_doodad(void* handle, const float* origin, const float* direction, const float max_distance) {
	if (handle == nullptr) {
		return -1;
	}
	return pick_from_ray(static_cast<HiveHandle*>(handle), origin, direction, max_distance, false);
}

int32_t hive_pick_unit(void* handle, const float* origin, const float* direction, const float max_distance) {
	if (handle == nullptr) {
		return -1;
	}
	return pick_from_ray(static_cast<HiveHandle*>(handle), origin, direction, max_distance, true);
}

int32_t hive_query_area(void* handle, const float x0, const float y0, const float x1, const float y1,
						const int32_t is_unit, int32_t* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	const float left = std::min(x0, x1);
	const float right = std::max(x0, x1);
	const float bottom = std::min(y0, y1);
	const float top = std::max(y0, y1);
	const TerrainRectF area(left, bottom, right - left, top - bottom);

	std::vector<int32_t> hits;
	if (is_unit != 0) {
		auto& list = hive->map->units.units;
		for (auto* unit : hive->map->units.query_area(area)) {
			hits.push_back(static_cast<int32_t>(unit - list.data()));
		}
	} else {
		auto& list = hive->map->doodads.doodads;
		for (auto* doodad : hive->map->doodads.query_area(area)) {
			hits.push_back(static_cast<int32_t>(doodad - list.data()));
		}
	}

	std::sort(hits.begin(), hits.end());
	const auto count = std::min<std::size_t>(hits.size(), static_cast<std::size_t>(capacity));
	std::copy_n(hits.begin(), count, out);
	return static_cast<int32_t>(count);
}

int32_t hive_remove_selection(void* handle, const int32_t is_unit, const int32_t* indices, const int32_t count) {
	if (handle == nullptr || indices == nullptr || count <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	WorldEditContext ctx = hive->map->edit_context();

	if (is_unit != 0) {
		auto& list = ctx.units.units;
		auto action = std::make_unique<UnitDeleteAction>();
		for (int32_t i = 0; i < count; ++i) {
			const int32_t index = indices[i];
			if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			action->units.push_back(list[static_cast<std::size_t>(index)]);
		}
		if (action->units.empty()) {
			return 0;
		}

		std::unordered_set<int> numbers;
		for (const Unit& unit : action->units) {
			numbers.insert(unit.creation_number);
		}
		const auto removed = static_cast<int32_t>(action->units.size());

		hive->map->world_undo.new_undo_group();
		hive->map->world_undo.add_undo_action(std::move(action));
		std::erase_if(list, [&](const Unit& unit) { return numbers.contains(unit.creation_number); });
		return removed;
	}

	auto& list = ctx.doodads.doodads;
	auto action = std::make_unique<DoodadDeleteAction>();
	for (int32_t i = 0; i < count; ++i) {
		const int32_t index = indices[i];
		if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
			continue;
		}
		action->doodads.push_back(list[static_cast<std::size_t>(index)]);
	}
	if (action->doodads.empty()) {
		return 0;
	}

	std::unordered_set<int> numbers;
	for (const Doodad& doodad : action->doodads) {
		numbers.insert(doodad.creation_number);
	}
	const auto removed = static_cast<int32_t>(action->doodads.size());

	hive->map->world_undo.new_undo_group();
	hive->map->world_undo.add_undo_action(std::move(action));
	std::erase_if(list, [&](const Doodad& doodad) { return numbers.contains(doodad.creation_number); });
	return removed;
}




int32_t hive_begin_group(void* handle, const int32_t is_unit, const int32_t* indices, const int32_t count,
						 const float x, const float y) {
	if (handle == nullptr || indices == nullptr || count <= 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	HiveHandle::GroupEdit& group = hive->group;

	group = HiveHandle::GroupEdit {};
	group.unit = is_unit != 0;
	group.cursor = glm::vec2(x, y);

	if (group.unit) {
		const auto& list = hive->map->units.units;
		for (int32_t i = 0; i < count; ++i) {
			const int32_t index = indices[i];
			if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			group.indices.push_back(index);
			group.units.push_back(list[static_cast<std::size_t>(index)]);
		}
	} else {
		const auto& list = hive->map->doodads.doodads;
		for (int32_t i = 0; i < count; ++i) {
			const int32_t index = indices[i];
			if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			const Doodad& doodad = list[static_cast<std::size_t>(index)];
			if (doodad.pathing) {
				group.grid_aligned = true;
			}
			group.indices.push_back(index);
			group.doodads.push_back(doodad);
		}
	}

	group.active = !group.indices.empty();
	return group.active ? 0 : -1;
}

int32_t hive_move_group_to(void* handle, const int32_t is_unit, const float x, const float y) {
	if (handle == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	HiveHandle::GroupEdit& group = hive->group;
	if (!group.active || group.unit != (is_unit != 0)) {
		return -1;
	}

	const glm::vec2 mouse(x, y);
	glm::vec2 offset;
	if (group.grid_aligned) {
		offset = glm::round(mouse * 2.f + 0.5f) / 2.f - 0.25f;
		offset -= glm::round(group.cursor * 2.f + 0.5f) / 2.f - 0.25f;
		if (offset.x == 0.f && offset.y == 0.f) {
			return 0;
		}
	} else {
		offset = mouse - group.cursor;
	}
	group.cursor = mouse;

	if (group.unit) {
		auto& list = hive->map->units.units;
		for (const int32_t index : group.indices) {
			if (static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			Unit& unit = list[static_cast<std::size_t>(index)];
			unit.position.x += offset.x;
			unit.position.y += offset.y;
			unit.position.z = hive->map->terrain.interpolated_height(unit.position.x, unit.position.y, true);
			unit.update();
		}
	} else {
		auto& list = hive->map->doodads.doodads;
		for (const int32_t index : group.indices) {
			if (static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			Doodad& doodad = list[static_cast<std::size_t>(index)];
			doodad.position.x += offset.x;
			doodad.position.y += offset.y;
			doodad.position.z = hive->map->terrain.interpolated_height(doodad.position.x, doodad.position.y, true);
			doodad.update(hive->map->terrain);
		}
	}

	return 0;
}

int32_t hive_rotate_group_to(void* handle, const int32_t is_unit, const float x, const float y) {
	if (handle == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	HiveHandle::GroupEdit& group = hive->group;
	if (!group.active || group.unit != (is_unit != 0)) {
		return -1;
	}

	if (group.unit) {
		auto& list = hive->map->units.units;
		for (const int32_t index : group.indices) {
			if (static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			Unit& unit = list[static_cast<std::size_t>(index)];
			float angle = std::atan2(y - unit.position.y, x - unit.position.x);
			if (angle < 0.f) {
				angle += 2.f * glm::pi<float>();
			}
			unit.angle = angle;
			unit.update();
		}
	} else {
		auto& list = hive->map->doodads.doodads;
		for (const int32_t index : group.indices) {
			if (static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			Doodad& doodad = list[static_cast<std::size_t>(index)];
			float angle = std::atan2(y - doodad.position.y, x - doodad.position.x);
			if (angle < 0.f) {
				angle += 2.f * glm::pi<float>();
			}
			doodad.angle = Doodad::acceptable_angle(doodad.id, doodad.pathing, doodad.angle, angle);
			doodad.position = glm::vec3(
				Doodad::acceptable_position(doodad.position, doodad.pathing, doodad.angle), doodad.position.z);
			doodad.update(hive->map->terrain);
		}
	}

	return 0;
}

int32_t hive_end_group(void* handle, const int32_t is_unit) {
	if (handle == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	HiveHandle::GroupEdit& group = hive->group;
	if (!group.active || group.unit != (is_unit != 0)) {
		return -1;
	}
	group.active = false;

	if (group.unit) {
		const auto& list = hive->map->units.units;
		auto action = std::make_unique<UnitStateAction>();
		for (std::size_t i = 0; i < group.indices.size(); ++i) {
			const int32_t index = group.indices[i];
			if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			action->old_units.push_back(group.units[i]);
			action->new_units.push_back(list[static_cast<std::size_t>(index)]);
		}
		if (action->old_units.empty()) {
			return 0;
		}

		hive->map->world_undo.new_undo_group();
		hive->map->world_undo.add_undo_action(std::move(action));
	} else {
		const auto& list = hive->map->doodads.doodads;
		auto action = std::make_unique<DoodadStateAction>();
		for (std::size_t i = 0; i < group.indices.size(); ++i) {
			const int32_t index = group.indices[i];
			if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
				continue;
			}
			action->old_doodads.push_back(group.doodads[i]);
			action->new_doodads.push_back(list[static_cast<std::size_t>(index)]);
		}
		if (action->old_doodads.empty()) {
			return 0;
		}

		hive->map->world_undo.new_undo_group();
		hive->map->world_undo.add_undo_action(std::move(action));
	}

	return 0;
}


float hive_object_angle(void* handle, const int32_t is_unit, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return 0.f;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	if (is_unit != 0) {
		const auto& list = hive->map->units.units;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return 0.f;
		}
		return list[static_cast<std::size_t>(index)].angle;
	}

	const auto& list = hive->map->doodads.doodads;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return 0.f;
	}
	return list[static_cast<std::size_t>(index)].angle;
}

int32_t hive_selection_box(void* handle, const int32_t is_unit, const int32_t index, float* out) {
	if (handle == nullptr || out == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	std::string id;
	glm::vec3 position(0.f);
	float angle = 0.f;
	glm::vec3 scale(1.f);

	if (is_unit != 0) {
		const auto& list = hive->map->units.units;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}
		id = list[static_cast<std::size_t>(index)].id;
		position = list[static_cast<std::size_t>(index)].position;
		angle = list[static_cast<std::size_t>(index)].angle;
		scale = list[static_cast<std::size_t>(index)].scale;
	} else {
		const auto& list = hive->map->doodads.doodads;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}
		id = list[static_cast<std::size_t>(index)].id;
		position = list[static_cast<std::size_t>(index)].position;
		angle = list[static_cast<std::size_t>(index)].angle;
		scale = list[static_cast<std::size_t>(index)].scale;
	}

	auto mesh = is_unit != 0 ? hive->map->units.get_mesh(id)
							 : hive->map->doodads.get_mesh(id, static_cast<int>(hive->map->doodads.doodads[static_cast<std::size_t>(index)].variation));
	if (!mesh || !mesh->mdx) {
		return -1;
	}

	const auto bounds = mesh_bounds(mesh->mdx);
	glm::vec3 minimum;
	glm::vec3 maximum;
	transform_aabb_non_uniform(bounds.first, bounds.second, minimum, maximum,
							   placement_matrix(position, angle, scale));

	// 模型单位换回 tile，并按 Godot 的轴序（X 水平、Y 高度、Z 水平）排列
	out[0] = minimum.x / model_scale;
	out[1] = minimum.z / model_scale;
	out[2] = minimum.y / model_scale;
	out[3] = maximum.x / model_scale;
	out[4] = maximum.z / model_scale;
	out[5] = maximum.y / model_scale;
	return 0;
}

const std::vector<std::string>& doodad_type_ids(HiveHandle* hive) {
	if (hive->doodad_types.empty()) {
		for (const auto& [id, row] : doodads_slk.row_headers) {
			if (id.size() == 4) {
				hive->doodad_types.push_back(id);
			}
		}
		for (const auto& [id, row] : destructibles_slk.row_headers) {
			if (id.size() == 4) {
				hive->doodad_types.push_back(id);
			}
		}
		std::sort(hive->doodad_types.begin(), hive->doodad_types.end());
		hive->doodad_types.erase(std::unique(hive->doodad_types.begin(), hive->doodad_types.end()),
								 hive->doodad_types.end());
	}
	return hive->doodad_types;
}

int32_t hive_doodad_type_count(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(doodad_type_ids(static_cast<HiveHandle*>(handle)).size());
}

const char* hive_doodad_type_id(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& ids = doodad_type_ids(static_cast<HiveHandle*>(handle));
	if (static_cast<std::size_t>(index) >= ids.size()) {
		return nullptr;
	}
	return ids[static_cast<std::size_t>(index)].c_str();
}

int32_t hive_add_doodad(void* handle, const char* id, const float x, const float y, const float z, const float angle) {
	if (handle == nullptr || id == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	const bool is_doodad = doodads_slk.row_headers.contains(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

	const int variation_count = slk.data<int>("numvar", id);
	int variation = 0;
	if ((hive->doodad_random & 1) != 0 && variation_count > 0) {
		std::uniform_int_distribution<int> dist(0, variation_count - 1);
		variation = dist(hive->random_engine);
	}

	auto mesh = hive->map->doodads.get_mesh(id, variation);
	if (!mesh || !mesh->mdx) {
		return -1;
	}

	Doodad doodad;
	doodad.id = id;
	doodad.variation = variation;
	doodad.position = glm::vec3(x, y, z);
	doodad.angle = angle;
	doodad.scale = glm::vec3(1.f);
	doodad.init(id, mesh, hive->map->terrain);

	if ((hive->doodad_random & 2) != 0) {
		std::uniform_real_distribution<float> dist(0.f, glm::pi<float>() * 2.f);
		const float target = dist(hive->random_engine);
		doodad.angle = Doodad::acceptable_angle(doodad.id, doodad.pathing, target, target);
	}

	if ((hive->doodad_random & 4) != 0) {
		const float min_scale = slk.data<float>("minscale", id);
		const float max_scale = slk.data<float>("maxscale", id);
		if (max_scale > min_scale) {
			std::uniform_real_distribution<float> dist(min_scale, max_scale);
			doodad.scale = glm::vec3(dist(hive->random_engine));
		}
	}

	doodad.update(hive->map->terrain);

	hive->map->doodads.doodads.push_back(std::move(doodad));
	return static_cast<int32_t>(hive->map->doodads.doodads.size() - 1);
}

void hive_set_doodad_random(void* handle, const int32_t flags) {
	if (handle == nullptr) {
		return;
	}
	static_cast<HiveHandle*>(handle)->doodad_random = flags;
}



int32_t hive_copy_selection(void* handle, const int32_t* indices, const int32_t count) {
	if (handle == nullptr || indices == nullptr || count <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& list = hive->map->doodads.doodads;

	std::vector<Doodad> picked;
	glm::vec3 average_position(0.f);
	bool grid_aligned = false;

	for (int32_t i = 0; i < count; ++i) {
		const int32_t index = indices[i];
		if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
			continue;
		}
		const Doodad& doodad = list[static_cast<std::size_t>(index)];
		if (doodad.pathing) {
			grid_aligned = true;
		}
		picked.push_back(doodad);
		average_position += doodad.position;
	}

	if (picked.empty()) {
		hive->clipboard.clear();
		return 0;
	}

	hive->clipboard_anchor = glm::vec2(average_position) / static_cast<float>(picked.size());
	hive->clipboard_grid_aligned = grid_aligned;
	hive->clipboard = std::move(picked);
	return static_cast<int32_t>(hive->clipboard.size());
}

int32_t hive_clipboard_count(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(static_cast<HiveHandle*>(handle)->clipboard.size());
}

int32_t hive_copy_clipboard(void* handle, HiveDoodad* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->clipboard;
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	for (std::size_t i = 0; i < count; ++i) {
		out[i].x = source[i].position.x;
		out[i].y = source[i].position.y;
		out[i].z = source[i].position.z;
		out[i].angle = source[i].angle;
		out[i].scale_x = source[i].scale.x;
		out[i].scale_y = source[i].scale.y;
		out[i].scale_z = source[i].scale.z;
		out[i].variation = source[i].variation;
		out[i].solid = source[i].state == Doodad::State::visible_solid ? 1 : 0;
	}
	return static_cast<int32_t>(count);
}

void hive_clipboard_anchor(void* handle, float* out) {
	if (handle == nullptr || out == nullptr) {
		return;
	}
	const glm::vec2& anchor = static_cast<HiveHandle*>(handle)->clipboard_anchor;
	out[0] = anchor.x;
	out[1] = anchor.y;
}

const char* hive_clipboard_id(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->clipboard;
	if (static_cast<std::size_t>(index) >= source.size()) {
		return nullptr;
	}
	return source[static_cast<std::size_t>(index)].id.c_str();
}

int32_t hive_model_from_clipboard(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->clipboard;
	if (static_cast<std::size_t>(index) >= source.size()) {
		return -1;
	}
	const Doodad& doodad = source[static_cast<std::size_t>(index)];
	return model_for_doodad(hive, doodad.id, doodad.variation);
}

int32_t hive_place_clipboard(void* handle, const float x, const float y) {
	if (handle == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (hive->clipboard.empty()) {
		return -1;
	}

	WorldEditContext ctx = hive->map->edit_context();
	auto action = std::make_unique<DoodadAddAction>();
	const int32_t first = static_cast<int32_t>(hive->map->doodads.doodads.size());
	const glm::vec2 mouse(x, y);

	for (const Doodad& source : hive->clipboard) {
		glm::vec3 final_position(
			Doodad::acceptable_position(glm::vec2(source.position) + mouse - hive->clipboard_anchor,
										source.pathing,
										source.angle,
										hive->clipboard_grid_aligned),
			0.f);
		final_position.z = ctx.terrain.interpolated_height(final_position.x, final_position.y, true);

		Doodad& doodad = ctx.doodads.add_doodad(source);
		doodad.creation_number = ++Doodad::auto_increment;
		doodad.position = final_position;
		doodad.update(ctx.terrain);

		action->doodads.push_back(doodad);
	}

	hive->map->world_undo.new_undo_group();
	hive->map->world_undo.add_undo_action(std::move(action));
	return first;
}

int32_t hive_copy_units(void* handle, HiveUnit* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->map->units.units;
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	for (std::size_t i = 0; i < count; ++i) {
		out[i].x = source[i].position.x;
		out[i].y = source[i].position.y;
		out[i].z = source[i].position.z;
		out[i].angle = source[i].angle;
		out[i].scale_x = source[i].scale.x;
		out[i].scale_y = source[i].scale.y;
		out[i].scale_z = source[i].scale.z;
		out[i].player = source[i].player;
		out[i].variation = source[i].variation;
	}
	return static_cast<int32_t>(count);
}

const char* hive_unit_id(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->map->units.units;
	if (static_cast<std::size_t>(index) >= source.size()) {
		return nullptr;
	}
	return source[static_cast<std::size_t>(index)].id.c_str();
}

int32_t model_for_unit(HiveHandle* hive, const std::string& id) {
	const std::string key = "unit:" + id;

	if (const auto found = hive->model_keys.find(key); found != hive->model_keys.end()) {
		return found->second;
	}

	auto mesh = hive->map->units.get_mesh(id);
	if (!mesh || !mesh->mdx) {
		hive->model_keys[key] = -1;
		return -1;
	}

	hive->models.push_back(build_model(mesh->mdx));
	const auto index = static_cast<int32_t>(hive->models.size() - 1);
	hive->model_keys[key] = index;
	return index;
}

int32_t hive_model_from_unit(void* handle, const int32_t unit_index) {
	if (handle == nullptr || unit_index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->map->units.units;
	if (static_cast<std::size_t>(unit_index) >= source.size()) {
		return -1;
	}
	return model_for_unit(hive, source[static_cast<std::size_t>(unit_index)].id);
}

const std::vector<std::string>& unit_type_ids(HiveHandle* hive) {
	if (hive->unit_types.empty()) {
		for (const auto& [id, row] : units_slk.row_headers) {
			if (id.size() == 4) {
				hive->unit_types.push_back(id);
			}
		}
		std::sort(hive->unit_types.begin(), hive->unit_types.end());
	}
	return hive->unit_types;
}

int32_t hive_unit_type_count(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(unit_type_ids(static_cast<HiveHandle*>(handle)).size());
}

const char* hive_unit_type_id(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& ids = unit_type_ids(static_cast<HiveHandle*>(handle));
	if (static_cast<std::size_t>(index) >= ids.size()) {
		return nullptr;
	}
	return ids[static_cast<std::size_t>(index)].c_str();
}

int32_t hive_add_unit(void* handle, const char* id, const float x, const float y, const float angle, const int32_t player) {
	if (handle == nullptr || id == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	WorldEditContext ctx = hive->map->edit_context();

	auto mesh = ctx.units.get_mesh(id);
	if (!mesh || !mesh->mdx) {
		return -1;
	}

	glm::vec3 position(x, y, ctx.terrain.interpolated_height(x, y, true));
	Unit& unit = ctx.units.add_unit(id, position);
	unit.angle = angle;
	unit.player = player;
	unit.update();

	auto action = std::make_unique<UnitAddAction>();
	action->units.push_back(unit);

	hive->map->world_undo.new_undo_group();
	hive->map->world_undo.add_undo_action(std::move(action));

	return static_cast<int32_t>(ctx.units.units.size() - 1);
}

int32_t hive_copy_unit_selection(void* handle, const int32_t* indices, const int32_t count) {
	if (handle == nullptr || indices == nullptr || count <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& list = hive->map->units.units;

	std::vector<Unit> picked;
	glm::vec3 average_position(0.f);

	for (int32_t i = 0; i < count; ++i) {
		const int32_t index = indices[i];
		if (index < 0 || static_cast<std::size_t>(index) >= list.size()) {
			continue;
		}
		picked.push_back(list[static_cast<std::size_t>(index)]);
		average_position += list[static_cast<std::size_t>(index)].position;
	}

	if (picked.empty()) {
		hive->unit_clipboard.clear();
		return 0;
	}

	hive->unit_clipboard_anchor = glm::vec2(average_position) / static_cast<float>(picked.size());
	hive->unit_clipboard = std::move(picked);
	return static_cast<int32_t>(hive->unit_clipboard.size());
}

int32_t hive_unit_clipboard_count(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(static_cast<HiveHandle*>(handle)->unit_clipboard.size());
}

int32_t hive_copy_unit_clipboard(void* handle, HiveUnit* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->unit_clipboard;
	const auto count = std::min<std::size_t>(source.size(), static_cast<std::size_t>(capacity));
	for (std::size_t i = 0; i < count; ++i) {
		out[i].x = source[i].position.x;
		out[i].y = source[i].position.y;
		out[i].z = source[i].position.z;
		out[i].angle = source[i].angle;
		out[i].scale_x = source[i].scale.x;
		out[i].scale_y = source[i].scale.y;
		out[i].scale_z = source[i].scale.z;
		out[i].player = source[i].player;
		out[i].variation = source[i].variation;
	}
	return static_cast<int32_t>(count);
}

void hive_unit_clipboard_anchor(void* handle, float* out) {
	if (handle == nullptr || out == nullptr) {
		return;
	}
	const glm::vec2& anchor = static_cast<HiveHandle*>(handle)->unit_clipboard_anchor;
	out[0] = anchor.x;
	out[1] = anchor.y;
}

const char* hive_unit_clipboard_id(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& source = static_cast<HiveHandle*>(handle)->unit_clipboard;
	if (static_cast<std::size_t>(index) >= source.size()) {
		return nullptr;
	}
	return source[static_cast<std::size_t>(index)].id.c_str();
}

int32_t hive_model_from_unit_clipboard(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->unit_clipboard;
	if (static_cast<std::size_t>(index) >= source.size()) {
		return -1;
	}
	return model_for_unit(hive, source[static_cast<std::size_t>(index)].id);
}

int32_t hive_place_unit_clipboard(void* handle, const float x, const float y) {
	if (handle == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (hive->unit_clipboard.empty()) {
		return -1;
	}

	WorldEditContext ctx = hive->map->edit_context();
	auto action = std::make_unique<UnitAddAction>();
	const int32_t first = static_cast<int32_t>(ctx.units.units.size());
	const glm::vec2 mouse(x, y);

	for (const Unit& source : hive->unit_clipboard) {
		glm::vec2 flat = glm::vec2(source.position) + mouse - hive->unit_clipboard_anchor;
		glm::vec3 final_position(flat.x, flat.y, ctx.terrain.interpolated_height(flat.x, flat.y, true));

		Unit& unit = ctx.units.add_unit(source);
		unit.creation_number = ++Unit::auto_increment;
		unit.position = final_position;
		unit.update();

		action->units.push_back(unit);
	}

	hive->map->world_undo.new_undo_group();
	hive->map->world_undo.add_undo_action(std::move(action));
	return first;
}

int32_t hive_model_from_path(void* handle, const char* path) {
	if (handle == nullptr || path == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	auto opened = hierarchy.open_file(path, Hierarchy::FileSource::all, {".mdx", ".mdl"});
	if (!opened) {
		return -1;
	}

	auto mdx = std::make_shared<mdx::MDX>(*opened);
	hive->models.push_back(build_model(mdx));
	return static_cast<int32_t>(hive->models.size() - 1);
}

int32_t hive_model_info(void* handle, const int32_t model, HiveModelInfo* out) {
	if (handle == nullptr || out == nullptr || model < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(model) >= hive->models.size()) {
		return -1;
	}
	*out = hive->models[static_cast<std::size_t>(model)]->info;
	return 0;
}

int32_t hive_model_geoset(void* handle, const int32_t model, const int32_t geoset, HiveGeosetInfo* out) {
	if (handle == nullptr || out == nullptr || model < 0 || geoset < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(model) >= hive->models.size()) {
		return -1;
	}
	const auto& cached = *hive->models[static_cast<std::size_t>(model)];
	if (static_cast<std::size_t>(geoset) >= cached.geosets.size()) {
		return -1;
	}
	*out = cached.geosets[static_cast<std::size_t>(geoset)];
	return 0;
}

const char* hive_model_geoset_texture(void* handle, const int32_t model, const int32_t geoset) {
	if (handle == nullptr || model < 0 || geoset < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(model) >= hive->models.size()) {
		return nullptr;
	}
	const auto& cached = *hive->models[static_cast<std::size_t>(model)];
	if (static_cast<std::size_t>(geoset) >= cached.texture_paths.size()) {
		return nullptr;
	}
	if (cached.texture_paths[static_cast<std::size_t>(geoset)].empty()) {
		return nullptr;
	}
	return cached.texture_paths[static_cast<std::size_t>(geoset)].c_str();
}

int32_t hive_copy_model_positions(void* handle, const int32_t model, float* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || model < 0 || capacity <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(model) >= hive->models.size()) {
		return 0;
	}
	const auto& data = hive->models[static_cast<std::size_t>(model)]->positions;
	const auto count = std::min<std::size_t>(data.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, data.data(), count * sizeof(float));
	return static_cast<int32_t>(count);
}

int32_t hive_copy_model_normals(void* handle, const int32_t model, float* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || model < 0 || capacity <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(model) >= hive->models.size()) {
		return 0;
	}
	const auto& data = hive->models[static_cast<std::size_t>(model)]->normals;
	const auto count = std::min<std::size_t>(data.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, data.data(), count * sizeof(float));
	return static_cast<int32_t>(count);
}

int32_t hive_copy_model_uvs(void* handle, const int32_t model, float* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || model < 0 || capacity <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(model) >= hive->models.size()) {
		return 0;
	}
	const auto& data = hive->models[static_cast<std::size_t>(model)]->uvs;
	const auto count = std::min<std::size_t>(data.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, data.data(), count * sizeof(float));
	return static_cast<int32_t>(count);
}

int32_t hive_copy_model_indices(void* handle, const int32_t model, const int32_t geoset, uint32_t* out, const int32_t capacity) {
	if (handle == nullptr || out == nullptr || model < 0 || geoset < 0 || capacity <= 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(model) >= hive->models.size()) {
		return 0;
	}
	const auto& cached = *hive->models[static_cast<std::size_t>(model)];
	if (static_cast<std::size_t>(geoset) >= cached.geosets.size()) {
		return 0;
	}

	const HiveGeosetInfo& info = cached.geosets[static_cast<std::size_t>(geoset)];
	const auto count = std::min<std::size_t>(static_cast<std::size_t>(info.index_count), static_cast<std::size_t>(capacity));
	std::memcpy(out, cached.indices.data() + info.index_offset, count * sizeof(uint32_t));
	return static_cast<int32_t>(count);
}

int32_t hive_image_info_path(void* handle, const char* path, int32_t* width, int32_t* height) {
	if (handle == nullptr || path == nullptr) {
		return -1;
	}
	bool ok = false;
	const blp::Image* image = load_replacement_image(static_cast<HiveHandle*>(handle), path, ok);
	if (!ok || image == nullptr) {
		return -1;
	}
	if (width != nullptr) {
		*width = image->width;
	}
	if (height != nullptr) {
		*height = image->height;
	}
	return 0;
}

int32_t hive_copy_image_path(void* handle, const char* path, uint8_t* out, const int32_t capacity) {
	if (handle == nullptr || path == nullptr || out == nullptr || capacity <= 0) {
		return 0;
	}
	bool ok = false;
	const blp::Image* image = load_replacement_image(static_cast<HiveHandle*>(handle), path, ok);
	if (!ok || image == nullptr) {
		return 0;
	}
	const auto count = std::min<std::size_t>(image->data.size(), static_cast<std::size_t>(capacity));
	std::memcpy(out, image->data.data(), count);
	return static_cast<int32_t>(count);
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

slk::SLK* category_slk(const int32_t category) {
	switch (category) {
		case 0:
			return &units_slk;
		case 1:
			return &items_slk;
		case 2:
			return &doodads_slk;
		case 3:
			return &destructibles_slk;
		case 4:
			return &abilities_slk;
		case 5:
			return &upgrade_slk;
		case 6:
			return &buff_slk;
		default:
			return nullptr;
	}
}

slk::SLK* category_meta_slk(const int32_t category) {
	switch (category) {
		case 0:
			return &units_meta_slk;
		case 1:
			return &items_meta_slk;
		case 2:
			return &doodads_meta_slk;
		case 3:
			return &destructibles_meta_slk;
		case 4:
			return &abilities_meta_slk;
		case 5:
			return &upgrade_meta_slk;
		case 6:
			return &buff_meta_slk;
		default:
			return nullptr;
	}
}

const HiveHandle::ObjectFields& build_object_fields(HiveHandle* hive, const int32_t category, const std::string& id) {
	HiveHandle::ObjectFields& cache = hive->object_fields;
	if (cache.category == category && cache.id == id) {
		return cache;
	}

	cache = HiveHandle::ObjectFields {};
	cache.category = category;
	cache.id = id;

	slk::SLK* slk = category_slk(category);
	slk::SLK* meta = category_meta_slk(category);
	if (slk == nullptr || meta == nullptr || !slk->row_headers.contains(id)) {
		return cache;
	}

	std::string_view id_to_check = id;
	if (slk->shadow_data.contains(id) && slk->shadow_data.at(id).contains("oldid")) {
		id_to_check = slk->shadow_data.at(id).at("oldid");
	}
	const std::string_view aliasing = slk->data<std::string_view>("code", id);

	for (const auto& [key, index] : meta->row_headers) {
		const std::string_view use_specific = meta->data<std::string_view>("usespecific", key);
		if (!use_specific.empty() && !use_specific.contains(id_to_check) && !use_specific.contains(aliasing)) {
			continue;
		}

		const std::string_view not_specific = meta->data<std::string_view>("notspecific", key);
		if (!not_specific.empty() && not_specific.contains(id_to_check) && not_specific.contains(aliasing)) {
			continue;
		}

		std::string field_name = to_lowercase_copy(meta->data<std::string_view>("field", key));
		if (meta->column_headers.contains("data") && meta->data<int>("data", key) > 0) {
			field_name += static_cast<char>('a' + (meta->data<int>("data", key) - 1));
		}

		const std::string_view display_name = meta->data<std::string_view>("displayname", key);
		const std::string category_name = string_replaced(
			world_edit_data.data<std::string_view>("ObjectEditorCategories", meta->data<std::string_view>("category", key)), "&", "");
		const std::string_view type = meta->data<std::string_view>("type", key);
		const std::string_view description = meta->data<std::string_view>("description", key);

		const auto append = [&](std::string column, const int level) {
			if (!slk->column_headers.contains(column)) {
				slk->add_column(column);
			}
			HiveHandle::ObjectField field;
			field.key = key;
			field.column = std::move(column);
			field.level = level;
			field.type = type;
			field.description = description;

			if (level > 0) {
				field.label = std::format("{} - {} - 第 {} 级 ({})", category_name, display_name, level, key);
			} else if (category_name.empty()) {
				field.label = std::format("{} ({})", display_name, key);
			} else {
				field.label = std::format("{} - {} ({})", category_name, display_name, key);
			}
			cache.fields.push_back(std::move(field));
		};

		if (meta->column_headers.contains("repeat") && meta->data<int>("repeat", key) > 0) {
			int iterations = 1;
			if (slk->column_headers.contains("levels")) {
				iterations = slk->data<int>("levels", id);
			} else if (slk->column_headers.contains("numvar")) {
				iterations = slk->data<int>("numvar", id);
			} else if (slk->column_headers.contains("maxlevel")) {
				iterations = slk->data<int>("maxlevel", id);
			}

			const bool append_index = meta->column_headers.contains("appendindex") && meta->data<int>("appendindex", key) > 0;
			for (int i = 0; i < iterations; ++i) {
				std::string name;
				if (append_index && i == 0) {
					name = field_name;
				} else if (append_index) {
					name = field_name + std::to_string(i);
				} else {
					name = field_name + std::to_string(i + 1);
				}
				append(std::move(name), i + 1);
			}
		} else {
			append(field_name, 0);
		}
	}

	std::ranges::sort(cache.fields, [](const HiveHandle::ObjectField& left, const HiveHandle::ObjectField& right) {
		return left.label + left.column < right.label + right.column;
	});

	return cache;
}

int32_t hive_doodad_category(void* handle, const char* id) {
	if (handle == nullptr || id == nullptr) {
		return -1;
	}
	if (doodads_slk.row_headers.contains(id)) {
		return 2;
	}
	if (destructibles_slk.row_headers.contains(id)) {
		return 3;
	}
	return -1;
}

int32_t hive_object_field_count(void* handle, const int32_t category, const char* id) {
	if (handle == nullptr || id == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(build_object_fields(static_cast<HiveHandle*>(handle), category, id).fields.size());
}

const char* hive_object_field_key(void* handle, const int32_t category, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(index) >= hive->object_fields.fields.size()) {
		return nullptr;
	}
	return hive->object_fields.fields[static_cast<std::size_t>(index)].key.c_str();
}

const char* hive_object_field_label(void* handle, const int32_t category, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(index) >= hive->object_fields.fields.size()) {
		return nullptr;
	}
	return hive->object_fields.fields[static_cast<std::size_t>(index)].label.c_str();
}

const char* hive_object_field_type(void* handle, const int32_t category, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(index) >= hive->object_fields.fields.size()) {
		return nullptr;
	}
	return hive->object_fields.fields[static_cast<std::size_t>(index)].type.c_str();
}

const char* hive_object_field_description(void* handle, const int32_t category, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (static_cast<std::size_t>(index) >= hive->object_fields.fields.size()) {
		return nullptr;
	}
	return hive->object_fields.fields[static_cast<std::size_t>(index)].description.c_str();
}

const char* hive_object_value(void* handle, const int32_t category, const char* id, const int32_t index) {
	if (handle == nullptr || id == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& fields = build_object_fields(hive, category, id);
	if (static_cast<std::size_t>(index) >= fields.fields.size()) {
		return nullptr;
	}

	slk::SLK* slk = category_slk(category);
	if (slk == nullptr) {
		return nullptr;
	}

	hive->transient_string = slk->data<std::string_view>(fields.fields[static_cast<std::size_t>(index)].column, id);
	return hive->transient_string.c_str();
}

int32_t hive_set_object_value(void* handle, const int32_t category, const char* id, const int32_t index, const char* value) {
	if (handle == nullptr || id == nullptr || value == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& fields = build_object_fields(hive, category, id);
	if (static_cast<std::size_t>(index) >= fields.fields.size()) {
		return -1;
	}

	slk::SLK* slk = category_slk(category);
	if (slk == nullptr) {
		return -1;
	}

	slk->set_shadow_data(fields.fields[static_cast<std::size_t>(index)].column, id, value);
	return 0;
}

int32_t hive_object_value_modified(void* handle, const int32_t category, const char* id, const int32_t index) {
	if (handle == nullptr || id == nullptr || index < 0) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& fields = build_object_fields(hive, category, id);
	if (static_cast<std::size_t>(index) >= fields.fields.size()) {
		return 0;
	}

	slk::SLK* slk = category_slk(category);
	if (slk == nullptr) {
		return 0;
	}

	const auto row = slk->shadow_data.find(id);
	if (row == slk->shadow_data.end()) {
		return 0;
	}
	return row->second.contains(fields.fields[static_cast<std::size_t>(index)].column) ? 1 : 0;
}

int32_t hive_create_object(void* handle, const int32_t category, const char* source_id, const char* new_id) {
	if (handle == nullptr || source_id == nullptr || new_id == nullptr) {
		return -1;
	}
	slk::SLK* slk = category_slk(category);
	if (slk == nullptr) {
		return -1;
	}
	if (!slk->row_headers.contains(source_id) || slk->row_headers.contains(new_id)) {
		return -1;
	}

	slk->copy_row(source_id, new_id, true);

	auto* hive = static_cast<HiveHandle*>(handle);
	hive->doodad_types.clear();
	hive->unit_types.clear();
	hive->object_fields = HiveHandle::ObjectFields {};
	return 0;
}

int32_t hive_delete_object(void* handle, const int32_t category, const char* id) {
	if (handle == nullptr || id == nullptr) {
		return -1;
	}
	slk::SLK* slk = category_slk(category);
	if (slk == nullptr || !slk->row_headers.contains(id)) {
		return -1;
	}
	const auto row = slk->shadow_data.find(id);
	if (row == slk->shadow_data.end() || !row->second.contains("oldid")) {
		return -1;
	}

	slk->remove_row(id);

	auto* hive = static_cast<HiveHandle*>(handle);
	hive->doodad_types.clear();
	hive->unit_types.clear();
	hive->object_fields = HiveHandle::ObjectFields {};
	return 0;
}

int32_t hive_object_is_custom(void* handle, const int32_t category, const char* id) {
	if (handle == nullptr || id == nullptr) {
		return 0;
	}
	slk::SLK* slk = category_slk(category);
	if (slk == nullptr) {
		return 0;
	}
	const auto row = slk->shadow_data.find(id);
	if (row == slk->shadow_data.end()) {
		return 0;
	}
	return row->second.contains("oldid") ? 1 : 0;
}

const char* hive_unique_object_id(void* handle, const char* source_id) {
	if (handle == nullptr || source_id == nullptr || source_id[0] == '\0') {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->transient_string = get_unique_id(!std::islower(static_cast<unsigned char>(source_id[0])));
	return hive->transient_string.c_str();
}

const char* hive_map_name(void* handle) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->transient_string = hive->map->trigger_strings.string(hive->map->info.name);
	return hive->transient_string.c_str();
}

int32_t hive_set_map_name(void* handle, const char* value) {
	if (handle == nullptr || value == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->map->trigger_strings.set_string(hive->map->info.name, value);
	return 0;
}

const char* hive_map_author(void* handle) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->transient_string = hive->map->trigger_strings.string(hive->map->info.author);
	return hive->transient_string.c_str();
}

int32_t hive_set_map_author(void* handle, const char* value) {
	if (handle == nullptr || value == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->map->trigger_strings.set_string(hive->map->info.author, value);
	return 0;
}

const char* hive_map_description(void* handle) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->transient_string = hive->map->trigger_strings.string(hive->map->info.description);
	return hive->transient_string.c_str();
}

int32_t hive_set_map_description(void* handle, const char* value) {
	if (handle == nullptr || value == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->map->trigger_strings.set_string(hive->map->info.description, value);
	return 0;
}

const char* hive_map_suggested_players(void* handle) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->transient_string = hive->map->trigger_strings.string(hive->map->info.suggested_players);
	return hive->transient_string.c_str();
}

int32_t hive_set_map_suggested_players(void* handle, const char* value) {
	if (handle == nullptr || value == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->map->trigger_strings.set_string(hive->map->info.suggested_players, value);
	return 0;
}

void hive_map_versions(void* handle, int32_t* out) {
	if (handle == nullptr || out == nullptr) {
		return;
	}
	const MapInfo& info = static_cast<HiveHandle*>(handle)->map->info;
	out[0] = info.map_version;
	out[1] = info.editor_version;
	out[2] = info.game_version_major;
	out[3] = info.game_version_minor;
	out[4] = info.game_version_patch;
	out[5] = info.game_version_build;
}

void hive_map_playable_size(void* handle, int32_t* width, int32_t* height) {
	if (handle == nullptr || width == nullptr || height == nullptr) {
		return;
	}
	const MapInfo& info = static_cast<HiveHandle*>(handle)->map->info;
	*width = info.playable_width;
	*height = info.playable_height;
}

int32_t hive_resize_map(void* handle, const int32_t delta_left, const int32_t delta_right, const int32_t delta_top,
						const int32_t delta_bottom) {
	if (handle == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const int new_width = hive->map->terrain.width - 1 + delta_left + delta_right;
	const int new_height = hive->map->terrain.height - 1 + delta_top + delta_bottom;
	if (new_width < 32 || new_width > 480 || new_height < 32 || new_height > 480) {
		return -1;
	}

	hive->map->resize(delta_left, delta_right, delta_top, delta_bottom);
	return 0;
}

int32_t hive_set_playable_area(void* handle, const int32_t left, const int32_t right, const int32_t top,
							   const int32_t bottom) {
	if (handle == nullptr || left < 0 || right < 0 || top < 0 || bottom < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->map->set_playable_area(left, right, top, bottom);
	return 0;
}

int32_t hive_trigger_category_count(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(static_cast<HiveHandle*>(handle)->map->triggers.categories.size());
}

int32_t hive_trigger_category_id(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	const auto& list = static_cast<HiveHandle*>(handle)->map->triggers.categories;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return -1;
	}
	return list[static_cast<std::size_t>(index)].id;
}

const char* hive_trigger_category_name(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& list = static_cast<HiveHandle*>(handle)->map->triggers.categories;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return nullptr;
	}
	return list[static_cast<std::size_t>(index)].name.c_str();
}

int32_t hive_trigger_category_parent(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return 0;
	}
	const auto& list = static_cast<HiveHandle*>(handle)->map->triggers.categories;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return 0;
	}
	return list[static_cast<std::size_t>(index)].parent_id;
}

int32_t hive_trigger_count(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(static_cast<HiveHandle*>(handle)->map->triggers.triggers.size());
}

Trigger* trigger_at(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto& list = static_cast<HiveHandle*>(handle)->map->triggers.triggers;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return nullptr;
	}
	return &list[static_cast<std::size_t>(index)];
}

int32_t hive_trigger_id(void* handle, const int32_t index) {
	const Trigger* trigger = trigger_at(handle, index);
	return trigger == nullptr ? -1 : trigger->id;
}

const char* hive_trigger_name(void* handle, const int32_t index) {
	const Trigger* trigger = trigger_at(handle, index);
	return trigger == nullptr ? nullptr : trigger->name.c_str();
}

const char* hive_trigger_description(void* handle, const int32_t index) {
	const Trigger* trigger = trigger_at(handle, index);
	return trigger == nullptr ? nullptr : trigger->description.c_str();
}

int32_t hive_trigger_parent(void* handle, const int32_t index) {
	const Trigger* trigger = trigger_at(handle, index);
	return trigger == nullptr ? 0 : trigger->parent_id;
}

int32_t hive_trigger_is_comment(void* handle, const int32_t index) {
	const Trigger* trigger = trigger_at(handle, index);
	return trigger != nullptr && trigger->is_comment ? 1 : 0;
}

int32_t hive_trigger_is_enabled(void* handle, const int32_t index) {
	const Trigger* trigger = trigger_at(handle, index);
	return trigger != nullptr && trigger->is_enabled ? 1 : 0;
}

int32_t hive_trigger_initially_on(void* handle, const int32_t index) {
	const Trigger* trigger = trigger_at(handle, index);
	return trigger != nullptr && trigger->initially_on ? 1 : 0;
}

int32_t hive_trigger_eca_count(void* handle, const int32_t index, const int32_t type) {
	const Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr) {
		return 0;
	}
	int count = 0;
	for (const ECA& eca : trigger->ecas) {
		if (static_cast<int32_t>(eca.type) == type) {
			count++;
		}
	}
	return count;
}

const ECA* eca_at(void* handle, const int32_t index, const int32_t type, const int32_t eca_index) {
	const Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr || eca_index < 0) {
		return nullptr;
	}
	int seen = 0;
	for (const ECA& eca : trigger->ecas) {
		if (static_cast<int32_t>(eca.type) != type) {
			continue;
		}
		if (seen == eca_index) {
			return &eca;
		}
		seen++;
	}
	return nullptr;
}

const char* hive_trigger_eca_name(void* handle, const int32_t index, const int32_t type, const int32_t eca_index) {
	const ECA* eca = eca_at(handle, index, type, eca_index);
	return eca == nullptr ? nullptr : eca->name.c_str();
}

int32_t hive_trigger_eca_parameter_count(void* handle, const int32_t index, const int32_t type, const int32_t eca_index) {
	const ECA* eca = eca_at(handle, index, type, eca_index);
	return eca == nullptr ? 0 : static_cast<int32_t>(eca->parameters.size());
}

int32_t hive_trigger_eca_parameter_type(void* handle, const int32_t index, const int32_t type, const int32_t eca_index,
										const int32_t parameter) {
	const ECA* eca = eca_at(handle, index, type, eca_index);
	if (eca == nullptr || parameter < 0 || static_cast<std::size_t>(parameter) >= eca->parameters.size()) {
		return -1;
	}
	return static_cast<int32_t>(eca->parameters[static_cast<std::size_t>(parameter)].type);
}

const char* hive_trigger_eca_parameter_value(void* handle, const int32_t index, const int32_t type,
											 const int32_t eca_index, const int32_t parameter) {
	const ECA* eca = eca_at(handle, index, type, eca_index);
	if (eca == nullptr || parameter < 0 || static_cast<std::size_t>(parameter) >= eca->parameters.size()) {
		return nullptr;
	}
	return eca->parameters[static_cast<std::size_t>(parameter)].value.c_str();
}

const char* eca_section(const int32_t type) {
	switch (type) {
		case 0:
			return "TriggerEvents";
		case 1:
			return "TriggerConditions";
		case 2:
			return "TriggerActions";
		default:
			return nullptr;
	}
}

const std::vector<std::string>* eca_definition(HiveHandle* hive, const int32_t type, const std::string& name) {
	const char* section = eca_section(type);
	if (section == nullptr || !hive->map->triggers.trigger_data.section_exists(section)) {
		return nullptr;
	}
	const auto& data = hive->map->triggers.trigger_data.section(section);
	const auto found = data.find(name);
	return found == data.end() ? nullptr : &found->second;
}

int32_t hive_set_trigger_eca_parameter_value(void* handle, const int32_t index, const int32_t type, const int32_t eca_index,
											 const int32_t parameter, const char* value) {
	if (handle == nullptr || value == nullptr) {
		return -1;
	}
	const ECA* const_eca = eca_at(handle, index, type, eca_index);
	if (const_eca == nullptr || parameter < 0
		|| static_cast<std::size_t>(parameter) >= const_eca->parameters.size()) {
		return -1;
	}

	auto* hive = static_cast<HiveHandle*>(handle);
	Trigger* trigger = trigger_at(handle, index);
	int seen = 0;
	for (ECA& eca : trigger->ecas) {
		if (static_cast<int32_t>(eca.type) != type) {
			continue;
		}
		if (seen == eca_index) {
			TriggerParameter& entry = eca.parameters[static_cast<std::size_t>(parameter)];
			entry.value = value;
			if (entry.type == TriggerParameter::Type::invalid) {
				entry.type = TriggerParameter::Type::preset;
			}
			return 0;
		}
		seen++;
	}
	return -1;
}

const char* hive_trigger_eca_parameter_type_name(void* handle, const int32_t index, const int32_t type,
												 const int32_t eca_index, const int32_t parameter) {
	const ECA* eca = eca_at(handle, index, type, eca_index);
	if (eca == nullptr || parameter < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const std::vector<std::string>* definition = eca_definition(hive, type, eca->name);
	if (definition == nullptr) {
		return nullptr;
	}

	int seen = 0;
	for (std::size_t i = 1; i < definition->size(); ++i) {
		const std::string& entry = (*definition)[i];
		if (entry.empty() || is_number(entry) || entry == "nothing") {
			continue;
		}
		if (seen == parameter) {
			return entry.c_str();
		}
		seen++;
	}
	return nullptr;
}

int32_t hive_trigger_preset_count(void* handle, const char* type_name) {
	if (handle == nullptr || type_name == nullptr) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (!hive->map->triggers.trigger_data.section_exists("TriggerParams")) {
		return 0;
	}
	int count = 0;
	for (const auto& [key, value] : hive->map->triggers.trigger_data.section("TriggerParams")) {
		if (value.size() >= 2 && value[1] == type_name) {
			count++;
		}
	}
	return count;
}

const std::string* preset_at(HiveHandle* hive, const char* type_name, const int32_t index, const int32_t field) {
	if (type_name == nullptr || !hive->map->triggers.trigger_data.section_exists("TriggerParams")) {
		return nullptr;
	}
	int seen = 0;
	for (const auto& [key, value] : hive->map->triggers.trigger_data.section("TriggerParams")) {
		if (value.size() < 2 || value[1] != type_name) {
			continue;
		}
		if (seen == index) {
			if (static_cast<std::size_t>(field) >= value.size()) {
				hive->transient_string.clear();
				return &hive->transient_string;
			}
			hive->transient_string = value[static_cast<std::size_t>(field)];
			return &hive->transient_string;
		}
		seen++;
	}
	return nullptr;
}

const char* hive_trigger_preset_code(void* handle, const char* type_name, const int32_t index) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const std::string* value = preset_at(hive, type_name, index, 2);
	return value == nullptr ? nullptr : value->c_str();
}

const char* hive_trigger_preset_display(void* handle, const char* type_name, const int32_t index) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const std::string* value = preset_at(hive, type_name, index, 3);
	return value == nullptr ? nullptr : value->c_str();
}

int32_t hive_trigger_function_count(void* handle, const int32_t type) {
	if (handle == nullptr) {
		return 0;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const char* section = eca_section(type);
	if (section == nullptr || !hive->map->triggers.trigger_data.section_exists(section)) {
		return 0;
	}
	int count = 0;
	for (const auto& [key, value] : hive->map->triggers.trigger_data.section(section)) {
		if (!key.empty() && key.front() != '_') {
			count++;
		}
	}
	return count;
}

const char* hive_trigger_function_name(void* handle, const int32_t type, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const char* section = eca_section(type);
	if (section == nullptr || !hive->map->triggers.trigger_data.section_exists(section)) {
		return nullptr;
	}
	int seen = 0;
	for (const auto& [key, value] : hive->map->triggers.trigger_data.section(section)) {
		if (key.empty() || key.front() == '_') {
			continue;
		}
		if (seen == index) {
			return key.c_str();
		}
		seen++;
	}
	return nullptr;
}

int32_t hive_trigger_add_eca(void* handle, const int32_t index, const int32_t type, const char* name) {
	if (handle == nullptr || name == nullptr) {
		return -1;
	}
	Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const std::vector<std::string>* definition = eca_definition(hive, type, name);
	if (definition == nullptr) {
		return -1;
	}

	ECA eca;
	eca.type = static_cast<ECA::Type>(type);
	eca.group = 0;
	eca.name = name;
	eca.enabled = true;

	int parameter_count = 0;
	for (std::size_t i = 1; i < definition->size(); ++i) {
		const std::string& entry = (*definition)[i];
		if (!entry.empty() && !is_number(entry) && entry != "nothing") {
			parameter_count++;
		}
	}
	eca.parameters.resize(static_cast<std::size_t>(parameter_count));

	trigger->ecas.push_back(std::move(eca));

	int seen = 0;
	for (const ECA& item : trigger->ecas) {
		if (static_cast<int32_t>(item.type) == type) {
			seen++;
		}
	}
	return seen - 1;
}

int32_t hive_trigger_remove_eca(void* handle, const int32_t index, const int32_t type, const int32_t eca_index) {
	Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr || eca_index < 0) {
		return -1;
	}

	int seen = 0;
	for (auto iterator = trigger->ecas.begin(); iterator != trigger->ecas.end(); ++iterator) {
		if (static_cast<int32_t>(iterator->type) != type) {
			continue;
		}
		if (seen == eca_index) {
			trigger->ecas.erase(iterator);
			return 0;
		}
		seen++;
	}
	return -1;
}

int32_t hive_create_trigger(void* handle, const char* name, const int32_t parent_id) {
	if (handle == nullptr) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto& list = hive->map->triggers.triggers;

	Trigger trigger;
	trigger.classifier = Classifier::gui;
	trigger.name = name != nullptr && name[0] != '\0' ? name : "New Trigger";
	trigger.id = ++Trigger::next_id;
	trigger.parent_id = parent_id;
	list.push_back(std::move(trigger));

	return static_cast<int32_t>(list.size() - 1);
}

int32_t hive_delete_trigger(void* handle, const int32_t index) {
	Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr) {
		return -1;
	}
	auto& list = static_cast<HiveHandle*>(handle)->map->triggers.triggers;
	list.erase(list.begin() + index);
	return 0;
}

int32_t hive_set_trigger_name(void* handle, const int32_t index, const char* value) {
	Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr || value == nullptr) {
		return -1;
	}
	trigger->name = value;
	return 0;
}

int32_t hive_set_trigger_description(void* handle, const int32_t index, const char* value) {
	Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr || value == nullptr) {
		return -1;
	}
	trigger->description = value;
	return 0;
}

int32_t hive_set_trigger_enabled(void* handle, const int32_t index, const int32_t enabled) {
	Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr) {
		return -1;
	}
	trigger->is_enabled = enabled != 0;
	return 0;
}

int32_t hive_set_trigger_initially_on(void* handle, const int32_t index, const int32_t on) {
	Trigger* trigger = trigger_at(handle, index);
	if (trigger == nullptr) {
		return -1;
	}
	trigger->initially_on = on != 0;
	return 0;
}

int32_t hive_trigger_variable_count(void* handle) {
	if (handle == nullptr) {
		return 0;
	}
	return static_cast<int32_t>(static_cast<HiveHandle*>(handle)->map->triggers.variables.size());
}

const char* hive_trigger_variable_name(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& list = static_cast<HiveHandle*>(handle)->map->triggers.variables;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return nullptr;
	}
	return list[static_cast<std::size_t>(index)].name.c_str();
}

const char* hive_trigger_variable_type(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& list = static_cast<HiveHandle*>(handle)->map->triggers.variables;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return nullptr;
	}
	return list[static_cast<std::size_t>(index)].type.c_str();
}

int32_t hive_trigger_variable_is_array(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return 0;
	}
	const auto& list = static_cast<HiveHandle*>(handle)->map->triggers.variables;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return 0;
	}
	return list[static_cast<std::size_t>(index)].is_array ? 1 : 0;
}

const char* hive_trigger_variable_initial(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return nullptr;
	}
	const auto& list = static_cast<HiveHandle*>(handle)->map->triggers.variables;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return nullptr;
	}
	return list[static_cast<std::size_t>(index)].initial_value.c_str();
}

const char* hive_map_global_script(void* handle) {
	if (handle == nullptr) {
		return nullptr;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	hive->transient_string = hive->map->triggers.global_jass;
	return hive->transient_string.c_str();
}

int32_t hive_set_map_global_script(void* handle, const char* value) {
	if (handle == nullptr || value == nullptr) {
		return -1;
	}
	static_cast<HiveHandle*>(handle)->map->triggers.global_jass = value;
	return 0;
}

}
