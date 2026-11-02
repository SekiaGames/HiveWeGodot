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

namespace fs = std::filesystem;

namespace paths {

	/// Returns the path to the HiveWE data folder inside the map directory
	fs::path hive_data_folder(const fs::path& map_directory) {
		return map_directory / "hiveWE";
	}

	/// Returns the path to the terrain pathing override file
	fs::path terrain_pathing_file(const fs::path& map_directory) {
		return hive_data_folder(map_directory) / "terrain_pathing.json";
	}

	/// Returns the path to the map info file
	fs::path map_info_extras_file(const fs::path& map_directory) {
		return hive_data_folder(map_directory) / "map_info_extras.json";
	}

} // namespace paths
