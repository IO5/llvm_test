#pragma once

#include "utils/static_string.h"

#include <llvm/ADT/SmallVector.h>

#include <variant>
#include <deque>
#include <memory>
#include <string>

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
}
