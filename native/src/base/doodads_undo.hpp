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
#include "Doodad.hpp"
#include "WorldUndoManager.hpp"
#include "Rects.hpp"

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
