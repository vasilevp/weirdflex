%{
	#include "node.hpp"
	using namespace Node;
	Block *programBlock;

	extern int yylex();
	void yyerror(const char *msg) { printf("ERROR: %s\n", msg); }
%}

%union {
	Node::NodeBase *node;
	Node::Block *block;
	Node::Expression *expr;
	Node::Statement *stmt;
	Node::Identifier *ident;
	Node::VariableDeclaration *vardecl;
	Node::VariableList *varlist;
	Node::ExpressionList *exprlist;
	std::string *string;
	int token;
}

/* terminals */
%token <string> IDENTIFIER INTEGER DOUBLE
%token <token> EQ NE LT GT LE GE ASSIGN LET FUNC
%token <token> LPAREN RPAREN LBRACE RBRACE COMMA DOT
%token <token> PLUS MINUS MUL DIV

/* nonterminals */

%type <ident> ident
%type <expr> numeric expr func_decl
%type <varlist> func_decl_args
%type <exprlist> call_args
%type <block> program stmts block
%type <stmt> stmt var_decl
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

program			: stmts												{ programBlock = $1; }
				;

stmts			: stmt												{ $$ = new Block(); $$->stmts.push_back($1); }
				| stmts stmt										{ $1->stmts.push_back($2); }
				;

stmt			: var_decl
				| expr %prec REDUCE									{ $$ = new ExpressionStatement(*$1); }
				;

block			: LBRACE stmts RBRACE								{ $$ = $2; }
				| LBRACE RBRACE										{ $$ = new Block(); }
				| stmt												{ $$ = new Block(); $$->stmts.push_back($1); }
				;

var_decl		: LET ident ident ASSIGN expr						{ $$ = new VariableDeclaration($3, *$2, *$5); }
				| ident DECLAS expr									{ $$ = new VariableDeclaration(nullptr, *$1, *$3); }
				;

func_decl		: FUNC ident LPAREN func_decl_args RPAREN block		{ $$ = new FunctionDeclaration(nullptr, *$2, *$4, *$6); delete $4; }
				;

func_decl_args	: %empty											{ $$ = new VariableList(); }
				| var_decl											{ $$ = new VariableList(); $$->push_back($<vardecl>1); }
				| func_decl_args COMMA var_decl						{ $1->push_back($<vardecl>3); }
				;

ident			: IDENTIFIER										{ $$ = new Identifier(*$1); delete $1; }
				;

numeric			: INTEGER											{ $$ = new Integer(atol($1->c_str())); delete $1; }
				| DOUBLE											{ $$ = new Double(atof($1->c_str())); delete $1; }
				| MINUS INTEGER										{ $$ = new Integer(-atol($2->c_str())); delete $2; }
				| MINUS DOUBLE										{ $$ = new Double(-atof($2->c_str())); delete $2; }
				;

expr			: ident ASSIGN expr									{ $$ = new Assignment(*$1, *$3); }
				| ident LPAREN call_args RPAREN						{ $$ = new MethodCall(*$1, *$3); delete $3; }
				// | ident												{ $$ = $1; }
				| numeric
				| expr PLUS expr									{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr MINUS expr									{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr MUL expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr DIV expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr NE expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr LT expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr GT expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr GE expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| expr EQ expr										{ $$ = new BinaryOperator(*$1, $2, *$3); }
				| LPAREN expr RPAREN								{ $$ = $2; }
				| func_decl
				;

call_args		: %empty											{ $$ = new ExpressionList(); }
				| expr												{ $$ = new ExpressionList(); $$->push_back($1); }
				| call_args COMMA expr								{ $1->push_back($3); }
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