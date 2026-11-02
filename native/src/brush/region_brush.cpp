#include "region_brush.h"

#include <QKeyEvent>

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
#include "world_undo_manager.hpp"
#include "camera.hpp"
#include "globals.hpp"
#include <glm/glm.hpp>

RegionBrush::RegionBrush(Regions& regions, WorldUndoManager& world_undo)
	: Brush(), regions(regions), world_undo(world_undo) {
	// Make the ground projected brush square invisible as regions are drawn by the terrain shaders
	brush_color = {0, 0, 0, 0};
	set_shape(shape);
}

float RegionBrush::border_size() const {
	// Same thickness as the region borders drawn by the terrain/cliff shaders so the grab zone matches the visuals
	return region_border_size();
}

float RegionBrush::corner_size() const {
	// Must match the `corner` constant in terrain.frag and cliff.frag
	return 0.2f;
}

std::optional<RegionBrush::ResizeEdges> RegionBrush::edges_under(const Region& region, const glm::vec2 position) const {
	const float left = std::min(region.left, region.right);
	const float right = std::max(region.left, region.right);
	const float bottom = std::min(region.bottom, region.top);
	const float top = std::max(region.bottom, region.top);

	const float corner = corner_size();
	const float border = border_size();
	// The border/corners are drawn inside the region, the margin makes them easier to grab
	const float margin = border / 2.f;

	// Corners take priority as they resize in both axes
	const bool near_left = position.x >= left - margin && position.x <= left + corner + margin;
	const bool near_right = position.x <= right + margin && position.x >= right - corner - margin;
	const bool near_bottom = position.y >= bottom - margin && position.y <= bottom + corner + margin;
	const bool near_top = position.y <= top + margin && position.y >= top - corner - margin;

	if ((near_left || near_right) && (near_bottom || near_top)) {
		return ResizeEdges {
			.left = near_left,
			.right = near_right && !near_left,
			.bottom = near_bottom,
			.top = near_top && !near_bottom,
		};
	}

	const bool within_x = position.x >= left - margin && position.x <= right + margin;
	const bool within_y = position.y >= bottom - margin && position.y <= top + margin;

	if (within_y && position.x >= left - margin && position.x <= left + border + margin) {
		return ResizeEdges { .left = true };
	}
	if (within_y && position.x <= right + margin && position.x >= right - border - margin) {
		return ResizeEdges { .right = true };
	}
	if (within_x && position.y >= bottom - margin && position.y <= bottom + border + margin) {
		return ResizeEdges { .bottom = true };
	}
	if (within_x && position.y <= top + margin && position.y >= top - border - margin) {
		return ResizeEdges { .top = true };
	}

	return std::nullopt;
}

std::vector<Region*> RegionBrush::regions_under() const {
	std::vector<Region*> result;

	const glm::vec2 position = input_handler.mouse_world;
	// In reverse so the topmost (drawn last) region comes first
	for (auto& region : regions.regions | std::views::reverse) {
		const float left = std::min(region.left, region.right);
		const float right = std::max(region.left, region.right);
		const float bottom = std::min(region.bottom, region.top);
		const float top = std::max(region.bottom, region.top);

		if (position.x >= left && position.x <= right && position.y >= bottom && position.y <= top) {
			result.push_back(&region);
		}
	}

	return result;
}

void RegionBrush::key_press_event(WorldEditContext& ctx, const QKeyEvent* event) {
	if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_A) {
		selections.clear();
		selections.reserve(regions.regions.size());
		for (auto& region : ctx.regions.regions) {
			selections.emplace(&region);
		}
		if (selection_changed) { selection_changed(); }
		return;
	}
	Brush::key_press_event(ctx, event);
}

void RegionBrush::mouse_press_event(WorldEditContext& ctx, const QMouseEvent* event, double frame_delta) {
	// The mouse.y check is needed as sometimes it is negative for unknown reasons
	if (event->button() == Qt::LeftButton && input_handler.mouse.y > 0.f) {
		if (mode == Mode::placement) {
			if (!selections.empty()) {
				selections.clear();
				if (selection_changed) { selection_changed(); }
			}

			// Immediately create the region and drag out its size through the resize machinery
			// so the drag is drawn by the terrain shaders just like finished regions
			const glm::vec2 start = snap(input_handler.mouse_world);

			Region region;
			region.left = start.x;
			region.right = start.x;
			region.bottom = start.y;
			region.top = start.y;
			region.name = regions.get_unique_name();
			region.creation_number = regions.get_unique_creation_number();
			region.color = region_preset_colors[region.creation_number % region_preset_colors.size()];

			regions.regions.push_back(region);

			creating = true;
			resizing = true;
			resize_region = &regions.regions.back();
			resize_edges = { .right = true, .top = true };

			if (regions_changed) { regions_changed(); }
			return;
		}

		if (mode == Mode::selection) {
			if (event->modifiers() & Qt::ShiftModifier) {
				if (const auto under = regions_under(); !under.empty()) {
					Region* region = under.front();
					if (selections.contains(region)) {
						selections.erase(region);
					} else {
						selections.emplace(region);
					}
					if (selection_changed) { selection_changed(); }
					return;
				}
			}

			if (!event->modifiers()) {
				for (const auto& region : selections) {
					if (const auto edges = edges_under(*region, input_handler.mouse_world)) {
						// Normalize the edges so the resize flags match which value to move
						if (region->left > region->right) {
							std::swap(region->left, region->right);
						}
						if (region->bottom > region->top) {
							std::swap(region->bottom, region->top);
						}
						resizing = true;
						resize_region = region;
						resize_edges = *edges;
						press_position = snap(input_handler.mouse_world);
						dragged = false;
						// A plain click on the border still cycles the selection on release
						select_on_release = true;
						return;
					}
				}

				if (const auto under = regions_under(); !under.empty()) {
					dragging = true;
					drag_start = snap(input_handler.mouse_world);
					press_position = drag_start;
					dragged = false;

					const bool any_selected = std::ranges::any_of(under, [&](Region* i) { return selections.contains(i); });
					if (any_selected) {
						// Keep the selection so it can be dragged. Cycling to the region
						// below only happens when releasing without having dragged
						select_on_release = true;
					} else {
						selections = { under.front() };
						if (selection_changed) { selection_changed(); }
					}
					return;
				}
			}
		}
	}
	Brush::mouse_press_event(ctx, event, frame_delta);
}

void RegionBrush::mouse_move_event(WorldEditContext& ctx, const QMouseEvent* event, double frame_delta) {
	Brush::mouse_move_event(ctx, event, frame_delta);

	if (event->buttons() == Qt::LeftButton) {
		if ((dragging || resizing) && snap(input_handler.mouse_world) != press_position) {
			dragged = true;
		}

		if (resizing && (dragged || creating)) {
			// Only resize once the mouse leaves the press cell so a plain click on the
			// border doesn't nudge the edge to the snapped click position
			// A creation drag gets a single RegionAddAction undo entry on release instead
			if (!state_undo && !creating) {
				start_action();
			}

			const glm::vec2 position = snap(input_handler.mouse_world);
			if (resize_edges.left) {
				resize_region->left = position.x;
			}
			if (resize_edges.right) {
				resize_region->right = position.x;
			}
			if (resize_edges.bottom) {
				resize_region->bottom = position.y;
			}
			if (resize_edges.top) {
				resize_region->top = position.y;
			}

			// Allow dragging an edge past the opposite edge
			if (resize_region->left > resize_region->right) {
				std::swap(resize_region->left, resize_region->right);
				std::swap(resize_edges.left, resize_edges.right);
			}
			if (resize_region->bottom > resize_region->top) {
				std::swap(resize_region->bottom, resize_region->top);
				std::swap(resize_edges.bottom, resize_edges.top);
			}
		} else if (dragging) {
			const glm::vec2 offset = snap(input_handler.mouse_world) - drag_start;
			if (offset.x == 0.f && offset.y == 0.f) {
				return;
			}

			if (!state_undo) {
				start_action();
			}

			drag_start = snap(input_handler.mouse_world);
			for (const auto& region : selections) {
				region->left += offset.x;
				region->right += offset.x;
				region->bottom += offset.y;
				region->top += offset.y;
			}
		} else if (selection_started) {
			const glm::vec2 low = glm::min(glm::vec2(selection_start), glm::vec2(input_handler.mouse_world));
			const glm::vec2 high = glm::max(glm::vec2(selection_start), glm::vec2(input_handler.mouse_world));

			std::unordered_set<Region*> query;
			for (auto& region : regions.regions) {
				const float left = std::min(region.left, region.right);
				const float right = std::max(region.left, region.right);
				const float bottom = std::min(region.bottom, region.top);
				const float top = std::max(region.bottom, region.top);

				if (left <= high.x && right >= low.x && bottom <= high.y && top >= low.y) {
					query.emplace(&region);
				}
			}

			if (event->modifiers() & Qt::ShiftModifier) {
				selections.insert(query.begin(), query.end());
			} else if (event->modifiers() & Qt::AltModifier) {
				for (const auto& region : query) {
					selections.erase(region);
				}
			} else {
				selections = query;
			}

			if (selection_changed) { selection_changed(); }
		}
	}
}

void RegionBrush::mouse_release_event(WorldEditContext& ctx, const QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		if (creating) {
			creating = false;
			resizing = false;

			Region& region = *resize_region;
			resize_region = nullptr;

			// A click without dragging results in a single cell sized region
			if (region.left == region.right) {
				region.right += 0.25f;
			}
			if (region.bottom == region.top) {
				region.top += 0.25f;
			}

			auto undo = std::make_unique<RegionAddAction>();
			undo->regions.push_back(region);
			world_undo.new_undo_group();
			world_undo.add_undo_action(std::move(undo));

			if (regions_changed) { regions_changed(); }
		} else if (dragging || resizing) {
			dragging = false;
			resizing = false;
			resize_region = nullptr;
			if (state_undo) {
				end_action();
			}

			// A click without dragging cycles the selection to the region below the selected one
			if (select_on_release && !dragged) {
				if (const auto under = regions_under(); !under.empty()) {
					Region* target = under.front();
					for (size_t i = 0; i < under.size(); i++) {
						if (selections.contains(under[i])) {
							target = under[(i + 1) % under.size()];
							break;
						}
					}
					selections = { target };
					if (selection_changed) { selection_changed(); }
				}
			}
			select_on_release = false;
		}
	}

	Brush::mouse_release_event(ctx, event);
}

void RegionBrush::delete_selection() {
	if (selections.empty()) {
		return;
	}

	auto undo = std::make_unique<RegionDeleteAction>();
	for (size_t i = 0; i < regions.regions.size(); i++) {
		if (selections.contains(&regions.regions[i])) {
			undo->regions.emplace_back(i, regions.regions[i]);
		}
	}
	world_undo.new_undo_group();
	world_undo.add_undo_action(std::move(undo));

	regions.remove_regions(selections);

	selections.clear();
	if (selection_changed) { selection_changed(); }
	if (regions_changed) { regions_changed(); }
}

void RegionBrush::clear_selection() {
	// Cancel an in-progress creation drag (Escape, mode switches and undo/redo end up here)
	if (creating && resize_region) {
		regions.remove_region(resize_region);
		creating = false;
		if (regions_changed) { regions_changed(); }
	}

	selections.clear();
	// Abort any in-progress drag/resize as the regions it refers to may be going away
	dragging = false;
	resizing = false;
	resize_region = nullptr;
	state_undo.reset();
	if (selection_changed) { selection_changed(); }
}

void RegionBrush::apply(WorldEditContext& ctx, double frame_delta) {
	// Regions are created in the mouse handlers as they are drawn by dragging out a rectangle
}

void RegionBrush::start_action() {
	world_undo.new_undo_group();
	state_undo = std::make_unique<RegionStateAction>();
	for (const auto& region : selections) {
		state_undo->old_regions.push_back(*region);
	}
}

void RegionBrush::end_action() {
	for (const auto& region : selections) {
		state_undo->new_regions.push_back(*region);
	}
	world_undo.add_undo_action(std::move(state_undo));
}
