#include <iostream>
#include "node.hpp"

extern Node::Block *programBlock;
extern int yyparse();

int main(int argc, char **argv)
{
    yyparse();
    printf("Parsed %d statements\n", programBlock->stmts.size());
    return 0;
}