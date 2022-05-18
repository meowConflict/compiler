%{
    #include "ast.hpp"

    int yylex(void);
    int yyparse(void);
    int yywrap(void);
    int yyerror(std::string str)
    { 
        fprintf(stderr, "[ERROR] %s\n", str);
        return 0;
    }
    int main() {
        yyparse();
    }
    #define YYSTYPE std::shared_ptr<lyf::ASTNode>
    extern std::string lex_text;
    std::stringstream ss;
%}

// %union {
//     std::shared_ptr<lyf::VarType> vtype;
//     std::shared_ptr<std::pair<std::string, lyf::VarType>> var;
//     std::shared_ptr<std::vector<std::pair<std::string, lyf::VarType>>> varL;
//     std::shared_ptr<std::deque<int>> arrBr;
//     std::shared_ptr<lyf::CompdStmtNode> cpdStmt;
//     int iVal;
//     double fVal;
//     char *sVal;
//     char cVal;
//     std::shared_ptr<lyf::VDeclStmtNode> vDecLs;
//     std::shared_ptr<lyf::ExprNode> expr;
//     std::shared_ptr<lyf::StmtNode> stmt;
//     std::shared_ptr<lyf::ConstExprNode> constExpr;
//     std::shared_ptr<lyf::PrototypeNode> proto;
//     std::shared_ptr<lyf::FunctionNode> funct;
// }

%token INTV 
%token FLOATV 
%token CHARV 
%token STRINGV
%token ID
%token IF ELSE WHILE FOR BREAK CONTINUE RETURN INT FLOAT CHAR VOID
%token ASSIGN PLUS MINUS STAR DIV REM AMP
%token GT LT EQUAL GE LE NE
%token LEFTP RIGHTP LEFTSB RIGHTSB LEFTB RIGHTB
%token AND OR NOT
%token COMMA SEMI

%right ASSIGN
%left  OR
%left  AND
%left  GT LT EQUAL GE LE NE
%left  PLUS MINUS
%left  STAR DIV REM
%right NOT USTAR UMINUS
%left  LEFTP RIGHTP LEFTSB RIGHTSB

// %type<vtype> retBaseType retType pointerType baseType type
// %type<var> param varName
// %type<varL> paramList
// %type<arrBr> arrayBrackets
// %type<cpdStmt> compdStmt singleStepStmt
// %type<vDecLs> varDec varList
// %type<expr> varInit expression exprOr exprAnd exprCmp exprMul exprUnaryOp exprAdd factor var call CONF
// %type<funct> func
// %type<proto> funcProto
// %type<stmt> jmpStmt selStmt statement iterStmt stepStmt retStmt expStmt stmtList

%%
program     : funcList 
            ;
funcList    : funcList func 
            | %empty
            ;
// func: StmtNode
func        : funcProto SEMI 
{
    $$ = std::move($1);
}
            | funcProto compdStmt
{
    $$ = std::make_shared<lyf::FunctionNode>(
        std::static_pointer_cast<lyf::PrototypeNode>($1),
        std::static_pointer_cast<lyf::StmtNode>($2)
    );
}
            ;
// funcProto: PrototypeNode
funcProto   : retType ID LEFTP paramList RIGHTP
{
    $$ = std::make_shared<lyf::PrototypeNode>(
        lex_text,
        std::static_pointer_cast<lyf::VDecListStmtNode>($4),
        std::static_pointer_cast<lyf::VarType>($1)
    );
}
            ;
retType     : retBaseType
{
    $$ = std::move($1);
}
            | pointerType 
{
    $$ = std::move($1);
}
            ;
retBaseType   : baseType 
{
    $$ = std::move($1);
}
            | VOID 
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::VOID,
        std::deque<int>()
    );
}
            ;
baseType    : INT 
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::INT,
        std::deque<int>()
    );
}
            | FLOAT 
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::FLOAT,
        std::deque<int>()
    );
}
            | CHAR 
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::CHAR,
        std::deque<int>()
    );
}
            ;
pointerType : pointerType STAR
{
    auto tmp = std::static_pointer_cast<lyf::VarType>($1);
    tmp->pLevel += 1;
    $$ = std::move(tmp);
}
            | retBaseType STAR
{
    auto tmp = std::static_pointer_cast<lyf::VarType>($1);
    tmp->pLevel += 1;
    $$ = std::move(tmp);
}
            ;
type        : baseType
{
    $$ = std::move($1);
}
            | pointerType 
{
    $$ = std::move($1);
}
            ;
// paramList: VDecListStmtNode
paramList   : paramList COMMA param
{
    auto tmp = std::static_pointer_cast<lyf::VDecListStmtNode>($1);
    if ($3 != nullptr) {
        tmp->push_dec(std::static_pointer_cast<lyf::VDeclStmtNode>($3));
    }
    $$ = std::move(tmp);
}
            | param 
{
    auto tmp = std::make_shared<lyf::VDecListStmtNode>(
        std::vector<std::shared_ptr<lyf::VDeclStmtNode>>()
    );
    if ($1 != nullptr) {
        tmp->push_dec(std::static_pointer_cast<lyf::VDeclStmtNode>($1));
    }
    $$ = tmp;
}
            ;
// param: VDeclStmtNode
param       : type varName
{
    // $$ = std::move($2);
    auto type = std::static_pointer_cast<lyf::VarType>($1);
    auto varName = std::static_pointer_cast<lyf::VDeclStmtNode>($2);
    varName->mergeType(type->type, type->pLevel);
    $$ = std::move(varName);
}
            | %empty  
{
    $$ = nullptr;
    // return;
}
            ;
// varName: VDeclStmtNode
varName     : ID arrayBrackets 
{
    $$ = std::make_shared<lyf::VDeclStmtNode> (
        std::static_pointer_cast<lyf::VarType>($2),
        lex_text,
        nullptr
    );
}
            ;
// arrayBrackets: VarType
arrayBrackets: arrayBrackets LEFTSB INTV RIGHTSB
{
    ss << lex_text;
    int val;
    ss >> val;
    auto tmp = std::static_pointer_cast<lyf::VarType>($1);
    tmp->dim.push_back(val);
    $$ = std::move(tmp);
}
            | %empty
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::INT,
        std::deque<int>()
    );
}
// compdStmt: CompdStmtNode
compdStmt   : LEFTB stmtList RIGHTB                      
{
    $$ = std::move($2);
}
            ;
// varDec: VDecListStmtNode
varDec      : type varList SEMI 
{
    auto tmp = std::static_pointer_cast<lyf::VDecListStmtNode>($2);
    auto type = std::static_pointer_cast<lyf::VarType>($1);
    tmp->mergeType(type->type, type->pLevel);
    $$=std::move(tmp);
}
            ;
// varList: VDecListStmtNode
varList     : varList COMMA varInit 
{
    auto tmp = std::static_pointer_cast<lyf::VDecListStmtNode>($1);
    tmp->push_dec(std::static_pointer_cast<lyf::VDeclStmtNode>($3));
    $$=std::move(tmp);
}
            | varInit                
{
    $$ = std::make_shared<lyf::VDecListStmtNode>(
        std::vector<std::shared_ptr<lyf::VDeclStmtNode>> {
            std::static_pointer_cast<lyf::VDeclStmtNode>($1)
        }
    );
}
            ;
// varInit: VDeclStmtNode
varInit     : varName ASSIGN expression
{
    auto decl = std::static_pointer_cast<lyf::VDeclStmtNode> ($1);
    auto exp = std::static_pointer_cast<lyf::ExprNode>($3);
    decl->addInit(exp);
    $$ = decl;
}
            | varName ASSIGN arrayInitList
{
    auto decl = std::static_pointer_cast<lyf::VDeclStmtNode> ($1);
    auto exp = std::static_pointer_cast<lyf::ExprNode>($3);
    decl->addInit(exp);
    $$ = decl;
}
            | varName
{
    $$ = std::move($1);
}
            ;
// arrayInitList: ArrayInitExpr
arrayInitList: LEFTB expList RIGHTB
{
    $$ = std::move($2);
}
            | LEFTB RIGHTB
{
    $$ = nullptr;
}
            ;
// expList: ArrayInitExpr
expList     : expList COMMA expression
{
    auto tmp = std::static_pointer_cast<lyf::ArrayInitExpr>($1);
    tmp->add_item(std::static_pointer_cast<lyf::ExprNode>($3));
    $$=tmp;
}
            | expression
{
    auto tmp = std::static_pointer_cast<lyf::ExprNode> ($1);
    auto type = std::make_shared<lyf::VarType>(*(tmp->getType()));
    $$ = std::make_shared<lyf::ArrayInitExpr>(
        std::vector<std::shared_ptr<lyf::ExprNode>>{tmp},
        type
    );
}
            ;
// stmtList: CompdStmtNode
stmtList    : stmtList statement
{
    auto tmp = std::static_pointer_cast<lyf::CompdStmtNode>($1);
    if($2!=nullptr) {
        tmp->push_item(std::static_pointer_cast<lyf::StmtNode>($2));
    }
    $$ = std::move(tmp);
}
            | %empty
{
    $$=std::make_shared<lyf::CompdStmtNode>(
        std::vector<std::shared_ptr<lyf::StmtNode>>{}
    );
}
            ;
// statement: StmtNode
statement   : expStmt
{
    $$=std::move($1);
}
            | compdStmt
{
    $$=std::move($1);
}                                                   
            | selStmt
{
    $$=std::move($1);
}                                                     
            | iterStmt
{
    $$=std::move($1);
}                                                    
            | jmpStmt
{
    $$=std::move($1);
}                                                     
            | retStmt
{
    $$=std::move($1);
}
            | varDec
{
    $$ = std::move($1);
}
            ;
expStmt     : expression SEMI
{
    $$=std::move($1);
}                                             
            | SEMI
{
    $$=nullptr;
}                                                       
            ;
expression  : expression ASSIGN exprOr
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    if (isPtr1 and isPtr2) {
        if (!type1->match(*type2)) {
            throw lyf::CompileError("Pointer type does not match.");
        }
    }
    if (!left->left()) {
        throw lyf::CompileError("Right value cannot be assigned!");
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        throw lyf::CompileError("Invalid assignment between pointer and non-pointer.");
    }
    auto type = std::make_shared<lyf::VarType> (*type1);
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "ASSIGN",
        left,
        right,
        std::move(type)
    );
}                                    
            | exprOr
{
    $$ =std::move($1);  
}                                                        
            ;
exprOr      : exprOr OR exprAnd 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "OR",
        left,
        right,
        std::move(type)
    );
}                                        
            | exprAnd
{
    $$ =std::move($1);  
}                                                       
            ;
exprAnd     : exprAnd AND exprCmp
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "AND",
        left,
        right,
        std::move(type)
    );
}
            | exprCmp
{
    $$ =std::move($1);  
}  
            ;
exprCmp     : exprCmp GE exprAdd
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        throw lyf::CompileError("Pointer cannot be compared with non-pointer!");
    }
    if (isPtr1 and isPtr2) {
        if (!type1->match(*type2)) {
            throw lyf::CompileError("Pointer type does not match.");
        }
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "GE",
        left,
        right,
        std::move(type)
    );
}
            | exprCmp LE exprAdd 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        throw lyf::CompileError("Pointer cannot be compared with non-pointer!");
    }
    if (isPtr1 and isPtr2) {
        if (!type1->match(*type2)) {
            throw lyf::CompileError("Pointer type does not match.");
        }
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "LE",
        left,
        right,
        std::move(type)
    );
}
            | exprCmp GT exprAdd
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        throw lyf::CompileError("Pointer cannot be compared with non-pointer!");
    }
    if (isPtr1 and isPtr2) {
        if (!type1->match(*type2)) {
            throw lyf::CompileError("Pointer type does not match.");
        }
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "GT",
        left,
        right,
        std::move(type)
    );
}
            | exprCmp LT exprAdd 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        throw lyf::CompileError("Pointer cannot be compared with non-pointer!");
    }
    if (isPtr1 and isPtr2) {
        if (!type1->match(*type2)) {
            throw lyf::CompileError("Pointer type does not match.");
        }
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "LT",
        left,
        right,
        std::move(type)
    );
}
            | exprCmp EQUAL exprAdd 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        throw lyf::CompileError("Pointer cannot be compared with non-pointer!");
    }
    if (isPtr1 and isPtr2) {
        if (!type1->match(*type2)) {
            throw lyf::CompileError("Pointer type does not match.");
        }
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "EQUAL",
        left,
        right,
        std::move(type)
    );
}
            | exprCmp NE exprAdd 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = type1->type==lyf::VOID and type1->pLevel==0 and type1->dim.size()==0;
    bool isVoid2 = type2->type==lyf::VOID and type2->pLevel==0 and type2->dim.size()==0;
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("Void type is invalid.");
    }
    if ((isPtr1 and !isPtr2) or (isPtr2 and !isPtr1)) {
        throw lyf::CompileError("Pointer cannot be compared with non-pointer!");
    }
    if (isPtr1 and isPtr2) {
        if (!type1->match(*type2)) {
            throw lyf::CompileError("Pointer type does not match.");
        }
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::INT,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "NE",
        left,
        right,
        std::move(type)
    );
}
            | exprAdd
{
    $$ =std::move($1);  
}                                                       
            ;
exprAdd     : exprAdd PLUS exprMul 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = (type1->type==lyf::VOID) and ((type1->pLevel==0) or (type1->pLevel==1 and type1->dim.size()==0));
    bool isVoid2 = (type2->type==lyf::VOID) and ((type2->pLevel==0) or (type2->pLevel==1 and type2->dim.size()==0));
    if (isPtr1 and isPtr2) {
        throw lyf::CompileError("Pointer cannot be added with pointer!");
    }
    if ((isPtr1 and type2->type == lyf::FLOAT) or (isPtr2 and type1->type == lyf::FLOAT)) {
        throw lyf::CompileError("Pointer can't be added with FLOAT!");
    }
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("void or void pointer can't be operated!");
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::CHAR,
        std::deque<int>{}
    );
    if (isPtr1) {
        type->type = type1->type;
        type->dim = type1->dim;
        type->pLevel = type1->pLevel;
    } else if (isPtr2) {
        type->type = type2->type;
        type->dim = type2->dim;
        type->pLevel = type2->pLevel;
    }
    else if (type1->type == lyf::FLOAT or type2->type == lyf::FLOAT) {
        type->type = lyf::FLOAT;
    } else if (type1->type == lyf::INT or type2->type == lyf::INT) {
        type->type = lyf::INT;
    }
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "PLUS",
        left,
        right,
        std::move(type)
    );
}
            | exprAdd MINUS exprMul                              
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    bool isPtr1 = type1->pLevel != 0 or type1->dim.size() != 0;
    bool isPtr2 = type2->pLevel != 0 or type2->dim.size() != 0;
    bool isVoid1 = (type1->type==lyf::VOID) and ((type1->pLevel==0) or (type1->pLevel==1 and type1->dim.size()==0));
    bool isVoid2 = (type2->type==lyf::VOID) and ((type2->pLevel==0) or (type2->pLevel==1 and type2->dim.size()==0));
    if (isPtr2) {
        throw lyf::CompileError("Arrays or pointers can't be subtrahend!");
    }
    if (isPtr1 and type2->type == lyf::FLOAT) {
        throw lyf::CompileError("Subtracting can't be done between pointer and float!");
    }
    if (isVoid1 or isVoid2) {
        throw lyf::CompileError("void or void pointer can't be operated!");
    }
    auto type = std::make_shared<lyf::VarType>(
        lyf::CHAR,
        std::deque<int>{}
    );
    if (isPtr1) {
        type->type = type1->type;
        type->dim = type1->dim;
        type->pLevel = type1->pLevel;
    } else if (type1->type == lyf::FLOAT or type2->type == lyf::FLOAT) {
        type->type = lyf::FLOAT;
    } else if (type1->type == lyf::INT or type2->type == lyf::INT) {
        type->type = lyf::INT;
    }
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "MINUS",
        left,
        right,
        std::move(type)
    );
}
            | exprMul
{
    $$ =std::move($1);  
}                                                       
            ;
exprMul     : exprMul STAR exprUnaryOp 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    if ((type1->pLevel != 0 or type1->dim.size() != 0) or (type2->dim.size() != 0 or type2->pLevel != 0)) {
        throw lyf::CompileError("MUL not allowed on array or pointer!");
    }
    if (type1->type == lyf::CHAR or type2->type == lyf::CHAR) {
        throw lyf::CompileError("MUL not allowed on char!");
    }
    if (type1->type == lyf::VOID or type2->type == lyf::VOID) {
        throw lyf::CompileError("void or void pointer can't be operated!");
    }
    lyf::BaseType bType = lyf::INT;
    if (type1->type == lyf::FLOAT or type2->type == lyf::FLOAT) {
        bType = lyf::FLOAT;
    }
    auto type = std::make_shared<lyf::VarType>(
        bType,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "STAR",
        left,
        right,
        std::move(type)
    );
}
            | exprMul DIV exprUnaryOp 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    if ((type1->pLevel != 0 or type1->dim.size() != 0) or (type2->dim.size() != 0 or type2->pLevel != 0)) {
        throw lyf::CompileError("DIV not allowed on array or pointer!");
    }
    if (type1->type == lyf::CHAR or type2->type == lyf::CHAR) {
        throw lyf::CompileError("DIV not allowed on char!");
    }
    if (type1->type == lyf::VOID or type2->type == lyf::VOID) {
        throw lyf::CompileError("void or void pointer can't be operated!");
    }
    lyf::BaseType bType = lyf::INT;
    if (type1->type == lyf::FLOAT or type2->type == lyf::FLOAT) {
        bType = lyf::FLOAT;
    }
    auto type = std::make_shared<lyf::VarType>(
        bType,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "DIV",
        left,
        right,
        std::move(type)
    );
}
            | exprMul REM exprUnaryOp 
{
    auto left = std::static_pointer_cast<lyf::ExprNode>($1);
    auto right = std::static_pointer_cast<lyf::ExprNode>($3);
    auto type1 = left->getType();
    auto type2 = right->getType();
    if ((type1->pLevel != 0 or type1->dim.size() != 0) or (type2->dim.size() != 0 or type2->pLevel != 0)) {
        throw lyf::CompileError("MOD not allowed on array or pointer!");
    }
    if (type1->type == lyf::CHAR or type2->type == lyf::CHAR) {
        throw lyf::CompileError("MOD not allowed on char!");
    }
    if (type1->type == lyf::VOID or type2->type == lyf::VOID) {
        throw lyf::CompileError("void or void pointer can't be operated!");
    }
    lyf::BaseType bType = lyf::INT;
    if (type1->type == lyf::FLOAT or type2->type == lyf::FLOAT) {
        bType = lyf::FLOAT;
    }
    auto type = std::make_shared<lyf::VarType>(
        bType,
        std::deque<int>{}
    );
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "REM",
        left,
        right,
        std::move(type)
    );
}
            | exprUnaryOp
{
    $$ =std::move($1);  
}                 
// exprUnaryOp: UnaryExprNode
exprUnaryOp : MINUS factor %prec UMINUS
{
    auto operand = std::static_pointer_cast<lyf::ExprNode>($2);
    auto type = operand->getType();
    auto cur_type = std::make_shared<lyf::VarType> (*type);
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "UMINUS", 
        std::move(operand),
        std::move(cur_type)
    );
}                            
            | NOT factor
{
    auto operand = std::static_pointer_cast<lyf::ExprNode>($2);
    auto type = operand->getType();
    auto cur_type = std::make_shared<lyf::VarType> (*type);
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "NOT", 
        std::move(operand),
        std::move(cur_type)
    );
}                               
            | STAR factor %prec USTAR
{
    auto operand = std::static_pointer_cast<lyf::ExprNode>($2);
    auto type = operand->getType();
    auto cur_type = std::make_shared<lyf::VarType> (*type);
    if (type->dim.size()==0) {
        if (type->pLevel==0) {
            throw lyf::CompileError("Non-pointer cannot be dereferenced.");
        } else {
            cur_type->pLevel -= 1;
        }
    } else {
        cur_type->dim.pop_front();
    }
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "USTAR",
        std::move(operand),
        std::move(cur_type),
        true
    );
}                             
            | AMP factor 
{
    auto operand = std::static_pointer_cast<lyf::ExprNode>($2);
    auto type = operand->getType();
    auto cur_type = std::make_shared<lyf::VarType> (*type);
    if (operand->left()) {
        throw lyf::CompileError("Right value cannot be Addressed.");
    }
    cur_type->pLevel += 1;
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "AMP", 
        std::move(operand),
        std::move(cur_type)
    );
}
            | factor
{
    $$ = std::move($1);
}     
factor      : LEFTP expression RIGHTP 
{
    $$=std::move($2);
}
            | var
{
    $$=std::move($1);
}
            | call
{
    $$=std::move($1);
}                                                        
            | CONF 
{
    $$=std::move($1);
}
            ;
// var: ExprNode
var         : var LEFTSB expression RIGHTSB
            | ID
{
    $$ = std::make_shared<lyf::VarExprNode>()
}
            ;
call        : ID LEFTP argList RIGHTP                                     
            | ID LEFTP RIGHTP                                             
            ;
// argList: CallExprNode
argList     : argList COMMA expression
{
    auto tmp = std::static_pointer_cast<lyf::CallExprNode>($1);
    tmp->push_arg(std::static_pointer_cast<lyf::ExprNode>($3));
    $$=std::move(tmp);
}
            | expression                                                  
            ;
// CONF: ConstExprNode
CONF        : INTV
{
    ss << lex_text;
    int val;
    ss >> val;
    $$=std::make_shared<lyf::ConstIntNode>(val);
}                                                    
            | FLOATV
{
    ss << lex_text;
    double val;
    ss >> val;
    $$=std::make_shared<lyf::ConstFloatNode>(val);
}                                                      
            | CHARV   
{
    $$=std::make_shared<lyf::ConstCharNode>(lex_text[1]);
}                                                    
            | STRINGV 
{
    $$=std::make_shared<lyf::ConstStringNode>(
        lex_text.substr(1, lex_text.length()-2)
    );
}                                                    
            ;
selStmt     : IF LEFTP expression RIGHTP compdStmt ELSE compdStmt
{
    $$=std::make_shared<lyf::IfStmtNode>(
        std::static_pointer_cast<lyf::ExprNode>($3), 
        std::static_pointer_cast<lyf::StmtNode>($5),
        std::static_pointer_cast<lyf::StmtNode>($7)
    );
}
            | IF LEFTP expression RIGHTP compdStmt
{
    $$=std::make_shared<lyf::IfStmtNode>(
        std::static_pointer_cast<lyf::ExprNode>($3), 
        std::static_pointer_cast<lyf::StmtNode>($5),
        nullptr
    );
}
            | IF LEFTP expression RIGHTP compdStmt ELSE selStmt
{
    $$=std::make_shared<lyf::IfStmtNode>(
        std::static_pointer_cast<lyf::ExprNode>($3), 
        std::static_pointer_cast<lyf::StmtNode>($5),
        std::static_pointer_cast<lyf::StmtNode>($7)
    );
}
            ;
iterStmt    : FOR LEFTP expStmt expStmt stepStmt RIGHTP statement
{
    $$=std::make_shared<lyf::ForStmtNode>(
        std::static_pointer_cast<lyf::StmtNode>($3),
        std::static_pointer_cast<lyf::ExprNode>($4),
        std::static_pointer_cast<lyf::CompdStmtNode>($5),
        std::static_pointer_cast<lyf::StmtNode>($7)
    );
}    
            | WHILE LEFTP expression RIGHTP statement
{
    $$=std::make_shared<lyf::WhileStmtNode>(
        std::static_pointer_cast<lyf::ExprNode>($3), 
        std::static_pointer_cast<lyf::StmtNode>($5)
    );
}                  
            ;
// stepStmt: CompdStmtNode
stepStmt    : singleStepStmt
{
    $$=std::move($1);
}
            | %empty
{
    $$=nullptr;
}
            ;
// singleStepStmt: CompdStmtNode
singleStepStmt: singleStepStmt COMMA expression
{
    auto tmp = std::static_pointer_cast<lyf::CompdStmtNode>($1);
    tmp->push_item(std::static_pointer_cast<lyf::ExprNode>($3));
    $$ = std::move(tmp);
}
            | expression
{
    $$ = std::make_shared<lyf::CompdStmtNode>(
        std::vector<std::shared_ptr<lyf::StmtNode>>{std::static_pointer_cast<lyf::StmtNode>($1)}
    );
}
            ;
jmpStmt     : BREAK SEMI   
{
    $$=std::make_shared<lyf::BreakStmtNode>();
}                                              
            | CONTINUE SEMI  
{
    $$=std::make_shared<lyf::ContinueStmtNode>();
}                                            
            ;
retStmt     : RETURN SEMI
{
    $$ = std::make_shared<lyf::RetStmtNode>(nullptr);
}                                              
            | RETURN expression SEMI
{
    $$ = std::make_shared<lyf::RetStmtNode>(
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}           
%%
//expression 不能为空，导致for语句最后一项一定要有内容，哪怕是a=a