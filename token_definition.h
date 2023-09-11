#pragma once

#include "static_string.h"

#include "argpack.h"
#include "overload.h"

#include <variant>
#include <cassert>

namespace token {

	struct eof {};

	/// base class for tokens defined by an exact character sequence
	template <static_string Str>
	struct lexeme {
		static constexpr std::string_view text = Str;
	};

	template <static_string Str>
	struct op : lexeme<Str> {};

	template <static_string Str>
	struct sym : lexeme<Str> {};

	template <static_string Str>
	struct keyword : lexeme<Str> {};

	template <typename T>
	struct literal {
		T value;
	};

	struct identifier {
		std::string name;
	};

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

		constexpr token_definition(std::size_t source_offset) :
			source_offset(source_offset) {}

		template <typename T>
			requires (std::is_same_v<T, Tokens> || ...)
		constexpr token_definition(std::size_t source_offset, T&& t) :
			v(std::forward<T>(t)),
			source_offset(source_offset) {}

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
		
	private:
		constexpr token_definition(std::variant<Tokens...>&& v) :
			v(std::move(v)) {}

		template <std::size_t... I>
		static consteval bool duplicate_tokens(std::index_sequence<I...>) {
			return ((id_of<Tokens> != I) || ...);
		}
		static_assert(!duplicate_tokens(token_list::index_sequence));

		std::variant<Tokens...> v;
		size_t source_offset = 0;
	};
}

