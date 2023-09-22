#include "lexer/fsm.h"

#include <catch2/catch_amalgamated.hpp>

using namespace lexer::fsm;

static consteval void compile_time_tests() {

	using namespace lexer::pattern;
	constexpr auto p = ('a'_p, zero_or_more('a'_p));

}

TEST_CASE("lexer::fsm") {

	compile_time_tests();

	using namespace lexer::pattern;
	constexpr auto p = (zero_or_one('a'_p), ('b'_p | range('b', 'd')), zero_or_more(one_or_more('d'_p)), at_least<2>(range('c', 'e')), at_most<2>('f'_p), times<1, 3>('z'_p));

	auto n = nfa<int>::from_pattern(p);
	n.states.back().action = 23;
	auto d = dfa<int>::from_nfa(n);

	size_t s = 0;
	for (char c : std::string_view("abee")) {
		s = d.step(s, c);
		if (s == -1)
			break;
	}

	std::optional<int> r;
	if (s != -1)
		r = d.states[s].action;

	int _ = 2;
}
