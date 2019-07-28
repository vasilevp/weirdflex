#pragma once
#include <iostream>
#include <map>
#include <stack>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

using namespace std;
using namespace llvm;

namespace Node
{
struct Block;
}

struct CodeGenBlock
{
    BasicBlock *block;
    map<string, Value *> locals;
};

struct CodeGenContext
{
    stack<CodeGenBlock> blocks;
    Function *mainFunction;
    unique_ptr<Module> module;

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

    void generateCode(Node::Block &root);
    void runCode();
};