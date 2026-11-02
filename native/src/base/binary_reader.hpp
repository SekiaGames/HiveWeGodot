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
#include "no_init_allocator.hpp"

class BinaryReader {
  public:
	std::vector<u8, default_init_allocator<u8>> buffer;
	unsigned long long int position = 0;

	explicit BinaryReader(std::vector<u8, default_init_allocator<u8>> buffer)
		: buffer(std::move(buffer)) {
	}

	template <typename T>
	[[nodiscard]] T read() {
		static_assert(std::is_trivial_v<T>, "T must be of trivial type.");

		if (position + sizeof(T) > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}

		T result;
		std::memcpy(&result, &buffer[position], sizeof(T));

		position += sizeof(T);
		return result;
	}

	[[nodiscard]] std::string read_string(const size_t size) {
		if (position + size > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}

		std::string result(reinterpret_cast<const char*>(&buffer[position]), size);

		if (const size_t pos = result.find('\0'); pos != std::string::npos) {
			result.resize(pos);
		}

		position += size;
		return result;
	}

	[[nodiscard]] std::string read_c_string() {
		const std::string string(reinterpret_cast<char*>(buffer.data() + position));
		position += string.size() + 1;

		if (position > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}

		return string;
	}

	template <typename T>
	[[nodiscard]] std::vector<T> read_vector(const size_t size) {
		static_assert(std::is_trivial_v<T>, "T must be of trivial type.");

		if (position + sizeof(T) * size > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
		std::vector<T> result(reinterpret_cast<T*>(&buffer[position]), reinterpret_cast<T*>(&buffer[position]) + size);
		position += sizeof(T) * size;
		return result;
	}

	[[nodiscard]] long long remaining() const {
		return buffer.size() - position;
	}

	void advance(const size_t amount) {
		if (position + amount > buffer.size()) {
			throw std::out_of_range("Trying to advance past the end of the buffer");
		}
		position += amount;
	}

	void advance_c_string() {
		position += std::string(reinterpret_cast<char*>(buffer.data() + position)).size() + 1;

		if (position > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
	}
};