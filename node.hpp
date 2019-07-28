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
struct ArgumentList;

using StatementList = std::vector<Statement *>;
using ExpressionList = std::vector<Expression *>;

extern const Identifier TypeAgnostic;

extern llvm::LLVMContext GlobalContext;

struct NodeBase
{
	virtual ~NodeBase() {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const = 0;
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
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual void Invert() override
	{
		value = -value;
	}
};

struct Double : Numeric
{
	double value;
	Double(double value) : value(value) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
	virtual void Invert() override
	{
		value = -value;
	}
};

struct String : Expression
{
	std::string value;
	String(const std::string &value) : value(value) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct Identifier : Expression
{
	std::string name;
	Identifier(const std::string &name) : name(name) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct MethodCall : Expression
{
	const Identifier &id;
	const ExpressionList &args;
	MethodCall(const Identifier &id, const ExpressionList &args = ExpressionList()) : id(id), args(args) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct BinaryOperator : Expression
{
	int op;
	Expression &lhs;
	Expression &rhs;
	BinaryOperator(Expression &lhs, int op, Expression &rhs) : op(op), lhs(lhs), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct Assignment : Expression
{
	const Identifier &lhs;
	Expression &rhs;
	Assignment(const Identifier &lhs, Expression &rhs) : lhs(lhs), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct Block : Expression
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

struct VariableDeclaration : Statement
{
	const Identifier *type;
	const Identifier *id;
	Expression *rhs;
	VariableDeclaration(Identifier *type, Identifier *id, Expression *rhs) : type(type ? type : &TypeAgnostic), id(id), rhs(rhs) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
};

struct FunctionDeclaration : Statement
{
	const Identifier *type;
	const Identifier &id;
	const ArgumentList &args;
	Block *block;
	FunctionDeclaration(Identifier *type, Identifier &id, ArgumentList &args, Block *block) : type(type ? type : &TypeAgnostic), id(id), args(args), block(block) {}
	virtual llvm::Value *codeGen(CodeGenContext &context) const override;
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
}; // namespace Node