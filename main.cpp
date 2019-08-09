#include <iostream>
#include "codegen.hpp"
#include "node.hpp"

using namespace std;

extern int yyparse();
extern Node::Block *programBlock;

extern int yydebug;

std::string tmpname()
{
	std::string objname = tmpnam(nullptr);
	if (objname.front() == '\\') // a fix for windows stupidity
	{
		objname = '.' + objname;
	}

	if (objname.back() == '.')
	{
		objname = objname.substr(0, objname.size() - 1);
	}

	return objname;
}

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

	// auto objname = tmpname();
	auto objname = "output.o";
	context.buildObject(objname);
	// context.buildExecutable("a.out", objname);

	// remove(objname.c_str());

	return 0;
}