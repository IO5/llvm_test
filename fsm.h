#pragma once

#include "pattern.h"

#include <string_view>
#include <tuple>
#include <format>

namespace lexer {

	using std::string_view;

	namespace pattern {

		constexpr auto exponent = (('e'_p | 'E'_p), ~('-'_p | '+'_p), +digit);
		constexpr auto float_literal = (~'-'_p, (*digit, '.'_p, +digit, ~exponent) | (+digit, exponent));
	}

	namespace fsm {

		using std::tuple, std::make_tuple, std::tuple_element_t, std::tuple_size_v, std::get, std::make_pair;
		using std::index_sequence, std::make_index_sequence, std::index_sequence_for;

		namespace detail {

			template <typename T, size_t... Is>
			constexpr auto tuple_pop_impl(T tuple, index_sequence<Is...>) {
				return make_pair(
					get<0>(tuple),
					make_tuple(std::get<Is + 1>(tuple) ...)
				);
			}

			template <bool Replace, typename T, typename U>
			constexpr auto conditional_replace(T original, U replacement) {
				if constexpr (Replace) {
					return replacement;
				} else {
					return original;
				}
			}

			template <size_t I, typename T, typename U, size_t... Is>
			constexpr auto tuple_set_impl(T tuple, U value, index_sequence<Is...>) {
				return make_tuple(
					conditional_replace<I == Is>(get<Is>(tuple), value) ...
				);
			}

		}

		template <size_t I>
		using constant = std::integral_constant<size_t, I>;

		template <typename T>
		constexpr auto tuple_indexes = make_index_sequence<tuple_size_v<T>>{};

		template <typename T>
		constexpr auto tuple_size(T tuple) {
			return std::tuple_size_v<T>;
		}

		// get first element + reminder of the tuple
		template <typename T>
		constexpr auto tuple_pop(T tuple) {
			constexpr auto new_size = tuple_size(tuple) - 1;
			return detail::tuple_pop_impl(tuple, make_index_sequence<new_size>{});
		}

		template <size_t I, typename T, typename U>
		constexpr auto tuple_set(T tuple, U value) {
			return detail::tuple_set_impl<I>(tuple, value, tuple_indexes<T>);
		}

		template <typename T, typename U>
		constexpr auto tuple_append(T tuple, U value) {
			return tuple_cat(tuple, make_tuple(value));
		}

		namespace p = pattern;

		struct state {};

		template <typename Action>
		struct final_state {

			Action action;

			decltype(auto) resolve(string_view lexeme) {
				return std::invoke(action, lexeme);
			}
		};

		template <typename T>
		constexpr bool is_final_state_v = !std::is_same_v<state, T>;

		template <p::pattern Pattern>
		struct transition {
			Pattern pattern;
			size_t next;
		};

		template <typename States, typename Transitions>
		class builder;

		template <typename States, typename Transitions>
		consteval auto make_builder(States states, Transitions transitions) {
			return builder<States, Transitions>(states, transitions);
		}

		template<int = 0>
		consteval auto make_builder() {
			return make_builder(tuple<state>{}, tuple<tuple<>>{});
		}

		template <typename States, typename Transitions>
		class builder {

		public:
			consteval builder(States states, Transitions transitions) :
				states(states), transitions(transitions) {}

			States states;
			Transitions transitions;

			std::string to_string() const {
				return std::format(
					"{{\n states: {}\n transitions:\n{}}}",
					states_to_string(state_indexes),
					transitions_to_string(transition_indexes)
				);
			}

		//private:
			static constexpr auto state_indexes = make_index_sequence<tuple_size_v<States>>{};
			static constexpr auto transition_indexes = make_index_sequence<tuple_size_v<Transitions>>{};

			consteval auto add_state() const {
				return make_builder(
					tuple_append(states, state{}),
					tuple_append(transitions, tuple<>{})
				);
			}

			consteval auto copy_state_with_transitions(size_t st) const {

				// We only alter the source, so the transition are exactly the same as the original ones,
				// but at a different index.
				auto trans = get<st>(transitions);
				
				auto new_states = tuple_append(states, get<st>(state)); // we need to preserve final-ness
				constexpr size_t new_state = tuple_size(new_states);

				return make_builder(new_states, tuple_append(transitions, trans));
			}

			template <size_t from_state>
			consteval auto add_pattern(p::single_char pattern) const {

				auto state_trans = get<from_state>(transitions);

				auto new_states = tuple_append(states, state{});
				constexpr size_t next_state = tuple_size_v<decltype(new_states)> - 1;
				auto new_state_trans = tuple_append(state_trans, transition{ pattern, next_state });

				auto new_trans = tuple_append(
					tuple_set<from_state>(transitions, new_state_trans),
					tuple<>{} // we `need to add an empty row for the new state
				);

				return make_pair(
					constant<next_state>{},
					make_builder(new_states, new_trans)
				);
			}

			/// Adds sequence of patterns to the machine.
			// Returns final state of the sequence + altered builder.
			template <p::pattern... Ps>
			consteval auto add_pattern_seq(size_t current_state, tuple<Ps...> patterns) const {

				auto& [head, tail] = tuple_pop(patterns);

				auto& [next_state, new_builder] = add_single_pattern(current_state, head);

				return new_builder.add_pattern_seq(next_state, tail);
			}

			consteval auto add_pattern_seq(size_t current_state, tuple<>) const {
				return make_pair(
					constant<current_state>,
					*this
				);
			}

			// --- debug ---

			template <size_t... Is>
			std::string states_to_string(index_sequence<Is...>) const {
				return (
					(is_final_state_v<tuple_element_t<Is, States>> ? '<' + std::to_string(Is) + '>' : std::to_string(Is) + ' ')
					+ ...
				);
			}

			template <typename T, size_t... Is>
			std::string state_transitions_to_string(T state_trans, index_sequence<Is...>) const {
				if constexpr (sizeof...(Is) == 0) {
					return "";
				} else {
					return (
						std::format(" =>{}", get<Is>(state_trans).next)
						+ ...
					);
				}
			}

			template <size_t... Is>
			std::string transitions_to_string(index_sequence<Is...>) const {
				return (
					std::format(
						"  {}:{}\n",
						Is,
						state_transitions_to_string(
							get<Is>(transitions),
							make_index_sequence<tuple_size_v<tuple_element_t<Is, Transitions>>>{}
						)
					) + ...
				);
			}
		};

	}
}
