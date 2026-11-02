#pragma once


#include <cstdint>


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
#include "WorldUndoManager.hpp"
#include "PathingMap.hpp"
#include "Rects.hpp"

class PathingMapAction: public WorldCommand {
  public:
	PathingRect area;
	std::vector<uint8_t> old_pathing;
	std::vector<uint8_t> new_pathing;

	void undo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.pathing_map.pathing_cells_static[j * ctx.pathing_map.width + i] =
					old_pathing[(j - area.top()) * area.width() + i - area.left()];
			}
		}
		ctx.pathing_map.upload_static_pathing();
	}

	void redo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.pathing_map.pathing_cells_static[j * ctx.pathing_map.width + i] =
					new_pathing[(j - area.top()) * area.width() + i - area.left()];
			}
		}
		ctx.pathing_map.upload_static_pathing();
	}
};
