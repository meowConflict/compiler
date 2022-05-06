%{
    #include <stdio.h>
    #include "ast.h"

    Node *ROOT;

    int yylex(void);
    int yyparse(void);
    int yywrap(void);
    void yyerror(const char *s)
    {
        printf("ERROR: %s\n", s);
    }
%}


%union
{
    Node* node;
}

%token <node> INTV FLOATV CHARV STRINGV
%token <node> IF ELSE ELSEIF WHILE FOR BREAK CONTINUE RETURN INT FLOAT CHAR VOID
%token <node> ID
%token <node> ASSIGN PLUS MINUS STAR DIV REM AMP
%token <node> GT LT EQUAL GE LE NE
%token <node> LEFTP RIGHTP LEFTSB RIGHTSB LEFTB RIGHTB
%token <node> AND OR NOT
%token <node> COMMA SEMI

%right ASSIGN
%left  OR
%left  AND
%left  GT LT EQUAL GE LE NE
%left  PLUS MINUS
%left  STAR DIV REM
%right NOT
%left  LEFTP RIGHTP LEFTSB RIGHTSB

%type <node> program funcList func funcProto paramList param varDec varList varName;
%type <node> type baseType retBaseType pointerType retType
%type <node> stmtList statement expStmt compdStmt selStmt eiList elseStmt iterStmt jmpStmt retStmt
%type <node> expression exprOr exprAnd exprCmp exprAdd exprMul exprUnaryOp factor var call argList CONF


%%
program       : funcList                                                    {$$ = new Node("", "program", 1, $1); ROOT = $$;}
              ;
funcList      : funcList func                                               {$$ = new Node("", "funcList", 2, $1, $2);}
              | %empty                                                      {$$ = NULL;}
              ;
func          : funcProto SEMI                                              {$$ = new Node("", "func", 2, $1, $2);}
              | funcProto compdStmt                                         {$$ = new Node("", "func", 2, $1, $2);}
              ;
funcProto     : retType ID LEFTP paramList RIGHTP                           {$$ = new Node("", "funcProto", 5, $1, $2, $3, $4, $5);}
              ;
retType       : retBaseType                                                 {$$ = new Node("", "retType", 1, $1);}
              | pointerType                                                 {$$ = new Node("", "retType", 1, $1);}
              ;
retBaseType   : baseType                                                    {$$ = new Node("", "retBaseType", 1, $1);}
              | VOID                                                        {$$ = new Node("", "retBaseType", 1, $1);}
              ;
baseType      : INT                                                         {$$ = new Node("", "baseType", 1, $1);}
              | FLOAT                                                       {$$ = new Node("", "baseType", 1, $1);}
              | CHAR                                                        {$$ = new Node("", "baseType", 1, $1);}
              ;
pointerType   : pointerType STAR                                            {$$ = new Node("", "pointerType", 2, $1, $2);}
              | retBaseType STAR                                            {$$ = new Node("", "pointerType", 2, $1, $2);}
              ;
type          : baseType                                                    {$$ = new Node("", "type", 1, $1);}
              | pointerType                                                 {$$ = new Node("", "type", 1, $1);}
              ;
paramList     : paramList COMMA param                                       {$$ = new Node("", "paramList", 3, $1, $2, $3);}
              | param                                                       {$$ = new Node("", "paramList", 1, $1);}
              ;
param         : type varName                                                {$$ = new Node("", "param", 2, $1, $2);}
              | %empty                                                      {$$ = NULL;}
              ;
varName       : ID arrayBrackets                  {$$ = new Node("", "varName", 7, $1, $2, $3, $4, $5, $6, $7);}
              ;
arrayBrackets : arrayBrackets LEFTSB INTV RIGHTSB
              | %empty
compdStmt     : LEFTB stmtList RIGHTB                            {$$ = new Node("", "compdStmt", 4, $1, $2, $3, $4);}
              ;
varDec        : type varList SEMI
              ;
varList       : varList COMMA varInit                                       {$$ = new Node("", "varList", 3, $1, $2, $3);}
              | varInit                                                     {$$ = new Node("", "varList", 1, $1);}
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
stmtList      : stmtList statement                                          {$$ = new Node("", "stmtList", 2, $1, $2);}
              | stmtList varDec
              | %empty                                                      {$$ = NULL;}
              ;
statement     : expStmt                                                     {$$ = new Node("", "statement", 1, $1);}
              | compdStmt                                                   {$$ = new Node("", "statement", 1, $1);}
              | selStmt                                                     {$$ = new Node("", "statement", 1, $1);}
              | iterStmt                                                    {$$ = new Node("", "statement", 1, $1);}
              | jmpStmt                                                     {$$ = new Node("", "statement", 1, $1);}
              | retStmt                                                     {$$ = new Node("", "statement", 1, $1);}
              ;
expStmt       : expression SEMI                                             {$$ = new Node("", "expStmt", 2, $1, $2);}
              | SEMI                                                        {$$ = new Node("", "expStmt", 1, $1);}
              ;
expression    : expression ASSIGN exprOr                                    {$$ = new Node("", "expression", 3, $1, $2, $3);}
              | exprOr                                                      {$$ = new Node("", "expression", 1, $1);}
              ;
exprOr        : exprOr OR exprAnd                                           {$$ = new Node("", "exprOr", 3, $1, $2, $3);}
              | exprAnd                                                     {$$ = new Node("", "exprOr", 1, $1);}
              ;
exprAnd       : exprAnd AND exprCmp                                         {$$ = new Node("", "exprAnd", 3, $1, $2, $3);}
              | exprCmp                                                     {$$ = new Node("", "exprAnd", 1, $1);}
              ;
exprCmp       : exprCmp GE exprAdd                                          {$$ = new Node("", "exprCmp", 3, $1, $2, $3);}
              | exprCmp LE exprAdd                                          {$$ = new Node("", "exprCmp", 3, $1, $2, $3);}
              | exprCmp GT exprAdd                                          {$$ = new Node("", "exprCmp", 3, $1, $2, $3);}
              | exprCmp LT exprAdd                                          {$$ = new Node("", "exprCmp", 3, $1, $2, $3);}
              | exprCmp EQUAL exprAdd                                       {$$ = new Node("", "exprCmp", 3, $1, $2, $3);}
              | exprCmp NE exprAdd                                          {$$ = new Node("", "exprCmp", 3, $1, $2, $3);}
              | exprAdd                                                     {$$ = new Node("", "exprCmp", 1, $1);}
              ;
exprAdd       : exprAdd PLUS exprMul                                        {$$ = new Node("", "exprAdd", 3, $1, $2, $3);}
              | exprAdd MINUS exprMul                                       {$$ = new Node("", "exprAdd", 3, $1, $2, $3);}
              | exprMul                                                     {$$ = new Node("", "exprAdd", 1, $1);}
              ;
exprMul       : exprMul STAR exprUnaryOp                                    {$$ = new Node("", "exprMul", 3, $1, $2, $3);}
              | exprMul DIV exprUnaryOp                                     {$$ = new Node("", "exprMul", 3, $1, $2, $3);}
              | exprMul REM exprUnaryOp                                     {$$ = new Node("", "exprMul", 3, $1, $2, $3);}
              | exprUnaryOp                                                 {$$ = new Node("", "exprMul", 1, $1);}
exprUnaryOp   : LEFTP expression RIGHTP                                     {$$ = new Node("", "exprUnaryOp", 3, $1, $2, $3);}
              | MINUS factor                                                {$$ = new Node("", "exprUnaryOp", 2, $1, $2);}
              | NOT factor                                                  {$$ = new Node("", "exprUnaryOp", 2, $1, $2);}
              | STAR factor                                                 {$$ = new Node("", "exprUnaryOp", 2, $1, $2);}
              | AMP factor                                                  {$$ = new Node("", "exprUnaryOp", 2, $1, $2);}
              | factor                                                      {$$ = new Node("", "exprUnaryOp", 1, $1);}
              ;
factor        : var                                                         {$$ = new Node("", "factor", 1, $1);}
              | call                                                        {$$ = new Node("", "factor", 1, $1);}
              | CONF                                                        {$$ = new Node("", "factor", 1, $1);}
              ;
var           : ID                                                          {$$ = new Node("", "var", 1, $1);}
              | ID LEFTSB expression RIGHTSB                                {$$ = new Node("", "var", 4, $1, $2, $3, $4);}
              | ID LEFTSB expression RIGHTSB LEFTSB expression RIGHTSB      {$$ = new Node("", "var", 7, $1, $2, $3, $4, $5, $6, $7);}
              ;
call          : ID LEFTP argList RIGHTP                                     {$$ = new Node("", "call", 4, $1, $2, $3, $4);}
              | ID LEFTP RIGHTP                                             {$$ = new Node("", "call", 3, $1, $2, $3);}
              ;
argList       : argList COMMA expression                                    {$$ = new Node("", "argList", 3, $1, $2, $3);}
              | expression                                                  {$$ = new Node("", "argList", 1, $1);}
              ;
CONF          : INTV                                                        {$$ = new Node("", "CONF", 1, $1);}
              | FLOATV                                                      {$$ = new Node("", "CONF", 1, $1);}
              | CHARV                                                       {$$ = new Node("", "CONF", 1, $1);}
              | STRINGV                                                     {$$ = new Node("", "CONF", 1, $1);}
              ;
selStmt       : IF LEFTP expression RIGHTP statement eiList elseStmt SEMI   {$$ = new Node("", "selStmt", 8, $1, $2, $3, $4, $5, $6, $7, $8);}
              ;
eiList        : eiList ELSEIF LEFTP expression RIGHTP statement             {$$ = new Node("", "eiList", 6, $1, $2, $3, $4, $5, $6);}
              | %empty                                                      {$$ = NULL;}
              ;
elseStmt      : ELSE statement                                              {$$ = new Node("", "elseStmt", 2, $1, $2);}
              | %empty                                                      {$$ = NULL;}
              ;
iterStmt      : FOR LEFTP expStmt expStmt expression RIGHTP statement       {$$ = new Node("", "iterStmt", 7, $1, $2, $3, $4, $5, $6, $7);}
              | WHILE LEFTP expression RIGHTP statement                     {$$ = new Node("", "iterStmt", 5, $1, $2, $3, $4, $5);}
              ;
jmpStmt       : BREAK SEMI                                                  {$$ = new Node("", "jmpStmt", 2, $1, $2);}
              | CONTINUE SEMI                                               {$$ = new Node("", "jmpStmt", 2, $1, $2);}
              ;
retStmt       : RETURN SEMI                                                 {$$ = new Node("", "retStmt", 2, $1, $2);}
              | RETURN expression SEMI                                      {$$ = new Node("", "retStmt", 3, $1, $2, $3);}
              ;
%%
//expression 不能为空，导致for语句最后一项一定要有内容，哪怕是a=a