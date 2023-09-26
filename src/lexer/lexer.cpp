#include "lexer.h"

#include "scanner.h"

namespace lexer {

	tk::token reject(std::string_view lexeme) {
		return tk::error{ tk::error::unknown_token, std::string(lexeme) };
	}

	static constexpr auto builder = scanner::builder<tk::token, token::custom_patterns>{ .reject_action = reject };

	constexpr auto fsm_scanner = builder.make_scanner();

	tk::token lexer::scan(const char* str) {

		return fsm_scanner.scan(str);
	}
}