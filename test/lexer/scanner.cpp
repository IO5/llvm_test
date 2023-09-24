#include <catch2/catch_amalgamated.hpp>

#include <cmath>

extern double scan_float(const char* ptr);

TEST_CASE("lexer::scanner") {

	REQUIRE(scan_float("1e1") == 1e1);
	REQUIRE(scan_float("2e+2") == 2e+2);
	REQUIRE(scan_float("10E-3") == 10E-3);
	REQUIRE(scan_float("1.5") == 1.5);
	REQUIRE(scan_float("-02.3") == -02.3);
	REQUIRE(scan_float("2137") == 2137.0);
	REQUIRE(scan_float("-23") == -23.0);
	REQUIRE(std::isnan(scan_float("1.")));
	REQUIRE(std::isnan(scan_float("e")));
	REQUIRE(std::isnan(scan_float("E+")));
}
