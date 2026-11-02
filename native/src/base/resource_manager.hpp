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
#include "UnorderedMap.hpp"

namespace fs = std::filesystem;

class Resource {
  public:
	virtual ~Resource() = default;
};

class ResourceManager {
	std::mutex mutex;
	hive::unordered_map<std::string, std::shared_future<std::expected<std::shared_ptr<Resource>, std::string>>> resources;

	template<typename T, typename... Args>
	std::expected<std::shared_ptr<T>, std::string> load_impl(const std::string& key, Args&&... args) {
		std::unique_lock lock(mutex);

		if (const auto it = resources.find(key); it != resources.end()) {
			const auto fut = it->second;
			lock.unlock();
			auto result = fut.get();
			if (result) {
				return std::dynamic_pointer_cast<T>(*result);
			}
			return std::unexpected(result.error());
		}

		// We are the first caller for this key. Insert a future so concurrent callers block
		std::promise<std::expected<std::shared_ptr<Resource>, std::string>> promise;
		auto fut = promise.get_future().share();
		resources.emplace(key, fut);
		lock.unlock(); // Release before constructing. Constructor may call load() recursively

		try {
			auto res = std::make_shared<T>(std::forward<Args>(args)...);
			promise.set_value(res);
			return std::dynamic_pointer_cast<T>(res);
		} catch (const std::exception& e) {
			auto err = std::unexpected(std::string(e.what()));
			promise.set_value(err);
			return err;
		} catch (...) {
			auto err = std::unexpected(std::string("Unknown error loading resource"));
			promise.set_value(err);
			return err;
		}
	}

  public:
	/// Loads and caches a resource until clear() is called.
	/// Thread-safe: concurrent loads of the same key construct the resource exactly once.
	template<typename T, typename... Args>
	std::expected<std::shared_ptr<T>, std::string> load(const fs::path& path, const std::string& custom_identifier = "", Args... args) {
		static_assert(std::is_base_of_v<Resource, T>, "T must inherit from Resource");
		const std::string key = path.string() + T::name + custom_identifier;
		return load_impl<T>(key, path, std::forward<Args>(args)...);
	}

	template<typename T>
	std::expected<std::shared_ptr<T>, std::string> load(const std::initializer_list<fs::path> paths) {
		static_assert(std::is_base_of_v<Resource, T>, "T must inherit from Resource");

		std::string key;
		for (const auto& path : paths) {
			key += path.string();
		}
		key += T::name;

		return load_impl<T>(key, paths);
	}

	/// Releases all cached resources. Call when unloading a map.
	void clear() {
		std::lock_guard lock(mutex);
		resources.clear();
	}
};

inline ResourceManager resource_manager;
