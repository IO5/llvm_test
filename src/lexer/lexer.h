#pragma once

#include "token/tokens.h"

namespace lexer {

	class lexer {

	public:

		tk::token scan(const char* str);
	};
}
