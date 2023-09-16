#include "lexer/fsm.h"

#include <catch2/catch_amalgamated.hpp>

using namespace lexer::fsm;

static consteval void compile_time_tests() {

	auto n = nfa<int>();
	auto d = dfa<int>::from_nfa(n);
}

TEST_CASE("lexer::fsm") {

	compile_time_tests();
}
