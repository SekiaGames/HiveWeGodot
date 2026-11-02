#pragma once


#include <volk.h>


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
#include "vk_context.hpp"
#include "vk_resources.hpp"
#include <glm/glm.hpp>

/// Mirrors PushConstants in data/shaders/selection.vert
struct SelectionPushConstants {
	glm::mat4 MVP;
	uint32_t shape;
};

/// Draws brush overlays: selection rectangle outlines and selection rings, over the terrain without depth testing
class BrushOverlayRenderer {
	Pipeline rectangle_pipeline {{
		.vertex_shader = "data/shaders/selection.vert.spv",
		.fragment_shader = "data/shaders/selection.frag.spv",
		.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	}};
	Pipeline circle_pipeline {{
		.vertex_shader = "data/shaders/selection.vert.spv",
		.fragment_shader = "data/shaders/selection_circle.frag.spv",
		.blend = true,
		.src_factor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dst_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	}};

  public:
	/// Each model maps the unit square into the world. Expects rendering to have begun with the viewport set.
	void render(
		const VkCommandBuffer cmd,
		const std::span<const glm::mat4> rectangles,
		const std::span<const glm::mat4> circles,
		const glm::mat4& projection_view
	) const {
		if (rectangles.empty() && circles.empty()) {
			return;
		}
		vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
		vkCmdSetDepthTestEnable(cmd, VK_FALSE);
		vkCmdSetDepthWriteEnable(cmd, VK_FALSE);

		const auto draw = [&](const VkPipeline pipeline, const std::span<const glm::mat4> models, const uint32_t shape, const uint32_t vertices) {
			if (models.empty()) {
				return;
			}
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			for (const auto& model : models) {
				const SelectionPushConstants push = {projection_view * model, shape};
				vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);
				vkCmdDraw(cmd, vertices, 1, 0, 0);
			}
		};
		draw(rectangle_pipeline, rectangles, 0, 5);
		draw(circle_pipeline, circles, 1, 6);
	}
};
