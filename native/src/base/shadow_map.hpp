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
#include "Hierarchy.hpp"
#include "BinaryReader.hpp"

class ShadowMap {
	size_t width;
	size_t height;

	std::vector<u8> cells;

  public:
	bool load(size_t terrain_width, size_t terrain_height) {
		BinaryReader reader = hierarchy.map_file_read("war3map.shd").value();

		width = terrain_width * 4;
		height = terrain_height * 4;

		// check if the shadow map is correct size
		const size_t expected_size = width * height;
		const size_t file_size = reader.buffer.size();
		if (file_size != expected_size) {
			std::println("Error: Shadow map file size mismatch!");
			cells.resize(expected_size, 0);
		} else {
			cells = reader.read_vector<u8>(expected_size);
		}

		return true;
	}

	void save() const {
		hierarchy.map_file_write("war3map.shd", cells);
	}

	void resize(const size_t new_width, const size_t new_height) {
		width = new_width;
		height = new_height;
		cells.resize(width * height, 0);
	}

	void resize(const int delta_left, const int delta_right, const int delta_top, const int delta_bottom) {
		const size_t new_width = static_cast<size_t>(static_cast<int>(width) + delta_left + delta_right);
		const size_t new_height = static_cast<size_t>(static_cast<int>(height) + delta_top + delta_bottom);

		width = new_width;
		height = new_height;
		cells.resize(width * height, 0);
	}
};
