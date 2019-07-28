%{
	#include "node.hpp"
	using namespace Node;
	Block *programBlock;

	extern int yylex();
	void yyerror(const char *msg) { printf("ERROR: %s\n", msg); }
	#define YYDEBUG 1
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
%token <string> IDENTIFIER INTEGER DOUBLE STRING
%token <token> EQ NE LT GT LE GE ASSIGN LET FUNC EXTERN
%token <token> LPAREN RPAREN LBRACE RBRACE COMMA DOT ELLIPSIS
%token <token> PLUS MINUS MUL DIV

/* nonterminals */

%type <ident> ident
%type <expr> numeric expr string
%type <arglist> func_decl_args
%type <exprlist> call_args
%type <block> program stmts block
%type <stmt> stmt var_decl func_decl_arg func_decl
// %type <token> binaryop

/* precedence */

%nonassoc REDUCE
%left COMMA				// 18: ,
%nonassoc DECLAS		// 17: :=
%right ASSIGN			// 16 R: = += -= *= /= %= <<= >>= &= ^= |= throw a?b:c
						// 15: ||
						// 14: &&
						// 13: |
						// 12: ^
						// 11: &
%left EQ NE				// 10: == !=
%nonassoc LT GT LE GE	// 9: < > <= >=
						// 8: <=>
						// 7: << >>
%left PLUS MINUS		// 6: a+b a-b
%left MUL DIV			// 5: a*b a/b a%b
						// 4: . ->
%nonassoc UMINUS			// 3 R: ++a --a +a -a ! ~ (type) *a &a sizeof new new[] delete delete[]
%left FCALL				// 2: a++ a-- type() func_call()
						// 1: A::B


%start program

%%

program			: stmts													{ programBlock = $1; }
				;

stmts			: stmt													{ $$ = new Block(); $$->stmts.push_back($1); }
				| stmts stmt											{ $1->stmts.push_back($2); }
				;

stmt			: var_decl
				| func_decl
				| expr %prec REDUCE										{ $$ = new ExpressionStatement(*$1); }
				;

block			: LBRACE stmts RBRACE									{ $$ = $2; }
				| LBRACE RBRACE											{ $$ = new Block(); }
				| stmt													{ $$ = new Block(); $$->stmts.push_back($1); }
				;

var_decl		: LET ident ASSIGN expr									{ $$ = new VariableDeclaration(nullptr, $2, $4); }
				| ident DECLAS expr										{ $$ = new VariableDeclaration(nullptr, $1, $3); }
				;

func_decl		: FUNC ident LPAREN func_decl_args RPAREN ident block	{ $$ = new FunctionDeclaration($6, *$2, *$4, $7); }
				| FUNC ident LPAREN func_decl_args RPAREN block			{ $$ = new FunctionDeclaration(nullptr, *$2, *$4, $6); }
				| EXTERN ident LPAREN func_decl_args RPAREN				{ $$ = new FunctionDeclaration(nullptr, *$2, *$4, nullptr); }
				;

func_decl_arg	: ident ident											{ $$ = new VariableDeclaration($2, $1, nullptr); }
				| ident													{ $$ = new VariableDeclaration($1, nullptr, nullptr); }
				;

func_decl_args	: %empty												{ $$ = new ArgumentList(); }
				| func_decl_arg											{ $$ = new ArgumentList(); $$->push_back($<vardecl>1); }
				| func_decl_args COMMA func_decl_arg					{ $1->push_back($<vardecl>3); }
				| func_decl_args COMMA ELLIPSIS							{ $1->variadic = true; }
				;

ident			: IDENTIFIER											{ $$ = new Identifier(*$1); }
				;

numeric			: INTEGER												{ $$ = new Integer(atol($1->c_str())); }
				| DOUBLE												{ $$ = new Double(atof($1->c_str())); }
				| MINUS INTEGER											{ $$ = new Integer(-atol($2->c_str())); }
				| MINUS DOUBLE											{ $$ = new Double(-atof($2->c_str())); }
				;

string			: STRING												{ $$ = new String(*$1); }
				;

expr			: ident ASSIGN expr										{ $$ = new Assignment(*$1, *$3); }
				| ident LPAREN call_args RPAREN							{ $$ = new MethodCall(*$1, *$3); }
				| ident	%prec REDUCE									{ $$ = $1; }
				| numeric
				| string
				| expr PLUS expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr MINUS expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr MUL expr											{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr DIV expr											{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr NE expr											{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr LT expr											{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr GT expr											{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr GE expr											{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr EQ expr											{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| LPAREN expr RPAREN									{ $$ = $2; }
				;

call_args		: %empty												{ $$ = new ExpressionList(); }
				| expr													{ $$ = new ExpressionList(); $$->push_back($1); }
				| call_args COMMA expr									{ $1->push_back($3); }
				;

// binaryop 		: PLUS
// 				| MINUS
// 				| MUL
// 				| DIV
// 				| NE 
// 				| LT 
// 				| LE 
// 				| GT 
// 				| GE 
// 				| EQ
//           		;

%%