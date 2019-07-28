#include <iostream>
#include "codegen.hpp"
#include "node.hpp"

using namespace std;

extern int yyparse();
extern Node::Block *programBlock;

extern int yydebug;

int main(int argc, char **argv)
{
    yydebug = 1;

    int buf[4096];

    if (yyparse())
    {
        return 1;
    }

    int buf2[4096];

    std::cout << programBlock << std::endl;

    CodeGenContext context;
    context.generateCode(*programBlock);
    context.runCode();

    return 0;
}