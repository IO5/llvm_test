#include "utils/flat_map.h"
#include "utils/constexpr_utils.h"

#include <catch2/catch_amalgamated.hpp>

static consteval void compile_time_tests() {

	{ // ctors
		flat_map<int, int> def;
		compile_assert(def.empty());

		flat_map<int, int> init({ {1,2}, {2,1} });
		compile_assert(init.size() == 2);
		compile_assert(init.contains(1));
		compile_assert(init.contains(2));
		compile_assert(!init.contains(0));
		compile_assert(!init.contains(3));
		compile_assert(*init.get(1) == 2);
		compile_assert(*init.get(2) == 1);

		flat_map<int, int> multi({ {2, 3}, {1,1}, {2,2} });
		compile_assert(init.size() == 2);
		compile_assert(init.contains(1));
		compile_assert(init.contains(2));
		compile_assert(!init.contains(0));
		compile_assert(!init.contains(3));
		compile_assert(init.get(0) == nullptr);
	}

	{ // add
		flat_map<int, int> m;
		m.add({ 0, 0 });
		compile_assert(m.size() == 1);
		compile_assert(m.contains(0));
		m.add({ 1, 1 });
		compile_assert(m.size() == 2);
		compile_assert(m.contains(0));
		compile_assert(m.contains(1));
		m.add({ 0, 5 });
		compile_assert(m.size() == 2);
		compile_assert(m.contains(0));
		compile_assert(m.contains(1));
		compile_assert(*m.get(0) == 5);
		compile_assert(m.get(3) == nullptr);
	}

	{ // del
		flat_map<int, int> m({ {3, 3}, {1,1}, {2,2} });
		m.del(2);
		compile_assert(!m.contains(2));
		compile_assert(m.size() == 2);
		m.del(1);
		m.del(3);
		compile_assert(m.empty());
	}
}

TEST_CASE("flat_map") {

	compile_time_tests();

	SECTION("ctors") {
		flat_map<int, int> def;
		REQUIRE(def.empty());

		flat_map<int, int> init({ {1,2}, {2,1} });
		REQUIRE(init.size() == 2);
		REQUIRE(init.contains(1));
		REQUIRE(init.contains(2));
		REQUIRE(!init.contains(0));
		REQUIRE(!init.contains(3));
		REQUIRE(*init.get(1) == 2);
		REQUIRE(*init.get(2) == 1);
		REQUIRE(init.get(0) == nullptr);

		flat_map<int, int> multi({ {2, 3}, {1,1}, {2,2} });
		REQUIRE(init.size() == 2);
		REQUIRE(init.contains(1));
		REQUIRE(init.contains(2));
		REQUIRE(!init.contains(0));
		REQUIRE(!init.contains(3));
	}

	SECTION("add") {
		flat_map<int, int> m;
		m.add({ 0, 0 });
		REQUIRE(m.size() == 1);
		REQUIRE(m.contains(0));
		m.add({ 1, 1 });
		REQUIRE(m.size() == 2);
		REQUIRE(m.contains(0));
		REQUIRE(m.contains(1));
		m.add({ 0, 5 });
		REQUIRE(m.size() == 2);
		REQUIRE(m.contains(0));
		REQUIRE(m.contains(1));
		REQUIRE(*m.get(0) == 5);
		REQUIRE(m.get(3) == nullptr);
	}

	SECTION("del") {
		flat_map<int, int> m({ {3, 3}, {1,1}, {2,2} });
		m.del(2);
		REQUIRE(!m.contains(2));
		REQUIRE(m.size() == 2);
		m.del(1);
		m.del(3);
		REQUIRE(m.empty());
	}
}

