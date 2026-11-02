#pragma once

#include <string>
#include <vector>

#include "terrain.hpp"

/* terrain.hpp 里标了 friend，用来读 Terrain 的私有成员 */
struct HiveBridge {
	static const std::vector<std::shared_ptr<GroundTexture>>& ground_textures(const Terrain& terrain) {
		return terrain.ground_textures;
	}

	static const std::vector<float>& heights(const Terrain& terrain) {
		return terrain.gpu_final_ground_heights;
	}

	static const std::vector<glm::uvec4>& tiles(const Terrain& terrain) {
		return terrain.gpu_ground_texture_list;
	}

	static const std::vector<uint8_t>& ground_exists(const Terrain& terrain) {
		return terrain.gpu_ground_exists_data;
	}

	static const std::vector<uint8_t>& water_exists(const Terrain& terrain) {
		return terrain.gpu_water_exists_data;
	}

	static const std::vector<std::string>& tileset_ids(const Terrain& terrain) {
		return terrain.tileset_ids;
	}
};
