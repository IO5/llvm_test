#pragma once

#include "fsm.h"
#include "pattern_action.h"

#include <functional>

namespace lexer::scanner {

	template <action Action, Action RejectAction, auto... States>
	class scanner {

	private:

		static constexpr auto rejected = size_t(-1);

		template <auto& State>
		static constexpr size_t move(char c) {

			for (const auto& [next, input] : State.transitions)
				if (input.contains(c))
					return next;

			return rejected;
		}

		static constexpr auto move_lut = std::array{
			+[](char c) { return move<States>(c); }
			...
		};

		static constexpr auto action_lut = std::array{
			States.action
			...
		};

	public:

		static constexpr auto scan(const char* ptr) {

			auto begin = ptr;

			size_t current = 0;
			while (true) {

				size_t next = move_lut[current](*ptr);
				if (next == rejected)
					break;

				++ptr;
				current = next;
			}

			auto lexeme = std::string_view(begin, ptr);

			return std::invoke(action_lut[current], lexeme);
		}
	};

	using transition = fsm::transition;

	template <action Action, size_t TransNum>
	struct state {

		Action action;
		std::array<transition, TransNum> transitions;
	};

	template <action Action, Action RejectAction, auto... Definitions>
	struct builder {

		using dfa = fsm::dfa<Action>;
		using dfa_state = fsm::dfa<Action>::state;

		template <pattern::pattern P>
		static constexpr auto make_nfa(const std::pair<P, Action>& definition) {

			auto result = fsm::nfa<Action>::from_pattern(definition.first);
			result.states.back().action = definition.second;
			return result;
		}

		static constexpr auto make_dfa() {

			auto merged = merge_nfas<Action>(std::array{ make_nfa(Definitions)... });

			auto dfa = dfa::from_nfa(merged);

			return dfa;
		}

		template <size_t... Is>
		static constexpr auto get_num_trans(std::index_sequence<Is...>) {

			auto dfa = make_dfa();
			return std::array{ dfa.states[Is].trans.size() ... };
		}

		static constexpr size_t num_states = make_dfa().states.size();
		static constexpr auto num_trans = get_num_trans(std::make_index_sequence<num_states>{});

		template <size_t Idx>
		static constexpr auto make_state() {

			auto dfa_state = make_dfa().states[Idx];

			auto trans = std::array<transition, num_trans[Idx]>{};
			std::copy(dfa_state.trans.begin(), dfa_state.trans.end(), trans.begin());

			return state<Action, num_trans[Idx]>{
				.action = (dfa_state.action ? *dfa_state.action : RejectAction),
				.transitions = trans
			};
		}

		template <size_t... Is>
		static constexpr auto make_scanner_impl(std::index_sequence<Is...>) {

			return scanner<
				Action,
				RejectAction,
				make_state<Is>()...
			>{};
		}

		static constexpr auto make_scanner() {

			return make_scanner_impl(
				std::make_index_sequence<num_states>{}
			);
		}
	};
}
