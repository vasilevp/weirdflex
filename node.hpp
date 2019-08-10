#pragma once
#include <iostream>
#include <vector>
#include <optional>
#include <map>

#include <llvm/IR/Value.h>

#include "codegen.hpp"

struct CodeGenContext;

namespace Node
{
struct Statement;
struct Expression;
struct VariableDeclaration;
struct Identifier;
struct ArgumentList;

using StatementList = std::vector<Statement *>;
using ExpressionList = std::vector<Expression *>;

extern llvm::LLVMContext GlobalContext;

enum class InternalType
{
	Invalid,
	Integer,
	Float,
	String,
};

llvm::Type *typeOf(const Identifier &type);
InternalType typeOf2(const Identifier &type);

struct NodeBase
{
	virtual ~NodeBase() {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const = 0;
};

struct Expression : NodeBase
{
	virtual InternalType GetType(CodeGenContext &context) const = 0;
};

struct Statement : NodeBase
{
};

struct Numeric : Expression
{
};

struct Integer : Numeric
{
	uint64_t value;
	Integer(uint64_t value) : value(value) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		return InternalType::Integer;
	}
};

struct Float : Numeric
{
	double value;
	Float(double value) : value(value) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		return InternalType::Float;
	}
};

struct String : Expression
{
	std::string value;
	String(const std::string &value) : value(value) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		return InternalType::String;
	}
};

struct Identifier : Expression
{
	std::string name;
	Identifier(const std::string &name) : name(name) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		auto ident = context.args().find(name);
		if (!ident)
		{
			return InternalType::Invalid;
		}

		auto expr = dynamic_cast<const Expression *>(ident->node);
		if (!expr)
		{
			return InternalType::Invalid;
		}

		return expr->GetType(context);
	}
};

struct MethodCall : Expression
{
	const Identifier &id;
	const ExpressionList &args;
	MethodCall(const Identifier &id, const ExpressionList &args = ExpressionList()) : id(id), args(args) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		auto ident = context.functions.find(id.name);
		if (!ident)
		{
			return InternalType::Invalid;
		}

		auto expr = dynamic_cast<const Expression *>(ident->node);
		if (!expr)
		{
			return InternalType::Invalid;
		}

		return expr->GetType(context);
	}
};

struct BinaryOperator : Expression
{
	int op;
	Expression *lhs;
	Expression *rhs;
	BinaryOperator(Expression *lhs, int op, Expression *rhs) : op(op), lhs(lhs), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		auto left = lhs->GetType(context);
		auto right = rhs->GetType(context);

		if (left != right)
		{
			return InternalType::Invalid;
		}

		return left;
	}
};

struct Assignment : Expression
{
	const Identifier &lhs;
	Expression &rhs;
	Assignment(const Identifier &lhs, Expression &rhs) : lhs(lhs), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		return rhs.GetType(context);
	}
};

struct Block : Statement
{
	StatementList stmts;
	Block() {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct ExpressionStatement : Statement
{
	Expression &expr;
	ExpressionStatement(Expression &expr) : expr(expr) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct ReturnStatement : Statement
{
	const Expression &rhs;
	ReturnStatement(const Expression &rhs) : rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct VariableDeclaration : Statement
{
	const Identifier *type;
	const Identifier *id;
	Expression *rhs;
	VariableDeclaration(Identifier *type, Identifier *id, Expression *rhs) : type(type), id(id), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct FunctionDeclaration : Expression, Statement
{
	const Identifier *type;
	const Identifier *id;
	const ArgumentList &args;
	Block *block;
	FunctionDeclaration(Identifier *type, Identifier *id, ArgumentList &args, Block *block) : type(type), id(id), args(args), block(block) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		return type ? typeOf2(*type) : InternalType::Invalid;
	}
};

struct ArgumentList : Statement
{
	std::vector<VariableDeclaration *> args;
	bool variadic = false;
	ArgumentList() {}
	auto begin() const { return args.begin(); }
	auto end() const { return args.end(); }
	void push_back(VariableDeclaration *x) { args.push_back(x); }
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct AddressOf : Expression
{
	const Identifier *ident;
	AddressOf(Identifier *ident) : ident(ident) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual InternalType GetType(CodeGenContext &context) const override
	{
		return InternalType::Invalid;
	}
};
}; // namespace Node