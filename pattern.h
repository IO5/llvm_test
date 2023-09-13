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

		template <typename P>
		struct at_most_ {

			using is_pattern = std::true_type;

			size_t max;
			P inner;
		};

		template <size_t max, typename P>
		consteval auto at_most(P inner) {
			return at_most_(max, inner);
		}

		template <typename P>
		struct at_least_ {

			using is_pattern = std::true_type;

			size_t min;
			P inner;
		};

		template <size_t min, typename P>
		consteval auto at_least(P inner) {
			return at_least(min, inner);
		}

		template <typename P>
		struct times_ {

			using is_pattern = std::true_type;

			size_t min;
			size_t max;
			P inner;
		};

		template <size_t min, size_t max, typename P>
		consteval auto times(P inner) {
			return times_(min, max, inner);
		}

		template <typename P>
		struct zero_or_more {

			using is_pattern = std::true_type;

			P inner;
		};

		consteval auto operator*(pattern auto p) {
			return zero_or_more(p);
		}

		template <typename P>
		struct one_or_more {

			using is_pattern = std::true_type;

			P inner;
		};

		consteval auto operator+(pattern auto p) {
			return one_or_more(p);
		}

		template <typename P>
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

			constexpr bool operator==(const range&) const = default;

			constexpr bool is_empty() const {
				return min > max;
			}

			constexpr range operator&(const range& other) const {
				return range{
					.min = std::max(min, other.min),
					.max = std::min(max, other.max)
				};
			}

			constexpr bool contains(const range& other) const {
				return min <= other.min && max >= other.max;
			}

			enum class overlap { disjoint, joint, contains, contained_by };

			constexpr overlap test_overlap(const range& other) const {

				using enum overlap;

				auto intersection = *this & other;

				if (intersection.is_empty())
					return disjoint;

				if (intersection == *this)
					return contained_by;

				if (intersection == other)
					return contains;

				return joint;
			}
		};

		constexpr auto single_char(char c) {
			return range(c, c);
		}
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
