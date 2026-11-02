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
#include "BinaryReader.hpp"
#include "MDX.hpp"
#include "Utilities.hpp"
#include <glm/glm.hpp>

namespace fs = std::filesystem;

void parse_all_mdx() {
	std::vector<fs::path> paths;

	for (const auto i : fs::recursive_directory_iterator("D:/Warcraft/2.0/war3.w3mod")) {
		if (i.is_regular_file() && (i.path().extension() == ".mdx" || i.path().extension() == ".MDX")) {
			paths.push_back(i.path());
		}
	}

	for (const auto i : fs::recursive_directory_iterator("C:/stack/Projects/HiveWE/HiveWE/hive_models")) {
		if (i.is_regular_file() && (i.path().extension() == ".mdx" || i.path().extension() == ".MDX")) {
			paths.push_back(i.path());
		}
	}

	size_t outside_range = 0;
	size_t rows = 0;
	size_t columns = 0;
	std::atomic<uint64_t> checked(0);
	std::for_each(std::execution::seq, paths.begin(), paths.end(), [&](const fs::path& path) {
		auto buffer = read_file(path).value();
		const auto mdx = mdx::MDX(buffer);

		checked.fetch_add(1);

		for (const auto& emitter : mdx.emitters2) {
			// rows += emitter.rows;
			// if (emitter.rows == 0 || emitter.rows > 10) {
			// 	std::println("Value of {}", emitter.rows);
			// 	outside_range += 1;
			// }
			// columns += emitter.columns;
			// if (emitter.columns == 0 || emitter.columns > 10) {
			// 	std::println("Value of {}", emitter.rows);
			// 	outside_range += 1;
			// }

			if (emitter.tail_length < 0.0f) {
				std::println("Value of {}", emitter.tail_length);
				outside_range += 1;
			}
		}
	});
	std::println("Outside range {}, checked {}, {}% odd", outside_range, checked.load(), static_cast<float>(outside_range) / checked.load() * 100.f);
	// std::println("Rows avg {}, Cols avg {}", static_cast<float>(rows) / checked.load(), static_cast<float>(columns) / checked.load());
}

void execute_tests() {
	std::print("[INFO] Parsing all MDX files\n");
	auto begin = std::chrono::steady_clock::now();
	parse_all_mdx();
	auto delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000.f;
	std::print("[INFO] Done parsing in {}ms\n", delta);
}
