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
#include "terrain.hpp"
#include "units.hpp"
#include "world_undo_manager.hpp"
#include "rects.hpp"

enum class TerrainUndoType {
	texture,
	height,
	cliff,
	water
};

class TerrainGenericAction final: public WorldCommand {
  public:
	TerrainRect area;
	std::vector<Corner> old_corners;
	std::vector<Corner> new_corners;
	TerrainUndoType undo_type;

	void undo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.terrain.set_corner(i, j, old_corners[(j - area.top()) * area.width() + i - area.left()]);
			}
		}

		if (undo_type == TerrainUndoType::height) {
			ctx.terrain.update_ground_heights(area);
		}

		if (undo_type == TerrainUndoType::texture) {
			ctx.terrain.update_ground_textures(area);
		}

		if (undo_type == TerrainUndoType::cliff) {
			ctx.terrain.update_ground_heights(area);
			ctx.terrain.update_cliff_meshes(area);
			ctx.terrain.update_ground_textures(area);
			ctx.terrain.update_water(area);
		}

		if (undo_type == TerrainUndoType::water) {
			ctx.terrain.update_water(area);
		}

		ctx.terrain.update_minimap();
		ctx.units.update_area(area, ctx.terrain);
	}

	void redo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.terrain.set_corner(i, j, new_corners[(j - area.top()) * area.width() + i - area.left()]);
			}
		}

		if (undo_type == TerrainUndoType::height) {
			ctx.terrain.update_ground_heights(area);
		}

		if (undo_type == TerrainUndoType::texture) {
			ctx.terrain.update_ground_textures(area);
		}

		if (undo_type == TerrainUndoType::cliff) {
			ctx.terrain.update_ground_heights(area);
			ctx.terrain.update_cliff_meshes(area);
			ctx.terrain.update_ground_textures(area);
			ctx.terrain.update_water(area);
		}

		if (undo_type == TerrainUndoType::water) {
			ctx.terrain.update_water(area);
		}

		ctx.terrain.update_minimap();
		ctx.units.update_area(area, ctx.terrain);
	}
};
