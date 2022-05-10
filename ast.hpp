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
    };
    class CompdStmtNode: StmtNode {
        std::vector<std::unique_ptr<StmtNode>> statements;
    public:
        CompdStmtNode() {}
        llvm::Value *codeGen() override;
    };

    class ExprNode: public StmtNode {
    public:
        llvm::Value *codeGen() override;
        Json::Value jsonGen();
    };

    class VarExprNode: public ExprNode {
    private:
        VarType var;
        std::string name;
    public:
        VarExprNode(VarType var, std::string name)
                : var(std::move(var)), name(name) {}
        llvm::Value *codeGen() override;
    };

    template<typename T>
    class ConstExprNode: public ExprNode {
    protected:
        T value;
        ConstExprNode(T v): value(v) {}
    };

    class ConstIntNode: public ConstExprNode<long> {
    public:
        ConstIntNode(long v): ConstExprNode(v) {}
        llvm::Value *codeGen() override;
    };

    class ConstFloatNode: public ConstExprNode<double> {
    public:
        ConstFloatNode(double v): ConstExprNode(v) {}
        llvm::Value *codeGen() override;
    };

    class ConstCharNode: public ConstExprNode<char> {
    public:
        ConstCharNode(char v): ConstExprNode(v) {}
        llvm::Value *codeGen() override;
    };

    class ConstStringNode: public ConstExprNode<char*> {
    public:
        ConstStringNode(char* c): ConstExprNode(c) {}
        llvm::Value *codeGen() override;
    };
    class UnaryExprNode: public ExprNode {
        char Op;
        std::unique_ptr<ExprNode> child;
    public:
        UnaryExprNode(std::unique_ptr<ExprNode> child): child(std::move(child)) {}
        llvm::Value *codeGen() override;
    };
    class BinaryExprNode: public ExprNode {
    private:
        char Op;
        std::unique_ptr<ExprNode> LHS, RHS;

    public:
        BinaryExprNode(char Op, std::unique_ptr<ExprNode> LHS,
                    std::unique_ptr<ExprNode> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

        llvm::Value *codeGen() override;
    };

    class CallExprAST : public ExprNode {
        std::string Callee;
        std::vector<std::pair<std::unique_ptr<ExprNode>, VarType>> Args;

    public:
        CallExprAST(const std::string &Callee,
                    std::vector<std::pair<std::unique_ptr<ExprNode>, VarType>> &Args)
            : Callee(Callee), Args(std::move(Args)) {}

    llvm::Value *codeGen() override;
    };

    class PrototypeNode: public ASTNode {
        std::string Name;
        std::vector<std::pair<std::string, VarType>> Args;

    public:
        PrototypeNode(const std::string &Name, std::vector<std::pair<std::string, VarType>> &Args)
            : Name(Name), Args(std::move(Args)) {}

        llvm::Value *codeGen() override;
        const std::string &getName() const { return Name; }
    };

    class FunctionNode: public ASTNode {
        std::unique_ptr<PrototypeNode> Proto;
        std::unique_ptr<ExprNode> Body;

    public:
        FunctionNode(std::unique_ptr<PrototypeNode> Proto,
                    std::unique_ptr<ExprNode> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}

        llvm::Value *codeGen() override;
    };

    class IfStmtNode: public StmtNode {
        std::unique_ptr<StmtNode> Then, Else;
        std::unique_ptr<ExprNode> Cond;

    public:
        IfStmtNode(std::unique_ptr<ExprNode> Cond, std::unique_ptr<StmtNode> Then,
                    std::unique_ptr<StmtNode> Else)
            : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
        llvm::Value *codeGen() override;
    };

    class ForStmtNode: public StmtNode {
        std::unique_ptr<StmtNode> Start, Body;
        std::unique_ptr<CompdStmtNode> Step;
        std::unique_ptr<ExprNode> End;

    public:
        ForStmtNode(std::unique_ptr<StmtNode> Start,
                    std::unique_ptr<ExprNode> End, std::unique_ptr<CompdStmtNode> Step,
                    std::unique_ptr<StmtNode> Body)
            : Start(std::move(Start)), End(std::move(End)),
            Step(std::move(Step)), Body(std::move(Body)) {}

        llvm::Value *codeGen() override;
    };
    class WhileStmtNode: public StmtNode {
        std::unique_ptr<ExprNode> End;
        std::unique_ptr<StmtNode> Body;
    public:
        WhileStmtNode(std::unique_ptr<ExprNode> End, std::unique_ptr<StmtNode> Body)
            : End(std::move(End)), Body(std::move(Body)) {}
        llvm::Value *codeGen() override;
    };
}