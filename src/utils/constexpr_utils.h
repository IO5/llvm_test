#pragma once

#include <cstdlib>

constexpr void compile_assert(bool test) {

	if (!test)
		std::abort();
}

