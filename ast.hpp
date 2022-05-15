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
        BaseType type;
        std::vector<int> dim;
        VarType(BaseType type, std::vector<int> dim)
                : type(type), dim(std::move(dim)) {}
        VarType(const VarType &rhs): type(rhs.type) ,dim(rhs.dim) {}
        VarType(VarType &&rhs): type(rhs.type), dim(std::move(rhs.dim)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class StmtNode: public ASTNode {
    public:
        StmtNode() {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ExprNode: public StmtNode {
        std::shared_ptr<VarType> type;
    public:
        ExprNode(std::shared_ptr<VarType> var): type(std::move(var)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        std::shared_ptr<VarType> getType() { return type; }
    };

    class ArrayInitExpr: public ExprNode {
        std::vector<std::shared_ptr<ExprNode>> items;
    public:
        ArrayInitExpr(std::vector<std::shared_ptr<ExprNode>> items, std::shared_ptr<VarType> type): items(std::move(items)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class BreakStmtNode: public StmtNode {
    public:
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        BreakStmtNode() = default;
    };

    class RetStmtNode: public StmtNode {
        std::shared_ptr<ExprNode> expr;
    public:
        RetStmtNode(std::shared_ptr<ExprNode> child)
            : expr(std::move(child)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ContinueStmtNode: public StmtNode {
    public:
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        ContinueStmtNode() = default;
    };

    class VDeclStmtNode: public StmtNode {
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
            std::vector<int> tmp(pLevel, 0);
            tmp.insert(tmp.end(), this->type->dim.begin(), this->type->dim.end());
            this->type->dim = std::move(tmp);
        }
    };

    class VDecListStmtNode: public StmtNode {
        std::vector<std::shared_ptr<VDeclStmtNode>> decls;
    public:
        VDecListStmtNode(std::vector<std::shared_ptr<VDeclStmtNode>> decls)
            : decls(std::move(decls)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        void mergeType(BaseType type, int pLevel) {
            for (auto &decl: decls) {
                decl->mergeType(type, pLevel);
            }
        }
        void push_dec(std::shared_ptr<VDeclStmtNode> decl) {
            decls.push_back(std::move(decl));
        }
    };
    
    class CompdStmtNode: public StmtNode {
        std::vector<std::shared_ptr<StmtNode>> statements;
    public:
        CompdStmtNode(std::vector<std::shared_ptr<StmtNode>> stmts)
            : statements(std::move(stmts)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        void push_item(std::shared_ptr<StmtNode> item) {
            statements.push_back(std::move(item));
        }
    };

    class VarExprNode: public ExprNode {
    private:
        std::string name;
    public:
        VarExprNode(std::shared_ptr<VarType> type, const std::string &name)
                : ExprNode(std::move(type)), name(name) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    template<typename T>
    class ConstExprNode: public ExprNode {
    protected:
        T value;
        ConstExprNode(T v, std::shared_ptr<VarType> type): value(v), ExprNode(std::move(type)) {}
    };

    class ConstIntNode: public ConstExprNode<long> {
    public:
        ConstIntNode(long v): ConstExprNode(v, std::make_shared<VarType>(INT, std::vector<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstFloatNode: public ConstExprNode<double> {
    public:
        ConstFloatNode(double v): ConstExprNode(v, std::make_shared<VarType>(FLOAT, std::vector<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstCharNode: public ConstExprNode<char> {
    public:
        ConstCharNode(char v): ConstExprNode(v, std::make_shared<VarType>(CHAR, std::vector<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstStringNode: public ConstExprNode<std::string> {
    public:
        ConstStringNode(const std::string &c): ConstExprNode(c, std::make_shared<VarType>(CHAR, std::vector<int>{0})) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class SubscriptExprNode: public ExprNode {
        int index;
        std::shared_ptr<ExprNode> child;
    public:
        SubscriptExprNode(int index, std::shared_ptr<ExprNode> child, std::shared_ptr<VarType> type)
            : index(index), child(std::move(child)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class UnaryExprNode: public ExprNode {
        std::string op;
        std::shared_ptr<ExprNode> child;
    public:
        UnaryExprNode(const char *op, std::shared_ptr<ExprNode> child): op(op), child(std::move(child)), ExprNode(this->child->getType()) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class BinaryExprNode: public ExprNode {
    private:
        std::string op;
        std::shared_ptr<ExprNode> lhs, rhs;
    public:
        BinaryExprNode(const char *op, std::shared_ptr<ExprNode> lhs,
                    std::shared_ptr<ExprNode> rhs, std::shared_ptr<VarType> type)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class CallExprAST : public ExprNode {
        std::string callee;
        std::vector<std::shared_ptr<ExprNode>> args;
    public:
        CallExprAST(const std::string &callee, std::vector<std::shared_ptr<ExprNode>> args, std::shared_ptr<VarType> type)
            : callee(callee), args(std::move(args)), ExprNode(std::move(type)) {}
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
        std::string name;
        std::shared_ptr<VarType> retType;
        std::shared_ptr<VDecListStmtNode> args;
    public:
        PrototypeNode(const std::string &name, std::shared_ptr<VDecListStmtNode> args, std::shared_ptr<VarType> retType)
            : name(name), args(std::move(args)), retType(std::move(retType)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        // void push_arg(std::shared_ptr<VDeclStmtNode> arg) {
        //     args->push_dec(std::move(arg));
        // }
        const std::string &getName() const { return name; }
    };

    class FunctionNode: public ASTNode {
        std::shared_ptr<PrototypeNode> proto;
        std::shared_ptr<StmtNode> body;
    public:
        FunctionNode(std::shared_ptr<PrototypeNode> proto,
                    std::shared_ptr<ExprNode> body)
            : proto(std::move(proto)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class IfStmtNode: public StmtNode {
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
        std::shared_ptr<ExprNode> end;
        std::shared_ptr<StmtNode> body;
    public:
        WhileStmtNode(std::shared_ptr<ExprNode> end, std::shared_ptr<StmtNode> body)
            : end(std::move(end)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };
}