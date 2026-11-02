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
#include "Units.hpp"
#include "WorldUndoManager.hpp"

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






