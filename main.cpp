//#include "tokens.h"
//#include "fsm_scanner.h"
#include "lexer/fsm.h"

/*
#include <deque>
#include <variant>
#include <string>
#include <string_view>
#include <array>
#include <print>
#include <type_traits>
#include <memory>
#include <ranges>
#include <concepts>
#include <map>
#include <algorithm>
#include <optional>
#include <typeinfo>

#include <llvm/Support/MemoryBuffer.h>

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>*/

using std::string;
/*

class Lexer {
public:
	Lexer(const llvm::MemoryBuffer& input) : input(input.getBuffer()) {
		(void)get_next_token();
	}

	Token get_next_token() {
		auto token = std::move(current);

		input = input.drop_while(isspace);

		if (!isalnum(input.front())) {

		}
		
		return token;
	}

private:
	Token current;
	llvm::StringRef input;
};*/
/*
constexpr auto precedence = make_map_const<std::string_view, int>({
	{"not", 2}, //?
	{"==", 3},
	{"!=", 3},
	{"<",  3},
	{">",  3},
	{"<=", 3},
	{">=", 3},
	{"+",  5},
	{"-",  5},
	{"*",  6},
	{"/",  6},
});*/
/*
namespace ast {
	struct Expression;

	template <static_string Op>
	struct BinaryOp {
		std::unique_ptr<const Expression> lhs;
		std::unique_ptr<const Expression> rhs;
	};

	template <static_string Op>
	struct UnaryOp {
		std::unique_ptr<const Expression> expr;
	};

	template <typename T>
	struct Literal {
		T value;
	};

	struct Expression : std::variant<
		BinaryOp<"+">,
		BinaryOp<"-">,
		UnaryOp<"-">,
		Literal<bool>
	> {};
		//llvm::Value* codegen(llvm::IRBuilderBase& builder) const;


	struct Statement {
		std::unique_ptr<const Expression> expr;
	};

	struct Block {
		std::deque<Statement> statements;
	};

	struct Function {
		std::string name;
		llvm::SmallVector<std::pair<std::string, std::string>, 5> params;
		Block body;
	};

}*/

/*
using llvm::Value;
class IRCompiler {
	Value* codegen(const ast::Expression& expr) {
		return std::visit([this](auto& v){ return this->codegen(v); }, expr);
	}

	template <static_string Op>
	Value* codegen(const ast::BinaryOp<Op>& op) {
		Value* l = codegen(*op.lhs);
		Value* r = codegen(*op.rhs);

		if (!l || !r)
			return nullptr;

		return nullptr;//std::invoke(Op, builder, l, r, llvm::Twine(Name, "tmp"));
	}

	template <static_string Op>
	Value* codegen(const ast::UnaryOp<Op>& op) {
		Value* expr = codegen(*op.expr);

		if (!expr)
			return nullptr;

		return nullptr;
	}

	template <typename T>
	Value* codegen(const ast::Literal<T>& lt) {
		return nullptr;
	}
};*/

using namespace std::string_literals;
/*
	template <size_t Idx>
	std::pair<std::string_view, uint8_t> calculateHash() {

		using Tk = std::tuple_element_t<Idx, tk::Tokens>;

		if constexpr (requires { Tk::text; }) {

			if (std::ranges::all_of(Tk::text, isalnum))
				return { Tk::text, tk::getHash(Tk::text) };
		}
		return { {}, 0 };
	}


template <size_t... I>
auto makeLut(std::index_sequence<I...>) {

	std::map<std::string_view, uint8_t> lut = {};

	for (auto& [text, hash] : { calculateHash<I>()... }) {
		if (text.empty())
			continue;

		lut[text] = hash;
	}

	return lut;
}
auto lut = makeLut(std::make_index_sequence<std::tuple_size_v<tk::Tokens>>{});
*/

int main() {
	//static std::unique_ptr<llvm::IRBuilder<>> Builder;

	//auto context = std::make_unique<llvm::LLVMContext>();
	//auto module = std::make_unique<llvm::Module>("my cool jit", *context);
	//auto builder = std::make_unique<llvm::IRBuilder<>>(*context);

	//constexpr lexer::token t = lexer::token::from_id(5);
	//static_assert(t.id() == 5);
	//static_assert(t.is<token::op<"..">>());
	//static_assert(lexer::token::id_of<token::op<"+">> == 1);
	//static_assert(std::is_same_v<lexer::token::type_of_id<1>, token::op<"+">>);
	//std::println("{}", t.visit(
	//	[](auto& val) {
	//		if constexpr (requires { val.text; })
	//			return std::string_view(val.text);
	//		return std::string_view("not a lexeme");
	//	}
	//));

	//static_assert(token::is_lexeme<token::op<"+">>::value);
	//static_assert(!token::is_lexeme<token::identifier>::value);

	/*std::string name = typeid(lexer::pattern::float_literal).name();
	std::string_view prefix = "struct lexer::pattern::";
	auto pos = name.find(prefix);
	while (pos != name.npos) {
		name.replace(pos, prefix.size(), "");
		pos = name.find(prefix);
	}
	std::println("{}", name);*/

	/*std::println("{}", lexer::pattern::float_literal.to_string());

	using namespace lexer::pattern;
	constexpr auto b = lexer::fsm::make_builder().
		add_pattern<0>(single_char('x')).second.
		add_pattern<0>(single_char('y')).second.
		add_pattern<1>(single_char('z')).second;
	std::println("{}", b.to_string());
*/
}
