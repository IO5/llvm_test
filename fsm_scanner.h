#pragma once

#include "static_string.h"
#include "argpack.h"

namespace lexer {

	using std::string_view, std::size_t;

	constexpr bool is_whitespace(char c) {
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}

	constexpr bool is_boundary(char c) {
		return c == '\0' || is_whitespace(c);
	}

	template <typename... Transitions>
	struct state {
	};

	template <char C, typename NextState>
	struct transition {
		using next = NextState;

		static constexpr bool matches(char c) {
			return c == C;
		}
	};

	template <auto Result>
	struct terminal_transition {
		static constexpr auto result = Result;

		static constexpr bool matches(char c) {
			return is_boundary(c);
		}
	};

	// --- meta functions ---

	template <static_string Str, auto Result, typename State>
	struct add_string_to_state;

	template <typename T, static_string Str>
	concept matching_transition = T::matches(Str[0]);

	template <static_string Str, auto Result, typename Transition>
	struct try_add_string_to_transition {
		using type = Transition;
	};

	template <static_string Str, auto Result, matching_transition Transition>
	struct try_add_string_to_transition<Str, Result, Transition> {
		static constexpr auto StrReminder = Str.substr<1>();

		using type = transition<
			Str[0],
			typename add_string_to_state<
				StrReminder, Result, typename Transition::next
			>::type
		>;
	};

	template <static_string Str, auto Result, typename... Transitions>
		requires (matching_transition<Transitions> || ...)
	struct add_string_to_state<Str, Result, state<Transitions...>> {
		using type = state<
			typename try_add_string_to_transition<
				Str, Result, Transitions
			>::type
			...
		>;
	};

	template <static_string Str, auto Result, typename... Transitions>
	struct add_string_to_state<Str, Result, state<Transitions...>> {
		static constexpr auto StrReminder = Str.substr<1>();

		using type = state<
			Transitions...,
			transition<
				Str[0],
				typename add_string_to_state<
					StrReminder, Result, state<>
				>::type
			>
		>;
	};

	template <auto Result, typename... Transitions>
	struct add_string_to_state<static_string(""), Result, state<Transitions...>> {
		using type = state<
			Transitions...,
			terminal_transition<Result>
		>;
	};


	using A = add_string_to_state<"\0\1\2\3", 23, state<>>::type;
	using B = add_string_to_state<"\0\1", 42, A>::type;
	using FSM = add_string_to_state<"\0\1\5", 2137, B>::type;

	void foo() {
	}




/*

	namespace fsm::detail {

		template <typename V>
		struct terminal {
			V value;
		};

		template <char C, typename State>
		struct transition {
			static constexpr char ch = C;
			using state = State;
		};

		template <char C, typename Transition>
		struct is_transition_for : std::bool_constant<C == Transition::ch> {};

		template <typename Head, typename... Tail>
		std::size_t step_impl(const char*& ptr) {
			if (*ptr == Head::ch) {
				++ptr;
				return Head::state::step();
			}

			if constexpr (sizeof...(Tail) > 0)
				return step_impl(ptr);

			return 0;
		}

		template <typename... Transitions>
		struct state {
			using transitions = argpack<Transitions...>;

			static std::size_t step(const char*& ptr) {
				return step_impl<Transitions...>(ptr);
			}

			template <char C>
			struct transition_tester {
				template <typename T>
				struct test : std::bool_constant<C == T::ch> {};
			};

			template <char C>
			using transition_for = transitions::template select<template transition_tester<C>::test>;
		};
	}

	template <std::size_t N, typename V>
	consteval auto make_fsm(const std::pair<string_view, V>(&mapping)[N]) {

		for (auto& [key, val] : mapping) {

			
		}

	}

	template <typename Fsm, static_string Seq, std::size_t Result> 
	auto add_sequence() {

		constexpr char c = Seq::data.front();
		using transition = Fsm::transition_for<c>;
		if constexpr (!std::is_same_v<transition, void>) {

		}
		else
		{

		}
	}
	*/
}
