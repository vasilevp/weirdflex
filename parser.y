%{
	#include "node.hpp"
	using namespace Node;
	Block *programBlock;

	extern int yylex();
	void yyerror(const char *msg) { printf("Parse error: %s\n", msg); }
	#ifdef _DEBUG
	#define YYDEBUG 1
	#endif
%}

%union {
	Node::NodeBase *node;
	Node::Block *block;
	Node::Expression *expr;
	Node::Statement *stmt;
	Node::Identifier *ident;
	Node::VariableDeclaration *vardecl;
	Node::ArgumentList *arglist;
	Node::ExpressionList *exprlist;
	std::string *string;
	int token;
}

/* terminals */
%token <string> IDENTIFIER INTEGER FLOAT STRING
%token <token> EQ NE LT GT LE GE ASSIGN LET FUNC EXTERN RETURN
%token <token> LPAREN RPAREN LBRACE RBRACE COMMA DOT ELLIPSIS
%token <token> PLUS MINUS MUL DIV AMP

/* nonterminals */

%type <ident> ident
%type <expr> numeric expr string func_expr
%type <arglist> func_decl_args func_decl_arg_set
%type <exprlist> call_args
%type <block> program stmts block
%type <stmt> stmt var_decl func_decl_arg func_decl
%type <token> binaryop

/* precedence */

%precedence REDUCE		// LOWEST: special case for naughty rules
// TODO %precedence COMMA		// 18: ,
%precedence DECLAS		// 17: :=
%precedence ASSIGN		// 16 R: = += -= *= /= %= <<= >>= &= ^= |= throw a?b:c
						// 15: ||
						// 14: &&
						// 13: |
						// 12: ^
						// 11: &
%precedence EQ NE		// 10: == !=
%precedence LT GT LE GE	// 9: < > <= >=
						// 8: <=>
						// 7: << >>
%precedence PLUS MINUS	// 6: a+b a-b
%precedence MUL DIV		// 5: a*b a/b a%b
						// 4: . ->
%precedence UMINUS		// 3 R: ++a --a +a -a ! ~ (type) *a &a sizeof new new[] delete delete[]
%precedence LPAREN		// 2: a++ a-- type() func_call()
						// 1: A::B

%start program

%%

program	: stmts	{ programBlock = $1; }
		;

stmts	: stmt			{ $$ = new Block(); $$->stmts.push_back($1); }
		| stmts stmt	{ $1->stmts.push_back($2); }
		;

stmt	: var_decl
		| func_decl
		| RETURN expr %prec REDUCE	{ $$ = new ReturnStatement(*$2); }
		| expr %prec REDUCE			{ $$ = new ExpressionStatement(*$1); }
		;

block	: LBRACE stmts RBRACE	{ $$ = $2; }
		| LBRACE RBRACE			{ $$ = new Block(); }
		| stmt					{ $$ = new Block(); $$->stmts.push_back($1); }
		;

var_decl	: LET ident ident ASSIGN expr	{ $$ = new VariableDeclaration($3, $2, $5); }
			| ident DECLAS expr				{ $$ = new VariableDeclaration(nullptr, $1, $3); }
			;

func_decl	: FUNC ident LPAREN func_decl_arg_set RPAREN ident block	{ $$ = new FunctionDeclaration($6, $2, *$4, $7); }
			| FUNC ident LPAREN func_decl_arg_set RPAREN block			{ $$ = new FunctionDeclaration(nullptr, $2, *$4, $6); }
			| FUNC ident LPAREN func_decl_arg_set RPAREN ident EXTERN	{ $$ = new FunctionDeclaration($6, $2, *$4, nullptr); }
			| FUNC ident LPAREN func_decl_arg_set RPAREN EXTERN			{ $$ = new FunctionDeclaration(nullptr, $2, *$4, nullptr); }
			;

func_expr	: FUNC LPAREN func_decl_arg_set RPAREN ident block	{ $$ = new FunctionDeclaration($5, nullptr, *$3, $6); }
			| FUNC LPAREN func_decl_arg_set RPAREN block		{ $$ = new FunctionDeclaration(nullptr, nullptr, *$3, $5); }


func_decl_arg	: ident ident	{ $$ = new VariableDeclaration($2, $1, nullptr); }
				| ident			{ $$ = new VariableDeclaration($1, nullptr, nullptr); }
				;

func_decl_args	: %empty								{ $$ = new ArgumentList(); }
				| func_decl_arg							{ $$ = new ArgumentList(); $$->push_back($<vardecl>1); }
				| func_decl_args COMMA func_decl_arg	{ $1->push_back($<vardecl>3); }
				;

func_decl_arg_set	: func_decl_args
					| func_decl_args COMMA ELLIPSIS	{ $1->variadic = true; }
					;

ident	: IDENTIFIER	{ $$ = new Identifier(*$1); }
		;

numeric	: INTEGER						{ $$ = new Integer(atol($1->c_str())); }
		| FLOAT							{ $$ = new Float(atof($1->c_str())); }
		| MINUS INTEGER	%prec UMINUS	{ $$ = new Integer(-atol($2->c_str())); }
		| MINUS FLOAT %prec UMINUS		{ $$ = new Float(-atof($2->c_str())); }
		;

string	: STRING %prec REDUCE	{ $$ = new String(*$1); }
		// | STRING STRING			{ $$ = new String(std::string(*$1) + *$2); }
		;

expr	: ident ASSIGN expr					{ $$ = new Assignment(*$1, *$3); }
		| ident LPAREN call_args RPAREN		{ $$ = new MethodCall(*$1, *$3); }
		| ident	%prec REDUCE				{ $$ = $1; }
		| numeric
		| string
		| func_expr
		| expr binaryop expr %prec UMINUS	{ $$ = new BinaryOperator($1, $2, $3); }
		| LPAREN expr RPAREN				{ $$ = $2; }
		| AMP ident							{ $$ = new AddressOf($2); }
		;

call_args	: %empty				{ $$ = new ExpressionList(); }
			| expr %prec REDUCE		{ $$ = new ExpressionList(); $$->push_back($1); }
			| call_args COMMA expr	{ $1->push_back($3); }
			;

binaryop	: PLUS
			| MINUS
			| MUL
			| DIV
			| NE
			| LT
			| LE
			| GT
			| GE
			| EQ
			;

%%