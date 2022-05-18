#include "ast.hpp"
using namespace lyf;

static llvm::LLVMContext context;
static std::unique_ptr<llvm::Module> mod;
static llvm::IRBuilder<> builder(context);
static llvm::Function *fun;
static std::map<std::string, llvm::Value *> named_values;
static std::map<std::string, std::unique_ptr<PrototypeNode>> FunctionProtos;

std::unique_ptr<ProgramNode> root;

inline llvm::Value *LogError(const char* str) {
    std::cerr << str << std::endl;
    return nullptr;
}

llvm::Value *ProgramNode::codeGen() {
    for (auto &node: nodes) {
        node->codeGen();
    }
}

llvm::Type *VarType::toLLVMtype() const {
    if (dim.size() != 0) {
        int size = 1;
        for (int s: dim) {
            size *= s;
        }
        VarType base(type, std::deque<int>{});
        return llvm::ArrayType::get(base.toLLVMtype(), size);
    } else if (pLevel == 0) {
        switch (type) {
            case INT: {
                return llvm::Type::getInt32Ty(context);
            }
            case FLOAT: {
                return llvm::Type::getDoubleTy(context);
            }
            case CHAR: {
                return llvm::Type::getInt8Ty(context);
            }
            case VOID: {
                return llvm::Type::getVoidTy(context);
            }
        }
    } else {
        return llvm::Type::getInt64Ty(context);
    }
}

llvm::Value *PrototypeNode::codeGen() {
    auto arg_type = std::move(args->getTypes());
    auto llvm_arg_type = std::vector<llvm::Type*>();
    for (auto &arg: arg_type) {
        llvm_arg_type.push_back(arg->toLLVMtype());
    }
    llvm::FunctionType *funcType = llvm::FunctionType::get(retType->toLLVMtype(), llvm_arg_type, false);
    llvm::Function *function = llvm::Function::Create(funcType, llvm::GlobalValue::ExternalLinkage, name, mod.get());
    int idx = 0;
    auto names = args->getNames();
    for (auto &arg: function->args()) {
        arg.setName(names[idx++]);
    }
    return function;
}

llvm::Value *FunctionNode::codeGen() {
    llvm::Function *function = mod->getFunction(proto->getName());
    if (!function) {
        function = static_cast<llvm::Function*>(proto->codeGen());
    }
    if (!function) {
        return nullptr;
    }
    if (!function->empty()) {
        return LogError("Function cannot be redefined!");
    }
    llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(bb);
    named_values.clear();
    for (auto &arg: function->args()) {
        named_values[std::string(arg.getName())] = &arg;
    }
    if (llvm::Value *ret_val = body->codeGen()) {
        builder.CreateRet(ret_val);
        llvm::verifyFunction(*function);
        return function;
    }
    function->eraseFromParent();
    return nullptr;
}