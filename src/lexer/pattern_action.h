#pragma once

#include "pattern.h"

#include <functional>

namespace lexer {

	namespace {

		namespace p = pattern;

		template <typename Action>
		concept action = requires(const Action& a, std::string_view lexeme) {
			std::invoke(a, lexeme);
		};

		template <p::pattern P, action Action>
		struct pattern_action {

			P pattern;
			Action action;
		};

		template <p::pattern P, action Action>
		consteval auto operator>>(const P& pattern, const Action& action) {
			
			return pattern_action{ pattern, action };
		}

		template <p::pattern P, typename Value>
		struct pattern_value {

			P pattern;
			Value value;
		};

		template <p::pattern P, typename Value>
		consteval auto operator>>(const P& pattern, const Value& value) {
			
			return pattern_value{ pattern, value };
		}

		template <auto... PatternActions>
		struct pattern_action_list {};
	}
}

