#pragma once


#include <stdexcept>
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
#include "binary_reader.hpp"
#include "resource_manager.hpp"
#include "hierarchy.hpp"
#include "mdx.hpp"
#include "vk_resources.hpp"
#include <glm/glm.hpp>

namespace fs = std::filesystem;

/// Mirrors PushConstants in data/shaders/terrain_common.glsl
struct CliffPushConstants {
	VkDeviceAddress frame;
	VkDeviceAddress positions;
	VkDeviceAddress uvs;
	VkDeviceAddress normals;
	VkDeviceAddress instances;
};

/// A cliff or ramp model, drawn instanced once per cliff tile using it
class CliffMesh : public Resource {
  public:
	/// Positions, then UVs, then normals, then 16 bit indices
	Buffer buffer;
	std::array<VkDeviceSize, 3> stream_offsets {};
	VkDeviceSize index_offset = 0;
	uint32_t indices = 0;

	static constexpr const char* name = "CliffMesh";

	/// Per instance: tile x, tile y, base layer height, cliff texture index
	std::vector<glm::vec4> render_jobs;

	explicit CliffMesh(const fs::path& path) {
		BinaryReader reader = hierarchy.open_file(path, Hierarchy::FileSource::all, {".mdx", ".mdl"}).value();

		mdx::MDX mdx;
		if (mdx::is_mdx(reader)) {
			mdx = mdx::MDX(reader);
		} else {
			const auto view = std::string_view(reinterpret_cast<const char*>(reader.buffer.data()), reader.buffer.size());
			mdx = mdx::MDX::from_mdl(view).value();
		}

		if (!mdx.is_valid()) {
			throw std::runtime_error(
				std::format("Mesh {} has severe errors and cannot be rendered. Check them in the model editor.", path.string())
			);
		}

		mdx.fix_up();

		const auto& set = mdx.geosets.front();

		const auto positions = std::as_bytes(std::span(set.vertices));
		const auto uvs = std::as_bytes(std::span(set.uv_sets.front()));
		const auto normals = std::as_bytes(std::span(set.normals));
		const auto faces = std::as_bytes(std::span(set.faces));

		std::vector<std::byte> bytes;
		for (size_t i = 0; const auto& stream : {positions, uvs, normals}) {
			stream_offsets[i++] = bytes.size();
			bytes.insert(bytes.end(), stream.begin(), stream.end());
		}
		index_offset = bytes.size();
		bytes.insert(bytes.end(), faces.begin(), faces.end());

		indices = static_cast<uint32_t>(set.faces.size());
		buffer = create_device_buffer(bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

	~CliffMesh() override {
		destroy_buffer_deferred(buffer);
	}

	void render_queue(const glm::vec4 position) {
		render_jobs.push_back(position);
	}

	/// Draws every queued instance and clears the queue. Expects the cliff pipeline to be bound.
	/// `frame` is the TerrainFrameData address.
	void render(const VkCommandBuffer cmd, FrameAllocator& allocator, const VkDeviceAddress frame) {
		if (render_jobs.empty()) {
			return;
		}

		const auto instances = allocator.upload(std::span<const glm::vec4>(render_jobs), 16);

		const CliffPushConstants push = {
			.frame = frame,
			.positions = buffer.address + stream_offsets[0],
			.uvs = buffer.address + stream_offsets[1],
			.normals = buffer.address + stream_offsets[2],
			.instances = instances.address,
		};
		vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);
		vkCmdBindIndexBuffer(cmd, buffer.buffer, index_offset, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(cmd, indices, static_cast<uint32_t>(render_jobs.size()), 0, 0, 0);

		render_jobs.clear();
	}
};
