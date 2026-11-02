#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "mpq.hpp"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
	if (argc < 4) {
		std::printf("用法: extract_assets <游戏目录> <清单文件> <输出目录>\n");
		return 1;
	}

	const fs::path game_dir = argv[1];
	const fs::path list_path = argv[2];
	const fs::path out_dir = argv[3];

	mpq::MPQ archive;
	if (!archive.open(game_dir / "WAR3.MPQ")) {
		std::printf("打不开 WAR3.MPQ\n");
		return 1;
	}
	for (const char* patch : {"War3x.mpq", "War3xlocal.mpq", "War3Patch.mpq", "Patch127.mpq"}) {
		archive.add_patch(game_dir / patch);
	}

	std::ifstream list(list_path);
	std::string line;
	int extracted = 0;
	int skipped = 0;

	while (std::getline(list, line)) {
		if (line.empty()) {
			continue;
		}

		const auto data = archive.open_file(line);
		if (!data) {
			++skipped;
			continue;
		}

		std::string normalized = line;
		std::ranges::replace(normalized, '\\', '/');
		const fs::path target = out_dir / normalized;
		fs::create_directories(target.parent_path());

		std::ofstream out(target, std::ios::binary);
		out.write(reinterpret_cast<const char*>(data->buffer.data()), static_cast<std::streamsize>(data->buffer.size()));
		++extracted;
	}

	std::printf("提取 %d 个，跳过 %d 个\n", extracted, skipped);
	return 0;
}
