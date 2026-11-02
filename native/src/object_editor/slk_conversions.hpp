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
#include "map.hpp"
#include "slk.hpp"
#include "globals.hpp"

/// Returns the new destructible ID
std::string convert_doodad_to_destructible(const std::string_view doodad_id, const std::string_view base_destructible) {
	const auto new_id = get_unique_id(true);
	destructibles_slk.copy_row(base_destructible, new_id, false);

	const auto mapping = std::unordered_map<std::string, std::string>{
		{"fixedrot", "fixedrot"},
		{"maxroll", "maxroll"},
		{"maxpitch", "maxpitch"},
		{"showinmm", "showinmm"},
		{"usemmcolor", "usemmcolor"},
		{"mmred", "mmred"},
		{"mmgreen", "mmgreen"},
		{"mmblue", "mmblue"},
		{"file", "file"},
		{"selsize", "selsize"},
		{"showinfog", "fogvis"},
		{"vertr1", "colorr"}, // We grab the colors from variation 1
		{"vertg1", "colorg"},
		{"vertb1", "colorb"},
		{"numvar", "numvar"},
		{"canscalerandscale", "canscalerandscale"},
		{"tilesetspecific", "tilesetspecific"},
		{"maxscale", "maxscale"},
		{"minscale", "minscale"},
		{"userlist", "userlist"},
		{"oncliffs", "oncliffs"},
		{"onwater", "onwater"},
		{"tilesets", "tilesets"},
		{"useclickhelper", "useclickhelper"},
		{"pathtex", "pathtex"},
		{"walkable", "walkable"},
		{"soundloop", "loopsound"},
		{"name", "name"},
	};

	for (const auto& [header_from, header_to] : mapping) {
		const auto data = doodads_slk.data(header_from, doodad_id);
		destructibles_slk.set_shadow_data(header_to, new_id, data);
	}
	return new_id;
}
