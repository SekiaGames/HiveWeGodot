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
