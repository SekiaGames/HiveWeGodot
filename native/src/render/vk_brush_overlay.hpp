#pragma once


#include <volk.h>


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
#include "VkContext.hpp"
#include "VkResources.hpp"
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
