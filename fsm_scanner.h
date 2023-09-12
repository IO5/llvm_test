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

	template <char C>
	struct match_char {
		constexpr bool operator()(char c) {
			return c = C;
		};
	};

	struct match_endline {
		constexpr bool operator()(char c) {
			return c == '\n';
		};
	};

	// ---


	template <typename... Transitions>
	struct state {
	};

	template <auto Matcher, typename NextState>
	struct transition {
		using next = NextState;
		static constexpr auto match = Matcher;
	};

	template <auto Result>
	struct terminal_transition {
		static constexpr auto result = Result;
	};

	// ---

	struct terminal_matcher {};

	template <static_string Str>
	struct string_matcher {
		static constexpr auto reminder = Str.substr<1>();
		static constexpr auto match = match_char<Str[0]>{};
		static constexpr auto next = std::conditional_t<reminder.size == 0, terminal_matcher, string_matcher<reminder>>();
	};

	// --- meta functions ---

	template <auto Matcher, auto Result, typename State>
	struct add_matcher_to_state;

	template <typename T, auto Matcher>
	concept matching_transition = std::is_same_v<decltype(T::match), decltype(Matcher.match)>;

	template <auto Matcher, auto Result, typename Transition>
	struct try_add_matcher_to_transition {
		using type = Transition;
	};

	template <auto Matcher, auto Result, matching_transition<Matcher> Transition>
	struct try_add_matcher_to_transition<Matcher, Result, Transition> {
		using type = transition<
			Matcher.match,
			typename add_matcher_to_state<
				Matcher.next, Result, typename Transition::next
			>::type
		>;
	};

	template <auto Matcher, auto Result, typename... Transitions>
		requires (matching_transition<Transitions, Matcher> || ...)
	struct add_matcher_to_state<Matcher, Result, state<Transitions...>> {
		using type = state<
			typename try_add_matcher_to_transition<
				Matcher, Result, Transitions
			>::type
			...
		>;
	};

	template <auto Matcher, auto Result, typename... Transitions>
	struct add_matcher_to_state<Matcher, Result, state<Transitions...>> {
		using type = state<
			Transitions...,
			transition<
				Matcher.match,
				typename add_matcher_to_state<
					Matcher.next, Result, state<>
				>::type
			>
		>;
	};

	template <auto Result, typename... Transitions>
	struct add_matcher_to_state<terminal_matcher{}, Result, state<Transitions... >> {
		using type = state<
			Transitions...,
			terminal_transition<Result>
		>;
	};

	using A = add_matcher_to_state<(string_matcher<"\0\1\2\3">()), 23, state<>>::type;
	using B = add_matcher_to_state<(string_matcher<"\0\1">()), 42, A>::type;
	using FSM = add_matcher_to_state<(string_matcher<"\0\1\5">()), 2137, B>::type;

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
