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
#include <llvm/Support/raw_ostream.h>

#include "parser.hpp"
#include "codegen.hpp"

using namespace llvm;
using namespace std;
using namespace Node;

llvm::LLVMContext Node::Context;

const Identifier Node::TypeAgnostic = Identifier("_untyped");

Value *Integer::codeGen(CodeGenContext &context)
{
    cout << "Creating integer: " << value << endl;
    return ConstantInt::get(Type::getInt64Ty(Context), value);
}

Value *Double::codeGen(CodeGenContext &context)
{
    cout << "Creating double: " << value << endl;
    return ConstantFP::get(Type::getDoubleTy(Context), value);
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
    Value *last = nullptr;
    for (auto s : stmts)
    {
        cout << "Generating code for " << typeid(*s).name() << '\n';
        last = s->codeGen(context);
    }
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

Value *FunctionDeclaration::codeGen(CodeGenContext &context)
{
    throw runtime_error("not implemented");
}

Value *MethodCall::codeGen(CodeGenContext &context)
{
    throw runtime_error("not implemented");
}

Value *VariableDeclaration::codeGen(CodeGenContext &context)
{
    throw runtime_error("not implemented");
}

Value *ExpressionStatement::codeGen(CodeGenContext &context)
{
    throw runtime_error("not implemented");
}
