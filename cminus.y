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
%}

%union {
    lyf::VarType *vtype;
    std::pair<std::string, lyf::VarType> *var;
    std::vector<std::pair<std::string, lyf::VarType>> *varL;
    std::vector<int> *arrBr;
    int iVal;
    double fVal;
    char *sVal;
    char cVal;
}

%token<iVal> INTV 
%token<cVal> FLOATV 
%token<cVal> CHARV 
%token<sVal> STRINGV
%token<sVal> ID
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

%type<vtype> retBaseType retType pointerType baseType type
%type<var> param varName
%type<varL> paramList
%type<arrBr> arrayBrackets

%%
program     : funcList 
            ;
funcList    : funcList func 
            | %empty
            ;
func        : funcProto SEMI 
            | funcProto compdStmt
            ;
funcProto   : retType ID LEFTP paramList RIGHTP  
            ;
retType     : retBaseType
{
    $$ = $1;
}
            | pointerType 
{
    $$ = $1;
}
            ;
retBaseType   : baseType 
{
    $$ = $1;
}
            | VOID 
{
    $$ = new lyf::VarType {
        lyf::VOID,
        std::vector<int>()
    };
}
            ;
baseType    : INT 
{
    $$ = new lyf::VarType {
        lyf::INT,
        std::vector<int>()
    };
}
            | FLOAT 
{
    $$ = new lyf::VarType {
        lyf::FLOAT,
        std::vector<int>()
    };
}
            | CHAR 
{
    $$ = new lyf::VarType {
        lyf::CHAR,
        std::vector<int>()
    };
}
            ;
pointerType : pointerType STAR
{
    $1->dim.push_back(0);
    $$ = $1;
}
            | retBaseType STAR
{
    $1->dim.push_back(0);
    $$ = $1;
}
            ;
type        : baseType
{
    $$ = $1;
}
            | pointerType 
{
    $$ = $1;
}
            ;
paramList   : paramList COMMA param
{
    if ($3 != nullptr) {
        $1->push_back(std::move(*$3));
    }
    $$ = $1;
}
            | param 
{
    $$ = new std::vector<std::pair<std::string, lyf::VarType>>();
    if ($1 != nullptr) {
        $$->push_back(std::move(*$1));
    }
    delete $1;
}
            ;
param       : type varName
{
    /* param: std::pair<string, VarType> */
    $$ = $2;
    auto &[type, dim] = $$->second;
    for (int i=0; i<dim.size(); i++) {
        $1->dim.push_back(dim[i]);
    }
    dim = std::move($1->dim);
    delete $1;
}
            | %empty  
{
    $$ = nullptr;
    // return;
}
            ;
varName     : ID arrayBrackets 
{
    $$ = new std::pair<std::string, lyf::VarType> (std::string($1), lyf::VarType (
        lyf::INT,
        std::move(*$2)
    ));
    delete $2;
}
            ;
arrayBrackets: arrayBrackets LEFTSB INTV RIGHTSB
{
    $$->push_back($3);
}
            | %empty
{
    $$ = new std::vector<int>{};
}
compdStmt   : LEFTB stmtList RIGHTB                      
            ;
varDec      : type varList SEMI
            ;
varList     : varList COMMA varInit                      
            | varInit                                                    
            ;
varInit     : varName ASSIGN expression
            | varName ASSIGN arrayInitList
            | varName
            ;
arrayInitList: LEFTB expList RIGHTB
            | LEFTB RIGHTB
            ;
expList     : expList COMMA expression
            | expression
            ;
stmtList    : stmtList statement
            | %empty
            ;
statement   : expStmt
            | compdStmt                                                   
            | selStmt                                                     
            | iterStmt                                                    
            | jmpStmt                                                     
            | retStmt
            | varDec
            ;
expStmt     : expression SEMI                                             
            | SEMI                                                       
            ;
expression  : expression ASSIGN exprOr                                    
            | exprOr                                                      
            ;
exprOr      : exprOr OR exprAnd                                           
            | exprAnd                                                     
            ;
exprAnd     : exprAnd AND exprCmp
            | exprCmp
            ;
exprCmp     : exprCmp GE exprAdd                                          
            | exprCmp LE exprAdd                                          
            | exprCmp GT exprAdd
            | exprCmp LT exprAdd                                          
            | exprCmp EQUAL exprAdd                                       
            | exprCmp NE exprAdd                                          
            | exprAdd                                                     
            ;
exprAdd     : exprAdd PLUS exprMul                                        
            | exprAdd MINUS exprMul                                       
            | exprMul                                                     
            ;
exprMul     : exprMul STAR exprUnaryOp                  
            | exprMul DIV exprUnaryOp                   
            | exprMul REM exprUnaryOp                   
            | exprUnaryOp                              
exprUnaryOp : MINUS factor %prec UMINUS                            
            | NOT factor                               
            | STAR factor %prec USTAR                             
            | AMP factor 
            | factor                                                      
            ;
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
CONF        : INTV                                                        
            | FLOATV                                                      
            | CHARV                                                       
            | STRINGV                                                     
            ;
selStmt     : IF LEFTP expression RIGHTP compdStmt ELSE compdStmt
            | IF LEFTP expression RIGHTP compdStmt
            | IF LEFTP expression RIGHTP selStmt
            ;
iterStmt    : FOR LEFTP expStmt expStmt stepStmt RIGHTP statement    
            | WHILE LEFTP expression RIGHTP statement                  
            ;
stepStmt    : singleStepStmt
            | %empty
            ;
singleStepStmt: singleStepStmt COMMA expression
            | expression
            ;
jmpStmt     : BREAK SEMI                                                 
            | CONTINUE SEMI                                              
            ;
retStmt     : RETURN SEMI                                              
            | RETURN expression SEMI                                   
            ;
%%
//expression 不能为空，导致for语句最后一项一定要有内容，哪怕是a=a