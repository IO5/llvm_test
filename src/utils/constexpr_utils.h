#pragma once

#include <cstdlib>
#include <vector>
#include <array>
#include <algorithm>
#include <optional>

constexpr void compile_assert(bool test) {

	if (!test)
		std::abort();
}

template <std::size_t Size, typename T>
consteval auto to_array(const std::vector<T>& vec) {

	compile_assert(vec.size() == Size);
	std::array<T, Size> result;
	std::copy(vec.begin(), vec.end(), result.begin());
	return result;
}

template <bool HasValue, typename T>
consteval auto unwrap_optional(const std::optional<T>& opt) {

	if constexpr (HasValue)
		return *opt;
	return std::nullopt;
}

