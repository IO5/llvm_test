#pragma once

#include "token/token_definition.h"

#include <string>
#include <charconv>
#include <cassert>
#include <limits>

namespace token {

	struct error {

		enum class code {
			unknown_token,
			float_literal_out_of_range,
			integer_literal_out_of_range
		};
		using enum code;

		code code;
		std::string lexeme;

		constexpr bool operator==(const error&) const = default;
	};

	template <typename T>
	struct literal {
		T value;

		constexpr bool operator==(const literal&) const = default;
	};

	struct identifier {
		std::string name;

		constexpr bool operator==(const identifier&) const = default;
	};

	namespace pattern {

		using namespace lexer::pattern;

		constexpr auto integer_literal = (~'-'_p, +digit);
		constexpr auto exponent = (('e'_p | 'E'_p), ~('-'_p | '+'_p), +digit);
		constexpr auto float_literal = (~'-'_p, (*digit, '.'_p, +digit, ~exponent) | (+digit, exponent));
		constexpr auto identifier = (alpha | (('_'_p | alpha), +('_'_p | alpnum)));
	}

	using namespace token;

	struct token : token_definition <
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
		sym<"_">,
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
	> {
		using token_definition::token_definition;
	};

	token integer_parser(std::string_view lexeme);
	token float_parser(std::string_view lexeme);

	using lexer::pattern::operator""_p;
	using lexer::operator>>;

	using custom_patterns = lexer::pattern_action_list<
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

namespace tk = token;
