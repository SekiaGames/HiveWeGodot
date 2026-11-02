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
#include "ankerl/unordered_dense.h"
#include "StringHash.hpp"

namespace hive {
	/// Under the hood this is an ankerl::unordered_dense::set
	/// This template makes it pick the heterogeneous lookup version when the Key is std::string and otherwise uses the default
    template <typename Key>
    using unordered_set = std::conditional_t<
        StringLike<Key>,
        ankerl::unordered_dense::set<Key, string_hash, std::equal_to<>>,
        ankerl::unordered_dense::set<Key>
    >;
}
