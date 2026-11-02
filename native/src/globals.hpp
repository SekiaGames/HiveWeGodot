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
#include "INI.hpp"
#include "SLK.hpp"

inline ini::INI world_edit_strings;
inline ini::INI world_edit_game_strings;
inline ini::INI world_edit_data;

inline slk::SLK units_slk;
inline slk::SLK units_meta_slk;
inline ini::INI unit_editor_data;

inline slk::SLK items_slk;
inline slk::SLK items_meta_slk;

inline slk::SLK abilities_slk;
inline slk::SLK abilities_meta_slk;

inline slk::SLK doodads_slk;
inline slk::SLK doodads_meta_slk;

inline slk::SLK destructibles_slk;
inline slk::SLK destructibles_meta_slk;

inline slk::SLK upgrade_slk;
inline slk::SLK upgrade_meta_slk;

inline slk::SLK buff_slk;
inline slk::SLK buff_meta_slk;


/// Generates a four character ID that is not in use by any of the object types
std::string get_unique_id(bool first_uppercase) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, 25);
again:

	std::string id = std::string("") + char((first_uppercase ? 'A' : 'a') + dist(mt)) + char('a' + dist(mt)) + char('a' + dist(mt))
					 + char('a' + dist(mt));

	if (units_slk.row_headers.contains(id) || items_slk.row_headers.contains(id) || abilities_slk.row_headers.contains(id)
		|| doodads_slk.row_headers.contains(id) || destructibles_slk.row_headers.contains(id) || upgrade_slk.row_headers.contains(id)
		|| buff_slk.row_headers.contains(id)) {
		std::print("Generated an existing ID: {} what're the odds\n", id);
		goto again;
	}

	return id;
}