#pragma once
#include <iostream>
#include <map>
#include <stack>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

namespace Node
{
struct Block;
}

struct CodeGenBlock
{
	llvm::BasicBlock *block;
	std::map<std::string, llvm::Value *> args;
	std::map<std::string, llvm::Value *> locals;
};

struct CodeGenContext
{
	std::stack<CodeGenBlock> blocks;
	llvm::Function *mainFunction;
	std::unique_ptr<llvm::Module> module;

	CodeGenContext();

	auto &args()
	{
		return blocks.top().args;
	}

	auto &locals()
	{
		return blocks.top().locals;
	}

	llvm::BasicBlock *currentBlock()
	{
		return blocks.top().block;
	}

	void pushBlock(llvm::BasicBlock *block)
	{
		blocks.push(CodeGenBlock{block : block, args : {}, locals : {}});
	}

	void popBlock()
	{
		blocks.pop();
	}

	void generateCode(Node::Block &root);
	void buildObject(const std::string &filename);
	void buildExecutable(const std::string &output, const std::string &input);
};