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
IRBuilder<> Builder(GlobalContext);

static Type *typeOf(const Identifier &type)
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

Value *Integer::codeGen(CodeGenContext &context) const
{
	return ConstantInt::get(GlobalContext, APInt(64, value, false));
}

Value *Double::codeGen(CodeGenContext &context) const
{
	return ConstantFP::get(GlobalContext, APFloat(value));
}

Value *String::codeGen(CodeGenContext &context) const
{
	Constant *StrConstant = ConstantDataArray::getString(GlobalContext, value);
	Module &M = *context.module.get();
	auto *GV = new GlobalVariable(M, StrConstant->getType(), true, GlobalValue::PrivateLinkage, StrConstant);
	GV->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
	GV->setAlignment(1);

	return BitCastInst::Create(BitCastInst::BitCast, GV, Type::getInt8PtrTy(GlobalContext), "", context.currentBlock());
}

Value *Identifier::codeGen(CodeGenContext &context) const
{
	if (context.locals().find(name) == context.locals().end())
	{
		cerr << "(Identifier) undeclared variable " << name << '\n';
		return NULL;
	}
	return new LoadInst(context.locals()[name], "", false, context.currentBlock());
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
	auto it = context.locals().find(lhs.name);
	if (it == context.locals().end())
	{
		throw runtime_error("(Assignment) undeclared variable: " + lhs.name);
	}
	return new StoreInst(rhs.codeGen(context), it->second, false, context.currentBlock());
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
		return llvm::BinaryOperator::Create(instr, left, right, "", context.currentBlock());
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

	return CmpInst::Create(Instruction::ICmp, pred, left, right, "", context.currentBlock());
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

	return CmpInst::Create(Instruction::FCmp, pred, left, right, "", context.currentBlock());
}

Value *createOperator(CodeGenContext &context, Value *left, Value *right, int op)
{
	string name = "_operator_";

#define CREATE_CASE(x) \
	case x:            \
		name += #x;    \
		break;

	switch (op)
	{
		CREATE_CASE(PLUS)
		CREATE_CASE(MINUS)
		CREATE_CASE(MUL)
		CREATE_CASE(DIV)
		CREATE_CASE(EQ)
		CREATE_CASE(NE)
		CREATE_CASE(LT)
		CREATE_CASE(GT)
		CREATE_CASE(LE)
		CREATE_CASE(GE)

	default:
		throw runtime_error("unsupported operator" + to_string(op));
	}

	Function *function = context.module->getFunction(name);
	if (function == nullptr)
	{
		throw runtime_error("cannot find overloaded operator " + name);
	}

	vector<Value *> argv;
	argv.push_back(left);
	argv.push_back(right);

	CallInst *call = CallInst::Create(function, argv, "", context.currentBlock());
	return call;
}

Value *Node::BinaryOperator::codeGen(CodeGenContext &context) const
{
	CmpInst::Predicate pred;

	auto lefts = dynamic_cast<String *>(lhs);
	auto rights = dynamic_cast<String *>(rhs);
	if (lefts && rights)
	{
		return String(lefts->value + rights->value).codeGen(context);
	}

	auto left = lhs->codeGen(context);
	auto right = rhs->codeGen(context);

	auto binaryInst = [&](int opcode, CmpInst::Predicate predicate) {};
	auto cmpInst = [&](int opcode, CmpInst::Predicate predicate) {};

	if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy())
	{
		return createIntBinaryOp(context, left, right, op);
	}

	if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy())
	{
		return createDoubleBinaryOp(context, left, right, op);
	}

	return createOperator(context, left, right, op);
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
	auto linkage = id->name.front() == '_' ? GlobalValue::InternalLinkage : GlobalValue::ExternalLinkage;
	Function *function = Function::Create(ftype, linkage, id->name, context.module.get());

	if (!block) // declaration
	{
		return function;
	}

	BasicBlock *bblock = BasicBlock::Create(GlobalContext, "", function);
	context.pushBlock(bblock);

	Function::arg_iterator argsValues = function->arg_begin();
	for (auto it = args.begin(); it != args.end(); it++)
	{
		auto arg = *it;
		arg->codeGen(context);

		auto argumentValue = argsValues++;
		argumentValue->setName("_" + arg->id->name);
		StoreInst *inst = new StoreInst(argumentValue, context.locals()[arg->id->name], false, bblock);
	}

	block->codeGen(context);

	if (bblock->getTerminator() == nullptr) // implicit 'void' return
	{
		ReturnInst::Create(GlobalContext, bblock);
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

	CallInst *call = CallInst::Create(function, argv, "", context.currentBlock());
	return call;
}

Value *VariableDeclaration::codeGen(CodeGenContext &context) const
{
	if (!id) // extern function decl, no code generation needed
	{
		return nullptr;
	}

	if (rhs == nullptr)
	{
		AllocaInst *alloc = new AllocaInst(typeOf(*type), 0, id->name, context.currentBlock());
		context.locals()[id->name] = alloc;
		return alloc;
	}

	Value *rhsResult = rhs->codeGen(context);
	AllocaInst *alloc = new AllocaInst(type ? typeOf(*type) : rhsResult->getType(), 0, id->name, context.currentBlock());
	context.locals()[id->name] = alloc;

	return Assignment(*id, *rhs).codeGen(context);
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
		StoreInst *inst = new StoreInst(context.locals()[arg->id->name], result, false, context.currentBlock());
	}

	return result;
}

Value *ReturnStatement::codeGen(CodeGenContext &context) const
{
	return ReturnInst::Create(GlobalContext, rhs.codeGen(context), context.currentBlock());
}

Value *AddressOf::codeGen(CodeGenContext &context) const
{
	if (context.locals().find(ident->name) == context.locals().end())
	{
		cerr << "(Identifier) undeclared variable " << ident->name << '\n';
		return NULL;
	}
	return context.locals()[ident->name];
}
