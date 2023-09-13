#pragma once

#include "pattern.h"
#include "constexpr_utils.h"

#include <string_view>
#include <vector>
#include <algorithm>

namespace lexer {

	using std::string_view, std::size_t;

	namespace pattern {

		constexpr auto exponent = (('e'_p | 'E'_p), ~('-'_p | '+'_p), +digit);
		constexpr auto float_literal = (~'-'_p, (*digit, '.'_p, +digit, ~exponent) | (+digit, exponent));
	}

	namespace fsm {

		namespace p = pattern;
		using p::range;

		namespace detail {

			template <typename T, size_t... Is>
			constexpr auto tuple_pop(T tuple, std::index_sequence<Is...>) {
				return std::make_pair(
					std::make_tuple(std::get<Is>(tuple) ...),
					std::get<std::tuple_size_v<T> - 1>(tuple)
				);
			}
		}

		// get last element + reminder of the tuple
		template <typename T>
		constexpr auto tuple_pop(T tuple) {
			return detail::tuple_pop(
				tuple,
				std::make_index_sequence<std::tuple_size_v<T> - 1>{}
			);
		}

		constexpr auto eps = range(1, 0);

		class nfa {

		public:
			using state_id = size_t;

			struct transition {

				range input;
				state_id next;
			};

			struct state {

				std::vector<transition> trans;
				bool final = false;
			};

			// initial always first, final always last
			std::vector<state> states;

			static consteval nfa from_pattern(p::range pattern) {

				nfa res;
				res.states.resize(2);
				res.add_transition(0, 1, pattern);
				res.states[1].final = true;
				return res;
			}

			template <p::pattern... Ps>
			static consteval nfa from_pattern(p::seq<Ps...> seq) {

				return from_pattern(seq.patterns);
			}

			template <p::pattern... Ps>
			static consteval nfa from_pattern(std::tuple<Ps...> seq) {

				auto& [reminder, last] = tuple_pop(seq);
				auto res = from_pattern(reminder);
				
				res.join(from_pattern(last));

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(std::tuple<P> seq) {

				return from_pattern(std::get<0>(seq));
			}

			template <p::pattern L, p::pattern R>
			static consteval nfa from_pattern(p::or_<L, R> pattern) {

				auto l_nfa = from_pattern(pattern.lhs);
				auto r_nfa = from_pattern(pattern.rhs);

				nfa res;
				res.states.emplace_back(); // initial

				auto l_final = l_nfa.states.size(); 
				l_nfa.states.back().final = false;
				res.splice_states(std::move(l_nfa));

				auto r_final = l_final + r_nfa.states.size();
				r_nfa.states.back().final = false;
				res.splice_states(std::move(r_nfa));

				// make new final
				res.states.emplace_back().final = true;
				auto new_final = res.states.size() - 1;
				add_transition(l_final, new_final, eps);
				add_transition(r_final, new_final, eps);

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::one_or_more<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add back eps transition
				res.add_transition(res.states.size() - 2, 1, eps);

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::zero_or_one<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add skip eps transition
				res.add_transition(0, res.states.size() - 1, eps);

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::zero_or_more<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add skip eps transition
				res.add_transition(0, res.states.size() - 1, eps);

				// add back eps transition
				res.add_transition(res.states.size() - 2, 1, eps);

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::at_least_<P> pattern) {

				// transform A{n,} into A..AA+

				nfa res;
				res.states.emplace_back().final = true;

				if (pattern.min == 0)
					return from_pattern(p::zero_or_more{ pattern.inner });

				nfa single = from_pattern(pattern.inner);

				while (--pattern.min)
					res.join(nfa(single));

				single.extend();

				// add back eps transition
				single.add_transition(single.states.size() - 2, 1, eps);

				res.join(std::move(single));

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::at_most_<P> pattern) {

				// transform A{,n} into A?..A?
				compile_assert(pattern.max != 0);

				nfa res;
				res.states.emplace_back().final = true;

				nfa single = from_pattern(pattern.inner);
				single.extend();
				// add skip eps transition
				single.add_transition(0, single.states.size() - 1, eps);

				while (--pattern.max)
					res.join(nfa(single));
				res.join(std::move(single));

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::times_<P> pattern) {

				// transform A{n,m} into A..A A?..A?
				compile_assert(pattern.min <= pattern.max);

				nfa res;
				res.states.emplace_back().final = true;

				nfa single = from_pattern(pattern.inner);

				auto min = pattern.min;
				while (min--)
					res.join(nfa(single));

				single.extend();
				// add skip eps transition
				single.add_transition(0, single.states.size() - 1, eps);

				auto reminder = pattern.max - pattern.min;
				if (reminder > 0) {

					while (--reminder)
						res.join(nfa(single));

					res.join(std::move(single));
				}

				return res;
			}

		private:
			consteval void join(nfa&& other) {

				// merge this's final with other's initial

				// first, remove final temporarily
				auto final = std::move(states.back());
				states.pop_back();

				auto shift = splice_states(std::move(other));

				// merge states: copy transitions from the old final to the common state
				states[shift].trans.append_range(std::move(final.trans));
			}

			// add extra initial and final states with eps transitions
			consteval void extend() {

				// add new initial
				shift_indexes(1);
				states.emplace(states.begin());
				add_transition(0, 1, eps);

				// add new final
				states.back().final = false;
				states.emplace_back().final = true;
				add_transition(states.size() - 2, states.size() - 1, eps);
			}

			consteval void add_transition(state_id from, state_id to, range input) {

				states[from].trans.push_back({
					.input = input,
					.next = to
				});
			}

			consteval void shift_indexes(size_t shift) {

				for (auto& s : states)
					for (auto& t : s.trans)
						t.next += shift;
			}

			consteval size_t splice_states(nfa&& other) {

				auto shift = states.size();

				other.shift_indexes(shift);
				states.append_range(std::move(other.states));

				return shift;
			}
		};

		consteval void foo() {
		}
	}
}
