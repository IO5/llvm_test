#pragma once

#include "pattern.h"
#include "utils/constexpr_utils.h"
#include "utils/flat_set.h"
#include "utils/flat_map.h"
#include "utils/argpack.h"

#include <string_view>
#include <vector>
#include <algorithm>
#include <optional>

namespace lexer {

	using std::string_view, std::size_t;

	namespace fsm {

		namespace p = pattern;

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

			constexpr bool contains(char c) const {
				return c >= min && c <= max;
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
				res.states[0].trans.push_back({
					.next = 1,
					.input = interval{ pattern.ch, pattern.ch }
					});
				return res;
			}

			static constexpr nfa from_pattern(p::range pattern) {

				nfa res;
				res.states.emplace_back();
				res.states[0].trans.push_back({
					.next = 1,
					.input = { pattern.min, pattern.max }
					});
				return res;
			}

			template <p::pattern... Ps>
			static constexpr nfa from_pattern(p::seq<Ps...> seq) {

				auto& [front, last] = seq;
				auto res = from_pattern(front);

				res.join(from_pattern(last));

				return res;
			}

			template <p::pattern P>
			static constexpr nfa from_pattern(p::seq<P> seq) {

				return from_pattern(seq.last);
			}

			template <p::pattern L, p::pattern R>
			static constexpr nfa from_pattern(p::or_<L, R> pattern) {

				auto l_nfa = from_pattern(pattern.lhs);
				auto r_nfa = from_pattern(pattern.rhs);

				nfa res;

				auto l_final = l_nfa.states.size();
				res.splice_states(std::move(l_nfa));
				res.add_eps_transition(0, 1);

				auto r_final = l_final + r_nfa.states.size();
				res.splice_states(std::move(r_nfa));
				res.add_eps_transition(0, l_final + 1);

				// make new final
				res.states.emplace_back();
				auto new_final = res.states.size() - 1;
				res.add_eps_transition(l_final, new_final);
				res.add_eps_transition(r_final, new_final);

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
			friend constexpr auto merge_nfas(auto&& nfas) {

				nfa<Action> res;

				for (auto& n : nfas) {
					state_id initial = res.states.size();
					res.splice_states(std::move(n));
					res.add_eps_transition(0, initial);
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
					result.add(current);
					current = std::move(tmp);
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

				for (auto& s : states) {
					for (auto& t : s.trans)
						t.next += shift;

					for (auto& t : s.eps_trans)
						t.next += shift;
				}
			}

			constexpr size_t splice_states(nfa&& other) {

				auto shift = states.size();

				other.shift_indexes(shift);
				states.append_range(std::move(other.states));

				return shift;
			}
		};

		constexpr auto get_possible_inputs(flat_set<interval> trans_inputs) {

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

					if (auto lower = interval{ .min = sum.min, .max = intersect.min - 1 }; !lower.empty())
						inputs.push_back(lower);

					inputs.push_back(intersect);

					if (auto upper = interval{ .min = intersect.max + 1, .max = sum.max }; !upper.empty())
						inputs.push_back(upper);

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


		template <typename Action>
		class dfa {

		public:
			static constexpr auto rejected = state_id(-1);

			struct state {

				std::vector<transition> trans;
				std::optional<Action> action; // action to execute if the machine stops here/is final marker
			};

			std::vector<state> states;
			Action reject_action = {};

			constexpr dfa() = default;

			static constexpr dfa from_nfa(const nfa<Action>& nfa) {

				auto conv = converter(nfa);
				return conv.convert();
			}

			constexpr state_id step(state_id id, char c) const {
				for (auto& [next, input] : states[id].trans)
					if (input.contains(c))
						return next;

				return rejected;
			}

			constexpr auto scan(const char* ptr) {

				auto begin = ptr;

				size_t current = 0;
				while (true) {

					size_t next = step(current, *ptr);
					if (next == rejected)
						break;

					++ptr;
					current = next;
				}

				auto lexeme = std::string_view(begin, ptr);

				if (states[current].action)
					return std::invoke(*states[current].action, lexeme);

				return std::invoke(reject_action, lexeme);
			}

		private:
			struct converter {

				using nfa_states = flat_set<state_id>;

				struct transition {
					nfa_states next;
					interval input;
				};

				const nfa<Action>& source;

				flat_map<nfa_states, Action> finals;
				std::vector<nfa_states> states;
				flat_map<nfa_states, std::vector<transition>> trans;

				constexpr converter(const nfa<Action>& source) :
					source(source) {}

				constexpr auto convert() {
					build();
					return translate();
				}

				constexpr void build() {

					auto initial = source.eps_closure({ 0 });

					flat_set<nfa_states> states_set = { initial };
					std::vector<nfa_states> unmarked = { initial };

					while (!unmarked.empty()) {

						auto current = unmarked.back();
						unmarked.pop_back();

						handle_final(current);

						auto& current_trans = trans.add({ current, {} })->second;

						flat_set<interval> inputs;
						for (auto id : current)
							for (auto t : source.states[id].trans)
								inputs.add(t.input);

						for (auto input : get_possible_inputs(inputs)) {

							auto next = source.eps_closure(source.move(current, input));

							if (!states_set.contains(next)) {
								states_set.add(next);
								unmarked.push_back(next);
							}

							current_trans.push_back({ .next = next, .input = input });
						}
					}

					states_set.del(initial);
					states.push_back(std::move(initial));

					states.reserve(states_set.size() + 1);
					for (auto& state : states_set)
						states.push_back(std::move(state));
				}

				constexpr dfa translate() {

					dfa result;

					for (auto& state : states) {

						auto& dfa_state = result.states.emplace_back();
						if (auto* action = finals.get(state))
							dfa_state.action = *action;
					}

					for (size_t i = 0; i < states.size(); ++i) {

						auto& dfa_state = result.states[i];
						auto& state = states[i];

						for (auto& [next, input] : *trans.get(state)) {
							for (size_t j = 0; j < states.size(); ++j) {
								if (next == states[j]) {
									dfa_state.trans.push_back({ .next = j, .input = input });
									break;
								}
							}
						}
					}

					return result;
				}

				constexpr void handle_final(const nfa_states& states) {

					for (auto id : states) {
						auto& state = source.states[id];
						if (state.action) {
							//compile_assert(!finals.contains(states));
							if (finals.contains(states)) std::abort(); // hmm
							finals.add({ states, *state.action });
							break;
						}
					}
				}
			};

			struct minimizer {

				const dfa& source;

				std::vector<flat_set<state_id>> partition;

				minimizer(const dfa& source) :
					source(source) {

					// initial partition
					flat_map<std::optional<Action>, flat_set<state_id>> classes;

					for (state_id id = 0; id != states.size(); ++id) {
						auto& state = states[id];

						classes[state.action].add(id);
					}

					for (auto& [action, set] : classes)
						partition.push_back(std::move(set));
				}

				constexpr flat_set<state_id> get_class_containing(state_id state) {
					auto it = std::ranges::find_if(partition, [&](auto& state_class) { state_class.contains(state); });
					return *it;
				}

				constexpr transition* get_transition(state_id state, interval input) {

					for (auto& trans : source.states[state].trans)
						if (trans.input.contains(input))
							return &trans;

					return nullptr;
				}

				constexpr flat_set<state_id> split(flat_set<state_id> current_class, interval input) {

					flat_map<flat_set<state_id>, flat_set<state_id>> result;

					for (auto& state : current_class) {
						transition* trans = get_transition(state, input);
						if (!trans)
							continue;

						auto state_class = get_class_containing(trans.next);
						result[state_class].add(state);
					}

					return result;
				}

				// rong
				constexpr void minimize() {

					bool split = false;
					do {

						auto current_class = partition.back();
						partition.pop_back();

						for (interval input = { 0,1 };;) { // all inputs

							auto new_classes = split(current_class, input);
							if (new_classes.size() == 1)
								continue;

							split = true;
							partition.append_range(new_classes);
							break;
						}

					} while (split);
				}
			};

			/*
							std::vector<flat_set<state_id>> partition;

							//std::vector<flat_set<transition>> reverse_trans;
							//reverse_trans.resize(states.size());

							for (state_id id = 0; id != states.size(); ++id) {
								auto& state = states[id];

								if (state.action)
									partition.push_back({ *state.action, id });

								//for (auto& [next, input] : state.trans)
								//	reverse_trans[next].add({ .next = id, .input = input });
							}

							bool changed = false;
							do {
								for (int c = -128; c <= 127; ++c)
								{

								}
							} while (!changed);


							std::vector<flat_set<state_id>> waiting = { finals, non_finals };
							std::vector<flat_set<state_id>> partition = { finals, non_finals };

							while (!waiting.empty()) {

								auto a = w.back();
								w.pop_back();

								flat_set<interval> inputs;
								for (auto id : current)
									for (auto t : source.states[id].trans)
										inputs.add(t.input);

								for (auto input : get_possible_inputs(reverse_inputs) {


								}


							}*/
							//	P: = { F, Q \ F }
							//	W: = { F, Q \ F }

							//		while (W is not empty) do
							//			choose and remove a set A from W
							//			for each c in É∞ do
							//				let X be the set of states for which a transition on c leads to a state in A
							//				for each set Y in P for which X Åø Y is nonempty and Y \ X is nonempty do
							//					replace Y in P by the two sets X Åø Y and Y \ X
							//					if Y is in W
							//						replace Y in W by the same two sets
							//					else
							//						if | X Åø Y| <= |Y \ X |
							//							add X Åø Y to W
							//						else
							//							add Y \ X to W
		};

	}
}
