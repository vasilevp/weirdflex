%{
	#include <string>
	#include "node.hpp"
	#include "parser.hpp"

	#define WITH_LOG(t) (printf("token '%s', value '%s'\n", #t, yytext), t)
	// #define WITH_LOG(t) (t)
	#define SAVE_TOKEN (yylval.string = new std::string(yytext, yyleng))
	#define TOKEN(t) (yylval.token = t)
	extern "C" int yywrap() { return 1; }
%}

%%

"="						return TOKEN(WITH_LOG(ASSIGN));
"=="					return TOKEN(WITH_LOG(EQ));
"!="					return TOKEN(WITH_LOG(NE));
"<"						return TOKEN(WITH_LOG(LT));
">"						return TOKEN(WITH_LOG(GT));
"<="					return TOKEN(WITH_LOG(LE));
">="					return TOKEN(WITH_LOG(GE));
"("						return TOKEN(WITH_LOG(LPAREN));
")"						return TOKEN(WITH_LOG(RPAREN));
"{"						return TOKEN(WITH_LOG(LBRACE));
"}"						return TOKEN(WITH_LOG(RBRACE));
"."						return TOKEN(WITH_LOG(DOT));
","						return TOKEN(WITH_LOG(COMMA));
"+"						return TOKEN(WITH_LOG(PLUS));
"-"						return TOKEN(WITH_LOG(MINUS));
"*"						return TOKEN(WITH_LOG(MUL));
"/"						return TOKEN(WITH_LOG(DIV));
"let"					return TOKEN(WITH_LOG(LET));
"func"					return TOKEN(WITH_LOG(FUNC));
"extern"				return TOKEN(WITH_LOG(EXTERN));
":="					return TOKEN(WITH_LOG(DECLAS));
"..."					return TOKEN(WITH_LOG(ELLIPSIS));

[ \t\n]					;
[a-zA-Z_][a-zA-Z0-9_]*	SAVE_TOKEN; return WITH_LOG(IDENTIFIER);
[0-9]+\.[0-9]*			SAVE_TOKEN; return WITH_LOG(DOUBLE);
[0-9]+					SAVE_TOKEN; return WITH_LOG(INTEGER);

\".*\"					yylval.string = new std::string(yytext + 1, yyleng - 2); return WITH_LOG(STRING);

.						printf("Unknown token!\n"); yyterminate();

%%

