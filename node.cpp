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
    return Type::getVoidTy(GlobalContext);
}

Value *Integer::codeGen(CodeGenContext &context)
{
    cout << "Creating integer: " << value << endl;
    return ConstantInt::get(GlobalContext, APInt(64, value, false));
}

Value *Double::codeGen(CodeGenContext &context)
{
    cout << "Creating double: " << value << endl;
    return ConstantFP::get(GlobalContext, APFloat(value));
}

Value *Identifier::codeGen(CodeGenContext &context)
{
    cout << "Creating identifier reference: " << name << '\n';
    if (context.locals().find(name) == context.locals().end())
    {
        cerr << "undeclared variable " << name << '\n';
        return NULL;
    }
    return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value *Block::codeGen(CodeGenContext &context)
{
    int r = rand();
    std::cout << "Creating block " << r << endl;
    Value *last = nullptr;
    for (auto s : stmts)
    {
        cout << "Generating code for " << typeid(*s).name() << '\n';
        last = s->codeGen(context);
    }

    std::cout << "End of block " << r << endl;
    return last;
}

Value *Assignment::codeGen(CodeGenContext &context)
{
    cout << "Creating assignment for " << lhs.name << '\n';
    if (context.locals().find(lhs.name) == context.locals().end())
    {
        cerr << "undeclared variable " << lhs.name << '\n';
        return nullptr;
    }
    return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value *Node::BinaryOperator::codeGen(CodeGenContext &context)
{
    Instruction::BinaryOps instr;

    auto l = lhs.codeGen(context);
    auto r = rhs.codeGen(context);

    switch (op)
    {
    case PLUS:
        std::cout << "Creating binary op: +" << std::endl;
        instr = Instruction::Add;
        break;
    case MINUS:
        std::cout << "Creating binary op: -" << std::endl;
        instr = Instruction::Sub;
        break;
    case MUL:
        std::cout << "Creating binary op: *" << std::endl;
        instr = Instruction::Mul;
        break;
    case DIV:
        std::cout << "Creating binary op: /" << std::endl;
        instr = Instruction::SDiv;
        break;

    default:
        return nullptr;
    }

    return llvm::BinaryOperator::Create(instr, lhs.codeGen(context), rhs.codeGen(context), "", context.currentBlock());
}

Value *FunctionDeclaration::codeGen(CodeGenContext &context)
{
    vector<Type *> argTypes;
    for (auto arg : args)
    {
        argTypes.push_back(typeOf(*arg->type));
    }

    FunctionType *ftype = FunctionType::get(typeOf(*type), argTypes, false);
    Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name, context.module.get());
    BasicBlock *bblock = BasicBlock::Create(GlobalContext, "entry", function);

    context.pushBlock(bblock);

    for (auto arg : args)
    {
        arg->codeGen(context);
    }

    block.codeGen(context);
    ReturnInst::Create(GlobalContext, bblock);

    context.popBlock();
    std::cout << "Creating function: " << id.name << std::endl;
    return function;
}

Value *MethodCall::codeGen(CodeGenContext &context)
{
    Function *function = context.module->getFunction(id.name);
    if (function == nullptr)
    {
        std::cerr << "no such function " << id.name << std::endl;
    }

    std::vector<Value *> argv;
    for (auto arg : args)
    {
        argv.push_back(arg->codeGen(context));
    }

    CallInst *call = CallInst::Create(function, argv, "", context.currentBlock());
    std::cout << "Creating method call: " << id.name << std::endl;
    return call;
}

Value *VariableDeclaration::codeGen(CodeGenContext &context)
{
    std::cout << "Creating variable declaration " << type->name << " " << id.name << std::endl;
    AllocaInst *alloc = new AllocaInst(typeOf(*type), static_cast<unsigned int>(-1), id.name, context.currentBlock());
    context.locals()[id.name] = alloc;
    if (rhs != nullptr)
    {
        Assignment assn(id, *rhs);
        assn.codeGen(context);
    }
    return alloc;
}

Value *ExpressionStatement::codeGen(CodeGenContext &context)
{
    // std::cout << "Generating code for " << typeid(expr).name() << std::endl;
    return expr.codeGen(context);
}
