#pragma once

#include "lexer/pattern_action.h"

#include "utils/static_string.h"
#include "utils/argpack.h"
#include "utils/overload.h"

#include <variant>
#include <type_traits>

namespace token {

	struct eof {
		constexpr bool operator==(const eof&) const = default;

		static constexpr auto pattern = lexer::pattern::single_char{ '\0' };
	};

	/// base class for tokens defined by an exact character sequence
	template <static_string Str>
	struct lexeme {
		static constexpr std::string_view text = Str;

		constexpr bool operator==(const lexeme&) const = default;

		static constexpr auto pattern = lexer::pattern::char_seq(Str);
	};

	template <static_string Str>
	struct op : lexeme<Str> {};

	template <static_string Str>
	struct sym : lexeme<Str> {};

	template <static_string Str>
	struct keyword : lexeme<Str> {};

	namespace detail {
		template <template <auto> typename Base, typename T>
		struct is_specialization_of {
			template <auto Text> static void matcher(const Base<Text>&);
			static constexpr bool value = requires(T t) { is_specialization_of::matcher(t); };
		};
	}

	template <typename T>
	using is_lexeme = detail::is_specialization_of<lexeme, T>;

	template <typename... Tokens>
	class token_definition {
	public:
		static constexpr std::size_t count = sizeof...(Tokens);
		using token_list = argpack<Tokens...>;

		template <std::size_t Id>
		using type_of_id = token_list::template element<Id>;

		template <typename Tk>
		static constexpr std::size_t id_of = token_list::template index<Tk>;

		constexpr token_definition() = default;

		template <typename T>
			requires (std::is_same_v<T, Tokens> || ...)
		constexpr token_definition(T&& t) :
			v(std::move(t)) {}

		template <typename T>
			requires (std::is_same_v<T, Tokens> || ...)
		constexpr token_definition(const T& t) :
			v(t) {}

		static constexpr auto from_id(std::size_t id) {

			constexpr auto ctors = std::array{
				+[] { return (decltype(v))(std::in_place_type<Tokens>); } ...
			};

			return token_definition(ctors[id]());
		}

		constexpr std::size_t id() const {
			return v.index();
		}

		template <typename Tk>
		constexpr bool is() const {
			return is_one_of<Tk>();
		}

		/*template <template <auto> typename Base>
		constexpr bool is() const {
			return std::visit(
				[]<typename T>(auto&) { return detail::is_specialization_of<Base, T>::value; },
			v);
		}*/

		template <typename Tk, typename... Tks>
		constexpr bool is_one_of() const {
			return std::holds_alternative<Tk>(v) || (std::holds_alternative<Tks>(v) || ...);
		}

		template <typename Tk>
		constexpr bool is_not() const {
			return !is<Tk>();
		}

		template <typename Tk>
		constexpr auto* get_if(this auto& self) {
			return std::get_if<Tk>(&self.v);
		}

		template <typename... V>
		constexpr decltype(auto) visit(V&&... visitors) {
			return std::visit(
				overload{ std::forward<V>(visitors)... },
			v);
		}

		template <typename... V>
		constexpr decltype(auto) visit(V&&... visitors) const {
			return std::visit(
				overload{ std::forward<V>(visitors)... },
			v);
		}

		// use lexer to convert into source location
		std::size_t get_source_offset() const {
			return source_offset;
		}

		constexpr bool operator==(const token_definition& other) const = default;

		template <typename T>
			requires (std::is_same_v<T, Tokens> || ...)
		constexpr bool operator==(const T& other) const {

			if (const auto* value = std::get_if<T>(&v))
				return *value == other;
			return false;
		}

	protected:
		constexpr token_definition(std::variant<Tokens...>&& v) :
			v(std::move(v)) {}

		template <std::size_t... I>
		static consteval bool duplicate_tokens(std::index_sequence<I...>) {
			return ((id_of<Tokens> != I) || ...);
		}
		static_assert(!duplicate_tokens(token_list::index_sequence));

		std::variant<Tokens...> v;
		size_t source_offset = 0; // TODO
	};
}

