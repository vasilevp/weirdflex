%{
	#include <string>
	#include "node.hpp"
	#include "parser.hpp"
	#include "lexutils.hpp"
%}

%x include

%%

"="		return TOKEN(WITH_LOG(ASSIGN));
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

"let"		return TOKEN(WITH_LOG(LET));
"func"		return TOKEN(WITH_LOG(FUNC));
"extern"	return TOKEN(WITH_LOG(EXTERN));
"include"	BEGIN(include);

[ \t\r\n]	; // whitespace
"//".*\n	; // line comment
"/*".*"*/"	; // block comment

[a-zA-Z_][a-zA-Z_0-9]*	SAVE_TOKEN; return WITH_LOG(IDENTIFIER);
[0-9]+\.[0-9]+			SAVE_TOKEN; return WITH_LOG(DOUBLE);
[0-9]+					SAVE_TOKEN; return WITH_LOG(INTEGER);

\".*\"	{
		std::string literal(yytext + 1, yyleng - 2);
		auto [unescaped, ok] = unescape(literal);
		if (!ok)
		{
			yyterminate();
		}
		yylval.string = new std::string(unescaped);
		return WITH_LOG(STRING);
	}

.	printf("Unknown token '%s'\n", yytext); yyterminate();

<include>[ \t]*      /* eat the whitespace */
<include>\"[^ \t\r\n]+\"	{ /* got the include file name */
		auto fname = std::string(yytext + 1, yyleng - 2);
		yyin = fopen(fname.c_str(), "r");
		if (!yyin)
		{
			printf("Include file '%s' not found!\n", yytext); yyterminate();
		}
		yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));

		BEGIN(INITIAL);
	}

<<EOF>> {
		yypop_buffer_state();

		if (!YY_CURRENT_BUFFER)
		{
			yyterminate();
		}
	}

%%

