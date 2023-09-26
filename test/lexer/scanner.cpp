#include <catch2/catch_amalgamated.hpp>

#include "lexer/scanner.h"
#include "tokens.h"

#include <cmath>


namespace lexer::scanner {

	template <typename Token, size_t... NumTrans>
	class scanner2 {

	public:

		static constexpr size_t num_states = sizeof...(NumTrans);

		using token_type = Token;
		using action = token_type (*)(std::string_view);

		using state_id = fsm::state_id;

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
	struct builder2;

	template <typename Token, auto... CustomPatterns>
	struct builder2<Token, pattern_action_list<CustomPatterns...> > {

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

			auto result = scanner2<Token, num_trans[Is]...>{};

			for (int i = 0; i < num_states; ++i) {

				const auto& state = dfa.states[i];

				result.actions[i] = (state.action ? *state.action : reject_action);

				std::copy(state.trans.begin(), state.trans.end(), result.transitions[i].begin());
			}

			return result;
		}
	};
}

namespace lexer {

	namespace tk = token;

	inline token reject(std::string_view lexeme) {
		return tk::error{ tk::error::unknown_token, std::string(lexeme) };
	}

	static constexpr auto b = scanner::builder2<token, custom_patterns>{ .reject_action = reject };
}

TEST_CASE("lexer::scanner") {

	constexpr auto scanner = lexer::b.make_scanner();
	REQUIRE(scanner.scan("") == tk::eof{});
	REQUIRE(scanner.scan("+") == tk::op<"+">{});
	REQUIRE(scanner.scan("-") == tk::op<"-">{});
	REQUIRE(scanner.scan("*") == tk::op<"*">{});
	REQUIRE(scanner.scan("/") == tk::op<"/">{});
	REQUIRE(scanner.scan("..") == tk::op<"..">{});
	REQUIRE(scanner.scan("(") == tk::sym<"(">{});
	REQUIRE(scanner.scan(")") == tk::sym<")">{});
	REQUIRE(scanner.scan("{") == tk::sym<"{">{});
	REQUIRE(scanner.scan("}") == tk::sym<"}">{});
	REQUIRE(scanner.scan("=") == tk::sym<"=">{});
	REQUIRE(scanner.scan(",") == tk::sym<",">{});
	REQUIRE(scanner.scan("_") == tk::sym<"_">{});
	REQUIRE(scanner.scan("not") == tk::keyword<"not">{});
	REQUIRE(scanner.scan("in") == tk::keyword<"in">{});
	REQUIRE(scanner.scan("is") == tk::keyword<"is">{});
	REQUIRE(scanner.scan("import") == tk::keyword<"import">{});
	REQUIRE(scanner.scan("from") == tk::keyword<"from">{});
	REQUIRE(scanner.scan("if") == tk::keyword<"if">{});
	REQUIRE(scanner.scan("for") == tk::keyword<"for">{});
	REQUIRE(scanner.scan("while") == tk::keyword<"while">{});
	REQUIRE(scanner.scan("match") == tk::keyword<"match">{});
	REQUIRE(scanner.scan("fun") == tk::keyword<"fun">{});
	REQUIRE(scanner.scan("val") == tk::keyword<"val">{});
	REQUIRE(scanner.scan("var") == tk::keyword<"var">{});
	REQUIRE(scanner.scan("true") == tk::literal<bool>{true});
	REQUIRE(scanner.scan("false") == tk::literal<bool>{false});
	REQUIRE(scanner.scan("0") == tk::literal<int>{0});
	REQUIRE(scanner.scan("1") == tk::literal<int>{1});
	REQUIRE(scanner.scan("-23") == tk::literal<int>{-23});
	REQUIRE(scanner.scan("02137") == tk::literal<int>{2137});
	REQUIRE(scanner.scan("1e1") == tk::literal<double>{1e1});
	REQUIRE(scanner.scan("2e+2") == tk::literal<double>{2e+2});
	REQUIRE(scanner.scan("10E-3") == tk::literal<double>{10E-3});
	REQUIRE(scanner.scan("1.5") == tk::literal<double>{1.5});
	REQUIRE(scanner.scan("-02.3") == tk::literal<double>{-02.3});
	auto t = scanner.scan("nan");
	REQUIRE(t.is<tk::literal<double>>());
	auto* l = t.get_if<tk::literal<double>>();
	REQUIRE(std::isnan(l->value));
	REQUIRE(scanner.scan("inf") == tk::literal<double>{std::numeric_limits<double>::infinity()});
	REQUIRE(scanner.scan("-inf") == tk::literal<double>{-std::numeric_limits<double>::infinity()});
	REQUIRE(scanner.scan("falsely") == tk::identifier{"falsely"});
	REQUIRE(scanner.scan("xxx") == tk::identifier{"xxx"});
	REQUIRE(scanner.scan("iffy") == tk::identifier{"iffy"});
}
