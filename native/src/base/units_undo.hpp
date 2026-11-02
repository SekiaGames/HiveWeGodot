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
#include "units.hpp"
#include "world_undo_manager.hpp"

// Undo/redo structures
class UnitAddAction final : public WorldCommand {
public:
	std::vector<Unit> units;

	void undo(WorldEditContext& ctx) override {
		ctx.units.units.resize(ctx.units.units.size() - units.size());
	}

	void redo(WorldEditContext& ctx) override {
		ctx.units.units.insert(ctx.units.units.end(), units.begin(), units.end());
	}
};

class UnitDeleteAction final : public WorldCommand {
public:
	std::vector<Unit> units;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.units.units.insert(ctx.units.units.end(), units.begin(), units.end());
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.units.units.resize(ctx.units.units.size() - units.size());
	}
};

class UnitStateAction final : public WorldCommand {
public:
	std::vector<Unit> old_units;
	std::vector<Unit> new_units;

	void undo(WorldEditContext& ctx) override {
		for (auto& i : old_units) {
			for (auto& j : ctx.units.units) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}
	}

	void redo(WorldEditContext& ctx) override {
		for (auto& i : new_units) {
			for (auto& j : ctx.units.units) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}
	}
};






