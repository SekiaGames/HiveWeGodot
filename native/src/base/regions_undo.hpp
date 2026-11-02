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
#include "regions.hpp"
#include "world_undo_manager.hpp"

class RegionAddAction final : public WorldCommand {
  public:
	std::vector<Region> regions;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		std::erase_if(ctx.regions.regions, [&](const Region& region) {
			return std::ranges::any_of(regions, [&](const Region& i) { return i.creation_number == region.creation_number; });
		});
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.regions.regions.insert(ctx.regions.regions.end(), regions.begin(), regions.end());
	}
};

class RegionDeleteAction final : public WorldCommand {
  public:
	/// The deleted regions paired with the (ascending) indices they had,
	/// so undo restores them at their original draw order
	std::vector<std::pair<size_t, Region>> regions;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		for (const auto& [index, region] : regions) {
			ctx.regions.regions.insert(ctx.regions.regions.begin() + std::min(index, ctx.regions.regions.size()), region);
		}
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		std::erase_if(ctx.regions.regions, [&](const Region& region) {
			return std::ranges::any_of(regions, [&](const auto& i) { return i.second.creation_number == region.creation_number; });
		});
	}
};

class RegionStateAction final : public WorldCommand {
  public:
	std::vector<Region> old_regions;
	std::vector<Region> new_regions;

	void undo(WorldEditContext& ctx) override {
		for (const auto& i : old_regions) {
			for (auto& j : ctx.regions.regions) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}

		notify(ctx);
	}

	void redo(WorldEditContext& ctx) override {
		for (const auto& i : new_regions) {
			for (auto& j : ctx.regions.regions) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}

		notify(ctx);
	}

  private:
	/// The regions were modified in place so the selection still holds, but the palette
	/// listens to this signal to refresh the names/colors it displays
	static void notify(WorldEditContext& ctx) {
		if (ctx.brush) {
			ctx.brush->selection_changed();
		}
	}
};
