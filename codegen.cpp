#include "codegen.hpp"

#include <llvm/IR/Module.h>

#include "node.hpp"

using namespace Node;

CodeGenContext::CodeGenContext()
{
    module = new Module("main", Context);
}