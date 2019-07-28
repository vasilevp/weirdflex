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

CodeGenContext::CodeGenContext()
{
    module = llvm::make_unique<Module>("main module", GlobalContext);
}

/* Compile the AST into a module */
void CodeGenContext::generateCode(Node::Block &root)
{
    std::cout << "Generating code...\n";

    vector<Type *> argTypes;
    argTypes.push_back(Type::getInt8PtrTy(GlobalContext));
    llvm::FunctionType *fccType = llvm::FunctionType::get(llvm::Type::getVoidTy(GlobalContext), argTypes, true);
    Function *fcc = (Function *)module->getOrInsertFunction("printf", fccType);

    root.codeGen(*this);

    std::cout << "Code is generated.\n";

    /* Print the bytecode in a human-readable format 
       to see if our program compiled properly
     */
    legacy::PassManager pm;
    pm.add(createPrintModulePass(outs()));
    pm.run(*module);
}

void CodeGenContext::runCode()
{
    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    auto TargetTriple = sys::getDefaultTargetTriple();
    module->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target)
    {
        errs() << Error;
        return;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>(Reloc::Model::PIC_);
    auto TheTargetMachine =
        Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    module->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::F_None);

    if (EC)
    {
        errs() << "Could not open file: " << EC.message();
        return;
    }

    legacy::PassManager pass;

#ifdef __MINGW32__
    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, TargetMachine::CGFT_ObjectFile, false, nullptr))
    {
        errs() << "TheTargetMachine can't emit a file of this type";
        return;
    }
#else
    if (TheTargetMachine->addPassesToEmitFile(pass, dest, /* nullptr, */ TargetMachine::CGFT_ObjectFile, false, nullptr))
    {
        errs() << "TheTargetMachine can't emit a file of this type";
        return;
    }
#endif

    pass.run(*module);
    dest.flush();

    outs() << TargetTriple << ": Wrote " << Filename << " (" << dest.tell() << " bytes)\n";
}