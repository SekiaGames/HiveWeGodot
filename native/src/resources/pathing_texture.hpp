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
#include "resource_manager.hpp"
#include "hierarchy.hpp"
#include "blp.hpp"
#include "binary_reader.hpp"
#include <soil2/SOIL2.h>
#include <glm/glm.hpp>

namespace fs = std::filesystem;

class PathingTexture: public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<u8> data;

	bool homogeneous;

	static constexpr const char* name = "PathingTexture";

	explicit PathingTexture(const fs::path& path) {
		BinaryReader reader = hierarchy.open_file(path, Hierarchy::FileSource::all, {".dds", ".blp", ".tga"}).value();

		if (blp::is_blp(reader)) {
			auto image = blp::load(reader);
			if (!image) {
				throw std::runtime_error(std::format("Failed to load pathing texture {}: {}", path.string(), image.error()));
			}
			width = image->width;
			height = image->height;
			channels = image->channels;
			data = std::move(image->data);
		} else {
			uint8_t* image_data = SOIL_load_image_from_memory(
				reader.buffer.data(),
				static_cast<int>(reader.buffer.size()),
				&width,
				&height,
				&channels,
				SOIL_LOAD_AUTO
			);
			if (image_data == nullptr) {
				throw std::runtime_error(std::format("Failed to decode pathing texture {}", path.string()));
			}
			data = std::vector<u8>(image_data, image_data + static_cast<size_t>(width) * height * channels);
			free(image_data);
		}

		homogeneous = true;
		for (size_t i = 0; i < data.size(); i += channels) {
			if (channels == 3) {
				homogeneous =
					homogeneous && *reinterpret_cast<glm::u8vec3*>(data.data() + i) == *reinterpret_cast<glm::u8vec3*>(data.data());
			} else if (channels == 4) {
				homogeneous =
					homogeneous && *reinterpret_cast<glm::u8vec4*>(data.data() + i) == *reinterpret_cast<glm::u8vec4*>(data.data());
			}
		}
	}
};
