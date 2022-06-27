#pragma once

#include <bits/stdc++.h>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <json/json.h>

namespace lyf {
    enum BaseType {
        INT,
        FLOAT,
        CHAR,
        VOID
    };

    class ASTNode {
    public:
        virtual llvm::Value *codeGen() = 0;
        virtual Json::Value jsonGen() = 0;
        virtual ~ASTNode() = default;
    };

    struct VarType: public ASTNode {
        static constexpr const char* nodeType="Vartype";
        BaseType type;
        int pLevel;
        std::deque<int> dim;
        VarType(BaseType type, std::deque<int> dim, int pLevel=0)
                : type(type), dim(std::move(dim)), pLevel(pLevel) {}
        VarType(const VarType &rhs): type(rhs.type), dim(rhs.dim), pLevel(rhs.pLevel) {}
        VarType(VarType &&rhs): type(rhs.type), dim(std::move(rhs.dim)) {}
        llvm::Value *codeGen() override {
            return nullptr;
        }
        Json::Value jsonGen() override;
        bool operator==(const VarType &that) const {
            bool ret = type == that.type and pLevel == that.pLevel and dim.size() == that.dim.size();
            if (ret) {
                for (int i=1; i<dim.size(); i+=1) {
                    if (dim[i]!=that.dim[i]) {
                        return false;
                    }
                }
            }
            return ret;
        }
        bool isVoid() const {
            return type==VOID and pLevel==0;
        }
        bool operator!=(const VarType &that) const {
            return not (*this==that);
        }
        auto toUnwrapped() const {
            auto n_type = std::make_shared<VarType>(
                type,
                std::deque<int>(),
                pLevel
            );
            return n_type;
        }
        int getBaseSize() const {
            if (pLevel != 0) {
                return 8;
            }
            switch(type) {
                case INT: return 4;
                case FLOAT: return 8;
                case CHAR: return 1;
            }
            return 0;
        }
        llvm::Type *toLLVMtype() const;
        llvm::Type *toLLVMPtrType() const;
        llvm::Type *toLLVMArgType() const;
        bool match(const VarType &that) const {
            if (*this==that) {
                return true;
            }
            // if (type != VOID and that.type != VOID) {
            //     return false;
            // }
            // int*** -> int**[]
            if (type==that.type and pLevel==that.pLevel+1 and dim.size()==0 and that.dim.size() == 1) {
                return true;
            }
            if (type==that.type and pLevel==that.pLevel-1 and dim.size()==1 and that.dim.size() == 0) {
                return true;
            }
            // void* -> int[][]/int***...
            if ((dim.size() != 0 or pLevel != 0) and that.dim.size() == 0 and that.pLevel == 1 and that.type == VOID) {
                return true;
            }
            if ((that.dim.size() != 0 or that.pLevel != 0) and dim.size() == 0 and pLevel == 1 and type == VOID) {
                return true;
            }
            return false;
        }
    };

    struct VarRecord {
        std::shared_ptr<VarType> type;
        llvm::Value *val;
    };

    class StmtNode: public ASTNode {
        static constexpr const char* nodeType="StmtNode";
    public:
        StmtNode() {}
        llvm::Value *codeGen() override {
            return nullptr;
        }
        Json::Value jsonGen() override;
    };

    class ExprNode: public StmtNode {
        static constexpr const char* nodeType="ExprNode";
        std::shared_ptr<VarType> type;
        bool isLeft;
        bool assign;
    protected:
        void setType(std::shared_ptr<VarType> type) {
            this->type = type;
            if (type->dim.size() != 0) {
                isLeft = false;
            }
        }
        void setLeft() {
            isLeft = type->dim.size()==0;
        }
        bool getAssign() const { return assign; }
    public:
        virtual bool isArray() const {
            return false;
        }
        void setAssign() {
            this->assign = true;
        }
        // ExprNode(std::shared_ptr<VarType> var, bool isLeft = false): type(std::move(var)), isLeft(isLeft) {
        //     if (type->dim.size() != 0) {
        //         isLeft = false;
        //     }
        // }
        ExprNode(): type(nullptr), isLeft(false), assign(false) {}
        llvm::Value *codeGen() override {
            return nullptr;
        }
        Json::Value jsonGen() override;
        bool left() const { 
            if (isLeft) {
                assert(type->dim.size() == 0);
            }
            return isLeft;
        }
        std::shared_ptr<VarType> getType() const { return type; }
    };

    struct ArrayInitExpr: public ExprNode {
        static constexpr const char* nodeType="ArrayInitExpr";
        std::vector<std::shared_ptr<ExprNode>> items;
        bool isArray() const override {
            return true;
        }
        ArrayInitExpr(std::vector<std::shared_ptr<ExprNode>> items)
            : items(std::move(items)) {}
        llvm::Value *codeGen() override {
            return nullptr;
        }
        Json::Value jsonGen() override;
        void add_item(std::shared_ptr<ExprNode> item) {
            items.push_back(std::move(item));
        }
        int getSize() const {
            return items.size();
        }
    };

    class BreakStmtNode: public StmtNode {
        static constexpr const char* nodeType="BreakStmtNode";
    public:
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        BreakStmtNode() = default;
    };

    class RetStmtNode: public StmtNode {
        static constexpr const char* nodeType="RetStmtNode";
        std::shared_ptr<ExprNode> expr;
    public:
        RetStmtNode(std::shared_ptr<ExprNode> child)
            : expr(std::move(child)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ContinueStmtNode: public StmtNode {
        static constexpr const char* nodeType="ContinueStmtNode";
    public:
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        ContinueStmtNode() = default;
    };

    class VDeclStmtNode: public StmtNode {
        static constexpr const char* nodeType="VDeclStmtNode";
        std::shared_ptr<VarType> type;
        std::string name;
        std::shared_ptr<ExprNode> initValue;
    public:
        VDeclStmtNode(std::shared_ptr<VarType> type, const std::string &name, std::shared_ptr<ExprNode> initValue)
            : type(std::move(type)), name(name), initValue(std::move(initValue)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        void mergeType(BaseType type, int pLevel) {
            this->type->type = type;
            this->type->pLevel = pLevel;
        }
        void addInit(std::shared_ptr<ExprNode> initV) {
            initValue = std::move(initV);
        }
        std::shared_ptr<VarType> getType() const {
            return type;
        }
        std::string getName() const {
            return name;
        }
        llvm::Value *argCodeGen(llvm::Value *val);
    };

    class VDecListStmtNode: public StmtNode {
        static constexpr const char* nodeType="VDecListStmtNode";
        std::vector<std::shared_ptr<VDeclStmtNode>> decls;
    public:
        VDecListStmtNode(std::vector<std::shared_ptr<VDeclStmtNode>> decls)
            : decls(std::move(decls)) {}
        llvm::Value *codeGen() override;
        llvm::Value *codeGen(llvm::Function::arg_iterator &arg_it);
        Json::Value jsonGen() override;
        void mergeType(BaseType type, int pLevel) {
            for (auto &decl: decls) {
                decl->mergeType(type, pLevel);
            }
        }
        std::vector<std::string> getNames() const {
            std::vector<std::string> names;
            for (auto &decl: decls) {
                names.push_back(decl->getName());
            }
            return std::move(names);
        }
        int getSize() const {
            return decls.size();
        }
        std::vector<std::shared_ptr<VarType>> getTypes() const {
            std::vector<std::shared_ptr<VarType>> types;
            for (auto &decl: decls) {
                types.push_back(decl->getType());
            }
            return std::move(types);
        }
        void push_dec(std::shared_ptr<VDeclStmtNode> decl) {
            decls.push_back(std::move(decl));
        }
    };
    
    class CompdStmtNode: public StmtNode {
        static constexpr const char* nodeType="CompdStmtNode";
        std::vector<std::shared_ptr<StmtNode>> statements;
        bool isFunc;
    public:
        CompdStmtNode(std::vector<std::shared_ptr<StmtNode>> stmts)
            : statements(std::move(stmts)), isFunc(false) {}
        void setFunc() {
            isFunc = true;
        }
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        void push_item(std::shared_ptr<StmtNode> item) {
            statements.push_back(std::move(item));
        }
    };

    class VarExprNode: public ExprNode {
        static constexpr const char* nodeType="VarExprNode";
    private:
        std::string name;
    public:
        VarExprNode(const std::string &name): name(name) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    template<typename T>
    class ConstExprNode: public ExprNode {
    protected:
        T value;
        ConstExprNode(T v): value(v) {}
    };

    class ConstIntNode: public ConstExprNode<long> {
        static constexpr const char* nodeType="ConstIntNode";
    public:
        ConstIntNode(long v): ConstExprNode(v) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstFloatNode: public ConstExprNode<double> {
        static constexpr const char* nodeType="ConstFloatNode";
    public:
        ConstFloatNode(double v): ConstExprNode(v) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstCharNode: public ConstExprNode<char> {
        static constexpr const char* nodeType="ConstCharNode";
    public:
        ConstCharNode(char v): ConstExprNode(v) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstStringNode: public ConstExprNode<std::string> {
        static constexpr const char* nodeType="ConstStringNode";
    public:
        ConstStringNode(const std::string &c): ConstExprNode(c) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstNPtrNode: public ConstExprNode<void*> {
        static constexpr const char* nodeType="ConstNPtrNode";
    public:
        ConstNPtrNode(): ConstExprNode(nullptr) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class SubscriptExprNode: public ExprNode {
        static constexpr const char* nodeType="SubscriptExprNode";
        std::shared_ptr<ExprNode> var, index;
    public:
        SubscriptExprNode(std::shared_ptr<ExprNode> var, std::shared_ptr<ExprNode> index)
            : var(std::move(var)), index(std::move(index)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class UnaryExprNode: public ExprNode {
        static constexpr const char* nodeType="UnaryExprNode";
        std::string op;
        std::shared_ptr<ExprNode> child;
    public:
        UnaryExprNode(const char *op, std::shared_ptr<ExprNode> child)
                : op(op), child(std::move(child)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class BinaryExprNode: public ExprNode {
        static constexpr const char* nodeType="BinaryExprNode";
        std::string op;
        std::shared_ptr<ExprNode> lhs, rhs;
    public:
        BinaryExprNode(const char *op, std::shared_ptr<ExprNode> lhs,
                    std::shared_ptr<ExprNode> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class CallExprNode : public ExprNode {
        static constexpr const char* nodeType="CallExprNode";
        std::string callee;
        std::vector<std::shared_ptr<ExprNode>> args;
    public:
        CallExprNode(const std::string &callee, std::vector<std::shared_ptr<ExprNode>> args)
            : callee(callee), args(std::move(args)) {}
        void setCallee(const std::string &callee) {
            this->callee = callee;
        }
        void push_arg(std::shared_ptr<ExprNode> arg) {
            args.push_back(std::move(arg));
        }
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class PrototypeNode: public ASTNode {
        static constexpr const char* nodeType="PrototypeNode";
        std::string name;
        std::shared_ptr<VarType> retType;
        std::shared_ptr<VDecListStmtNode> args;
    public:
        PrototypeNode(const std::string &name, std::shared_ptr<VDecListStmtNode> args, std::shared_ptr<VarType> retType)
            : name(name), args(std::move(args)), retType(std::move(retType)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        std::shared_ptr<VarType> getRet() const {
            return retType;
        }
        // void push_arg(std::shared_ptr<VDeclStmtNode> arg) {
        //     args->push_dec(std::move(arg));
        // }
        auto getArgs() const {
            return args;
        }
        const std::string &getName() const { return name; }
    };

    class FunctionNode: public ASTNode {
        static constexpr const char* nodeType="FunctionNode";
        std::shared_ptr<PrototypeNode> proto;
        std::shared_ptr<StmtNode> body;
    public:
        FunctionNode(std::shared_ptr<PrototypeNode> proto,
                    std::shared_ptr<StmtNode> body)
            : proto(std::move(proto)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class IfStmtNode: public StmtNode {
        static constexpr const char* nodeType="IfStmtNode";
        std::shared_ptr<StmtNode> then, otherw;
        std::shared_ptr<ExprNode> cond;
    public:
        IfStmtNode(std::shared_ptr<ExprNode> cond, std::shared_ptr<StmtNode> then,
                    std::shared_ptr<StmtNode> otherw)
            : cond(std::move(cond)), then(std::move(then)), otherw(std::move(otherw)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ForStmtNode: public StmtNode {
        static constexpr const char* nodeType="ForStmtNode";
        std::shared_ptr<StmtNode> start, body;
        std::shared_ptr<CompdStmtNode> step;
        std::shared_ptr<ExprNode> end;
    public:
        ForStmtNode(std::shared_ptr<StmtNode> start,
                    std::shared_ptr<ExprNode> end, std::shared_ptr<CompdStmtNode> step,
                    std::shared_ptr<StmtNode> body)
            : start(std::move(start)), end(std::move(end)),
            step(std::move(step)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class WhileStmtNode: public StmtNode {
        static constexpr const char* nodeType="WhileStmtNode";
        std::shared_ptr<ExprNode> end;
        std::shared_ptr<StmtNode> body;
    public:
        WhileStmtNode(std::shared_ptr<ExprNode> end, std::shared_ptr<StmtNode> body)
            : end(std::move(end)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ProgramNode: public ASTNode {
        static constexpr const char* nodeType="ProgramNode";
        std::vector<std::shared_ptr<ASTNode>> nodes;
    public:
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        void push_node(std::shared_ptr<ASTNode> node) {
            nodes.push_back(node);
        }
    };

    void CreateScanf();
    void CreatePrintf();
}