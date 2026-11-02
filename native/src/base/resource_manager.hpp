#pragma once


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
#include "unordered_map.hpp"

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
