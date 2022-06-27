#include <fstream>
#include <json/json.h>
#include "ast.hpp"
#include "cminus.y.h"

extern std::shared_ptr<lyf::ASTNode> ROOT;
extern std::unique_ptr<llvm::Module> mod;

int main()
{
    yyparse();
    lyf::CreatePrintf();
    lyf::CreateScanf();
    // std::printf("%p", ROOT);
    auto res = ROOT->codeGen();
    if (res == nullptr) {
        std::cerr << "Error occured during compilation." << std::endl;
    }
    if (llvm::verifyModule(*mod, &llvm::errs())) {
        llvm::errs() << "There are errors in module!" << "\n";
        return -1;
    }
    std::error_code EC;
    std::string IRFileName = "a.ll";
    llvm::raw_fd_ostream IRFile(IRFileName, EC);
    // output IR file
    IRFile << *mod << "\n";
    std::ofstream astJson("ast.json");
    astJson << ROOT->jsonGen();
    astJson.close();

    return 0;
}
