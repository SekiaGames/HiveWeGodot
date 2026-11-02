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
#include "doodad.hpp"
#include "world_undo_manager.hpp"
#include "rects.hpp"

// Undo/redo structures
class DoodadAddAction final: public WorldCommand {
  public:
	std::vector<Doodad> doodads;

	void undo(WorldEditContext& ctx) override {
		ctx.doodads.doodads.resize(ctx.doodads.doodads.size() - doodads.size());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}

	void redo(WorldEditContext& ctx) override {
		ctx.doodads.doodads.insert(ctx.doodads.doodads.end(), doodads.begin(), doodads.end());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}
};

class DoodadDeleteAction final: public WorldCommand {
  public:
	std::vector<Doodad> doodads;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.doodads.doodads.insert(ctx.doodads.doodads.end(), doodads.begin(), doodads.end());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.doodads.doodads.resize(ctx.doodads.doodads.size() - doodads.size());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}
};

class DoodadStateAction final: public WorldCommand {
  public:
	std::vector<Doodad> old_doodads;
	std::vector<Doodad> new_doodads;

	void undo(WorldEditContext& ctx) override {
		PathingRect pathing_area;
		for (const auto& i : old_doodads) {
			for (auto& j : ctx.doodads.doodads) {
				if (i.creation_number == j.creation_number) {
					pathing_area |= i.get_pathing_bounding_box();
					pathing_area |= j.get_pathing_bounding_box();
					j = i;
				}
			}
		}
		ctx.doodads.update_doodad_pathing(pathing_area, ctx.pathing_map);
	}

	void redo(WorldEditContext& ctx) override {
		PathingRect pathing_area;
		for (const auto& i : new_doodads) {
			for (auto& j : ctx.doodads.doodads) {
				if (i.creation_number == j.creation_number) {
					pathing_area |= i.get_pathing_bounding_box();
					pathing_area |= j.get_pathing_bounding_box();
					j = i;
				}
			}
		}
		ctx.doodads.update_doodad_pathing(pathing_area, ctx.pathing_map);
	}
};
