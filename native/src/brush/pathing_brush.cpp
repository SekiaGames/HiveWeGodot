#include "pathing_brush.h"

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
#include <glm/glm.hpp>
#include "WorldUndoManager.hpp"
#include "PathingUndo.hpp"
#include "Camera.hpp"

PathingBrush::PathingBrush(WorldUndoManager& world_undo) : Brush(), world_undo(world_undo)  {
	position_granularity = 4.f;
	size_granularity = 1;
	brush_type = Brush::Type::cell;
}

void PathingBrush::apply_begin(WorldEditContext& ctx) {
	const glm::ivec2 pos = glm::vec2(input_handler.mouse_world) * 4.f - size.x / 2.f + 0.5f;
	const int x = pos.x;
	const int y = pos.y;

	applied_area = PathingRect(x, y, size.x, size.y).intersected({0, 0, ctx.pathing_map.width, ctx.pathing_map.height});

	world_undo.new_undo_group();
	old_pathing_cells_static = ctx.pathing_map.pathing_cells_static;
}

void PathingBrush::apply(WorldEditContext& ctx, double frame_delta) {
	const glm::ivec2 pos = glm::vec2(input_handler.mouse_world) * 4.f - size.x / 2.f + 0.5f;
	const PathingRect area = PathingRect(pos.x, pos.y, size.x, size.y).intersected({0, 0, ctx.pathing_map.width, ctx.pathing_map.height});

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	const int offset = area.y() * ctx.pathing_map.width + area.x();

	for (int i = 0; i < area.width(); i++) {
		for (int j = 0; j < area.height(); j++) {
			if (!contains(glm::ivec2(i - std::min(pos.x, 0), j - std::min(pos.y, 0)))) {
				continue;
			}

			const int index = offset + j * ctx.pathing_map.width + i;
			switch (operation) {
				case Operation::replace:
					ctx.pathing_map.pathing_cells_static[index] &= ~0b00001110;
					ctx.pathing_map.pathing_cells_static[index] |= brush_mask;
					break;
				case Operation::add:
					ctx.pathing_map.pathing_cells_static[index] |= brush_mask;
					break;
				case Operation::remove:
					ctx.pathing_map.pathing_cells_static[index] &= ~brush_mask;
					break;
			}
		}
	}

	applied_area = applied_area.united(area);

	ctx.pathing_map.upload_static_pathing();
}

void PathingBrush::apply_end(WorldEditContext& ctx) {
	add_pathing_undo(ctx, applied_area);
}

void PathingBrush::add_pathing_undo(WorldEditContext& ctx, const PathingRect& area) {
	auto undo_action = std::make_unique<PathingMapAction>();

	undo_action->area = area;
	const auto width = ctx.pathing_map.width;

	// Copy old corners
	undo_action->old_pathing.reserve(area.width() * area.height());
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			undo_action->old_pathing.push_back(old_pathing_cells_static[j * width + i]);
		}
	}

	// Copy new corners
	undo_action->new_pathing.reserve(area.width() * area.height());
	for (int j = area.top(); j <= area.bottom(); j++) {
		for (int i = area.left(); i <= area.right(); i++) {
			undo_action->new_pathing.push_back(ctx.pathing_map.pathing_cells_static[j * width + i]);
		}
	}

	world_undo.add_undo_action(std::move(undo_action));
}
