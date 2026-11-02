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
#include "PathingMap.hpp"
#include "Units.hpp"
#include "Doodads.hpp"
#include "Terrain.hpp"
#include "Regions.hpp"
#include "brush/brush.h"

/// So we don't have to pass a global variable around
struct WorldEditContext {
	Terrain& terrain;
	Units& units;
	Doodads& doodads;
	Regions& regions;
	Brush* brush;
	PathingMap& pathing_map;
};

/// Describes a change in the world. E.g., changing the terrain, adding a unit in the OE, placing some doodads.
class WorldCommand {
  public:
	virtual void undo(WorldEditContext& ctx) = 0;
	virtual void redo(WorldEditContext& ctx) = 0;

	virtual ~WorldCommand() = default;
};

class WorldUndoManager {
	std::vector<std::vector<std::unique_ptr<WorldCommand>>> undo_actions;
	std::vector<std::vector<std::unique_ptr<WorldCommand>>> redo_actions;

  public:
	void undo(WorldEditContext& ctx) {
		if (undo_actions.empty()) {
			return;
		}

		auto& actions = undo_actions.back();
		for (const auto& i : actions) {
			i->undo(ctx);
		}

		redo_actions.push_back(std::move(actions));
		undo_actions.pop_back();
	}

	void redo(WorldEditContext& ctx) {
		if (redo_actions.empty()) {
			return;
		}

		auto& actions = redo_actions.back();
		for (const auto& i : actions) {
			i->redo(ctx);
		}

		undo_actions.push_back(std::move(actions));
		redo_actions.pop_back();
	}

	void new_undo_group() {
		undo_actions.push_back({});
	}

	void add_undo_action(std::unique_ptr<WorldCommand> action) {
		if (undo_actions.empty()) {
			return;
		}

		undo_actions.back().push_back(std::move(action));
		redo_actions.clear();
	}

	/// Removes all undo/redo operations
	void clear_all_undo() {
		undo_actions.clear();
		redo_actions.clear();
	}
};
