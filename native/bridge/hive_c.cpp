#include "hive_c.h"

#include <algorithm>
#include <cstdio>
#include <unordered_map>
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
	bool dragging = false;
	bool drag_is_unit = false;
	int32_t drag_index = -1;
	Doodad drag_doodad;
	Unit drag_unit;
	std::unordered_map<std::string, blp::Image> images;
	std::string transient_string;
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
	const std::string key = doodad.id + "#" + std::to_string(doodad.variation);

	if (const auto found = hive->model_keys.find(key); found != hive->model_keys.end()) {
		return found->second;
	}

	auto mesh = hive->map->doodads.get_mesh(doodad.id, doodad.variation);
	if (!mesh || !mesh->mdx) {
		hive->model_keys[key] = -1;
		return -1;
	}

	hive->models.push_back(build_model(mesh->mdx));
	const auto index = static_cast<int32_t>(hive->models.size() - 1);
	hive->model_keys[key] = index;
	return index;
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

int32_t hive_begin_move(void* handle, const int32_t is_unit, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	if (is_unit != 0) {
		const auto& list = hive->map->units.units;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}
		hive->drag_unit = list[static_cast<std::size_t>(index)];
	} else {
		const auto& list = hive->map->doodads.doodads;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}
		hive->drag_doodad = list[static_cast<std::size_t>(index)];
	}

	hive->dragging = true;
	hive->drag_is_unit = is_unit != 0;
	hive->drag_index = index;
	return 0;
}

int32_t hive_move_to(void* handle, const int32_t is_unit, const int32_t index, const float x, const float y) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const float z = hive->map->terrain.interpolated_height(x, y, true);

	if (is_unit != 0) {
		auto& list = hive->map->units.units;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}
		list[static_cast<std::size_t>(index)].position = glm::vec3(x, y, z);
	} else {
		auto& list = hive->map->doodads.doodads;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}
		list[static_cast<std::size_t>(index)].position = glm::vec3(x, y, z);
		list[static_cast<std::size_t>(index)].update(hive->map->terrain);
	}
	return 0;
}

int32_t hive_end_move(void* handle, const int32_t is_unit, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	if (!hive->dragging || hive->drag_is_unit != (is_unit != 0) || hive->drag_index != index) {
		return -1;
	}
	hive->dragging = false;

	if (is_unit != 0) {
		const auto& list = hive->map->units.units;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}

		auto action = std::make_unique<UnitStateAction>();
		action->old_units.push_back(hive->drag_unit);
		action->new_units.push_back(list[static_cast<std::size_t>(index)]);

		hive->map->world_undo.new_undo_group();
		hive->map->world_undo.add_undo_action(std::move(action));
	} else {
		const auto& list = hive->map->doodads.doodads;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}

		auto action = std::make_unique<DoodadStateAction>();
		action->old_doodads.push_back(hive->drag_doodad);
		action->new_doodads.push_back(list[static_cast<std::size_t>(index)]);

		hive->map->world_undo.new_undo_group();
		hive->map->world_undo.add_undo_action(std::move(action));
	}

	return 0;
}

int32_t hive_set_object_angle(void* handle, const int32_t is_unit, const int32_t index, const float target_angle) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);

	if (is_unit != 0) {
		auto& list = hive->map->units.units;
		if (static_cast<std::size_t>(index) >= list.size()) {
			return -1;
		}
		list[static_cast<std::size_t>(index)].angle = target_angle;
		return 0;
	}

	auto& list = hive->map->doodads.doodads;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return -1;
	}

	Doodad& doodad = list[static_cast<std::size_t>(index)];
	doodad.angle = Doodad::acceptable_angle(doodad.id, doodad.pathing, doodad.angle, target_angle);
	doodad.position = glm::vec3(Doodad::acceptable_position(doodad.position, doodad.pathing, doodad.angle),
								doodad.position.z);
	doodad.update(hive->map->terrain);
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
		std::sort(hive->doodad_types.begin(), hive->doodad_types.end());
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

	auto mesh = hive->map->doodads.get_mesh(id, 0);
	if (!mesh || !mesh->mdx) {
		return -1;
	}

	Doodad doodad;
	doodad.id = id;
	doodad.variation = 0;
	doodad.position = glm::vec3(x, y, z);
	doodad.angle = angle;
	doodad.scale = glm::vec3(1.f);
	doodad.init(id, mesh, hive->map->terrain);

	hive->map->doodads.doodads.push_back(std::move(doodad));
	return static_cast<int32_t>(hive->map->doodads.doodads.size() - 1);
}

int32_t hive_remove_doodad(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto& list = hive->map->doodads.doodads;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return -1;
	}

	auto action = std::make_unique<DoodadDeleteAction>();
	action->doodads.push_back(list[static_cast<std::size_t>(index)]);

	hive->map->world_undo.new_undo_group();
	hive->map->world_undo.add_undo_action(std::move(action));

	list.erase(list.begin() + index);
	return 0;
}

int32_t hive_remove_unit(void* handle, const int32_t index) {
	if (handle == nullptr || index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	auto& list = hive->map->units.units;
	if (static_cast<std::size_t>(index) >= list.size()) {
		return -1;
	}

	auto action = std::make_unique<UnitDeleteAction>();
	action->units.push_back(list[static_cast<std::size_t>(index)]);

	hive->map->world_undo.new_undo_group();
	hive->map->world_undo.add_undo_action(std::move(action));

	list.erase(list.begin() + index);
	return 0;
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

int32_t hive_model_from_unit(void* handle, const int32_t unit_index) {
	if (handle == nullptr || unit_index < 0) {
		return -1;
	}
	auto* hive = static_cast<HiveHandle*>(handle);
	const auto& source = hive->map->units.units;
	if (static_cast<std::size_t>(unit_index) >= source.size()) {
		return -1;
	}

	const Unit& unit = source[static_cast<std::size_t>(unit_index)];
	const std::string key = "unit:" + unit.id;

	if (const auto found = hive->model_keys.find(key); found != hive->model_keys.end()) {
		return found->second;
	}

	auto mesh = hive->map->units.get_mesh(unit.id);
	if (!mesh || !mesh->mdx) {
		hive->model_keys[key] = -1;
		return -1;
	}

	hive->models.push_back(build_model(mesh->mdx));
	const auto index = static_cast<int32_t>(hive->models.size() - 1);
	hive->model_keys[key] = index;
	return index;
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

}
