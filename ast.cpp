#include "ast.hpp"
using namespace lyf;

static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::Module> TheModule;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::map<std::string, llvm::Value *> NamedValues;
static std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
static std::map<std::string, std::unique_ptr<PrototypeNode>> FunctionProtos;
static llvm::ExitOnError ExitOnErr;

