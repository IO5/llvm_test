#include "tokens.h"

namespace token {

	token integer_parser(std::string_view lexeme) {

		int result;
		auto [ptr, ec] = std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), result);
		assert(ptr == lexeme.data() + lexeme.size());

		if (ec == std::errc::result_out_of_range)
			return error{ error::integer_literal_out_of_range, std::string(lexeme) };

		return literal<int>{result};
	}

	token float_parser(std::string_view lexeme) {

		double result;
		auto [ptr, ec] = std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), result);
		assert(ptr == lexeme.data() + lexeme.size());

		if (ec == std::errc::result_out_of_range)
			return error{ error::float_literal_out_of_range, std::string(lexeme) };

		return literal<double>{result};
	}
}
