#pragma once
#include <iostream>
#include <vector>

#include <llvm/IR/Value.h>

struct CodeGenContext;

namespace Node
{
struct Statement;
struct Expression;
struct VariableDeclaration;
struct Identifier;

using StatementList = std::vector<Statement *>;
using ExpressionList = std::vector<Expression *>;
using VariableList = std::vector<VariableDeclaration *>;

extern const Identifier TypeAgnostic;

extern llvm::LLVMContext GlobalContext;

struct NodeBase
{
	virtual ~NodeBase() {}
	virtual llvm::Value *codeGen(CodeGenContext &context) = 0;
};

struct Expression : NodeBase
{
};

struct Statement : NodeBase
{
};

struct Numeric : Expression
{
	virtual void Invert() = 0;
};

struct Integer : Numeric
{
	uint64_t value;
	Integer(uint64_t value) : value(value) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
	virtual void Invert() override
	{
		value = -value;
	}
};

struct Double : Numeric
{
	double value;
	Double(double value) : value(value) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
	virtual void Invert() override
	{
		value = -value;
	}
};

struct Identifier : Expression
{
	std::string name;
	Identifier(const std::string &name) : name(name) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};

struct MethodCall : Expression
{
	const Identifier &id;
	const ExpressionList &args;
	MethodCall(const Identifier &id, const ExpressionList &args = ExpressionList()) : id(id), args(args) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};

struct BinaryOperator : Expression
{
	int op;
	Expression &lhs;
	Expression &rhs;
	BinaryOperator(Expression &lhs, int op, Expression &rhs) : op(op), lhs(lhs), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};

struct Assignment : Expression
{
	const Identifier &lhs;
	Expression &rhs;
	Assignment(const Identifier &lhs, Expression &rhs) : lhs(lhs), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};

struct Block : Expression
{
	StatementList stmts;
	Block() {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};

struct ExpressionStatement : Statement
{
	Expression &expr;
	ExpressionStatement(Expression &expr) : expr(expr) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};

struct VariableDeclaration : Statement
{
	const Identifier *type;
	const Identifier &id;
	Expression *rhs;
	VariableDeclaration(Identifier *type, Identifier &id, Expression *rhs) : type(type ? type : &TypeAgnostic), id(id), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};

struct FunctionDeclaration : Expression
{
	const Identifier *type;
	const Identifier &id;
	const VariableList &args;
	Block &block;
	FunctionDeclaration(Identifier *type, Identifier &id, VariableList &args, Block &block) : type(type ? type : &TypeAgnostic), id(id), args(args), block(block) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) override;
};
}; // namespace Node