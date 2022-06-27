#include "ast.hpp"
using namespace lyf;

Json::Value getJson(std::string name)
{
    Json::Value jout;
    jout["name"] = name;
    return jout;
}

Json::Value VarType :: jsonGen()
{
    std::string name;

    //需要在构造函数中添加nodeType属性 表示类名
    if(this->type==INT)
        name += "INT";
    else if (this->type == FLOAT)
        name += "FLOAT";
    else if (this->type == CHAR)
        name += "CHAR";
    else if (this->type == VOID)
        name += "VOID";
    for (int i = 0; i < this->pLevel; i++)
    {
        name = name + '*';
    }
    Json::Value jout;
    jout["name"]=this->nodeType;
    jout["children"].append(getJson(name));
    return jout;
}

//不太会,可能调用的都是它的衍生类，所以不用特意写？
Json::Value StmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    return jout;
}

Json::Value ExprNode :: jsonGen()
{
    Json::Value jout;
    jout["children"].append(this->type->jsonGen());
    jout["name"] = this->nodeType;
    return jout;
}

//待完善
Json::Value ArrayInitExpr :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    for (int i = 0; i < this->items.size(); i++)
    {
        if (this->items[i])
            jout["children"].append(this->items[i]->jsonGen());
    }
    return jout;
}

Json::Value BreakStmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    return jout;
}

Json::Value RetStmtNode :: jsonGen()
{
    Json::Value jout;
    if (this->expr)
    {
        jout["children"].append(this->expr->jsonGen());
    }
    jout["name"] = this->nodeType;
    return jout;
}

Json::Value ContinueStmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    return jout;
}

Json::Value VDeclStmtNode :: jsonGen()
{
    Json::Value jout, jname = getJson(this->name);
    jname["name"] = this->name;
    jout["name"] = this->nodeType;
    if (this->type)
        jout["children"].append(this->type->jsonGen());
    jout["children"].append(jname);
    if (this->initValue)
        jout["children"].append(this->initValue->jsonGen());
    return jout;
}

Json::Value VDecListStmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    for (int i = 0; i < this->decls.size(); i++)
    {
        if (this->decls[i])
            jout["children"].append(this->decls[i]->jsonGen());
    }
    return jout;
}

Json::Value CompdStmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    for (int i = 0; i < this->statements.size(); i++)
    {
        if (this->statements[i])
            jout["children"].append(this->statements[i]->jsonGen());
    }
    return jout;
}

//待完成
Json::Value VarExprNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson(this->name));
    return jout;
}

//实际上没有调用，而是调用它四个子类的jsonGen
// Json::Value ConstExprNode :: jsonGen()
// {
//     Json::Value jout;
//     jout["name"] = this->nodeType;
//     return jout;
// }

Json::Value ConstIntNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson(std::to_string(this->value)));
    return jout;
}

Json::Value ConstFloatNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson(std::to_string(this->value)));
    return jout;
}

Json::Value ConstCharNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson(std::to_string(this->value)));
    return jout;
}

Json::Value ConstStringNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson(this->value));
    return jout;
}

Json::Value ConstNPtrNode::jsonGen() {
    Json::Value jout;
    jout["name"]=nodeType;
    return jout;
}

Json::Value SubscriptExprNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    if (this->var)
        jout["children"].append(this->var->jsonGen());
    if (this->index)
        jout["children"].append(this->index->jsonGen());
    return jout;
}

Json::Value UnaryExprNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    //assert(getType()!=nullptr);
    //jout["children"].append(getJson(std::to_string(getType()->pLevel)));
    jout["children"].append(getJson(this->op));
    jout["children"].append(this->child->jsonGen());
    return jout;
}

Json::Value BinaryExprNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    if (this->lhs)
        jout["children"].append(this->lhs->jsonGen());
    jout["children"].append(getJson(this->op));
    if (this->rhs)
        jout["children"].append(this->rhs->jsonGen());
    return jout;
}

Json::Value CallExprNode ::jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson(this->callee));
    for (int i = 0; i < this->args.size(); i++)
    {
        if (this->args[i])
            jout["children"].append(this->args[i]->jsonGen());
    }
    return jout;
}

Json::Value PrototypeNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson(this->name));
    if (this->retType)
        jout["children"].append(this->retType->jsonGen());
    if(this->args)jout["children"].append(this->args->jsonGen());
    return jout;
}

Json::Value FunctionNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    if (this->proto)
        jout["children"].append(this->proto->jsonGen());
    if (this->body)
        jout["children"].append(this->body->jsonGen());
    return jout;
}

Json::Value IfStmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    if (this->cond){
        jout["children"].append(getJson("if"));
    jout["children"].append(this->cond->jsonGen());}
    if (this->then)
        jout["children"].append(this->then->jsonGen());

    if (this->otherw)
        {jout["children"].append(getJson("else"));

    jout["children"].append(this->otherw->jsonGen());
        }
    return jout;
}

Json::Value ForStmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson("for"));
    if (this->start)
        jout["children"].append(this->start->jsonGen());
    if (this->end)
        jout["children"].append(this->end->jsonGen());
    if (this->step)
    jout["children"].append(this->step->jsonGen());
    if (this->body)
        jout["children"].append(this->body->jsonGen());
    return jout;
}

Json::Value WhileStmtNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    jout["children"].append(getJson("while"));
    if (this->end)
        jout["children"].append(this->end->jsonGen());
    if (this->body)
        jout["children"].append(this->body->jsonGen());
    return jout;
}

Json::Value ProgramNode :: jsonGen()
{
    Json::Value jout;
    jout["name"] = this->nodeType;
    for (int i = 0; i < this->nodes.size(); i++)
    {
        if (this->nodes[i])
            jout["children"].append(this->nodes[i]->jsonGen());
    }
    return jout;
}