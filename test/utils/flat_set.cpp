#include "utils/flat_set.h"

#include <catch2/catch_amalgamated.hpp>

TEST_CASE("flat_set") {

	flat_set<size_t> s;
	flat_set<flat_set<size_t>> i = { s };
}
