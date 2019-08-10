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
struct NodeBase;
} // namespace Node

template <typename T>
struct Container
{
	std::map<std::string, T> value;

	std::optional<T> find(std::string name)
	{
		auto it = value.find(name);
		if (it == value.end())
		{
			return {};
		}

		return it->second;
	}

	T &operator[](const std::string &key)
	{
		return value[key];
	}
};

struct NodeInfo
{
	const Node::NodeBase *node;
	llvm::Value *value;
};

struct CodeGenBlock
{
	llvm::BasicBlock *block;
	Container<NodeInfo> args;
	Container<NodeInfo> locals;
};

struct CodeGenContext
{
	std::stack<CodeGenBlock> blocks;
	llvm::Function *mainFunction;
	std::unique_ptr<llvm::Module> module;
	Container<NodeInfo> functions;

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