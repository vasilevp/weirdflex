#pragma once
#include <iostream>
#include <map>
#include <stack>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

namespace Node
{
struct Block;
}

struct CodeGenBlock
{
    llvm::BasicBlock *block;
    std::map<std::string, llvm::Value *> locals;
};

struct CodeGenContext
{
    std::stack<CodeGenBlock> blocks;
    llvm::Function *mainFunction;
    std::unique_ptr<llvm::Module> module;

    CodeGenContext();

    std::map<std::string, llvm::Value *> &locals()
    {
        return blocks.top().locals;
    }

    llvm::BasicBlock *currentBlock()
    {
        return blocks.top().block;
    }

    void pushBlock(llvm::BasicBlock *block)
    {
        blocks.push(CodeGenBlock{block : block, locals : {}});
    }

    void popBlock()
    {
        blocks.pop();
    }

    void generateCode(Node::Block &root);
    void buildObject();
};