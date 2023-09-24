#include "lexer/scanner.h"

#include <cmath>
#include <charconv>

using namespace lexer::fsm;
using namespace lexer::pattern;

constexpr auto exponent = (('e'_p | 'E'_p), ~('-'_p | '+'_p), +digit);
constexpr auto float_literal = (~'-'_p, (*digit, '.'_p, +digit, ~exponent) | (+digit, exponent));

constexpr auto int_literal = (~'-'_p, +digit);

static consteval auto make_scanner() {

	using namespace lexer::pattern;

	using Action = double (*)(std::string_view lexeme);

	constexpr auto* reject = Action([](std::string_view str) -> double { return NAN; });

	constexpr auto* fl = Action([](std::string_view str) -> double {
		double result;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
		assert(ptr == str.data() + str.size());
		if (ec == std::errc::result_out_of_range)
			return NAN;
		return result;
	});
	constexpr auto* in = Action([](std::string_view str) -> double {
		int result;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
		assert(ptr == str.data() + str.size());
		if (ec == std::errc::result_out_of_range)
			return NAN;
		return result;
	});
	constexpr auto def1 = std::make_pair(float_literal, fl);
	constexpr auto def2 = std::make_pair(int_literal, in);

	using builder = lexer::scanner::builder<Action, reject, def1, def2>;
	return builder::make_scanner();
}

constinit auto scanner = make_scanner();

double scan_float(const char* ptr) {
	return scanner.scan(ptr);
}
