#pragma once

#include "outcome.hpp"

#define OUTCOME_TRY_CAT2(a, b) a##b
#define OUTCOME_TRY_CAT(a, b) OUTCOME_TRY_CAT2(a, b)

#define OUTCOME_TRY2(var, expr)                        \
	auto&& OUTCOME_TRY_CAT(_outcome_try_, __LINE__) = (expr);            \
	if (!OUTCOME_TRY_CAT(_outcome_try_, __LINE__)) {                     \
		return std::unexpected(OUTCOME_TRY_CAT(_outcome_try_, __LINE__).error()); \
	}                                                   \
	var = std::move(*OUTCOME_TRY_CAT(_outcome_try_, __LINE__))

#define OUTCOME_TRY1(expr)                              \
	if (const auto& OUTCOME_TRY_CAT(_outcome_try_, __LINE__) = (expr); !OUTCOME_TRY_CAT(_outcome_try_, __LINE__)) { \
		return std::unexpected(OUTCOME_TRY_CAT(_outcome_try_, __LINE__).error()); \
	}

#define OUTCOME_TRY_PICK(_1, _2, NAME, ...) NAME
#define OUTCOME_TRY(...) \
	OUTCOME_TRY_PICK(__VA_ARGS__, OUTCOME_TRY2, OUTCOME_TRY1)(__VA_ARGS__)
