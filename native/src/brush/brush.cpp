#include <volk.h>

#include "brush.h"

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
#include "vk_resources.hpp"
#include "vk_texture.hpp"
#include "camera.hpp"
#include "resource_manager.hpp"
#include "globals.hpp"
#include "world_undo_manager.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Brush::Brush() {
	set_size(size);
}

glm::vec2 Brush::get_position() const {
	const bool even_width = size.x % (2 * static_cast<size_t>(size_granularity)) == 0;
	const bool even_height = size.y % (2 * static_cast<size_t>(size_granularity)) == 0;

	glm::vec2 even = glm::floor(glm::vec2(input_handler.mouse_world) * position_granularity + 0.5f) / position_granularity;
	glm::vec2 uneven =
		glm::floor(glm::vec2(input_handler.mouse_world) * position_granularity) / position_granularity + (1.f / position_granularity / 2.f);

	if (brush_type == Type::corner) {
		std::swap(even, uneven);
	}

	glm::vec2 result;
	if (even_width) {
		result.x = even.x;
	} else {
		result.x = uneven.x;
	}

	if (even_height) {
		result.y = even.y;
	} else {
		result.y = uneven.y;
	}

	return result;
}

void Brush::set_size(const glm::ivec2 new_size) {
	size = glm::clamp(new_size * size_granularity, 1, 999);
	set_shape(shape);
	if (size_changed) { size_changed(size / size_granularity); }
}

void Brush::set_shape(const Shape new_shape) {
	std::vector<glm::u8vec4> brush(size.x * size.y, {0, 0, 0, 0});

	shape = new_shape;

	for (int i = 0; i < size.x; i++) {
		for (int j = 0; j < size.y; j++) {
			if (contains(glm::ivec2(i, j) / size_granularity)) {
				brush[j * size.x + i] = brush_color;
			} else {
				brush[j * size.x + i] = {0, 0, 0, 0};
			}
		}
	}

	auto texture = std::make_shared<UpdatableTexture>(VK_FORMAT_R8G8B8A8_UNORM, bindless.nearest_sampler, 4);
	texture->update(
		{static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)},
		std::span(reinterpret_cast<const uint8_t*>(brush.data()), brush.size() * sizeof(glm::u8vec4))
	);
	brush_texture = texture->slot;
	brush_texture_owner = std::move(texture);
}

/// Whether the brush shape contains the point, Arguments in brush coordinates
bool Brush::contains(const glm::ivec2 pos) const {
	const glm::ivec2 extent = size / size_granularity;
	if (pos.x < 0 || pos.y < 0 || pos.x >= extent.x || pos.y >= extent.y) {
		return false;
	}

	switch (shape) {
		case Shape::square:
			return true;
		case Shape::circle: {
			const int half_size = (size.x / 2) / size_granularity;
			const int distance = (pos.x - half_size) * (pos.x - half_size) + (pos.y - half_size) * (pos.y - half_size);
			return distance <= half_size * half_size;
		}
		case Shape::diamond:
			const int half_size = (size.x / 2) / size_granularity;
			return std::abs(pos.x - half_size) + std::abs(pos.y - half_size) <= half_size;
	}
	return true;
}

void Brush::increase_size(const int new_size) {
	set_size(size / size_granularity + new_size);
}

void Brush::decrease_size(const int new_size) {
	set_size(size / size_granularity - new_size);
}

void Brush::switch_mode() {
	if (mode != Mode::placement && mode != Mode::selection) {
		mode = return_mode;
	}
	mode = (mode == Mode::placement) ? Mode::selection : Mode::placement;

	clear_selection();
	selection_started = false;
}

void Brush::key_press_event(WorldEditContext& ctx, const QKeyEvent* event) {
	switch (event->key()) {
		case Qt::Key_Escape:
			clear_selection();
			break;
		case Qt::Key_Equal:
			increase_size(2);
			break;
		case Qt::Key_Minus:
			decrease_size(2);
			break;
		case Qt::Key_Delete:
			delete_selection();
			break;
		case Qt::Key_X:
			if (event->modifiers() & Qt::ControlModifier) {
				cut_selection();
			}
			break;
		case Qt::Key_C:
			if (event->modifiers() & Qt::ControlModifier) {
				copy_selection();
			}
			break;
		case Qt::Key_V:
			if (event->modifiers() & Qt::ControlModifier) {
				return_mode = mode;
				mode = Mode::pasting;
			}
			break;
	}
}

void Brush::mouse_move_event(WorldEditContext& ctx, const QMouseEvent* event, const double frame_delta) {
	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::placement && (can_place() || event->modifiers() & Qt::ShiftModifier)) {
			apply(ctx, frame_delta);
		}
	}
}

void Brush::mouse_press_event(WorldEditContext& ctx, const QMouseEvent* event, const double frame_delta) {
	if (event->button() != Qt::LeftButton) {
		return;
	}

	if (!event->modifiers()) {
		clear_selection();
	}

	if (mode == Mode::selection && !(event->modifiers() & Qt::ControlModifier)) {
		if (!selection_started) {
			selection_started = true;
			selection_start = input_handler.mouse_world;
		}
	} else if (mode == Mode::placement) {
		// Check if eligible for placement
		if (event->button() == Qt::LeftButton) {
			apply_begin(ctx);
			if (can_place() || event->modifiers() & Qt::ShiftModifier) {
				apply(ctx, 0.5);
			}
		}
	} else if (mode == Mode::pasting && (can_place() || event->modifiers() & Qt::ShiftModifier)) {
		clear_selection();
		place_clipboard(ctx);
		mode = Mode::selection;
	}
}

void Brush::mouse_release_event(WorldEditContext& ctx, const QMouseEvent* event) {
	if (mode == Mode::selection) {
		selection_started = false;
	} else if (mode == Mode::placement) {
		if (event->button() == Qt::LeftButton) {
			apply_end(ctx);
		}
	}
}

void Brush::render(BrushDrawList& draw_list) {
	if (mode == Mode::selection) {
		render_selector(draw_list);
	}
	if (mode == Mode::placement) {
		render_brush();
	}
	if (mode == Mode::pasting) {
		render_clipboard();
	}
	render_selection(draw_list);
}

void Brush::render_selector(BrushDrawList& draw_list) const {
	if (selection_started) {
		glm::mat4 model(1.f);
		model = glm::translate(model, selection_start);
		model = glm::scale(model, glm::vec3(glm::vec2(input_handler.mouse_world), 1.f) - glm::vec3(glm::vec2(selection_start), 1.f));
		draw_list.selection_rectangles.push_back(model);
	}
}

void Brush::render_brush() {}
