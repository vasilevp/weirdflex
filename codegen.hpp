#pragma once
#include <iostream>
#include <map>
#include <stack>
#include <llvm/IR/Constants.h>
#include <llvm/ExecutionEngine/GenericValue.h>

using namespace std;
using namespace llvm;

struct NBlock;

struct CodeGenBlock
{
    BasicBlock *block;
    map<string, Value *> locals;
};

struct CodeGenContext
{
    stack<CodeGenBlock> blocks;
    Function *mainFunction;
    Module *module;

    CodeGenContext();

    std::map<std::string, Value *> &locals()
    {
        return blocks.top().locals;
    }

    BasicBlock *currentBlock()
    {
        return blocks.top().block;
    }

    void pushBlock(BasicBlock *block)
    {
        blocks.push(CodeGenBlock{block : block, locals : {}});
    }

    void popBlock()
    {
        blocks.pop();
    }

    void generateCode(NBlock &root);
    GenericValue runCode();
};