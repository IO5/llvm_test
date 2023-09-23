#pragma once

#include "fsm.h"

#include <tuple>

namespace lexer::scanner {

	template <typename Action, Action RejectAction, auto... States>
	class scanner {

	public:

		static auto scan(const char* ptr);

		template <auto& State>
		static size_t move(char c) {

			for (const auto& [next, input] : State.transitions)
				if (input.contains(c))
					return next;

			return rejected;
		}

	private:
		template <size_t... Is>
		static consteval auto make_move_lut(std::index_sequence<Is...>) {

			return std::array{
				+[](char c) { return move<std::get<Is>(states)>(c); }
				...
			};
		}

		template <size_t... Is>
		static consteval auto make_action_lut(std::index_sequence<Is...>) {

			return std::array{
				std::get<Is>(states).action
				...
			};
		}

		static constexpr auto states = std::make_tuple(States...);
		static constexpr auto rejected = size_t(-1);
		static constexpr auto move_lut = make_move_lut(std::make_index_sequence<sizeof...(States)>{});
		static constexpr auto action_lut = make_action_lut(std::make_index_sequence<sizeof...(States)>{});
	};

	template <typename Action, Action RejectAction, auto... States>
	auto scanner<Action, RejectAction, States...>::scan(const char* ptr) {

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

	using transition = fsm::transition;

	template <typename Action, size_t TransNum>
	struct state {

		Action action;
		std::array<transition, TransNum> transitions;
	};

	template <typename Action, Action RejectAction, auto... Definitions>
	struct builder {

		using dfa = fsm::dfa<Action>;
		using dfa_state = fsm::dfa<Action>::state;

		//template <p::pattern P>
		static constexpr auto make_nfa(const auto& definition) {

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

			constexpr auto action = make_dfa().states[Idx].action;

			return state<Action, num_trans[Idx]>{
				.action = (action ? *action : RejectAction),
				.transitions = to_array<num_trans[Idx]>(make_dfa().states[Idx].trans)
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
