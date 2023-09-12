#pragma once

#include "static_string.h"

#include <utility>
#include <type_traits>
#include <tuple>
#include <string>

namespace lexer {

	using std::size_t;

	namespace pattern {

		template <typename T>
		concept pattern = T::is_pattern::value;

		template <typename... Ps>
		struct seq {

			using is_pattern = std::true_type;

			consteval seq(Ps... ps) :
				patterns(ps...) {}

			consteval seq(std::tuple<Ps...> tuple) :
				patterns(tuple) {}

			std::tuple<Ps...> patterns;

			std::string to_string() const {
				return '(' + to_string_impl(std::index_sequence_for<Ps...>{}) + ')';
			}

		private:
			template <size_t... Is>
			std::string to_string_impl(std::index_sequence<Is...>) const {
				return (std::get<Is>(patterns).to_string() + ...);
			}
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

		template <typename L, typename R>
		struct or_ {

			using is_pattern = std::true_type;

			L lhs;
			R rhs;

			std::string to_string() const {
				return '(' + lhs.to_string() + '|' + rhs.to_string() + ')';
			}
		};

		consteval auto operator|(pattern auto lhs, pattern auto rhs) {
			return or_(lhs, rhs);
		}

		constexpr size_t Any = size_t(-1);

		/*template <typename P>
		struct times_ : pattern_base {
			consteval times_(size_t min, size_t max, P inner) :
				min(min), max(max), inner(inner) {}

			size_t min;
			size_t max;
			P inner;

			consteval bool covers(pattern auto other) const {
				return inner.covers(other);
			}
		};

		template <size_t min, size_t max, typename P>
		consteval auto times(P inner) {
			return times_(min, max, inner);
		}*/

		template <typename P>
		struct zero_or_more {

			using is_pattern = std::true_type;

			P inner;

			std::string to_string() const {
				return inner.to_string() + '*';
			}
		};

		consteval auto operator*(pattern auto p) {
			return zero_or_more(p);
		}

		template <typename P>
		struct one_or_more {

			using is_pattern = std::true_type;

			P inner;

			std::string to_string() const {
				return inner.to_string() + '+';
			}
		};

		consteval auto operator+(pattern auto p) {
			return one_or_more(p);
		}

		template <typename P>
		struct zero_or_one {

			using is_pattern = std::true_type;

			P inner;

			std::string to_string() const {
				return inner.to_string() + '?';
			}
		};

		consteval auto operator~(pattern auto p) {
			return zero_or_one(p);
		}

		struct single_char {

			using is_pattern = std::true_type;

			char c;

			std::string to_string() const {
				return std::string(&c, 1);
			}
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

		template <static_string Str>
		consteval auto operator ""_p() {
			if constexpr (Str.size == 1) {
				return single_char(Str[0]);
			} else {
				return char_seq_impl(Str.data.data(), std::make_index_sequence<Str.size>{});
			}
		}

		struct any_char {

			using is_pattern = std::true_type;

			std::string to_string() const {
				return ".";
			}
		};

		struct range {

			using is_pattern = std::true_type;

			char min;
			char max;

			std::string to_string() const {
				return std::string("[") + min + '-' + max + ']';
			}
		};

		constexpr auto digit = range('0', '9');
		constexpr auto alpha_lowercase = range('a', 'z');
		constexpr auto alpha_uppercase = range('A', 'Z');
		constexpr auto alpha = alpha_lowercase | alpha_uppercase;
	}
}
