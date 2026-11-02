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
#include "no_init_allocator.hpp"

class BinaryWriter {
  public:
	std::vector<u8, default_init_allocator<u8>> buffer;

	template <typename T = void, typename U>
	void write(U value) {
		static_assert(std::is_standard_layout_v<U>, "U must be of standard layout.");

		if constexpr (not std::is_void_v<T>) {
			static_assert(std::is_standard_layout_v<T>, "T must be of standard layout.");

			T temp = static_cast<T>(value);
			buffer.resize(buffer.size() + sizeof(T));
			std::memcpy(buffer.data() + buffer.size() - sizeof(T), &temp, sizeof(T));
		} else {
			U temp = value;
			buffer.resize(buffer.size() + sizeof(U));
			std::memcpy(buffer.data() + buffer.size() - sizeof(U), &temp, sizeof(U));
		}
	}

	/// Writes the string to the buffer (null terminated if the input string is null terminated)
	void write_string(const std::string_view string) {
		buffer.resize(buffer.size() + string.size());
		std::copy(string.begin(), string.end(), buffer.end() - string.size());
	}

	/// Writes a null terminated string to the buffer
	void write_c_string(const std::string_view string) {
		if (!string.empty() && string.back() == '\0') {
			buffer.resize(buffer.size() + string.size());
			std::copy(string.begin(), string.end(), buffer.end() - string.size());
		} else {
			buffer.resize(buffer.size() + string.size() + 1);
			std::copy(string.begin(), string.end(), buffer.end() - string.size() - 1);
			buffer[buffer.size() - 1] = '\0';
		}
	}

	/// Writes a null terminated string to the buffer with padding
	void write_c_string_padded(const std::string_view string, const size_t final_size) {
		buffer.resize(buffer.size() + final_size);

		const size_t length_to_write = std::min(string.size(), final_size);
		std::copy(string.begin(), string.begin() + length_to_write, buffer.end() - final_size);
		std::fill(buffer.end() - final_size + length_to_write, buffer.end(), '\0');
	}

	/// Copies the contents of the array to the buffer, has special code for std::string
	template <typename T, typename Y>
	void write_vector(const std::vector<T, Y>& vector) {
		if constexpr (std::is_same_v<T, std::string>) {
			for (const auto& i : vector) {
				buffer.insert(buffer.end(), i.begin(), i.end());
			}
		} else {
			static_assert(std::is_standard_layout<T>::value, "T must be of standard layout or std::string.");
			buffer.resize(buffer.size() + vector.size() * sizeof(T));
			// std::copy(vector.begin(), vector.end(), buffer.end() - vector.size() * sizeof(T));
			std::memcpy(buffer.data() + buffer.size() - vector.size() * sizeof(T), vector.data(), vector.size() * sizeof(T));
		}
	}
};