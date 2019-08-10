#include "node.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include "llvm/IR/IRBuilder.h"
#include <llvm/Support/raw_ostream.h>

#include "parser.hpp"
#include "codegen.hpp"

using namespace llvm;
using namespace std;
using namespace Node;

llvm::LLVMContext Node::GlobalContext;

IRBuilder<> getBuilder(CodeGenContext &context)
{
	return IRBuilder<>(context.currentBlock());
}

Type *Node::typeOf(const Identifier &type)
{
	if (type.name == "int")
	{
		return Type::getInt64Ty(GlobalContext);
	}
	if (type.name == "double")
	{
		return Type::getDoubleTy(GlobalContext);
	}
	if (type.name == "string")
	{
		return Type::getInt8PtrTy(GlobalContext);
	}
	if (type.name == "_untyped")
	{
		return Type::getInt64PtrTy(GlobalContext);
	}

	return Type::getVoidTy(GlobalContext);
}

InternalType Node::typeOf2(const Identifier &type)
{
	if (type.name == "int")
	{
		return InternalType::Integer;
	}
	if (type.name == "double")
	{
		return InternalType::Float;
	}
	if (type.name == "string")
	{
		return InternalType::String;
	}

	return InternalType::Invalid;
}

Value *Integer::codeGen(CodeGenContext &context) const
{
	return ConstantInt::get(GlobalContext, APInt(64, value, false));
}

Value *Float::codeGen(CodeGenContext &context) const
{
	return ConstantFP::get(GlobalContext, APFloat(value));
}

Value *String::codeGen(CodeGenContext &context) const
{
	return getBuilder(context).CreateGlobalStringPtr(value);
}

Value *Identifier::codeGen(CodeGenContext &context) const
{
	if (auto arg = context.args().find(name))
	{
		return arg->value;
	}

	if (auto local = context.locals().find(name))
	{
		return getBuilder(context).CreateLoad(local->value);
	}

	throw runtime_error("(Identifier) undeclared variable " + name + '\n');
}

Value *Block::codeGen(CodeGenContext &context) const
{
	int r = rand();
	Value *last = nullptr;
	for (auto s : stmts)
	{
		last = s->codeGen(context);
	}

	return last;
}

Value *Assignment::codeGen(CodeGenContext &context) const
{
	auto l = context.locals().find(lhs.name);
	if (!l)
	{
		throw runtime_error("(Assignment) undeclared variable: " + lhs.name);
	}

	return getBuilder(context).CreateStore(rhs.codeGen(context), l->value);
}

Value *createArithmeticOp(CodeGenContext &context, Value *left, Value *right, int op)
{
	Instruction::BinaryOps instr;
	switch (op)
	{
	case PLUS:
		instr = Instruction::Add;
		break;
	case MINUS:
		instr = Instruction::Sub;
		break;
	case MUL:
		instr = Instruction::Mul;
		break;
	case DIV:
		instr = Instruction::SDiv;
		break;
	default:
		break;
	}

	if (instr != 0)
	{
		return getBuilder(context).CreateBinOp(instr, left, right);
	}

	return nullptr;
}

Value *createIntBinaryOp(CodeGenContext &context, Value *left, Value *right, int op)
{
	if (auto result = createArithmeticOp(context, left, right, op))
	{
		return result;
	}

	CmpInst::Predicate pred;

	switch (op)
	{
	case EQ:
		pred = CmpInst::ICMP_EQ;
	case NE:
		pred = CmpInst::ICMP_NE;
	case LT:
		pred = CmpInst::ICMP_SLT;
	case GT:
		pred = CmpInst::ICMP_SGT;
	case LE:
		pred = CmpInst::ICMP_SLE;
	case GE:
		pred = CmpInst::ICMP_SGE;

	default:
		throw runtime_error("unsupported operator " + to_string(op));
	}

	return getBuilder(context).CreateICmp(pred, left, right);
}

Value *createDoubleBinaryOp(CodeGenContext &context, Value *left, Value *right, int op)
{
	if (auto result = createArithmeticOp(context, left, right, op))
	{
		return result;
	}

	CmpInst::Predicate pred;

	switch (op)
	{
	case EQ:
		pred = CmpInst::FCMP_OEQ;
	case NE:
		pred = CmpInst::FCMP_ONE;
	case LT:
		pred = CmpInst::FCMP_OLT;
	case GT:
		pred = CmpInst::FCMP_OGT;
	case LE:
		pred = CmpInst::FCMP_OLE;
	case GE:
		pred = CmpInst::FCMP_OGE;

	default:
		throw runtime_error("unsupported operator " + to_string(op));
	}

	return getBuilder(context).CreateFCmp(pred, left, right);
}

Value *createStringAddition(CodeGenContext &context, Value *left, Value *right)
{
	Function *function = context.module->getFunction("concat");
	return getBuilder(context).CreateCall(function, {left, right});
}

Value *Node::BinaryOperator::codeGen(CodeGenContext &context) const
{
	CmpInst::Predicate pred;

	auto leftT = lhs->GetType(context);
	auto rightT = rhs->GetType(context);
	auto left = lhs->codeGen(context);
	auto right = rhs->codeGen(context);

	if (leftT != rightT)
	{
		throw runtime_error("cannot create binary operator for different argument types");
	}

	if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy())
	{
		return createIntBinaryOp(context, left, right, op);
	}

	if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy())
	{
		return createDoubleBinaryOp(context, left, right, op);
	}

	if (lhs->GetType(context) == InternalType::String && op == PLUS)
	{
		return createStringAddition(context, left, right);
	}

	throw runtime_error("operator not implemented for these arguments");
}

Value *FunctionDeclaration::codeGen(CodeGenContext &context) const
{
	vector<Type *> argTypes;
	for (auto arg : args)
	{
		argTypes.push_back(typeOf(*arg->type));
	}

	auto returnType = type ? typeOf(*type) : Type::getVoidTy(GlobalContext);
	FunctionType *ftype = FunctionType::get(returnType, argTypes, args.variadic);
	auto linkage = (id->name.empty() || id->name.front() == '_') ? GlobalValue::InternalLinkage : GlobalValue::ExternalLinkage;
	Function *function = Function::Create(ftype, linkage, id->name, context.module.get());

	context.functions[id->name] = NodeInfo{node : dynamic_cast<const Expression *>(this), value : function};
	if (!block) // declaration
	{
		return function;
	}

	BasicBlock *bblock = BasicBlock::Create(GlobalContext, "", function);
	context.pushBlock(bblock);

	auto argsValues = function->arg_begin();
	for (auto it = args.begin(); it != args.end(); it++)
	{
		auto arg = *it;
		auto argumentValue = argsValues++;
		argumentValue->setName(arg->id->name);
		context.args()[arg->id->name] = NodeInfo{node : arg, value : argumentValue};
	}

	block->codeGen(context);

	if (bblock->getTerminator() == nullptr) // implicit 'void' return
	{
		return getBuilder(context).CreateRetVoid();
	}

	context.popBlock();
	return function;
}

Value *MethodCall::codeGen(CodeGenContext &context) const
{
	Function *function = context.module->getFunction(id.name);
	if (function == nullptr)
	{
		throw runtime_error("function '" + id.name + "' not found");
	}

	vector<Value *> argv;
	for (auto arg : args)
	{
		argv.push_back(arg->codeGen(context));
	}

	return getBuilder(context).CreateCall(function, argv);
}

Value *VariableDeclaration::codeGen(CodeGenContext &context) const
{
	if (!id) // extern function decl, no code generation needed
	{
		return nullptr;
	}

	auto &store = context.locals()[id->name];

	store.node = this;

	if (rhs == nullptr)
	{
		store.value = getBuilder(context).CreateAlloca(typeOf(*type), nullptr, id->name);
		return store.value;
	}

	Value *rhsResult = rhs->codeGen(context);
	store.value = getBuilder(context).CreateAlloca(type ? typeOf(*type) : rhsResult->getType(), nullptr, id->name);
	return getBuilder(context).CreateStore(rhsResult, store.value);
}

Value *ExpressionStatement::codeGen(CodeGenContext &context) const
{
	return expr.codeGen(context);
}

Value *ArgumentList::codeGen(CodeGenContext &context) const
{
	Value *result = nullptr;
	for (auto arg : args)
	{
		result = arg->codeGen(context);
		result->setName(arg->id->name);
		getBuilder(context).CreateStore(context.locals()[arg->id->name].value, result);
	}

	return result;
}

Value *ReturnStatement::codeGen(CodeGenContext &context) const
{
	return getBuilder(context).CreateRet(rhs.codeGen(context));
}

Value *AddressOf::codeGen(CodeGenContext &context) const
{
	if (auto arg = context.args().find(ident->name))
	{
		return arg->value;
	}

	if (auto local = context.locals().find(ident->name))
	{
		return local->value;
	}

	throw runtime_error("(Identifier) undeclared variable " + ident->name + '\n');
}
