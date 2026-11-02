#pragma once


#include <vector>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <utility>


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
#include "hierarchy.hpp"
#include "blp.hpp"
#include "binary_reader.hpp"
#include "resource_manager.hpp"
#include <soil2/SOIL2.h>

namespace fs = std::filesystem;

fs::path diffuse_texture_path(const fs::path& path) {
	if (!hierarchy.remastered) {
		return path;
	}
	fs::path new_path = path;
	new_path.replace_filename(path.stem().string() + "_diffuse");
	return new_path;
}

class Texture : public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;

	static constexpr const char* name = "Texture";

	explicit Texture() = default;
	explicit Texture(const fs::path& path) {
		BinaryReader reader = hierarchy.open_file(path, Hierarchy::FileSource::all, { ".dds", ".blp", ".tga" })
			.or_else([&](const std::string&) {
				std::println("Error loading texture {}", path.string());
				return hierarchy.open_file("Textures/btntempw.dds");
			})
			.value();

		if (blp::is_blp(reader)) {
			auto image = blp::load(reader);
			if (!image) {
				throw std::runtime_error(std::format("Failed to load texture {}: {}", path.string(), image.error()));
			}
			width = image->width;
			height = image->height;
			channels = image->channels;
			data = std::move(image->data);
		} else {
			uint8_t* image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
			if (image_data == nullptr) {
				throw std::runtime_error(std::format("Failed to decode texture {}", path.string()));
			}
			data = std::vector<uint8_t>(image_data, image_data + static_cast<size_t>(width) * height * channels);
			free(image_data);
		}
	}
};