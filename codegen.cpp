#include "codegen.hpp"

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/FileSystem.h>
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <unistd.h>

#include "node.hpp"

using namespace Node;
using namespace llvm;
using namespace std::literals;

CodeGenContext::CodeGenContext()
{
	module = llvm::make_unique<Module>("main module", GlobalContext);

	// Initialize the target registry etc.
	InitializeAllTargetInfos();
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmParsers();
	InitializeAllAsmPrinters();
}

void CodeGenContext::generateCode(Node::Block &root)
{
	std::cout << "Generating code...\n";

	root.codeGen(*this);

	std::cout << "Code is generated.\n";

	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	legacy::PassManager pm;
	pm.add(createPrintModulePass(outs()));
	pm.run(*module);
}

void CodeGenContext::buildObject(const std::string &filename)
{
	auto targetTriple = sys::getDefaultTargetTriple();
	module->setTargetTriple(targetTriple);

	std::string Error;
	auto target = TargetRegistry::lookupTarget(targetTriple, Error);

	if (!target)
	{
		errs() << Error;
		return;
	}

	auto CPU = "generic";
	auto Features = "";

	TargetOptions opt;
	auto RM = Optional<Reloc::Model>(Reloc::Model::PIC_);
	auto targetMachine = target->createTargetMachine(targetTriple, CPU, Features, opt, RM);

	module->setDataLayout(targetMachine->createDataLayout());

	std::error_code EC;
	raw_fd_ostream dest(filename, EC, sys::fs::F_None);

	if (EC)
	{
		errs() << "Could not open file: " << EC.message();
		return;
	}

	legacy::PassManager pass;

#ifdef __MINGW32__
	if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, TargetMachine::CGFT_ObjectFile, false, nullptr))
#else
	if (targetMachine->addPassesToEmitFile(pass, dest, /* nullptr, */ TargetMachine::CGFT_ObjectFile, false, nullptr))
#endif
	{
		errs() << "TheTargetMachine can't emit a file of this type";
		return;
	}

	pass.run(*module);
	dest.flush();

	outs() << targetTriple << ": Wrote " << filename << " (" << dest.tell() << " bytes)\n";
}

void CodeGenContext::buildExecutable(const std::string &output, const std::string &input)
{
	char *argv[] = {
		"gcc",
		const_cast<char *>(input.c_str()),
		"-o",
		const_cast<char *>(output.c_str()),
	};

	// outs() << "trying to call gcc with " << argv[0] << ", " << argv[1] << ", " << argv[2] << '\n';

	if (int err = execv("/mingw64/bin/gcc", argv); err != 0)
	{
		perror(nullptr);
	}
}