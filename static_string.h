#pragma once

#include <string_view>
#include <array>
#include <algorithm>
#include <cassert>

template <std::size_t N>
struct static_string {

	consteval static_string(const char* begin, const char* end) {
		assert(end - begin == N);
		std::copy(begin, end, data.data());
		data[N] = '\0';
	}

	template <std::size_t M>
	consteval static_string(const char(&str)[M]) {
		static_assert(N + 1 == M);
		assert(str[N] == '\0');
		std::copy(str, str + N + 1, data.data());
	}

	constexpr operator std::string_view() const {
		return std::string_view(data.data(), N);
	}

	template <std::size_t off, std::size_t count = std::string_view::npos>
	consteval auto substr() const {
		static_assert(off <= N);
		constexpr std::size_t real_count = (count == std::string_view::npos) ? N - off : std::min(N - off, count);
		auto begin = data.data() + off;
		return static_string<real_count>(begin, begin + real_count);
	}

	constexpr char operator[](std::size_t idx) const {
		return data[idx];
	}

	std::array<char, N + 1> data;
	static constexpr std::size_t size = N;
};

template <std::size_t M>
static_string(const char(&str)[M]) -> static_string<M - 1>;

