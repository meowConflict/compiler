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

    struct VarType {
        BaseType type;
        std::vector<int> dim;
        VarType(BaseType type, std::vector<int> dim)
                : type(type), dim(std::move(dim)) {}
        VarType(const VarType &rhs): type(rhs.type) ,dim(rhs.dim) {}
        VarType(VarType &&rhs): type(rhs.type), dim(std::move(rhs.dim)) {}
    };

    class ASTNode {
    public:
        virtual llvm::Value *codeGen() = 0;
        virtual Json::Value jsonGen() = 0;
        virtual ~ASTNode() = default;
    };

    class StmtNode: public ASTNode {
    public:
        StmtNode() {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ExprNode: public StmtNode {
        VarType type;
    public:
        ExprNode(VarType &&var): type(var) {}
        // ExprNode(VarType var): type(std::move(var)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        const VarType &getType() { return type; }
    };

    class ArrayInitExpr: public ExprNode {
        std::vector<std::unique_ptr<ExprNode>> items;
    public:
        ArrayInitExpr(std::vector<std::unique_ptr<ExprNode>> items): items(std::move(items)) {}
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
        std::unique_ptr<ExprNode> expr;
    public:
        RetStmtNode(std::unique_ptr<ExprNode> child)
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
        VarType type;
        std::string name;
        std::unique_ptr<ExprNode> initValue;
    public:
        VDeclStmtNode(VarType type, const std::string &name, std::unique_ptr<ExprNode> initValue)
            : type(std::move(type)), name(name), initValue(std::move(initValue)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };
    
    class CompdStmtNode: StmtNode {
        std::vector<std::unique_ptr<StmtNode>> statements;
    public:
        CompdStmtNode() {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class VarExprNode: public ExprNode {
    private:
        std::string name;
    public:
        VarExprNode(VarType type, const std::string &name)
                : ExprNode(std::move(type)), name(name) {}
        VarExprNode(VarType &&type, const std::string &name)
                : ExprNode(std::move(type)), name(name) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    template<typename T>
    class ConstExprNode: public ExprNode {
    protected:
        T value;
        ConstExprNode(T v, const VarType &type): value(v), ExprNode(std::move(type)) {}
    };

    class ConstIntNode: public ConstExprNode<long> {
    public:
        ConstIntNode(long v): ConstExprNode(v, VarType(INT, std::vector<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstFloatNode: public ConstExprNode<double> {
    public:
        ConstFloatNode(double v): ConstExprNode(v, VarType(FLOAT, std::vector<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstCharNode: public ConstExprNode<char> {
    public:
        ConstCharNode(char v): ConstExprNode(v, VarType(CHAR, std::vector<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstStringNode: public ConstExprNode<char*> {
    public:
        ConstStringNode(char* c): ConstExprNode(c, VarType(CHAR, std::vector<int>{0})) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class UnaryExprNode: public ExprNode {
        char op;
        std::unique_ptr<ExprNode> child;
    public:
        UnaryExprNode(char op, std::unique_ptr<ExprNode> child, VarType type): op(op), child(std::move(child)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class SubscriptExprNode: public ExprNode {
        int index;
        std::unique_ptr<ExprNode> child;
    public:
        SubscriptExprNode(int index, std::unique_ptr<ExprNode> child, VarType type)
            : index(index), child(std::move(child)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class BinaryExprNode: public ExprNode {
    private:
        char op;
        std::unique_ptr<ExprNode> lhs, rhs;
    public:
        BinaryExprNode(char op, std::unique_ptr<ExprNode> lhs,
                    std::unique_ptr<ExprNode> rhs, VarType type)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class CallExprAST : public ExprNode {
        std::string callee;
        std::vector<std::pair<std::unique_ptr<ExprNode>, VarType>> args;
    public:
        CallExprAST(const std::string &callee, std::vector<std::pair<std::unique_ptr<ExprNode>, VarType>> args, VarType type)
            : callee(callee), args(std::move(args)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class PrototypeNode: public ASTNode {
        std::string name;
        std::vector<std::pair<std::string, VarType>> args;
    public:
        PrototypeNode(const std::string &name, std::vector<std::pair<std::string, VarType>> args)
            : name(name), args(std::move(args)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        const std::string &getName() const { return name; }
    };

    class FunctionNode: public ASTNode {
        std::unique_ptr<PrototypeNode> proto;
        std::unique_ptr<ExprNode> body;
    public:
        FunctionNode(std::unique_ptr<PrototypeNode> proto,
                    std::unique_ptr<ExprNode> body)
            : proto(std::move(proto)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class IfStmtNode: public StmtNode {
        std::unique_ptr<StmtNode> then, otherw;
        std::unique_ptr<ExprNode> cond;
    public:
        IfStmtNode(std::unique_ptr<ExprNode> cond, std::unique_ptr<StmtNode> then,
                    std::unique_ptr<StmtNode> otherw)
            : cond(std::move(cond)), then(std::move(then)), otherw(std::move(otherw)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ForStmtNode: public StmtNode {
        std::unique_ptr<StmtNode> start, body;
        std::unique_ptr<CompdStmtNode> step;
        std::unique_ptr<ExprNode> end;
    public:
        ForStmtNode(std::unique_ptr<StmtNode> start,
                    std::unique_ptr<ExprNode> end, std::unique_ptr<CompdStmtNode> step,
                    std::unique_ptr<StmtNode> body)
            : start(std::move(start)), end(std::move(end)),
            step(std::move(step)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class WhileStmtNode: public StmtNode {
        std::unique_ptr<ExprNode> end;
        std::unique_ptr<StmtNode> body;
    public:
        WhileStmtNode(std::unique_ptr<ExprNode> end, std::unique_ptr<StmtNode> body)
            : end(std::move(end)), body(std::move(body)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };
}