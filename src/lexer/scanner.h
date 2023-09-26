#pragma once

#include "fsm.h"
#include "pattern_action.h"

#include "utils/array_of_arrays.h"

#include <functional>

namespace lexer::scanner {

	template <typename Token, size_t... NumTrans>
	class scanner {

	public:

		static constexpr size_t num_states = sizeof...(NumTrans);

		using token_type = Token;
		using action = token_type (*)(std::string_view);

		using state_id = fsm::state_id;
		using transition = fsm::transition;

		array_of_arrays<transition, NumTrans...> transitions;
		std::array<action, num_states> actions;

	private:

		static constexpr auto rejected = state_id(-1);

		template <state_id State>
		constexpr state_id move(char c) const {

			for (const auto& [next, input] : transitions.get<State>())
				if (input.contains(c))
					return next;

			return rejected;
		}

		template <state_id... Is>
		static constexpr auto make_move_lut(std::index_sequence<Is...>) {
			return std::array{
				&move<Is>...
			};
		}

		static constexpr auto move_lut = make_move_lut(std::make_index_sequence<num_states>{});

	public:

		constexpr token_type scan(const char* ptr) const {

			auto begin = ptr;

			size_t current = 0;
			while (true) {

				size_t next = std::invoke(move_lut[current], this, *ptr);
				if (next == rejected)
					break;

				++ptr;
				current = next;
			}

			auto lexeme = std::string_view(begin, ptr);

			return std::invoke(actions[current], lexeme);
		}
	};

	template <typename T>
	struct has_defined_pattern : std::bool_constant<requires { T::pattern; }> {};

	template <typename Token, typename CustomPatterns>
	struct builder;

	template <typename Token, auto... CustomPatterns>
	struct builder<Token, pattern_action_list<CustomPatterns...> > {

		using token_type = Token;
		using action = token_type (*)(std::string_view);

		action reject_action;

		constexpr auto make_scanner() const {

			return make_scanner_impl(
				std::make_index_sequence<num_states>{}
			);
		}

	private:
		using dfa = fsm::dfa<action>;
		using dfa_state = fsm::dfa<action>::state;

		template <auto Action>
		static token_type invoke_action(std::string_view lexeme) {

			return token_type(std::invoke(Action, lexeme));
		}

		template <auto Value>
		static token_type return_value(std::string_view) {

			return token_type(Value);
		}

		template <auto Definition>
		static constexpr auto make_nfa() {

			auto result = fsm::nfa<action>::from_pattern(Definition.pattern);

			if constexpr (requires{ Definition.action; })
				result.states.back().action = &invoke_action<Definition.action>;
			else
				result.states.back().action = &return_value<Definition.value>;

			return result;
		}

		template <typename... Tokens>
		static constexpr auto make_nfas(argpack<Tokens...>) {

			auto result = std::vector{
				make_nfa< (Tokens::pattern >> &return_value<Tokens{}>) >()
				...,
				make_nfa< CustomPatterns >()
				...
			};
			return result;
		}

		static constexpr auto make_dfa() {

			using builtin_patterns = typename token_type::token_list::template filter<has_defined_pattern>;

			auto merged = merge_nfas<action>(make_nfas(builtin_patterns{}));

			auto dfa = dfa::from_nfa(merged);

			return dfa;
		}

		template <size_t... Is>
		static constexpr auto get_num_trans(std::index_sequence<Is...>) {

			auto dfa = make_dfa();
			return std::array{ dfa.states[Is].trans.size()... };
		}

		static constexpr size_t num_states = make_dfa().states.size();
		static constexpr auto num_trans = get_num_trans(std::make_index_sequence<num_states>{});

		template <size_t... Is>
		constexpr auto make_scanner_impl(std::index_sequence<Is...>) const {

			auto dfa = make_dfa();

			auto result = scanner<Token, num_trans[Is]...>{};

			for (int i = 0; i < num_states; ++i) {

				const auto& state = dfa.states[i];

				result.actions[i] = (state.action ? *state.action : reject_action);

				std::copy(state.trans.begin(), state.trans.end(), result.transitions[i].begin());
			}

			return result;
		}
	};
}
