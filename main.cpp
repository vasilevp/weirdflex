#include <iostream>
#include "codegen.hpp"
#include "node.hpp"

using namespace std;

extern int yyparse();
extern Node::Block *programBlock;

extern int yydebug;

int main(int argc, char **argv)
{
#ifdef _DEBUG
    yydebug = 1;
#endif

    if (yyparse())
    {
        return 1;
    }

    CodeGenContext context;
    context.generateCode(*programBlock);
    context.buildObject();

    return 0;
}