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
    class CompileError: public std::exception {
        std::string cont;
    public:
        CompileError(std::string cont): cont(cont) {}
        const char*  what() const noexcept override {
            return cont.c_str();
        }
    };

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
        int pLevel;
        std::deque<int> dim;
        VarType(BaseType type, std::deque<int> dim, int pLevel=0)
                : type(type), dim(std::move(dim)), pLevel(pLevel) {}
        VarType(const VarType &rhs): type(rhs.type) ,dim(rhs.dim) {}
        VarType(VarType &&rhs): type(rhs.type), dim(std::move(rhs.dim)) {}
        llvm::Value *codeGen() override;
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
            return true;
        }
        bool operator!=(const VarType &that) const {
            return not (*this==that);
        }
        llvm::Type *toLLVMtype() const;
        bool match(const VarType &that) const {
            if (*this==that) {
                return true;
            }
            // if (type != VOID and that.type != VOID) {
            //     return false;
            // }
            // void**[][] -> int**[][]
            if (dim.size() == that.dim.size()) {
                for (int i=1; i<dim.size(); i++) {
                    if (dim[i]!=that.dim[i]) {
                        return false;
                    }
                }
                return matchBase(that);
            }
            // int*** -> int**[]
            if (type==that.type and pLevel==that.pLevel+1 and dim.size()==0 and that.dim.size() == 1) {
                return true;
            }
            if (type==that.type and pLevel==that.pLevel-1 and dim.size()==1 and that.dim.size() == 0) {
                return true;
            }
            // void* -> int[][]...
            if (dim.size() != 0 and that.dim.size() == 0 and that.pLevel == 0 and that.type == VOID) {
                return true;
            }
            if (that.dim.size() != 0 and dim.size() == 0 and pLevel == 0 and type == VOID) {
                return true;
            }
            // void*** -> int**[]
            if (dim.size()==1 and that.dim.size()==0 and that.pLevel<=pLevel+1 and that.pLevel>0 and that.type==VOID) {
                return true;
            }
            if (that.dim.size()==1 and dim.size()==0 and pLevel<=pLevel+1 and pLevel>0 and type==VOID) {
                return true;
            }
            return false;
        }
        bool matchBase(const VarType &that) const {
            if (type==VOID and pLevel != 0 and pLevel <= that.pLevel) {
                return true;
            }
            if (that.type==VOID and that.pLevel != 0 and that.pLevel <= pLevel) {
                return true;
            }
            return type==that.type and pLevel==that.pLevel;
        }
    };

    class StmtNode: public ASTNode {
    public:
        StmtNode() {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ExprNode: public StmtNode {
        std::shared_ptr<VarType> type;
        bool isLeft;
    public:
        ExprNode(std::shared_ptr<VarType> var, bool isLeft = false): type(std::move(var)), isLeft(isLeft) {
            if (type->dim.size() != 0) {
                isLeft = false;
            }
        }
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        bool left() const { return isLeft; }
        std::shared_ptr<VarType> getType() { return type; }
    };

    class ArrayInitExpr: public ExprNode {
        std::vector<std::shared_ptr<ExprNode>> items;
    public:
        ArrayInitExpr(std::vector<std::shared_ptr<ExprNode>> items, 
            std::shared_ptr<VarType> type): 
                items(std::move(items)), ExprNode(std::move(type)) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
        void add_item(std::shared_ptr<ExprNode> item) {
            items.push_back(std::move(item));
        }
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
            this->type->pLevel = pLevel;
        }
        void addInit(std::shared_ptr<ExprNode> initV) {
            initValue = std::move(initV);
        }
        llvm::Type *getType() const {
            return type;
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
        std::vector<llvm::Type*> getTypes() const {
            std::vector<llvm::Type*> types;
            for (auto &decl: decls) {
                auto type = decl->getType();
                types.push_back(std::move(type));
            }
            return std::move(types);
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
                : ExprNode(std::move(type), true), name(name) {}
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
        ConstIntNode(long v): ConstExprNode(v, std::make_shared<VarType>(INT, std::deque<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstFloatNode: public ConstExprNode<double> {
    public:
        ConstFloatNode(double v): ConstExprNode(v, std::make_shared<VarType>(FLOAT, std::deque<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstCharNode: public ConstExprNode<char> {
    public:
        ConstCharNode(char v): ConstExprNode(v, std::make_shared<VarType>(CHAR, std::deque<int>())) {}
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };

    class ConstStringNode: public ConstExprNode<std::string> {
    public:
        ConstStringNode(const std::string &c): ConstExprNode(c, std::make_shared<VarType>(CHAR, std::deque<int>{})) {}
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
        UnaryExprNode(const char *op, std::shared_ptr<ExprNode> child, 
            std::shared_ptr<VarType> type, bool isLeft=false)
                : op(op), child(std::move(child)), 
                    ExprNode(std::move(type), isLeft) {}
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

    class CallExprNode : public ExprNode {
        std::string callee;
        std::vector<std::shared_ptr<ExprNode>> args;
    public:
        CallExprNode(const std::string &callee, std::vector<std::shared_ptr<ExprNode>> args, std::shared_ptr<VarType> type)
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
                    std::shared_ptr<StmtNode> body)
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

    class ProgramNode: public ASTNode {
        std::vector<std::shared_ptr<ASTNode>> nodes;
    public:
        llvm::Value *codeGen() override;
        Json::Value jsonGen() override;
    };
}