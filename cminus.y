%{
    #include <stdio.h>
    #include "ast.hpp"

    int yylex(void);
    int yyparse(void);
    int yywrap(void);
    void yyerror(const char *s)
    {
        printf("ERROR: %s\n", s);
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
//     std::shared_ptr<std::vector<int>> arrBr;
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
        std::static_pointer_cast<lyf::VarType>($1);
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
        std::vector<int>()
    );
}
            ;
baseType    : INT 
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::INT,
        std::vector<int>()
    );
}
            | FLOAT 
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::FLOAT,
        std::vector<int>()
    );
}
            | CHAR 
{
    $$ = std::make_shared<lyf::VarType> (
        lyf::CHAR,
        std::vector<int>()
    );
}
            ;
pointerType : pointerType STAR
{
    auto tmp = std::static_pointer_cast<lyf::VarType>($1);
    tmp->dim.push_back(0);
    $$ = std::move(tmp);
}
            | retBaseType STAR
{
    auto tmp = std::static_pointer_cast<lyf::VarType>($1);
    tmp->dim.push_back(0);
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
    varName->mergeType(type->type, type->dim.size());
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
        std::vector<int>()
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
    tmp->mergeType(type->type, type->dim.size());
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
    
}
            | varName ASSIGN arrayInitList
            | varName
            ;
arrayInitList: LEFTB expList RIGHTB
            | LEFTB RIGHTB
            ;
expList     : expList COMMA expression
            | expression
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
            | exprOr
{
    $$ =std::move($1);  
}                                                        
            ;
exprOr      : exprOr OR exprAnd                                         
            | exprAnd
{
    $$ =std::move($1);  
}                                                       
            ;
exprAnd     : exprAnd AND exprCmp
            | exprCmp
{
    $$ =std::move($1);  
}  
            ;
exprCmp     : exprCmp GE exprAdd                                          
            | exprCmp LE exprAdd                                          
            | exprCmp GT exprAdd
            | exprCmp LT exprAdd                                          
            | exprCmp EQUAL exprAdd                                       
            | exprCmp NE exprAdd                                          
            | exprAdd
{
    $$ =std::move($1);  
}                                                       
            ;
exprAdd     : exprAdd PLUS exprMul                                    
            | exprAdd MINUS exprMul                                       
            | exprMul
{
    $$ =std::move($1);  
}                                                       
            ;
exprMul     : exprMul STAR exprUnaryOp                  
            | exprMul DIV exprUnaryOp                   
            | exprMul REM exprUnaryOp 

            | exprUnaryOp
{
    $$ =std::move($1);  
}                 
// exprUnaryOp: UnaryExprNode
exprUnaryOp : MINUS factor %prec UMINUS
{
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "UMINUS", 
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}                            
            | NOT factor
{
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "NOT", 
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}                               
            | STAR factor %prec USTAR
{
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "USTAR",
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}                             
            | AMP factor 
{
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "AMP", 
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}
            | factor
{
    $$ = std::move($1);
}     
factor      : LEFTP expression RIGHTP                  
            | var                                                         
            | call                                                        
            | CONF                                                        
            ;
var         : var LEFTSB expression RIGHTSB
            | ID
            ;
call        : ID LEFTP argList RIGHTP                                     
            | ID LEFTP RIGHTP                                             
            ;
argList     : argList COMMA expression                                    
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