#pragma once

#include "token_definition.h"

#include <string>

namespace lexer {

	using namespace token;

	using token = token_definition<
		eof,
		op<"+">,
		op<"-">,
		op<"*">,
		op<"/">,
		op<"..">,
		sym<"(">,
		sym<")">,
		sym<"{">,
		sym<"}">,
		sym<"=">,
		sym<",">,
		keyword<"not">,
		keyword<"in">,
		keyword<"is">,
		keyword<"import">,
		keyword<"from">,
		keyword<"if">,
		keyword<"for">,
		keyword<"while">,
		keyword<"match">,
		keyword<"fun">,
		keyword<"val">,
		keyword<"var">,
		literal<bool>,
		literal<int>,
		literal<std::string>,
		identifier
	>;
}

