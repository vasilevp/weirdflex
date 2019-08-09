%{
	#include <string>
	#include "node.hpp"
	#include "parser.hpp"
	#include "lexutils.hpp"
%}

%x sc_include
%x sc_comment
%x sc_string

%%
	char *string_literal_start = 0;

"="		return TOKEN(WITH_LOG(ASSIGN)); // operators
"=="	return TOKEN(WITH_LOG(EQ));
"!="	return TOKEN(WITH_LOG(NE));
"<"		return TOKEN(WITH_LOG(LT));
">"		return TOKEN(WITH_LOG(GT));
"<="	return TOKEN(WITH_LOG(LE));
">="	return TOKEN(WITH_LOG(GE));
"("		return TOKEN(WITH_LOG(LPAREN));
")"		return TOKEN(WITH_LOG(RPAREN));
"{"		return TOKEN(WITH_LOG(LBRACE));
"}"		return TOKEN(WITH_LOG(RBRACE));
"."		return TOKEN(WITH_LOG(DOT));
","		return TOKEN(WITH_LOG(COMMA));
"+"		return TOKEN(WITH_LOG(PLUS));
"-"		return TOKEN(WITH_LOG(MINUS));
"*"		return TOKEN(WITH_LOG(MUL));
"/"		return TOKEN(WITH_LOG(DIV));
":="	return TOKEN(WITH_LOG(DECLAS));
"..."	return TOKEN(WITH_LOG(ELLIPSIS));
"&"		return TOKEN(WITH_LOG(AMP));

"let"		return TOKEN(WITH_LOG(LET)); // keywords
"func"		return TOKEN(WITH_LOG(FUNC));
"extern"	return TOKEN(WITH_LOG(EXTERN));
"return"	return TOKEN(WITH_LOG(RETURN));

"include"					BEGIN(sc_include); // include "file.h"
<sc_include>[ \t]*      	/* eat the whitespace */
<sc_include>\"[^ \t\r\n]+\"	{ /* got the include file name */
		auto fname = std::string(yytext + 1, yyleng - 2);
		yyin = fopen(fname.c_str(), "r");
		if (!yyin)
		{
			printf("Include file '%s' not found!\n", yytext); yyterminate();
		}
		yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));

		BEGIN(INITIAL);
	}

[ \t\r\n]				; // whitespace
"//".*\n				; // line comment
[a-zA-Z_][a-zA-Z_0-9]*	SAVE_TOKEN; return WITH_LOG(IDENTIFIER);
[0-9]+\.[0-9]+			SAVE_TOKEN; return WITH_LOG(DOUBLE);
[0-9]+					SAVE_TOKEN; return WITH_LOG(INTEGER);

"/*"				BEGIN(sc_comment); // block comment
<sc_comment>"*/"	BEGIN(INITIAL); // block comment end
<sc_comment>.		;


\"				string_literal_start = ++yytext; BEGIN(sc_string);
<sc_string>\\\"	; // ignore quote
<sc_string>\"	{
	BEGIN(INITIAL);
	std::string literal(string_literal_start, std::distance(string_literal_start, yytext));
	auto [unescaped, ok] = unescape(literal);
	if (!ok)
	{
		yyterminate();
	}
	yylval.string = new std::string(unescaped);
	return WITH_LOG(STRING);
}

<<EOF>> {
		yypop_buffer_state();

		if (!YY_CURRENT_BUFFER)
		{
			yyterminate();
		}
	}


.	printf("Unknown token '%s'\n", yytext); yyterminate();

%%

