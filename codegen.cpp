#include "codegen.hpp"

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "node.hpp"

using namespace Node;
using namespace llvm;
using namespace std::literals;

uint32_t default_addrspace = static_cast<uint32_t>(-1);

CodeGenContext::CodeGenContext()
{
    module = llvm::make_unique<Module>("main module", GlobalContext);
}

/* Compile the AST into a module */
void CodeGenContext::generateCode(Node::Block &root)
{
    std::cout << "Generating code...\n";

    /* Create the top level interpreter function to call as entry */
    // vector<Type *> argTypes;
    // FunctionType *ftype = FunctionType::get(Type::getVoidTy(GlobalContext), argTypes, false);
    // mainFunction = Function::Create(ftype, GlobalValue::InternalLinkage, default_addrspace, "main", module.get());
    // BasicBlock *bblock = BasicBlock::Create(GlobalContext, "entry", mainFunction, 0);

    /* Push a new variable/block context */
    // pushBlock(bblock);
    root.codeGen(*this); /* emit bytecode for the toplevel block */
    // ReturnInst::Create(GlobalContext, bblock);
    // popBlock();

    /* Print the bytecode in a human-readable format 
       to see if our program compiled properly
     */
    std::cout << "Code is generated.\n";
}

/* Executes the AST by running the main function */
// GenericValue CodeGenContext::runCode()
// {
//     std::cout << "Running code...\n";

//     ExecutionEngine *ee = EngineBuilder(unique_ptr<Module>(module)).create();
//     vector<GenericValue> noargs;
//     GenericValue v = ee->runFunction(mainFunction, noargs);
//     std::cout << "Code was run.\n";
//     return v;
// }
void CodeGenContext::runCode()
{
    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    auto TargetTriple = sys::getDefaultTargetTriple();
    cout << TargetTriple << "," << module.get() << endl;
    module->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target)
    {
        errs() << Error;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto TheTargetMachine =
        Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    module->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::F_None);

    if (EC)
    {
        errs() << "Could not open file: " << EC.message();
    }

    legacy::PassManager pass;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, TargetMachine::CGFT_ObjectFile, false))
    {
        errs() << "TheTargetMachine can't emit a file of this type";
    }

    pass.run(*module);
    dest.flush();

    outs() << TargetTriple << ": Wrote " << Filename << " (" << dest.tell() << " bytes)\n";
}