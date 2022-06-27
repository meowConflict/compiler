#include "ast.hpp"
using namespace lyf;

class NameStack {
    std::vector<std::unordered_map<std::string, VarRecord>> records;
public:
    VarRecord operator[](const std::string &name) {
        for (int i=records.size()-1; i>=0; i--) {
            auto &record = records[i];
            if (record.find(name) != record.end()) {
                return record[name];
            }
        }
        return (VarRecord){nullptr, nullptr};
    }
    void clear() {
        records.clear();
        push();
    }
    void push() {
        records.emplace_back();
    }
    bool add(const std::string &name, VarRecord rec) {
        auto &record = records.back();
        if (record.find(name) != record.end()) {
            return false;
        } else {
            record[name] = rec;
        }
        return true;
    }
    void pop() {
        records.pop_back();
    }
};

static llvm::LLVMContext context;
std::unique_ptr<llvm::Module> mod = std::make_unique<llvm::Module>("lyf", context);
static llvm::IRBuilder<> builder(context);
static llvm::Function *fun;
static NameStack named_rec;
static std::unordered_map<std::string, PrototypeNode*> func_proto;
static std::string cur_fun;
static llvm::BasicBlock *cur_merge = nullptr;
static llvm::BasicBlock *cur_cond = nullptr;
static llvm::BasicBlock *entry = nullptr;

inline llvm::Value *LogError(const char* str) {
    std::cerr << str << std::endl;
    return nullptr;
}

void lyf::CreateScanf() {
    std::vector<llvm::Type*> scanfArgs;
    scanfArgs.push_back(llvm::Type::getInt8PtrTy(context));
    llvm::FunctionType *scanfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), scanfArgs, true);
    // create printf
    llvm::Function::Create(scanfType, llvm::Function::ExternalLinkage, "scanf", mod.get());
}

void lyf::CreatePrintf() {
    std::vector<llvm::Type*> printfArgs;
    printfArgs.push_back(llvm::Type::getInt8PtrTy(context));
    llvm::FunctionType *printfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), printfArgs, true);
    // create printf
    llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", mod.get());
}

llvm::Value *ProgramNode::codeGen() {
    llvm::Value *ret = llvm::ConstantInt::get(context, llvm::APInt(64, 0));
    for (auto &node: nodes) {
        ret = node->codeGen();
        if (ret==nullptr) {
            break;
        }
    }
    return ret;
}

llvm::Type *VarType::toLLVMtype() const {
    if (dim.size() != 0) {
        int size = 1;
        for (int s: dim) {
            size *= s;
        }
        VarType base(type, std::deque<int>{}, pLevel);
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
    return llvm::Type::getVoidTy(context);
}

llvm::Type *VarType::toLLVMPtrType() const {
    assert(dim.size() == 0);
    if (pLevel == 0) {
        switch (type) {
            case INT: {
                return llvm::Type::getInt32PtrTy(context);
            }
            case FLOAT: {
                return llvm::Type::getDoublePtrTy(context);
            }
            case CHAR: {
                return llvm::Type::getInt8PtrTy(context);
            }
        }
    } else {
        return llvm::Type::getInt64PtrTy(context);
    }
    return llvm::Type::getVoidTy(context);
}

llvm::Type *VarType::toLLVMArgType() const {
    if (dim.size()!=0) {
        return llvm::Type::getInt64Ty(context);
    } else {
        return toLLVMtype();
    }
}

llvm::Value *PrototypeNode::codeGen() {
    if (name == "scanf" or name == "printf") {
        return LogError("You cannot define scanf or printf yoirself.");
    }
    auto arg_type = args->getTypes();
    if (func_proto.find(name) != func_proto.end()) {
        auto pre = func_proto[name];
        auto pre_type = pre->args->getTypes();
        bool match = true;
        if (pre_type.size() != arg_type.size()) {
            match = false;
        } else {
            for (int i=0; i<pre_type.size(); i++) {
                if (*(pre_type[i])!=*(arg_type[i])) {
                    match = false;
                    break;
                }
            }
        }
        if (match) {
            match = *(pre->getRet())==*retType;
        }
        if (!match) {
            return LogError(("Function "+name+" declaration conflicts.").c_str());
        } else {
            return mod->getFunction(name);
        }
    }
    auto llvm_arg_type = std::vector<llvm::Type*>();
    for (auto &arg: arg_type) {
        llvm_arg_type.push_back(arg->toLLVMArgType());
    }
    llvm::FunctionType *funcType = llvm::FunctionType::get(retType->toLLVMtype(), llvm_arg_type, false);
    llvm::Function *function = llvm::Function::Create(funcType, llvm::GlobalValue::ExternalLinkage, name, mod.get());
    int idx = 0;
    func_proto[name] = this;
    return function;
}

llvm::Value *FunctionNode::codeGen() {
    llvm::Function *function = static_cast<llvm::Function*>(proto->codeGen());
    if (!function) {
        return nullptr;
    }
    if (!function->empty()) {
        return LogError("Function cannot be redefined!");
    }
    cur_fun = proto->getName();
    llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", function);
    entry = bb;
    llvm::BasicBlock *nextBB = llvm::BasicBlock::Create(context, "next", function);
    builder.SetInsertPoint(nextBB);
    named_rec.clear();
    auto args = proto->getArgs();
    if (args->getSize() != 0) {
        auto it = function->arg_begin();
        if (args->codeGen(it) == nullptr) {
            return nullptr;
        }
    }
    if (body->codeGen()) {
        auto curBB = builder.GetInsertBlock();
        builder.SetInsertPoint(entry);
        builder.CreateBr(nextBB);
        builder.SetInsertPoint(curBB);
        auto ret_type = proto->getRet();
        if (!builder.GetInsertBlock()->getTerminator()) {
            if (ret_type->pLevel != 0) {
                builder.CreateRet(llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, 0)));
            } else if (ret_type->type == INT) {
                builder.CreateRet(llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(context), llvm::APInt(32, 0)));
            } else if (ret_type->type == FLOAT) {
                builder.CreateRet(llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0));
            } else if (ret_type->type == CHAR) {
                builder.CreateRet(llvm::Constant::getIntegerValue(llvm::Type::getInt8Ty(context), llvm::APInt(8, 0)));
            } else if (ret_type->type == VOID) {
                builder.CreateRetVoid();
            }
        }
        llvm::verifyFunction(*function);
        return function;
    }
    function->eraseFromParent();
    return LogError("What the hell happened here?");
}

llvm::Value *VarExprNode::codeGen() {
    auto r = named_rec[name];
    if (r.type == nullptr) {
        return LogError(("Unknown variable name: "+name).c_str());
    } else {
        setType(r.type);
        if (!getAssign()) {
            if (r.type->dim.size() != 0) {
                return r.val;
            } else {
                return builder.CreateLoad(r.type->toLLVMtype(), r.val);
            }
        } else {
            setLeft();
            return r.val;
        }
    }
}

llvm::Value *ConstIntNode::codeGen() {
    setType(std::make_shared<VarType>(
        INT,
        std::deque<int>()
    ));
    return llvm::ConstantInt::get(context, llvm::APInt(32, value, true));
}

llvm::Value *ConstNPtrNode::codeGen() {
    setType(std::make_shared<VarType>(
        VOID,
        std::deque<int>(),
        1
    ));
    return llvm::ConstantInt::get(context, llvm::APInt(64, 0, false));
}

llvm::Value *ConstFloatNode::codeGen() {
    setType(std::make_shared<VarType>(
        FLOAT,
        std::deque<int>()
    ));
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), value);
}

llvm::Value *ConstCharNode::codeGen() {
    setType(std::make_shared<VarType>(
        CHAR,
        std::deque<int>()
    ));
    return llvm::ConstantInt::get(context, llvm::APInt(8, value, true));
}

llvm::Value *ConstStringNode::codeGen() {
    auto type = std::make_shared<VarType>(
        CHAR,
        std::deque<int>{(int)value.length()+1},
        0
    );
    setType(type);
    std::vector<llvm::Constant *> str;
    for (auto ch: value) {
        str.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), ch));
    }
    str.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), '\0'));
    auto stringType = static_cast<llvm::ArrayType *>(type->toLLVMtype());
    auto globalDeclaration = new llvm::GlobalVariable(*mod, stringType, true, llvm::GlobalValue::PrivateLinkage, llvm::ConstantArray::get(stringType, str));
    // (llvm::GlobalVariable *) mod.getOrInsertGlobal(".str" + std::to_string(i++), stringType);
    globalDeclaration->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    return builder.CreatePtrToInt(globalDeclaration, llvm::Type::getInt64Ty(context));
}

llvm::Value *BinaryExprNode::codeGen() {
    if (op == "ASSIGN") {
        lhs->setAssign();
    }
    llvm::Value *L = lhs->codeGen();
    llvm::Value *R = rhs->codeGen();
    if (L == nullptr or R == nullptr) {
        return nullptr;
    }
    auto type1 = lhs->getType();
    auto type2 = rhs->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    if (op == "PLUS") {
        bool isVoid1 = (type1->type==VOID) and ((type1->pLevel==0) or (type1->pLevel==1 and type1->dim.size()==0));
        bool isVoid2 = (type2->type==VOID) and ((type2->pLevel==0) or (type2->pLevel==1 and type2->dim.size()==0));
        if (isPtr1 and isPtr2) {
            return LogError("Pointer cannot be added with pointer!");
        }
        if ((isPtr1 and type2->type == FLOAT) or (isPtr2 and type1->type == FLOAT)) {
            return LogError("Pointer can't be added with FLOAT!");
        }
        if (isVoid1 or isVoid2) {
            return LogError("Void or void pointer can't be operated!");
        }
        auto type = std::make_shared<VarType>(
            CHAR,
            std::deque<int>{}
        );
        if (isPtr1) {
            type->type = type1->type;
            type->dim = type1->dim;
            type->pLevel = type1->pLevel;
            setType(type);
            auto val = builder.CreateSExtOrTrunc(R, llvm::Type::getInt64Ty(context));
            int size;
            if (type->dim.size() != 0) {
                size = type->getBaseSize();
            } else {
                type->pLevel -= 1;
                size = type->getBaseSize();
                type->pLevel += 1;
            }
            for (int i=1; i<type->dim.size(); i++) {
                size *= type->dim[i];
            }
            val = builder.CreateMul(llvm::ConstantInt::get(context, llvm::APInt(64, size, true)), val);
            return builder.CreateAdd(L, val);
        } else if (isPtr2) {
            type->type = type2->type;
            type->dim = type2->dim;
            type->pLevel = type2->pLevel;
            setType(type);
            auto val = builder.CreateSExtOrTrunc(L, llvm::Type::getInt64Ty(context));
            int size;
            if (type->dim.size() != 0) {
                size = type->getBaseSize();
            } else {
                type->pLevel -= 1;
                size = type->getBaseSize();
                type->pLevel += 1;
            }
            for (int i=1; i<type->dim.size(); i++) {
                size *= type->dim[i];
            }
            val = builder.CreateMul(llvm::ConstantInt::get(context, llvm::APInt(64, size, true)), val);
            return builder.CreateAdd(R, val);
        }
        else if (type1->type == FLOAT or type2->type == FLOAT) {
            type->type = FLOAT;
            setType(type);
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateFAdd(L, R);
        } else if (type1->type == INT or type2->type == INT) {
            type->type = INT;
            setType(type);
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateAdd(L, R);
        } else {
            setType(type);
            return builder.CreateAdd(L, R);
        }
    } else if (op == "MINUS") {
        bool isVoid1 = (type1->type==VOID) and ((type1->pLevel==0) or (type1->pLevel==1 and type1->dim.size()==0));
        bool isVoid2 = (type2->type==VOID) and ((type2->pLevel==0) or (type2->pLevel==1 and type2->dim.size()==0));
        if (isPtr2) {
            return LogError("Arrays or pointers can't be subtrahend!");
        }
        if (isPtr1 and type2->type == FLOAT) {
            return LogError("Subtracting can't be done between pointer and float!");
        }
        if (isVoid1 or isVoid2) {
            return LogError("Void or void pointer can't be operated!");
        }
        auto type = std::make_shared<VarType>(
            CHAR,
            std::deque<int>{}
        );
        if (isPtr1) {
            type->type = type1->type;
            type->dim = type1->dim;
            type->pLevel = type1->pLevel;
            setType(type);
            auto val = builder.CreateSExtOrTrunc(R, llvm::Type::getInt64Ty(context));
            int size;
            if (type->dim.size() != 0) {
                size = type->getBaseSize();
            } else {
                type->pLevel -= 1;
                size = type->getBaseSize();
                type->pLevel += 1;
            }
            for (int i=1; i<type->dim.size(); i++) {
                size *= type->dim[i];
            }
            val = builder.CreateMul(llvm::ConstantInt::get(context, llvm::APInt(64, size, true)), val);
            return builder.CreateSub(L, val);
        }
        else if (type1->type == FLOAT or type2->type == FLOAT) {
            type->type = FLOAT;
            setType(type);
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateFSub(L, R);
        } else if (type1->type == INT or type2->type == INT) {
            type->type = INT;
            setType(type);
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateSub(L, R);
        } else {
            setType(type);
            return builder.CreateSub(L, R);
        }
    } else if (op == "STAR") {
        if (isPtr1 or isPtr2) {
            return LogError("MUL not allowed on array or pointer!");
        }
        if (type1->type == CHAR and type2->type == CHAR) {
            return LogError("MUL not allowed on chars!");
        }
        if (type1->type == VOID or type2->type == VOID) {
            return LogError("Void or void pointer can't be operated!");
        }
        auto type = std::make_shared<VarType>(
            CHAR,
            std::deque<int>{}
        );
        if (type1->type == FLOAT or type2->type == FLOAT) {
            type->type = FLOAT;
            setType(type);
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateFMul(L, R);
        } else if (type1->type == INT or type2->type == INT) {
            type->type = INT;
            setType(type);
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateMul(L, R);
        }
    } else if (op == "DIV") {
        if (isPtr1 or isPtr2) {
            return LogError("DIV not allowed on array or pointer!");
        }
        if (type1->type == CHAR or type2->type == CHAR) {
            return LogError("DIV not allowed on char!");
        }
        if (type1->type == VOID or type2->type == VOID) {
            return LogError("Void or void pointer can't be operated!");
        }
        auto type = std::make_shared<VarType>(
            CHAR,
            std::deque<int>{}
        );
        if (type1->type == FLOAT or type2->type == FLOAT) {
            type->type = FLOAT;
            setType(type);
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateFDiv(L, R);
        } else if (type1->type == INT or type2->type == INT) {
            type->type = INT;
            setType(type);
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateSDiv(L, R);
        }
    } else if (op == "REM") {
        if (isPtr1 or isPtr2) {
            return LogError("REM not allowed on array or pointer!");
        }
        if (type1->type == CHAR or type2->type == CHAR) {
            return LogError("REM not allowed on char!");
        }
        if (type1->type == VOID or type2->type == VOID) {
            return LogError("Void or void pointer can't be operated!");
        }
        if (type1->type == FLOAT or type2->type == FLOAT) {
            return LogError("REM not allowed on float!");
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (type1->type != INT) {
            L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
        }
        else if (type2->type != INT) {
            R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
        }
        return builder.CreateSRem(L, R);
    } else if (op == "NE") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
            return LogError("Pointer cannot be compared with non-pointer!");
        }
        if (isPtr1 and isPtr2) {
            if (!type1->match(*type2)) {
                return LogError("Pointer type does not match.");
            }
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (isPtr1) {
            return builder.CreateZExtOrTrunc(builder.CreateICmpNE(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == FLOAT or type2->type == FLOAT) {
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateZExtOrTrunc(builder.CreateFCmpONE(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == INT or type2->type == INT) {
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateZExtOrTrunc(builder.CreateICmpNE(L, R), llvm::Type::getInt32Ty(context));
        } else {
            return builder.CreateZExtOrTrunc(builder.CreateICmpNE(L, R), llvm::Type::getInt32Ty(context));
        }
    } else if (op == "EQUAL") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
            return LogError("Pointer cannot be compared with non-pointer!");
        }
        if (isPtr1 and isPtr2) {
            if (!type1->match(*type2)) {
                return LogError("Pointer type does not match.");
            }
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (isPtr1) {
            return builder.CreateZExtOrTrunc(builder.CreateICmpEQ(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == FLOAT or type2->type == FLOAT) {
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateZExtOrTrunc(builder.CreateFCmpOEQ(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == INT or type2->type == INT) {
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateZExtOrTrunc(builder.CreateICmpEQ(L, R), llvm::Type::getInt32Ty(context));
        } else {
            return builder.CreateZExtOrTrunc(builder.CreateICmpEQ(L, R), llvm::Type::getInt32Ty(context));
        }
    } else if (op == "GT") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
            return LogError("Pointer cannot be compared with non-pointer!");
        }
        if (isPtr1 and isPtr2) {
            if (!type1->match(*type2)) {
                return LogError("Pointer type does not match.");
            }
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (isPtr1) {
            return builder.CreateZExtOrTrunc(builder.CreateICmpUGT(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == FLOAT or type2->type == FLOAT) {
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateZExtOrTrunc(builder.CreateFCmpOGT(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == INT or type2->type == INT) {
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateZExtOrTrunc(builder.CreateICmpSGT(L, R), llvm::Type::getInt32Ty(context));
        } else {
            return builder.CreateZExtOrTrunc(builder.CreateICmpSGT(L, R), llvm::Type::getInt32Ty(context));
        }
    } else if (op == "GE") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
            return LogError("Pointer cannot be compared with non-pointer!");
        }
        if (isPtr1 and isPtr2) {
            if (!type1->match(*type2)) {
                return LogError("Pointer type does not match.");
            }
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (isPtr1) {
            return builder.CreateZExtOrTrunc(builder.CreateICmpUGE(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == FLOAT or type2->type == FLOAT) {
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateZExtOrTrunc(builder.CreateFCmpOGE(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == INT or type2->type == INT) {
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateZExtOrTrunc(builder.CreateICmpSGE(L, R), llvm::Type::getInt32Ty(context));
        } else {
            return builder.CreateZExtOrTrunc(builder.CreateICmpSGE(L, R), llvm::Type::getInt32Ty(context));
        }
    } else if (op == "LT") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
            return LogError("Pointer cannot be compared with non-pointer!");
        }
        if (isPtr1 and isPtr2) {
            if (!type1->match(*type2)) {
                return LogError("Pointer type does not match.");
            }
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (isPtr1) {
            return builder.CreateZExtOrTrunc(builder.CreateICmpULT(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == FLOAT or type2->type == FLOAT) {
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateZExtOrTrunc(builder.CreateFCmpOLT(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == INT or type2->type == INT) {
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateZExtOrTrunc(builder.CreateICmpSLT(L, R), llvm::Type::getInt32Ty(context));
        } else {
            return builder.CreateZExtOrTrunc(builder.CreateICmpSLT(L, R), llvm::Type::getInt32Ty(context));
        }
    } else if (op == "LE") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
            return LogError("Pointer cannot be compared with non-pointer!");
        }
        if (isPtr1 and isPtr2) {
            if (!type1->match(*type2)) {
                return LogError("Pointer type does not match.");
            }
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (isPtr1) {
            return builder.CreateZExtOrTrunc(builder.CreateICmpULE(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == FLOAT or type2->type == FLOAT) {
            if (type1->type != FLOAT) {
                L = builder.CreateSIToFP(L, builder.getDoubleTy());
            }
            else if (type2->type != FLOAT) {
                R = builder.CreateSIToFP(R, builder.getDoubleTy());
            }
            return builder.CreateZExtOrTrunc(builder.CreateFCmpOLE(L, R), llvm::Type::getInt32Ty(context));
        } else if (type1->type == INT or type2->type == INT) {
            if (type1->type != INT) {
                L = builder.CreateSExtOrTrunc(L, builder.getInt32Ty());
            }
            else if (type2->type != INT) {
                R = builder.CreateSExtOrTrunc(R, builder.getInt32Ty());
            }
            return builder.CreateZExtOrTrunc(builder.CreateICmpSLE(L, R), llvm::Type::getInt32Ty(context));
        } else {
            return builder.CreateZExtOrTrunc(builder.CreateICmpSLE(L, R), llvm::Type::getInt32Ty(context));
        }
    } else if (op == "AND") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (type1->type == FLOAT and not isPtr1) {
            L = builder.CreateFCmpONE(L, llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0));
            L = builder.CreateZExtOrTrunc(L, llvm::Type::getInt32Ty(context));
        } else {
            L = builder.CreateZExtOrTrunc(L, llvm::Type::getInt64Ty(context));
            L = builder.CreateICmpNE(L, llvm::ConstantInt::get(context, llvm::APInt(64, 0, false)));
            L = builder.CreateZExtOrTrunc(L, llvm::Type::getInt32Ty(context));
        }
        if (type2->type == FLOAT and not isPtr2) {
            R = builder.CreateFCmpONE(R, llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0));
            R = builder.CreateZExtOrTrunc(R, llvm::Type::getInt32Ty(context));
        } else {
            R = builder.CreateZExtOrTrunc(R, llvm::Type::getInt64Ty(context));
            R = builder.CreateICmpNE(R, llvm::ConstantInt::get(context, llvm::APInt(64, 0, false)));
            R = builder.CreateZExtOrTrunc(R, llvm::Type::getInt32Ty(context));
        }
        return builder.CreateAnd(L, R);
    } else if (op == "OR") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        auto type = std::make_shared<VarType>(
            INT,
            std::deque<int>{}
        );
        setType(type);
        if (type1->type == FLOAT and not isPtr1) {
            L = builder.CreateFCmpONE(L, llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0));
            L = builder.CreateZExtOrTrunc(L, llvm::Type::getInt32Ty(context));
        } else {
            L = builder.CreateZExtOrTrunc(L, llvm::Type::getInt64Ty(context));
            L = builder.CreateICmpNE(L, llvm::ConstantInt::get(context, llvm::APInt(64, 0, false)));
            L = builder.CreateZExtOrTrunc(L, llvm::Type::getInt32Ty(context));
        }
        if (type2->type == FLOAT and not isPtr2) {
            R = builder.CreateFCmpONE(R, llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0));
            R = builder.CreateZExtOrTrunc(R, llvm::Type::getInt32Ty(context));
        } else {
            R = builder.CreateZExtOrTrunc(R, llvm::Type::getInt64Ty(context));
            R = builder.CreateICmpNE(R, llvm::ConstantInt::get(context, llvm::APInt(64, 0, false)));
            R = builder.CreateZExtOrTrunc(R, llvm::Type::getInt32Ty(context));
        }
        return builder.CreateOr(L, R);
    } else if (op == "ASSIGN") {
        bool isVoid1 = type1->type==VOID and type1->pLevel==0 and type1->dim.size()==0;
        bool isVoid2 = type2->type==VOID and type2->pLevel==0 and type2->dim.size()==0;
        bool isFloat1 = type1->type==FLOAT and not isPtr1;
        bool isFloat2 = type2->type==FLOAT and not isPtr2;
        if (isVoid1 or isVoid2) {
            return LogError("Void type is invalid.");
        }
        if (isPtr1 and isPtr2) {
            if (!type1->match(*type2)) {
                return LogError("Pointer type does not match.");
            }
        }
        if (!lhs->left()) {
            return LogError("Right value cannot be assigned!");
        }
        if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
            return LogError("Invalid assignment between pointer and non-pointer.");
        }
        setType(std::make_shared<VarType> (*type1));
        if (isFloat1) {
            if (!isFloat2) {
                R = builder.CreateSIToFP(R, llvm::Type::getDoubleTy(context));
            }
        } else if (!isPtr1) {
            if (isFloat2) {
                R = builder.CreateFPToSI(R, type1->toLLVMtype());
            } else if (type1->type != type2->type) {
                R = builder.CreateSExtOrTrunc(R, type1->toLLVMtype());
            }
        }
        builder.CreateStore(R, L);
        return R;
    } else {
        return LogError(("Unknown operation: "+op).c_str());
    }
    return nullptr;
}

llvm::Value *UnaryExprNode::codeGen() {
    if (op=="AMP") {
        child->setAssign();
    }
    auto V = child->codeGen();
    if (V == nullptr) {
        return nullptr;
    }
    auto type = child->getType();
    bool isPtr = type->dim.size()!=0 or type->pLevel!=0;
    bool isVoid = type->type==VOID and type->pLevel==0 and type->dim.size()==0;
    if (isVoid) {
        return LogError("Void type is invalid.");
    }
    if (op == "AMP") {
        if (!child->left()) {
            return LogError("Right value cannot be Addressed.");
        }
        auto cur_type = std::make_shared<VarType>(*type);
        cur_type->pLevel += 1;
        setType(cur_type);
        V = builder.CreatePtrToInt(V, llvm::Type::getInt64Ty(context));
        return V;
    } else if (op == "UMINUS") {
        if (isPtr) {
            return LogError("Pointer with minus is invalid.");
        }
        auto cur_type = std::make_shared<VarType>(*type);
        setType(cur_type);
        if (type->type==FLOAT) {
            llvm::Value *zero = llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0);
            return builder.CreateFSub(zero, V);
        } else {
            llvm::Value *zero = llvm::ConstantInt::get(V->getType(), 0);
            return builder.CreateSub(zero, V);
        }
    } else if (op == "NOT") {
        auto cur_type = std::make_shared<VarType>(
            INT,
            std::deque<int>()
        );
        setType(cur_type);
        if (type->type==FLOAT) {
            llvm::Value *zero = llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0);
            auto v = builder.CreateFCmpOEQ(zero, V);
            return builder.CreateZExtOrTrunc(v, llvm::Type::getInt32Ty(context));
        } else {
            llvm::Value *zero = llvm::ConstantInt::get(V->getType(), 0);
            auto v = builder.CreateICmpEQ(zero, V);
            return builder.CreateZExtOrTrunc(v, llvm::Type::getInt32Ty(context));
        }
    } else if (op == "USTAR") {
        if (isVoid or (type->type==VOID and type->pLevel==1 and type->dim.size()==0)) {
            return LogError("Void or Void* cannot be operated.");
        }
        if (not isPtr) {
            return LogError("Non-pointer cannot be derefenced.");
        }
        auto cur_type = std::make_shared<VarType>(*type);
        if (cur_type->dim.size()!=0) {
            cur_type->dim.pop_front();
            setType(cur_type);
            if (getAssign()) {
                setLeft();
            }
            if (type->dim.size() != 1) {
                return V; // V: int64
            } else {
                auto ptr = builder.CreateIntToPtr(V, cur_type->toLLVMPtrType());
                if (getAssign()) {
                    return ptr;
                } else {
                    return builder.CreateLoad(cur_type->toLLVMtype(), ptr);
                }
            }
        } else {
            cur_type->pLevel -= 1;
            setType(cur_type);
            auto ptr = builder.CreateIntToPtr(V, cur_type->toLLVMPtrType());
            if (getAssign()) {
                setLeft();
                return ptr;
            } else {
                return builder.CreateLoad(cur_type->toLLVMtype(), ptr);
            }
        }
    } else {
        return LogError((std::string("Undefined unary operator: ")+op).c_str());
    }
}

llvm::Value *CompdStmtNode::codeGen() {
    llvm::Value *ret = llvm::ConstantInt::get(context, llvm::APInt(64, 0));
    if (!isFunc) {
        named_rec.push();
    }
    for (auto &stmt: statements) {
        ret = stmt->codeGen();
        if (ret == nullptr) {
            break;
        }
    }
    if (!isFunc) {
        named_rec.pop();
    }
    return ret;
}

llvm::Value *VDecListStmtNode::codeGen(llvm::Function::arg_iterator &arg_it) {
    llvm::Value *ret = llvm::ConstantInt::get(context, llvm::APInt(64, 0));
    for (auto &decl: decls) {
        llvm::Value *argValue = &*arg_it++;
        argValue->setName(decl->getName());
        ret = decl->argCodeGen(argValue);
        if (ret==nullptr) {
            return nullptr;
        }
    }
    return ret;
}

llvm::Value *VDeclStmtNode::argCodeGen(llvm::Value *val) {
    llvm::Value *alloc;
    if (type->dim.size()!=0) {
        alloc = val;
    } else {
        auto curBB = builder.GetInsertBlock();
        builder.SetInsertPoint(entry);
        alloc = builder.CreateAlloca(type->toLLVMtype());
        builder.SetInsertPoint(curBB);
        builder.CreateStore(val, alloc);
    }
    bool res = named_rec.add(name, (VarRecord){type, alloc});
    if (!res) {
        return LogError(("Variable "+name+" redefined.").c_str());
    }
    return alloc;
}

llvm::Value *VDeclStmtNode::codeGen() {
    auto llvm_type = type->toLLVMtype();
    auto curBB = builder.GetInsertBlock();
    builder.SetInsertPoint(entry);
    llvm::Value *alloc = builder.CreateAlloca(llvm_type);
    builder.SetInsertPoint(curBB);
    if (type->dim.size()==0) {
        if (initValue != nullptr) {
            if (initValue->isArray()) {
                return LogError("You cannot use an array to init non-array variable");
            }
            llvm::Value *V = initValue->codeGen();
            if (V == nullptr) {
                return nullptr;
            }
            auto initType = initValue->getType();
            bool isPtr1 = type->pLevel!=0;
            bool isPtr2 = initType->pLevel!=0 or initType->dim.size()!=0;
            bool isVoid = initType->type==VOID and initType->pLevel==0 and initType->dim.size()==0;
            bool isFloat1 = type->type==FLOAT and not isPtr1;
            bool isFloat2 = initType->type==FLOAT and not isPtr2;
            if (isVoid) {
                return LogError("Void type is invalid.");
            }
            if (isPtr1 and isPtr2) {
                if (!type->match(*initType)) {
                    return LogError("Pointer type does not match.");
                }
            }
            if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
                return LogError("Invalid assignment between pointer and non-pointer.");
            }
            if (isFloat1) {
                if (!isFloat2) {
                    V = builder.CreateSIToFP(V, llvm::Type::getDoubleTy(context));
                }
            } else if (!isPtr1) {
                if (isFloat2) {
                    V = builder.CreateFPToSI(V, llvm_type);
                } else if (type->type != initType->type) {
                    V = builder.CreateSExtOrTrunc(V, llvm_type);
                }
            }
            builder.CreateStore(V, alloc);
        }
    } else {
        if (initValue!=nullptr) {
            if (!initValue->isArray()) {
                return LogError("Non-array value cannot be used to init array.");
            }
            auto initValues = std::static_pointer_cast<ArrayInitExpr>(initValue);
            int initNum = initValues->getSize();
            int size = 1;
            for (int s: type->dim) {
                size *= s;
            }
            if (initNum > size) {
                return LogError("Two many init values.");
            }
            int idx=0;
            bool isPtr1 = type->pLevel!=0;
            bool isFloat1 = type->type==FLOAT and not isPtr1;
            auto unwrapped = type->toUnwrapped();
            for (auto &item: initValues->items) {
                auto val = item->codeGen();
                auto initType = item->getType();
                bool isPtr2 = initType->pLevel!=0 or initType->dim.size()!=0;
                bool isVoid = initType->type==VOID and initType->pLevel==0 and initType->dim.size()==0;
                bool isFloat2 = initType->type==FLOAT and not isPtr2;
                if (isVoid) {
                    return LogError("Void type is invalid.");
                }
                if (isPtr1 and isPtr2) {
                    if (!unwrapped->match(*initType)) {
                        return LogError("Pointer type does not match.");
                    }
                }
                if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
                    return LogError("Invalid assignment between pointer and non-pointer.");
                }
                if (isFloat1) {
                    if (!isFloat2) {
                        val = builder.CreateSIToFP(val, llvm::Type::getDoubleTy(context));
                    }
                } else if (!isPtr1) {
                    auto btype = unwrapped->toLLVMtype();
                    if (isFloat2) {
                        val = builder.CreateFPToSI(val, btype);
                    } else if (type->type != initType->type) {
                        val = builder.CreateSExtOrTrunc(val, btype);
                    }
                }
                auto ptr = builder.CreateGEP(
                    llvm::cast<llvm::PointerType>(alloc->getType()->getScalarType())->getElementType(),
                    alloc,
                    {
                        builder.getInt32(0),
                        builder.getInt32(idx++)
                    }
                );
                builder.CreateStore(val, ptr);
            }
            llvm::Value *val;
            if (isPtr1) {
                val = llvm::ConstantInt::get(context, llvm::APInt(64, 0, false));
            } else if (isFloat1) {
                val = llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0);
            } else {
                val = llvm::ConstantInt::get(unwrapped->toLLVMtype(), 0, true);
            }
            for (int i=initNum; i<size; i++) {
                auto ptr = builder.CreateGEP(
                    llvm::cast<llvm::PointerType>(alloc->getType()->getScalarType())->getElementType(),
                    alloc,
                    {
                        builder.getInt32(0),
                        builder.getInt32(idx++)
                    }
                );
                builder.CreateStore(val, ptr);
            }
        }
        alloc = builder.CreatePtrToInt(alloc, llvm::Type::getInt64Ty(context));
    }
    bool res = named_rec.add(name, (VarRecord){type, alloc});
    if (!res) {
        return LogError(("Variable "+name+" redefined.").c_str());
    }
    return alloc;
}

llvm::Value *VDecListStmtNode::codeGen() {
    llvm::Value *ret;
    for (auto &decl: decls) {
        ret = decl->codeGen();
        if (ret == nullptr) {
            break;
        }
    }
    return ret;
}

llvm::Value *SubscriptExprNode::codeGen() {
    auto V = var->codeGen();
    auto I = index->codeGen();
    if (V == nullptr or I == nullptr) {
        return nullptr;
    }
    auto type1 = var->getType();
    auto type2 = index->getType();
    bool isPtr1 = type1->dim.size()!=0 or type1->pLevel!=0;
    bool isPtr2 = type2->dim.size()!=0 or type2->pLevel!=0;
    bool assign = getAssign();
    if (not isPtr1) {
        return LogError("Non-pointer(array) cannot be indexed.");
    } else if (type2->type==FLOAT and type2->dim.size()==0 and type2->pLevel==0) {
        return LogError("Float cannot be used as index.");
    } else if (isPtr2) {
        return LogError("Pointer cannot be used as index.");
    }
    auto type = std::make_shared<VarType>(*type1);
    if (type->dim.size()!=0) {
        type->dim.pop_front();
    } else {
        type->pLevel -= 1;
    }
    setType(type);
    if (assign) {
        setLeft();
    }
    auto val = builder.CreateSExtOrTrunc(I, llvm::Type::getInt64Ty(context));
    int size;
    if (type1->dim.size() != 0) {
        size = type1->getBaseSize();
    } else {
        size = type->getBaseSize();
    }
    for (int i: type->dim) {
        size *= i;
    }
    val = builder.CreateMul(llvm::ConstantInt::get(context, llvm::APInt(64, size, true)), val);
    val = builder.CreateAdd(V, val);
    if (type->dim.size()!=0 and !assign) {
        return val;
    }
    val = builder.CreateIntToPtr(val, type->toLLVMPtrType());
    if (assign) {
        return val;
    }
    return builder.CreateLoad(type->toLLVMtype(), val);
}

llvm::Value *IfStmtNode::codeGen() {
    llvm::Value *CondV = cond->codeGen();
    if (!CondV) {
        return nullptr;
    }
    // Convert condition to a bool by comparing non-equal to 0.0.
    auto cond_type = cond->getType();
    if (cond_type->type==FLOAT and cond_type->dim.size()==0 and cond_type->pLevel==0) {
        CondV = builder.CreateFCmpONE(
            CondV, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
    } else {
        CondV = builder.CreateICmpNE(
            CondV, llvm::ConstantInt::get(CondV->getType(), 0)
        );
    }
    llvm::Function *TheFunction = builder.GetInsertBlock()->getParent();
    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(context, "then", TheFunction);
    llvm::BasicBlock *ElseBB = nullptr;
    if (otherw != nullptr) {
        ElseBB = llvm::BasicBlock::Create(context, "else", TheFunction);
    }
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(context, "merge", TheFunction);
    if (otherw != nullptr) {
        builder.CreateCondBr(CondV, ThenBB, ElseBB);
    } else {
        builder.CreateCondBr(CondV, ThenBB, MergeBB);
    }
    // Emit then value.
    builder.SetInsertPoint(ThenBB);
    llvm::Value *ThenV = then->codeGen();
    if (!ThenV) {
        return nullptr;
    }
    builder.CreateBr(MergeBB);
    llvm::Value *ElseV;
    if (otherw != nullptr) {
        builder.SetInsertPoint(ElseBB);
        ElseV = otherw->codeGen();
        if (!ElseV) {
            return nullptr;
        }
        builder.CreateBr(MergeBB);
    }
    builder.SetInsertPoint(MergeBB);
    return ThenV;
}

llvm::Value *CallExprNode::codeGen() {
    llvm::Function *function = mod->getFunction(callee);
    if (callee != "scanf" and callee != "printf") {
        if (function == nullptr) {
            return LogError(("No function called "+callee).c_str());
        }
        auto node = func_proto[callee];
        auto types = node->getArgs()->getTypes();
        if (types.size() < args.size()) {
            return LogError("Too many arguments.");
        } else if (types.size() > args.size()) {
            return LogError("Not enough arguments.");
        }
        std::vector<llvm::Value*> arg_values;
        for (int i=0; i<args.size(); i++) {
            auto &arg = args[i];
            llvm::Value *val = arg->codeGen();
            if (val == nullptr) {
                return nullptr;
            }
            auto type1 = types[i];
            auto type2 = arg->getType();
            bool isPtr1 = type1->pLevel!=0 or type1->dim.size()!=0;
            bool isPtr2 = type2->pLevel!=0 or type2->dim.size()!=0;
            if (type2->type==VOID and type2->dim.size()==0 and type2->pLevel==0) {
                return LogError("Void is Invalid.");
            }
            if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
                return LogError("Function argument passing between pointer and non-pointer.");
            }
            if (isPtr1 and isPtr2) {
                if (!type1->match(*type2)) {
                    return LogError("Function argument passing between unmatched pointer.");
                }
            }
            if (!isPtr1) {
                if (type1->type==FLOAT and type2->type!=FLOAT) {
                    val = builder.CreateSIToFP(val, llvm::Type::getDoubleTy(context));
                } else if (type1->type!=FLOAT and type2->type==FLOAT) {
                    val = builder.CreateFPToSI(val, type1->toLLVMtype());
                } else if (type1->type!=type2->type) {
                    val = builder.CreateSExtOrTrunc(val, type1->toLLVMtype());
                }
            }
            arg_values.push_back(val);
        }
        auto type = func_proto[callee]->getRet();
        setType(std::make_shared<VarType>(*type));
        return builder.CreateCall(function, arg_values);
    } else {
        if (args.size() == 0) {
            return LogError(("Not enough arguments for "+callee+".").c_str());
        }
        std::vector<llvm::Value*> arg_values;
        auto ptr_val = args[0]->codeGen();
        if (ptr_val==nullptr) {
            return nullptr;
        }
        auto type = args[0]->getType();
        if (type->dim.size()==0 and type->pLevel==0) {
            return LogError("Function argument passing between pointer and non-pointer.");
        }
        auto first_type = std::make_shared<VarType> (
            CHAR,
            std::deque<int>(), 1
        );
        if (!first_type->match(*type)) {
            return LogError("Function argument passing between unmatched pointer.");
        }
        ptr_val = builder.CreateIntToPtr(ptr_val, llvm::Type::getInt8PtrTy(context));
        arg_values.push_back(ptr_val);
        for (int i=1; i<args.size(); i++) {
            auto &arg = args[i];
            auto val = arg->codeGen();
            if (val == nullptr) {
                return nullptr;
            }
            auto type = arg->getType();
            if (type->type==VOID and type->dim.size()==0 and type->pLevel==0) {
                return LogError("Void is Invalid.");
            }
            arg_values.push_back(val);
        }
        setType(std::make_shared<VarType>(
            INT,
            std::deque<int>{},
            0
        ));
        return builder.CreateCall(function, arg_values);
    }
}

llvm::Value *ForStmtNode::codeGen() {
    llvm::Function *function = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "loopbody", function);
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "cond", function);
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "merge", function);
    llvm::BasicBlock *stepBB = llvm::BasicBlock::Create(context, "step", function);
    auto last_merge = cur_merge;
    auto last_cond = cur_cond;
    cur_merge = mergeBB;
    cur_cond = stepBB;
    named_rec.push();
    if (start != nullptr) {
        if (start->codeGen() == nullptr) {
            return nullptr;
        }
    }
    builder.CreateBr(condBB);
    builder.SetInsertPoint(bodyBB);
    if (body->codeGen() == nullptr) {
        return nullptr;
    }
    builder.CreateBr(stepBB);
    builder.SetInsertPoint(stepBB);
    if (step != nullptr) {
        if (step->codeGen() == nullptr) {
            return nullptr;
        }
    }
    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    llvm::Value *val;
    if (end != nullptr) {
        val = end->codeGen();
        if (val==nullptr) {
            return nullptr;
        }
        auto cond_type = end->getType();
        if (cond_type->type==FLOAT and cond_type->dim.size()==0 and cond_type->pLevel==0) {
            val = builder.CreateFCmpONE(
                val, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
        } else {
            val = builder.CreateICmpNE(
                val, llvm::ConstantInt::get(val->getType(), 0)
            );
        }
    } else {
        val = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 1);
    }
    builder.CreateCondBr(val, bodyBB, mergeBB);
    builder.SetInsertPoint(mergeBB);
    named_rec.pop();
    cur_cond = last_cond;
    cur_merge = last_merge;
    return val;
}

llvm::Value *RetStmtNode::codeGen() {
    auto ret_type = func_proto[cur_fun]->getRet();
    bool isVoid = ret_type->type==VOID and ret_type->dim.size()==0 and ret_type->pLevel==0;
    if (isVoid and expr != nullptr) {
        return LogError("Void function cannot have return value.");
    } else if (not isVoid and expr == nullptr) {
        return LogError("Non-void function cannot return void.");
    } else if (isVoid and expr == nullptr) {
        llvm::Function *function = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context, "afterret", function);
        auto R = builder.CreateRetVoid();
        builder.SetInsertPoint(BB);
        return R;
    }
    auto val = expr->codeGen();
    if (val==nullptr) {
        return nullptr;
    }
    auto type = expr->getType();
    bool isPtr1 = ret_type->pLevel!=0;
    bool isPtr2 = type->pLevel!=0 or type->dim.size()!=0;
    if (isPtr1 and isPtr2) {
        if (!type->match(*ret_type)) {
            return LogError("Return pointer type does not match.");
        }
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        return LogError("Invalid return between pointer and non-pointer.");
    }
    if (!isPtr1) {
        if (ret_type->type==FLOAT and type->type!=FLOAT) {
            val = builder.CreateSIToFP(val, llvm::Type::getDoubleTy(context));
        } else if (ret_type->type!=FLOAT and type->type==FLOAT) {
            val = builder.CreateFPToSI(val, ret_type->toLLVMtype());
        } else if (ret_type->type!=type->type) {
            val = builder.CreateSExtOrTrunc(val, ret_type->toLLVMtype());
        }
    }
    llvm::Function *function = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(context, "afterret", function);
    auto R = builder.CreateRet(val);
    builder.SetInsertPoint(BB);
    return R;
}

llvm::Value *WhileStmtNode::codeGen() {
    llvm::Function *function = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "loopbody", function);
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "cond", function);
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "merge", function);
    auto last_merge = cur_merge;
    auto last_cond = cur_cond;
    cur_merge = mergeBB;
    cur_cond = condBB;
    builder.CreateBr(condBB);
    builder.SetInsertPoint(bodyBB);
    if (body->codeGen() == nullptr) {
        return nullptr;
    }
    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    llvm::Value *val;
    if (end != nullptr) {
        val = end->codeGen();
        if (val==nullptr) {
            return nullptr;
        }
        auto cond_type = end->getType();
        if (cond_type->type==FLOAT and cond_type->dim.size()==0 and cond_type->pLevel==0) {
            val = builder.CreateFCmpONE(
                val, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
        } else {
            val = builder.CreateICmpNE(
                val, llvm::ConstantInt::get(val->getType(), 0)
            );
        }
    } else {
        val = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 1);
    }
    builder.CreateCondBr(val, bodyBB, mergeBB);
    builder.SetInsertPoint(mergeBB);
    cur_cond = last_cond;
    cur_merge = last_merge;
    return val;
}

llvm::Value *BreakStmtNode::codeGen() {
    if (cur_merge != nullptr) {
        llvm::Function *function = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context, "afterbreak", function);
        auto val = builder.CreateBr(cur_merge);
        builder.SetInsertPoint(BB);
        return val;
    } else {
        return LogError("Cannot break without loop.");
    }
}

llvm::Value *ContinueStmtNode::codeGen() {
    if (cur_cond == nullptr) {
        return LogError("Cannot continue without loop.");
    } else {
        llvm::Function *function = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context, "aftercontinue", function);
        auto val = builder.CreateBr(cur_cond);
        builder.SetInsertPoint(BB);
        return val;
    }
}
