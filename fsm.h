#pragma once

#include "pattern.h"
#include "constexpr_utils.h"

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

		struct range {

			char min;
			char max;

			constexpr auto operator<=>(const range&) const = default;

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

			constexpr bool contains(char ch) const {
				return ch >= min && ch <= max;
			}
		};

		using state_id = size_t;

		struct eps_transition { state_id next; };
		struct char_transition { char input; state_id next; };
		struct range_transition { range input; state_id next; };

		template <typename Action>
		struct state {

			std::vector<eps_transition> eps_trans;
			std::vector<char_transition> char_trans;
			std::vector<range_transition> range_trans;
			std::optional<Action> action; // action to execute if the machine stops here/is final marker
		};

		template <typename Action>
		class nfa {

		public:
			using state = state<Action>;
			// initial always first, final always last
			std::vector<state> states;

			static consteval nfa from_pattern(p::single_char pattern) {

				nfa res;
				res.states.resize(2);
				states[0].char_trans.push_back({
					.input = pattern.ch,
					.next = 1
				});
				return res;
			}

			static consteval nfa from_pattern(p::range pattern) {

				nfa res;
				res.states.resize(2);
				states[0].range_trans.push_back({
					.input = { pattern.min, pattern.max },
					.next = 1
				});
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
			static consteval nfa from_pattern(p::one_or_more<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add back eps transition
				res.add_eps_transition(res.states.size() - 2, 1);

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::zero_or_one<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add skip eps transition
				res.add_eps_transition(0, res.states.size() - 1);

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::zero_or_more<P> pattern) {

				auto res = from_pattern(pattern.inner);
				res.extend();

				// add skip eps transition
				res.add_eps_transition(0, res.states.size() - 1);

				// add back eps transition
				res.add_eps_transition(res.states.size() - 2, 1);

				return res;
			}

			template <p::pattern P>
			static consteval nfa from_pattern(p::at_least_<P> pattern) {

				// transform A{n,} into A..AA+

				nfa res;
				res.states.emplace_back();

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
			static consteval nfa from_pattern(p::at_most_<P> pattern) {

				// transform A{,n} into A?..A?
				compile_assert(pattern.max != 0);

				nfa res;
				res.states.emplace_back();

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
			static consteval nfa from_pattern(p::times_<P> pattern) {

				// transform A{n,m} into A..A A?..A?
				compile_assert(pattern.min <= pattern.max);

				nfa res;
				res.states.emplace_back();

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
			friend consteval auto merge_nfas(std::vector<nfa>&& nfas) {

				nfa res;
				res.states.emplace_back(); // initial

				for (auto& n : nfas) {
					state_id initial = res.states.size();
					res.splice_states(std::move(n));
					add_eps_transition(0, initial);
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
				states[shift].eps_trans.append_range(std::move(final.eps_trans));
				states[shift].char_trans.append_range(std::move(final.char_trans));
				states[shift].range_trans.append_range(std::move(final.range_trans));
			}

			// add extra initial and final states with eps transitions
			consteval void extend() {

				// add new initial
				shift_indexes(1);
				states.emplace(states.begin());
				add_eps_transition(0, 1);

				// add new final
				states.emplace_back();
				add_eps_transition(states.size() - 2, states.size() - 1);
			}

			consteval void add_eps_transition(state_id from, state_id to) {

				states[from].eps_trans.push_back({ .next = to });
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

		template <typename T>
		struct flat_set : private std::vector<T> {

			using base = std::vector<T>;
			using base::begin;
			using base::end;
			using base::empty;
			using base::size;

			constexpr flat_set() = default;
			constexpr flat_set(base&& v) : base(std::move(v)) {
				std::sort(begin(), end());
				erase(std::unique(begin(), end()), end());
			}

			constexpr void add(T item) {
				auto it = std::upper_bound(begin(), end(), item);
				if (*it == item)
					return;
				insert(it, item);
			}

			constexpr void add(flat_set<T> other) {
				append_range(other);
				std::sort(begin(), end());
				erase(std::unique(begin(), end()), end());
			}

			constexpr bool contains(const T& item) const {
				return std::binary_search(begin(), end(), item) != end();
			}
		};

		template <typename Action>
		class dfa {

		public:
			using state = state<Action>;
			using nfa_state = state_id;
			using nfa_states = flat_set<nfa_state>;

			nfa<Action> source;
			std::vector<std::pair<nfa_states, state>> states;

			consteval dfa(nfa<Action>&& nfa) : source(std::move(nfa)) {

			}

		private:
			consteval auto eps_closure(const nfa_states& states) {

				nfa_states result;

				nfa_states current = states;
				while (!current.empty()) {

					nfa_states tmp;
					for (auto id : current) {
						for (auto& [next] : source.states[id].eps_trans) {
							if (!current.contains(next) && !result.contains(next))
								tmp.add(next);
						}
					}
					result.add(std::move(current));
					current = std::move(tmp);
				}

				return result;
			}

			static consteval auto get_trans_inputs(state& s) {

				flat_set<range> inputs = s.range_trans;

				for (auto& [ch, next] : s.char_trans)
					inputs.emplace_back(ch, ch);

				auto to_process = s.range_trans;
				while (!to_process.empty()) {

					auto current = to_process.back();
					to_process.pop_back();

					for (auto& tr : to_process)
						if (auto intersec = current.input & tr.input; !intersec.empty())
							inputs.add(itersec);

					// ????
				}
			}

		};

		consteval void foo() {

			dfa<int> d(nfa<int>{});
		}
	}
}
