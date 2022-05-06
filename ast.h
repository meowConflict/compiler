#pragma once

#include <bits/stdc++.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <json/json.h>

enum VarType {
    INT,
    FLOAT,
    CHAR
};

class ExprNode {
public:
    virtual llvm::Value *codeGen() = 0;
    virtual Json::Value jsonGen() = 0;
    virtual ~ExprNode() = default;
};

class VarExprNode: public ExprNode {
private:
    std::string name;
    VarType type;
    int pLevel;
public:
    VarExprNode(const std::string &_name, VarType _type, int _pLevel)
            : name(_name), type(_type), pLevel(_pLevel) {}
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

class PrototypeNode {
    std::string Name;
    std::vector<std::pair<std::string, VarType>> Args;

public:
    PrototypeNode(const std::string &Name, std::vector<std::pair<std::string, VarType>> &Args)
        : Name(Name), Args(std::move(Args)) {}

    llvm::Function *codeGen();
    const std::string &getName() const { return Name; }
};

class FunctionNode {
    std::unique_ptr<PrototypeNode> Proto;
    std::unique_ptr<ExprNode> Body;

public:
    FunctionNode(std::unique_ptr<PrototypeNode> Proto,
                std::unique_ptr<ExprNode> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}

    llvm::Function *codeGen();
};

