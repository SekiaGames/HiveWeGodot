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
#include "pathing_map.hpp"
#include "units.hpp"
#include "doodads.hpp"
#include "terrain.hpp"
#include "regions.hpp"
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
