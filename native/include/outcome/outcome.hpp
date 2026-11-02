#pragma once

#include <expected>
#include <string>
#include <type_traits>
#include <utility>

#define OUTCOME_V2_NAMESPACE outcome_v2

namespace outcome_v2 {
template <class T, class E = std::string>
using result = std::expected<T, E>;

inline std::expected<void, std::string> success() {
	return {};
}

template <class T>
std::expected<std::decay_t<T>, std::string> success(T&& value) {
	return std::expected<std::decay_t<T>, std::string>(std::forward<T>(value));
}

template <class E>
std::unexpected<std::decay_t<E>> failure(E&& error) {
	return std::unexpected<std::decay_t<E>>(std::forward<E>(error));
}
}
