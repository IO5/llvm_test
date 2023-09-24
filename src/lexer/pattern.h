#pragma once

#include "utils/static_string.h"

#include <utility>
#include <type_traits>
#include <string_view>

namespace lexer {

	using std::size_t;

	namespace pattern {

		struct pattern_base {};

		template <typename T>
		concept pattern = T::is_pattern::value;

		template <pattern... Ps>
		struct seq;

		template <pattern Last, pattern... Ps>
		struct seq<Last, Ps...> {

			using is_pattern = std::true_type;

			template <typename = void>
				requires (sizeof...(Ps) == 0)
			consteval seq(Last last) : last(last) {}

			consteval seq(seq<Ps...> front, Last last) :
				front(front), last(last) {}

			seq<Ps...> front;
			Last last;
		};

		template <pattern Last, pattern... Ps>
		seq(seq<Ps...> front, Last last) -> seq<Last, Ps...>;

		template<>
		struct seq<> {}; // intentionally not marked as pattern

		template <pattern L, pattern R>
		consteval auto operator,(L lhs, R rhs) {
			return seq<R, L>(seq<L>(lhs), rhs);
		}

		template <pattern... Ps, pattern R>
		consteval auto operator,(seq<Ps...> lhs, R rhs) {
			return seq<R, Ps...>(lhs, rhs);
		}

		template <pattern L, pattern R>
		struct or_ {

			using is_pattern = std::true_type;

			L lhs;
			R rhs;
		};

		consteval auto operator|(pattern auto lhs, pattern auto rhs) {
			return or_(lhs, rhs);
		}

		template <pattern P>
		struct at_most_ {

			using is_pattern = std::true_type;

			size_t max;
			P inner;
		};

		template <size_t max, pattern P>
		consteval auto at_most(P inner) {
			return at_most_(max, inner);
		}

		template <pattern P>
		struct at_least_ {

			using is_pattern = std::true_type;

			size_t min;
			P inner;
		};

		template <size_t min, pattern P>
		consteval auto at_least(P inner) {
			return at_least_(min, inner);
		}

		template <pattern P>
		struct times_ {

			using is_pattern = std::true_type;

			size_t min;
			size_t max;
			P inner;
		};

		template <size_t min, size_t max, pattern P>
		consteval auto times(P inner) {
			return times_(min, max, inner);
		}

		template <pattern P>
		struct zero_or_more {

			using is_pattern = std::true_type;

			P inner;
		};

		consteval auto operator*(pattern auto p) {
			return zero_or_more(p);
		}

		template <pattern P>
		struct one_or_more {

			using is_pattern = std::true_type;

			P inner;
		};

		consteval auto operator+(pattern auto p) {
			return one_or_more(p);
		}

		template <pattern P>
		struct zero_or_one {

			using is_pattern = std::true_type;

			P inner;
		};

		consteval auto operator~(pattern auto p) {
			return zero_or_one(p);
		}

		struct range {

			using is_pattern = std::true_type;

			char min;
			char max;
		};

		struct single_char {

			using is_pattern = std::true_type;

			char ch;
		};

		consteval auto char_seq(static_string<1> str) {
			return seq<single_char>{ single_char(str[0]) };
		}

		template <size_t N>
		consteval auto char_seq(static_string<N> str) {
			return seq(
				char_seq(str.substr<0, N - 1>()),
				single_char{ str[N - 1] }
			);
		};

		consteval auto operator ""_p(char c) {
			return single_char(c);
		}

		template <static_string Str>
		consteval auto operator ""_p() {
			if constexpr (Str.size == 1) {
				return single_char(Str[0]);
			} else {
				return char_seq(Str);
			}
		}

		constexpr auto any_char = range(-128, 127);
		constexpr auto digit = range('0', '9');
		constexpr auto alpha_lowercase = range('a', 'z');
		constexpr auto alpha_uppercase = range('A', 'Z');
		constexpr auto alpha = alpha_lowercase | alpha_uppercase;
		constexpr auto alpnum = alpha | digit;
		//constexpr auto non_ascii = range('\x80', '\xFF'); // TODO unicode pattern
	}
}
