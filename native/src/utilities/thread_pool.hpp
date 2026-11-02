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

/// Worker threads for loading work, such as constructing meshes and their GPU resources in parallel.
class ThreadPool {
	std::mutex mutex;
	std::condition_variable cv;
	std::queue<std::move_only_function<void()>> tasks;
	bool stopping = false;

	std::vector<std::thread> workers;

	void work_loop() {
		while (true) {
			std::move_only_function<void()> task;
			{
				std::unique_lock lock(mutex);
				cv.wait(lock, [&] {
					return stopping || !tasks.empty();
				});
				if (stopping && tasks.empty()) {
					return;
				}
				task = std::move(tasks.front());
				tasks.pop();
			}
			task();
		}
	}

  public:
	/// Starts `count` workers, or one fewer than the number of hardware threads when 0
	void init(int count = 0) {
		if (count == 0) {
			count = static_cast<int>(std::max(1u, std::thread::hardware_concurrency() - 1));
		}
		for (int i = 0; i < count; i++) {
			workers.emplace_back([this] {
				work_loop();
			});
		}
	}

	template <typename F, typename R = std::invoke_result_t<F>>
	std::future<R> submit(F&& f) {
		auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
		std::future<R> future = task->get_future();
		{
			std::lock_guard lock(mutex);
			tasks.push([task]() mutable {
				(*task)();
			});
		}
		cv.notify_one();
		return future;
	}

	bool is_initialized() const {
		return !workers.empty();
	}

	void stop() {
		{
			std::lock_guard lock(mutex);
			stopping = true;
		}
		cv.notify_all();
		for (auto& worker : workers) {
			worker.join();
		}
		workers.clear();
		stopping = false;
	}

	~ThreadPool() {
		stop();
	}
};

inline ThreadPool thread_pool;
