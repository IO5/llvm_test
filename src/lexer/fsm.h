#pragma once

#include "pattern.h"
#include "utils/constexpr_utils.h"
#include "utils/flat_set.h"

#include <string_view>
#include <vector>
#include <algorithm>
#include <optional>

namespace lexer {

	using std::string_view, std::size_t;

	// TODO
	namespace pattern {

		constexpr auto exponent = (('e'_p | 'E'_p), ~('-'_p | '+'_p), +digit);
		constexpr auto float_literal = (~'-'_p, (*digit, '.'_p, +digit, ~exponent) | (+digit, exponent));
	}

	namespace fsm {

		namespace p = pattern;

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

		struct interval {

			char min;
			char max;

			constexpr auto operator<=>(const interval&) const = default;

			constexpr bool is_empty() const {
				return min > max;
			}

			constexpr interval operator&(const interval& other) const {
				return interval{
					.min = std::max(min, other.min),
					.max = std::min(max, other.max)
				};
			}

			constexpr interval operator|(const interval& other) const {
				return interval{
					.min = std::min(min, other.min),
					.max = std::max(max, other.max),
				};
			}

			constexpr bool contains(const interval& other) const {
				return min <= other.min && max >= other.max;
			}

			constexpr bool empty() const {
				return min > max;
			}
		};

		using state_id = size_t;

		struct transition { state_id next; interval input; };
		struct eps_transition { state_id next; };

		template <typename Action>
		class nfa {

		public:
			struct state {

				std::vector<eps_transition> eps_trans;
				std::vector<transition> trans;
				std::optional<Action> action; // action to execute if the machine stops here/is final marker
			};

			// initial always first, final always last
			std::vector<state> states;

			constexpr nfa() {
				states.emplace_back();
			}

			static constexpr nfa from_pattern(p::single_char pattern) {

				nfa res;
				res.states.emplace_back();
				states[0].trans.push_back({
					.next = 1,
					.input = interval{ pattern.ch, pattern.ch }
				});
				return res;
			}

			static constexpr nfa from_pattern(p::range pattern) {

				nfa res;
				res.states.emplace_back();
				states[0].trans.push_back({
					.next = 1,
					.input = { pattern.min, pattern.max }
				});
				return res;
			}

			template <p::pattern... Ps>
			static constexpr nfa from_pattern(p::seq<Ps...> seq) {

				return from_pattern(seq.patterns);
			}

			template <p::pattern... Ps>
			static constexpr nfa from_pattern(std::tuple<Ps...> seq) {

				auto& [reminder, last] = tuple_pop(seq);
				auto res = from_pattern(reminder);
				
				res.join(from_pattern(last));

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(std::tuple<P> seq) {

				return from_pattern(std::get<0>(seq));
			}

			template <p::pattern L, p::pattern R>
			static constexpr nfa from_pattern(p::or_<L, R> pattern) {

				auto l_nfa = from_pattern(pattern.lhs);
				auto r_nfa = from_pattern(pattern.rhs);

				nfa res;

				auto l_final = l_nfa.states.size(); 
				res.splice_states(std::move(l_nfa));
				add_eps_transition(0, 1);

				auto r_final = l_final + r_nfa.states.size();
				res.splice_states(std::move(r_nfa));
				add_eps_transition(0, l_final + 1);

				// make new final
				res.states.emplace_back();
				auto new_final = res.states.size() - 1;
				add_eps_transition(l_final, new_final);
				add_eps_transition(r_final, new_final);

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(p::one_or_more<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add back eps transition
				res.add_eps_transition(res.states.size() - 2, 1);

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(p::zero_or_one<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add skip eps transition
				res.add_eps_transition(0, res.states.size() - 1);

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(p::zero_or_more<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add skip eps transition
				res.add_eps_transition(0, res.states.size() - 1);

				// add back eps transition
				res.add_eps_transition(res.states.size() - 2, 1);

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(p::at_least_<P> pattern) {

				// transform A{n,} into A..AA+

				nfa res;

				if (pattern.min == 0)
					return from_pattern(p::zero_or_more{ pattern.inner });

				nfa single = from_pattern(pattern.inner);

				while (--pattern.min)
					res.join(nfa(single));

				single.extend();

				// add back eps transition
				single.add_eps_transition(single.states.size() - 2, 1);

				res.join(std::move(single));

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(p::at_most_<P> pattern) {

				// transform A{,n} into A?..A?
				compile_assert(pattern.max != 0);

				nfa res;

				nfa single = from_pattern(pattern.inner);
				single.extend();
				// add skip eps transition
				single.add_eps_transition(0, single.states.size() - 1);

				while (--pattern.max)
					res.join(nfa(single));
				res.join(std::move(single));

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(p::times_<P> pattern) {

				// transform A{n,m} into A..A A?..A?
				compile_assert(pattern.min <= pattern.max);

				nfa res;

				nfa single = from_pattern(pattern.inner);

				auto min = pattern.min;
				while (min--)
					res.join(nfa(single));

				single.extend();
				// add skip eps transition
				single.add_eps_transition(0, single.states.size() - 1);

				auto reminder = pattern.max - pattern.min;
				if (reminder > 0) {

					while (--reminder)
						res.join(nfa(single));

					res.join(std::move(single));
				}

				return res;
			}

			// similar to or_ but disjoint final states, so only suitable for final mutliple pattern merge
			template <typename Action>
			friend constexpr auto merge_nfas(std::vector<nfa>&& nfas) {

				nfa res;

				for (auto& n : nfas) {
					state_id initial = res.states.size();
					res.splice_states(std::move(n));
					add_eps_transition(0, initial);
				}

				return res;
			}

			constexpr auto eps_closure(const flat_set<state_id>& state_ids) const {

				flat_set<state_id> result;
				flat_set<state_id> current = state_ids;
				while (!current.empty()) {

					flat_set<state_id> tmp;
					for (auto id : current) {
						for (auto [next] : states[id].eps_trans) {
							if (!current.contains(next) && !result.contains(next))
								tmp.add(next);
						}
					}
					result.add(std::move(current));
					current = std::move(tmp);
				}

				return result;
			}

			constexpr auto get_possible_inputs(const flat_set<state_id>& state_ids) const {

				flat_set<interval> trans_inputs;

				for (auto id : state_ids)
					for (auto t : states[id].trans)
						trans_inputs.add(t.input);
				
				std::vector<interval> inputs(std::from_range, std::move(trans_inputs));
				std::vector<interval> result;

				auto find_intersects = [&](interval current) {

					for (auto it = inputs.begin(); it != inputs.end(); ++it) {
						auto other_input = *it;

						auto intersect = current & other_input;
						if (intersect.empty())
							continue;

						auto sum = current | other_input;
						inputs.erase(it);
						inputs.push_back(interval{ .min = sum.min, .max = intersect.min - 1 });
						inputs.push_back(intersect);
						inputs.push_back(interval{ .min = intersect.max + 1, .max = sum.max });
						return;
					}

					result.push_back(current);
				};

				while (!inputs.empty()) {
					auto current = inputs.front();
					inputs.erase(inputs.begin());

					find_intersects(current);
				}

				return result;
			}

			constexpr auto move(const flat_set<state_id>& state_ids, interval input) const {

				flat_set<state_id> result;

				for (auto id : state_ids) {
					const auto& state = states[id];

					for (auto [t_next, t_input] : state.trans)
						if (t_input.contains(input))
							result.add(t_next);
				}

				return result;
			}

		private:
			constexpr void join(nfa&& other) {

				// merge this's final with other's initial

				// first, remove final temporarily
				auto final = std::move(states.back());
				states.pop_back();

				auto shift = splice_states(std::move(other));

				// merge states: copy transitions from the old final to the common state
				states[shift].eps_trans.append_range(std::move(final.eps_trans));
				states[shift].trans.append_range(std::move(final.trans));
			}

			// add extra initial and final states with eps transitions
			constexpr void extend() {

				// add new initial
				shift_indexes(1);
				states.emplace(states.begin());
				add_eps_transition(0, 1);

				// add new final
				states.emplace_back();
				add_eps_transition(states.size() - 2, states.size() - 1);
			}

			constexpr void add_eps_transition(state_id from, state_id to) {

				states[from].eps_trans.push_back({ .next = to });
			}

			constexpr void shift_indexes(size_t shift) {

				for (auto& s : states)
					for (auto& t : s.trans)
						t.next += shift;
			}

			constexpr size_t splice_states(nfa&& other) {

				auto shift = states.size();

				other.shift_indexes(shift);
				states.append_range(std::move(other.states));

				return shift;
			}
		};

		template <typename Action>
		class dfa {

		public:
			struct state {

				std::vector<transition> trans;
				std::optional<Action> action; // action to execute if the machine stops here/is final marker
			};

			constexpr dfa() = default;
			static constexpr dfa from_nfa(const nfa<Action>& nfa) {

				/*const auto initial = nfa.eps_closure({ 0 });
				std::vector<std::pair<flat_set<state_id>, Action>> finals;

				flat_set<flat_set<state_id>> states = { initial };
				std::vector<flat_set<state_id>> unmarked = { initial };

				struct transition {
					interval input;
					flat_set<state_id> from;
					flat_set<state_id> to;
				};
				std::vector<transition> trans;
				
				while (!unmarked.empty()) {
					auto current = unmarked.back();
					unmarked.pop_back();

					for (auto id : current) {
						auto& state = nfa.states[id];
						if (state.action) {
							finals.emplace_back(current, *state.action);
							break;
						}
					}

					for (auto input : nfa.get_possible_inputs(current)) {
						auto next = nfa.eps_closure(nfa.move(current, input));
						if (!states.contains(next)) {
							states.add(next);
							unmarked.push_back(next);
						}

						trans.push_back({
							.input = input,
							.from = current,
							.to = next
						});
					}
				}*/

				return dfa{};
			}

		private:

		};
	}
}