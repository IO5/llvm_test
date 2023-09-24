#pragma once

#include "token/token_definition.h"

#include <string>
#include <charconv>
#include <cassert>
#include <limits>

namespace lexer {

	namespace pattern {

		constexpr auto integer_literal = (~'-'_p, +digit);
		constexpr auto exponent = (('e'_p | 'E'_p), ~('-'_p | '+'_p), +digit);
		constexpr auto float_literal = (~'-'_p, (*digit, '.'_p, +digit, ~exponent) | (+digit, exponent));
		constexpr auto identifier = ((alpha, *alpnum) | ('_'_p, +alpnum));
	}

	using namespace token;
	using pattern::operator""_p;

	struct error {

		enum class code {
			unknown_token,
			float_literal_out_of_range,
			integer_literal_out_of_range
		};
		using enum code;

		code code;
		std::string lexeme;
	};

	template <typename T>
	struct literal {
		T value;
	};

	struct identifier {
		std::string name;
	};

	using token = token_definition <
		error,
		eof,
		op<"+">,
		op<"-">,
		op<"*">,
		op<"/">,
		op<"..">,
		sym<"(">,
		sym<")">,
		sym<"{">,
		sym<"}">,
		sym<"=">,
		sym<",">,
		keyword<"not">,
		keyword<"in">,
		keyword<"is">,
		keyword<"import">,
		keyword<"from">,
		keyword<"if">,
		keyword<"for">,
		keyword<"while">,
		keyword<"match">,
		keyword<"fun">,
		keyword<"val">,
		keyword<"var">,
		literal<bool>,
		literal<int>,
		literal<double>,
		literal<std::string>,
		identifier
	>;

	token integer_parser(std::string_view lexeme);
	token float_parser(std::string_view lexeme);

	using custom_patterns = pattern_action_list<
		("true"_p >> literal<bool>{true}),
		("false"_p >> literal<bool>{false}),
		("nan"_p >> literal<double>{ std::numeric_limits<double>::quiet_NaN() }),
		("inf"_p >> literal<double>{ std::numeric_limits<double>::infinity() }),
		("-inf"_p >> literal<double>{ -std::numeric_limits<double>::infinity() }),
		(pattern::integer_literal >> integer_parser),
		(pattern::float_literal >> float_parser),
		(pattern::identifier >>
			[](std::string_view lexeme) { return identifier{ std::string(lexeme) }; })
	>;
}

