#pragma once

#include "utils/static_string.h"

#include <utility>
#include <type_traits>
#include <tuple>
#include <string>

namespace lexer {

	using std::size_t;

	namespace pattern {

		template <typename T>
		concept pattern = T::is_pattern::value;

		template <pattern... Ps>
		struct seq {

			using is_pattern = std::true_type;

			consteval seq(Ps... ps) :
				patterns(ps...) {}

			consteval seq(std::tuple<Ps...> tuple) :
				patterns(tuple) {}

			std::tuple<Ps...> patterns;
		};

		template<>
		struct seq<> {}; // intentionally not marked as pattern

		template <typename... Ls, typename... Rs>
		consteval auto operator,(seq<Ls...> lhs, seq<Rs...> rhs) {
			return seq(std::tuple_cat(lhs.patterns, rhs.patterns));
		}

		template <typename... Ls>
		consteval auto operator,(seq<Ls...> lhs, pattern auto rhs) {
			return seq(std::tuple_cat(lhs.patterns, std::make_tuple(rhs)));
		}

		template <typename... Rs>
		consteval auto operator,(pattern auto lhs, seq<Rs...> rhs) {
			return seq(std::tuple_cat(std::make_tuple(lhs), rhs.patterns));
		}

		consteval auto operator,(pattern auto lhs, pattern auto rhs) {
			return seq(lhs, rhs);
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

		template <static_string Str>
		consteval auto operator ""_p() {
			if constexpr (Str.size == 1) {
				return single_char(Str[0]);
			} else {
				return char_seq_impl(Str.data.data(), std::make_index_sequence<Str.size>{});
			}
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

		consteval auto operator ""_p(char c) {
			return single_char(c);
		}

		template <size_t... Is>
		consteval auto char_seq_impl(const char* str, std::index_sequence<Is...>) {
			return seq(single_char(str[Is])...);
		}

		template <size_t N>
		consteval auto char_seq(const char(&str)[N]) {
			return char_seq_impl(str, std::make_index_sequence<N>{});
		};

		template <size_t N>
		consteval auto char_seq(static_string<N> str) {
			return char_seq_impl(str, std::make_index_sequence<N>{});
		};

		constexpr auto any_char = range(-128, 127);
		constexpr auto digit = range('0', '9');
		constexpr auto alpha_lowercase = range('a', 'z');
		constexpr auto alpha_uppercase = range('A', 'Z');
		constexpr auto alpha = alpha_lowercase | alpha_uppercase;
		constexpr auto alpnum = alpha | digit;
		//constexpr auto non_ascii = range('\x80', '\xFF'); // TODO unicode pattern
	}
}
