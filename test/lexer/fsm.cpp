#include "lexer/fsm.h"
#include "lexer/scanner.h"

#include <catch2/catch_amalgamated.hpp>

using namespace lexer::fsm;

using namespace lexer::pattern;

//TODO
constexpr auto exponent = (('e'_p | 'E'_p), ~('-'_p | '+'_p), +digit);
constexpr auto float_literal = (~'-'_p, (*digit, '.'_p, +digit, ~exponent) | (+digit, exponent));

static consteval auto compile_time_tests() {

	using namespace lexer::pattern;
	constexpr auto p = ('a'_p, zero_or_more('a'_p));

	using Action = int (*)(std::string_view lexeme);

	constexpr auto* reject = Action([](std::string_view) { return 0; });
	constexpr auto* accept = Action([](std::string_view) { return 1; });
	constexpr auto def = std::make_pair(p, accept);

	using builder = lexer::scanner::builder<Action, reject, def>;
	constexpr auto sc = builder::make_scanner();
	return sc;
}

TEST_CASE("lexer::fsm") {

	auto sc = compile_time_tests();
	using namespace lexer::pattern;

	REQUIRE(sc.scan("a") == 1);
	REQUIRE(sc.scan("aa") == 1);
	REQUIRE(sc.scan("aaaaaaa") == 1);
	REQUIRE(sc.scan("b") == 0);
	REQUIRE(sc.scan("ba") == 0);
	REQUIRE(sc.scan("") == 0);
	//REQUIRE(sc.scan("1.0") == 1);
	//REQUIRE(sc.scan("-2.5") == 1);
	//REQUIRE(sc.scan("0.0") == 1);


}
