#include <cstdio>
#include <exception>
#include <filesystem>

#include "globals.hpp"
#include "map.hpp"

struct HiveBridge {
	static const auto& ground_textures(const Terrain& terrain) { return terrain.ground_textures; }
	static const auto& heights(const Terrain& terrain) { return terrain.gpu_final_ground_heights; }
	static const auto& tiles(const Terrain& terrain) { return terrain.gpu_ground_texture_list; }
};

namespace fs = std::filesystem;

int main(int argc, char** argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	const fs::path game_dir = argc > 1 ? argv[1] : "/Volumes/Data/Work/Open-Warcraft3/Warcraft3-11hf";
	const fs::path map_path = argc > 2 ? argv[2] : "/Volumes/Data/Work/Open-Warcraft3/Warcraft3-11hf/Maps/(2)BootyBay.w3m";

	if (!hierarchy.game_data.open(game_dir / "WAR3.MPQ")) {
		std::printf("打不开 WAR3.MPQ\n");
		return 1;
	}

	for (const char* patch : {"War3x.mpq", "War3xlocal.mpq", "War3Patch.mpq", "Patch127.mpq"}) {
		if (!hierarchy.game_data.add_patch(game_dir / patch)) {
			std::printf("叠加 %s 失败\n", patch);
		}
	}

	hierarchy.root_directory = game_dir;
	hierarchy.remastered = false;
	hierarchy.allow_local_files = true;

	thread_pool.init();
	world_edit_strings.load("UI/WorldEditStrings.txt");
	world_edit_game_strings.load("UI/WorldEditGameStrings.txt");
	world_edit_data.load("UI/WorldEditData.txt");
	world_edit_data.substitute(world_edit_game_strings, "WorldEditStrings");
	world_edit_data.substitute(world_edit_strings, "WorldEditStrings");

	std::printf("游戏数据就绪，解包 %s\n", map_path.filename().string().c_str());

	const fs::path unpack_dir = fs::temp_directory_path() / ("hivewe_" + map_path.stem().string());
	{
		mpq::MPQ archive;
		if (!archive.open(map_path)) {
			std::printf("打不开地图归档\n");
			return 1;
		}
		if (!archive.unpack(unpack_dir)) {
			std::printf("解包失败\n");
			return 1;
		}
	}
	std::printf("解包到 %s\n", unpack_dir.string().c_str());

	Map map;
	try {
		map.load(unpack_dir);
	} catch (const std::exception& e) {
		std::printf("加载中断：%s\n", e.what());
	}

	std::printf("地图 %s：%d×%d tile，tileset 0x%02X\n",
				map.name.c_str(), map.terrain.width, map.terrain.height, map.terrain.tileset_id);
	std::printf("地面纹理 %zu 种\n", HiveBridge::ground_textures(map.terrain).size());
	std::printf("装饰物 %zu，单位 %zu，区域 %zu\n",
				map.doodads.doodads.size(), map.units.units.size(), map.regions.regions.size());
	std::printf("高度数组 %zu，纹理数组 %zu\n",
				HiveBridge::heights(map.terrain).size(), HiveBridge::tiles(map.terrain).size());
	return 0;
}
