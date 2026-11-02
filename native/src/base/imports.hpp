#pragma once


#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <cassert>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <print>
#include <queue>
#include <random>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include "types.hpp"
#include "BinaryWriter.hpp"
#include "Hierarchy.hpp"
#include "Paths.hpp"

namespace fs = std::filesystem;

// HiveWE does not use the war3map.imp file at all, but the old WE does
// That's why we write the file at save time
class Imports {
  public:
	const std::unordered_set<std::string> blacklist = {
		"conversation.json",
		"war3mapUnits.doo",
		"war3map.doo",
		"war3map.imp",
		"war3map.j",
		"war3map.lua",
		"war3map.mmp",
		"war3map.shd",
		"war3map.w3a",
		"war3map.w3b",
		"war3map.w3c",
		"war3map.w3d",
		"war3map.w3e",
		"war3map.w3h",
		"war3map.w3i",
		"war3map.w3q",
		"war3map.w3r",
		"war3map.w3t",
		"war3map.w3u",
		"war3map.wct",
		"war3map.wpm",
		"war3map.wtg",
		"war3map.wts",
		"war3map.w3s",
		"war3mapMap.blp",
		"war3mapExtra.txt",
		"war3mapMisc.txt",
		"war3mapSkin.txt",
		"war3mapSkin.w3a",
		"war3mapSkin.w3b",
		"war3mapSkin.w3d",
		"war3mapSkin.w3h",
		"war3mapSkin.w3q",
		"war3mapSkin.w3t",
		"war3mapSkin.w3u",
		"war3mapPreview.tga",

		// HiveWE editor files
		paths::terrain_pathing_file(hierarchy.map_directory).filename().string(),
		paths::map_info_extras_file(hierarchy.map_directory).filename().string(),
	};

	/// Requires the filesystem_path for the map to make the saved paths lexically relative
	void save(const fs::path& filesystem_path) const {
		BinaryWriter writer;

		writer.write<u32>(1);

		int count = 0;
		for (const auto& i : fs::recursive_directory_iterator(filesystem_path)) {
			if (i.is_regular_file()) {
				std::string file_name = i.path().filename().string();
				if (blacklist.contains(file_name)) {
					continue;
				}
				count++;
			}
		}
		writer.write<u32>(count);

		for (const auto& i : fs::recursive_directory_iterator(filesystem_path)) {
			if (i.is_regular_file()) {
				std::string path = i.path().lexically_relative(filesystem_path).string();
				std::string file_name = i.path().filename().string();
				if (blacklist.contains(file_name)) {
					continue;
				}

				writer.write<u8>(13);
				writer.write_c_string(path);
			}
		}

		hierarchy.map_file_write("war3map.imp", writer.buffer);
	}
};
