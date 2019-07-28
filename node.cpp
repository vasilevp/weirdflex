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

const Identifier Node::TypeAgnostic = Identifier("_untyped");

struct guard
{
    std::string name;
    guard(const std::string &name) : name(name)
    {
        cout << "Entering '" << name << "'\n";
    }

    ~guard()
    {
        cout << "Exiting '" << name << "'\n";
    }
};

#ifdef DEBUG
#define GUARD() auto _ = guard(__PRETTY_FUNCTION__);
#else
#define GUARD()
#endif

static Type *typeOf(const Identifier &type)
{
    if (type.name == "int")
    {
        return Type::getInt64Ty(GlobalContext);
    }
    else if (type.name == "double")
    {
        return Type::getDoubleTy(GlobalContext);
    }
    else if (type.name == "_untyped")
    {
        return Type::getDoubleTy(GlobalContext);
    }
    else if (type.name == "string")
    {
        return Type::getInt8PtrTy(GlobalContext);
    }
    return Type::getVoidTy(GlobalContext);
}

Value *Integer::codeGen(CodeGenContext &context) const
{
    GUARD();
    return ConstantInt::get(GlobalContext, APInt(64, value, false));
}

Value *Double::codeGen(CodeGenContext &context) const
{
    GUARD();
    return ConstantFP::get(GlobalContext, APFloat(value));
}

Value *String::codeGen(CodeGenContext &context) const
{
    GUARD();
    // return IRBuilder<>(GlobalContext).CreateGlobalStringPtr(value, "");

    Constant *StrConstant = ConstantDataArray::getString(GlobalContext, value);
    Module &M = *context.currentBlock()->getParent()->getParent();
    auto *GV = new GlobalVariable(M, StrConstant->getType(), true, GlobalValue::PrivateLinkage, StrConstant);
    GV->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
    GV->setAlignment(1);

    Constant *Zero = ConstantInt::get(Type::getInt32Ty(GlobalContext), 0);
    Constant *Indices[] = {Zero, Zero};
    return ConstantExpr::getInBoundsGetElementPtr(GV->getValueType(), GV, Indices);
}

Value *Identifier::codeGen(CodeGenContext &context) const
{
    GUARD();
    if (context.locals().find(name) == context.locals().end())
    {
        cerr << "undeclared variable " << name << '\n';
        return NULL;
    }
    return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value *Block::codeGen(CodeGenContext &context) const
{
    GUARD();
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
    GUARD();
    if (context.locals().find(lhs.name) == context.locals().end())
    {
        cerr << "undeclared variable " << lhs.name << '\n';
        return nullptr;
    }
    return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value *Node::BinaryOperator::codeGen(CodeGenContext &context) const
{
    GUARD();
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
        return nullptr;
    }

    return llvm::BinaryOperator::Create(instr, lhs.codeGen(context), rhs.codeGen(context), "", context.currentBlock());
}

Value *FunctionDeclaration::codeGen(CodeGenContext &context) const
{
    GUARD();
    vector<Type *> argTypes;
    for (auto arg : args)
    {
        argTypes.push_back(typeOf(*arg->type));
    }

    FunctionType *ftype = FunctionType::get(Type::getInt32Ty(GlobalContext), argTypes, false);
    Function *function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name, context.module.get());

    if (!block)
    {
        return function;
    }

    BasicBlock *bblock = BasicBlock::Create(GlobalContext, "entry", function);

    context.pushBlock(bblock);

    args.codeGen(context);

    block->codeGen(context);

    auto v = ConstantInt::get(GlobalContext, APInt(32, 0));
    ReturnInst::Create(GlobalContext, v, bblock);

    context.popBlock();
    return function;
}

Value *MethodCall::codeGen(CodeGenContext &context) const
{
    GUARD();
    Function *function = context.module->getFunction(id.name);
    if (function == nullptr)
    {
        throw runtime_error("function '" + id.name + "' not found");
    }

    std::vector<Value *> argv;
    for (auto arg : args)
    {
        argv.push_back(arg->codeGen(context));
    }

    CallInst *call = CallInst::Create(function, argv, "", context.currentBlock());
    return call;
}

Value *VariableDeclaration::codeGen(CodeGenContext &context) const
{
    GUARD();
    if (!id) // function decl
    {
        return nullptr;
    }

    AllocaInst *alloc = new AllocaInst(typeOf(*type), static_cast<unsigned int>(-1), id->name, context.currentBlock());
    context.locals()[id->name] = alloc;
    if (rhs != nullptr)
    {
        Assignment assn(*id, *rhs);
        assn.codeGen(context);
    }
    return alloc;
}

Value *ExpressionStatement::codeGen(CodeGenContext &context) const
{
    GUARD();
    return expr.codeGen(context);
}

Value *ArgumentList::codeGen(CodeGenContext &context) const
{
    GUARD();
    Value *result = nullptr;
    for (auto arg : args)
    {
        result = arg->codeGen(context);
    }

    return nullptr;
}
