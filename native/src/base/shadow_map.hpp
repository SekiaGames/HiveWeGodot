#pragma once


#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <codecvt>
#include <compare>
#include <complex>
#include <concepts>
#include <coroutine>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <cwctype>
#include <deque>
#include <exception>
#include <execution>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <ostream>
#include <print>
#include <queue>
#include <random>
#include <ranges>
#include <ratio>
#include <regex>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <stop_token>
#include <streambuf>
#include <string>
#include <string_view>
#include <syncstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
#include "types.hpp"
#include "hierarchy.hpp"
#include "binary_reader.hpp"

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
