#include <catch2/catch_amalgamated.hpp>

#include "lexer/lexer.h"

#include <cmath>

using namespace tk;

TEST_CASE("lexer::lexer") {

	lexer::lexer l;

	SECTION("single") {

		REQUIRE(l.scan("") == eof{});
		REQUIRE(l.scan("+") == op<"+">{});
		REQUIRE(l.scan("-") == op<"-">{});
		REQUIRE(l.scan("*") == op<"*">{});
		REQUIRE(l.scan("/") == op<"/">{});
		REQUIRE(l.scan("..") == op<"..">{});
		REQUIRE(l.scan("(") == sym<"(">{});
		REQUIRE(l.scan(")") == sym<")">{});
		REQUIRE(l.scan("{") == sym<"{">{});
		REQUIRE(l.scan("}") == sym<"}">{});
		REQUIRE(l.scan("=") == sym<"=">{});
		REQUIRE(l.scan(",") == sym<",">{});
		REQUIRE(l.scan("_") == sym<"_">{});
		REQUIRE(l.scan("not") == keyword<"not">{});
		REQUIRE(l.scan("in") == keyword<"in">{});
		REQUIRE(l.scan("is") == keyword<"is">{});
		REQUIRE(l.scan("import") == keyword<"import">{});
		REQUIRE(l.scan("from") == keyword<"from">{});
		REQUIRE(l.scan("if") == keyword<"if">{});
		REQUIRE(l.scan("for") == keyword<"for">{});
		REQUIRE(l.scan("while") == keyword<"while">{});
		REQUIRE(l.scan("match") == keyword<"match">{});
		REQUIRE(l.scan("fun") == keyword<"fun">{});
		REQUIRE(l.scan("val") == keyword<"val">{});
		REQUIRE(l.scan("var") == keyword<"var">{});
		REQUIRE(l.scan("true") == literal<bool>{true});
		REQUIRE(l.scan("false") == literal<bool>{false});
		REQUIRE(l.scan("0") == literal<int>{0});
		REQUIRE(l.scan("1") == literal<int>{1});
		REQUIRE(l.scan("-23") == literal<int>{-23});
		REQUIRE(l.scan("02137") == literal<int>{2137});
		REQUIRE(l.scan("1e1") == literal<double>{1e1});
		REQUIRE(l.scan("2e+2") == literal<double>{2e+2});
		REQUIRE(l.scan("10E-3") == literal<double>{10E-3});
		REQUIRE(l.scan("1.5") == literal<double>{1.5});
		REQUIRE(l.scan("-02.3") == literal<double>{-02.3});
		auto t = l.scan("nan");
		REQUIRE(t.is<literal<double>>());
		auto* d = t.get_if<literal<double>>();
		REQUIRE(std::isnan(d->value));
		REQUIRE(l.scan("inf") == literal<double>{std::numeric_limits<double>::infinity()});
		REQUIRE(l.scan("-inf") == literal<double>{-std::numeric_limits<double>::infinity()});
		REQUIRE(l.scan("falsely") == identifier{ "falsely" });
		REQUIRE(l.scan("xxx") == identifier{ "xxx" });
		REQUIRE(l.scan("iffy") == identifier{ "iffy" });
	}
}
