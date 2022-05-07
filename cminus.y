%{
    #include <stdio.h>
    #include "ast.h"

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

%token INTV FLOATV CHARV STRINGV
%token IF ELSE WHILE FOR BREAK CONTINUE RETURN INT FLOAT CHAR VOID
%token ID
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



%%
program       : funcList                     
              ;
funcList      : funcList func                      
              | %empty                                           
              ;
func          : funcProto SEMI                                       
              | funcProto compdStmt                                  
              ;
funcProto     : retType ID LEFTP paramList RIGHTP                    
              ;
retType       : retBaseType                                     
              | pointerType                                     
              ;
retBaseType   : baseType                                      
              | VOID                                            
              ;
baseType      : INT                                           
              | FLOAT                                         
              | CHAR                                          
              ;
pointerType   : pointerType STAR                                  
              | retBaseType STAR                                  
              ;
type          : baseType                                                
              | pointerType                                             
              ;
paramList     : paramList COMMA param                
              | param                                
              ;
param         : type varName                                      
              | %empty                                            
              ;
varName       : ID arrayBrackets        
              ;
arrayBrackets : arrayBrackets LEFTSB INTV RIGHTSB
              | %empty
compdStmt     : LEFTB stmtList RIGHTB                      
              ;
varDec        : type varList SEMI
              ;
varList       : varList COMMA varInit                      
              | varInit                                                    
              ;
varInit       : varName ASSIGN expression
              | varName ASSIGN arrayInitList
              | varName
              ;
arrayInitList : LEFTB expList RIGHTB
              | LEFTB RIGHTB
              ;
expList       : expList COMMA expression
              | expression
              ;
stmtList      : stmtList statement                                          
              | stmtList varDec
              | %empty                                                      
              ;
statement     : expStmt                                                     
              | compdStmt                                                   
              | selStmt                                                     
              | iterStmt                                                    
              | jmpStmt                                                     
              | retStmt                                                     
              ;
expStmt       : expression SEMI                                             
              | SEMI                                                       
              ;
expression    : expression ASSIGN exprOr                                    
              | exprOr                                                      
              ;
exprOr        : exprOr OR exprAnd                                           
              | exprAnd                                                     
              ;
exprAnd       : exprAnd AND exprCmp                                         
              | exprCmp                                                     
              ;
exprCmp       : exprCmp GE exprAdd                                          
              | exprCmp LE exprAdd                                          
              | exprCmp GT exprAdd                                          
              | exprCmp LT exprAdd                                          
              | exprCmp EQUAL exprAdd                                       
              | exprCmp NE exprAdd                                          
              | exprAdd                                                     
              ;
exprAdd       : exprAdd PLUS exprMul                                        
              | exprAdd MINUS exprMul                                       
              | exprMul                                                     
              ;
exprMul       : exprMul STAR exprUnaryOp                  
              | exprMul DIV exprUnaryOp                   
              | exprMul REM exprUnaryOp                   
              | exprUnaryOp                              
exprUnaryOp   : LEFTP expression RIGHTP                  
              | MINUS factor %prec UMINUS                            
              | NOT factor                               
              | STAR factor %prec USTAR                             
              | AMP factor                               
              | factor                                                      
              ;
factor        : var                                                         
              | call                                                        
              | CONF                                                        
              ;
var           : ID subscript  
              ;
subscript     : subscript LEFTSB expression RIGHTSB
              | %empty
call          : ID LEFTP argList RIGHTP                                     
              | ID LEFTP RIGHTP                                             
              ;
argList       : argList COMMA expression                                    
              | expression                                                  
              ;
CONF          : INTV                                                        
              | FLOATV                                                      
              | CHARV                                                       
              | STRINGV                                                     
              ;
selStmt       : IF LEFTP expression RIGHTP statement elseStmt SEMI   
              ;
elseStmt      : ELSE statement                                             
              | %empty                                                     
              ;
iterStmt      : FOR LEFTP expStmt expStmt incStmt RIGHTP statement    
              | WHILE LEFTP expression RIGHTP statement                  
              ;
incStmt       : expression
              | %empty
              ;
jmpStmt       : BREAK SEMI                                                 
              | CONTINUE SEMI                                              
              ;
retStmt       : RETURN SEMI                                              
              | RETURN expression SEMI                                   
              ;
%%
//expression 不能为空，导致for语句最后一项一定要有内容，哪怕是a=a