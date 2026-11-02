#pragma once


#include "unordered_map.hpp"
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
#include "binary_reader.hpp"
#include "utilities.hpp"

namespace fs = std::filesystem;

namespace json {
	class JSON {
	  public:
		hive::unordered_map<std::string, std::string> json_data;
		
		JSON() = default;

		explicit JSON(const BinaryReader& reader) {
			load(reader);
		}

		void load(const BinaryReader& reader) {
			json_data.clear();
			std::string_view data(reinterpret_cast<const char*>(reader.buffer.data()), reader.buffer.size());
			bool saw_array_start = false;

			while (!data.empty()) {
				const size_t eol = data.find('\n');
				std::string_view line = (eol == std::string_view::npos) ? data : data.substr(0, eol);
				if (eol == std::string_view::npos) {
					data = {};
				} else {
					data.remove_prefix(eol + 1);
				}

				if (!line.empty() && line.back() == '\r') {
					line.remove_suffix(1);
				}

				// Normally json files use ; for comments, but Blizzard uses //
				if (line.empty() || line.starts_with("//") || line.front() == ';') {
					continue;
				}

				if (!saw_array_start) {
					saw_array_start = line.front() == '[';
					if (!saw_array_start) {
						std::cout << "Malformed Alias JSON\n";
						return;
					}
					continue;
				}

				const size_t src_key = line.find(R"("src":")");
				if (src_key == std::string_view::npos) {
					continue;
				}

				const size_t src_start = src_key + 7;
				const size_t src_end = line.find('"', src_start);
				if (src_end == std::string_view::npos) {
					continue;
				}

				size_t dst_key = line.find(R"("dest":")", src_end);
				if (dst_key == std::string_view::npos) {
					dst_key = line.find(R"("dst":")", src_end);
				}
				if (dst_key == std::string_view::npos) {
					continue;
				}

				size_t dst_start = dst_key;
				if (line.substr(dst_key).starts_with(R"("dest":")")) {
					dst_start = dst_key + 8;
				} else {
					dst_start = dst_key + 7;
				}
				const size_t dst_end = line.find('"', dst_start);
				if (dst_end == std::string_view::npos) {
					continue;
				}

				std::string key(line.substr(src_start, src_end - src_start));
				std::string value(line.substr(dst_start, dst_end - dst_start));
				normalize_path_to_backslash(key);
				normalize_path_to_backslash(value);
				to_lowercase(key);

				json_data[key] = std::move(value);
			}
		}

		[[nodiscard]] bool exists(const std::string& file) const {
			std::string file_lower_case = file;
			normalize_path_to_backslash(file_lower_case);
			to_lowercase(file_lower_case);
			return json_data.contains(file_lower_case);
		}

		[[nodiscard]] std::string alias(const std::string& file) const {
			std::string file_lower_case = file;
			normalize_path_to_backslash(file_lower_case);
			to_lowercase(file_lower_case);
			return json_data.at(file_lower_case);
		}
	};
} // namespace json