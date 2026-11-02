#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "globals.hpp"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
	const fs::path game_dir = argc > 1 ? argv[1] : "/Volumes/Data/Work/Open-Warcraft3/Warcraft3-11hf";

	if (!hierarchy.game_data.open(game_dir / "WAR3.MPQ")) {
		std::printf("打不开 WAR3.MPQ\n");
		return 1;
	}
	for (const char* patch : {"War3x.mpq", "War3xlocal.mpq", "War3Patch.mpq", "Patch127.mpq"}) {
		std::printf("叠加 %s: %d\n", patch, hierarchy.game_data.add_patch(game_dir / patch));
	}

	hierarchy.root_directory = game_dir;
	hierarchy.remastered = false;
	hierarchy.allow_local_files = true;

	{
		auto direct = hierarchy.game_data.open_file("war3.w3mod:Units/UnitData.slk");
		std::printf("直连带前缀: %s\n", direct ? "OK" : direct.error().c_str());
		auto plain = hierarchy.game_data.open_file("Units/UnitData.slk");
		std::printf("直连无前缀: %s\n", plain ? "OK" : plain.error().c_str());
		std::printf("file_exists 带前缀: %d\n", hierarchy.game_data.file_exists("war3.w3mod:Units/UnitData.slk"));
	}

	{
		for (const char* pattern : {"Units\\Item*", "Units\\item*", "Units\\*.slk"}) {
			auto files = hierarchy.game_data.find_files(pattern);
			std::printf("模式 %s 命中 %zu 个：", pattern, files.size());
			for (size_t i = 0; i < std::min<size_t>(files.size(), 8); ++i) {
				std::printf("%s ", files[i].c_str());
			}
			std::printf("\n");
		}
	}

	for (const char* name : {"Units/ItemMetaData.slk", "Units/ItemData.slk", "Units/ItemFunc.txt", "Units/UnitEditorData.txt", "Units/UnitSkin.txt", "Units/UnitWeaponsFunc.txt",
							 "Units/UnitBalance.slk", "Units/unitUI.slk", "Units/HumanUnitFunc.txt",
							 "Units/UnitMetaData.slk", "UI/WorldEditStrings.txt",
							 "TerrainArt\\Terrain.slk", "Units\\UnitData.slk"}) {
		auto result = hierarchy.open_file(name);
		std::printf("%-28s %s\n", name, result ? "OK" : result.error().c_str());
	}
	return 0;
}
