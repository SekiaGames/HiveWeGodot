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
#include "ini.hpp"
#include "slk.hpp"
#include "hierarchy.hpp"
#include "globals.hpp"
#include "utilities.hpp"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

namespace fs = std::filesystem;

constexpr const char* constants_row_key = "yeet";

class GameplayConstants {
  public:
	slk::SLK metadata;
	slk::SLK data;

	void load() {
		metadata = slk::SLK("Units/MiscMetaData.slk");
		metadata.substitute(world_edit_strings, "WorldEditStrings");
		metadata.build_meta_map();

		// While there is a gameplay constants meta SLK there is no data SLK,
		// So we create it from the two gameplay constants INI files
		// No idea why it is split into two
		ini::INI defaults = ini::INI();
		defaults.load("Units/MiscData.txt");
		defaults.load("Units/MiscGame.txt");

		data.base_data[constants_row_key] = {};
		size_t index = data.row_headers.size();
		data.row_headers.emplace(constants_row_key, index);
		data.index_to_row[index] = constants_row_key;

		for (const auto& [key, values] : defaults.ini_data["Misc"]) {
			const auto lowercase_key = to_lowercase_copy(key);
			if (!metadata.meta_map.contains(lowercase_key)) {
				std::println("Warning: Gameplay meta data doesn't contain the {} column", key);
			}

			data.add_column( lowercase_key);
			data.base_data[constants_row_key][lowercase_key] = absl::StrJoin(values, ",");
		}

		if (hierarchy.map_file_exists("war3mapMisc.txt")) {
			// Use INI loader in local mode pointing to the map file on disk
			const ini::INI overrides = ini::INI(hierarchy.map_directory / "war3mapMisc.txt", true);

			for (const auto& [key, values] : overrides.ini_data.at("Misc")) {
				const auto lowercase_key = to_lowercase_copy(key);
				if (!metadata.meta_map.contains(lowercase_key)) {
					std::println("Warning: Gameplay meta data doesn't contain the {} column", key);
				}

				data.set_shadow_data(lowercase_key, constants_row_key, absl::StrJoin(values, ","));
			}
		}
	}

	void save() {
		ini::INI war3mapMisc;

		war3mapMisc.ini_data["Misc"] = {};
		for (const auto& [key, value] : data.shadow_data[constants_row_key]) {
			war3mapMisc.ini_data["Misc"][key] = absl::StrSplit(value, ',', absl::SkipEmpty());
		}
		war3mapMisc.save(hierarchy.map_directory / "war3mapMisc.txt");
	}
};
