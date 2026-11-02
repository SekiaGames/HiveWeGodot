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

template <typename T, typename A = std::allocator<T>>
class default_init_allocator : public A {
	typedef std::allocator_traits<A> a_t;

  public:
	using A::A; // Inherit constructors from A

	template <typename U>
	struct rebind {
		using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
	};

	template <typename U>
	void construct(U* ptr) noexcept(std::is_nothrow_default_constructible_v<U>) {
		::new (static_cast<void*>(ptr)) U;
	}

	template <typename U, typename... Args>
	void construct(U* ptr, Args&&... args) {
		a_t::construct(static_cast<A&>(*this),
					   ptr, std::forward<Args>(args)...);
	}
};