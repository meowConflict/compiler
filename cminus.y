%{
    #include <stdio.h>
    int main()
    {
        yyparse();
    }
%}


%token INTV FLOATV CHARV STRINGV
%token IF ELSE ELSEIF WHILE FOR BREAK CONTINUE RETURN INT FLOAT CHAR VOID
%token ID
%token ASSIGN PLUS MINUS STAR DIV REM
%token GT LT EQUAL GE LE NE
%token LEFTP RIGHTP LEFTSB RIGHTSB LEFTB RIGHTB
%token AND OR NOT
%token COMMA SEMI

%right ASSIGN
%left OR
%left AND
%left GT LT EQUAL GE LE NE
%left PLUS MINUS
%left STAR DIV REM
%right NOT
%left LEFTP RIGHTP LEFTSB RIGHTSB


%%
program       : funDecList                                                  {printf("program -> funDecList\n");}
              ;
funDecList    : funDecList funcDec                                          {printf("funDecList -> funDecList funcDec\n");}
              | funcDec                                                     {printf("funDecList -> funcDec\n");}
              ;
funcDec       : type ID LEFTP paramList RIGHTP compdStmt                    {printf("funcDec -> type ID LEFTP paramList RIGHTP compdStmt\n");}
              | type ID LEFTP RIGHTP compdStmt                              {printf("funcDec -> type ID LEFTP RIGHTP compdStmt\n");}
              ;
type          : INT                                                         {printf("type -> INT\n");}
              | FLOAT                                                       {printf("type -> FLOAT\n");}
              | CHAR                                                        {printf("type -> CHAR\n");}
              | VOID                                                        {printf("type -> VOID\n");}
              ;
paramList:      paramList COMMA param                                       {printf("paramList -> paramList COMMA param\n");}
              | param                                                       {printf("paramList -> param\n");}
              ;
param         : type varName                                                {printf("param -> type varName\n");}
              ;
varName       : ID                                                          {printf("varName -> ID\n");}
              | ID LEFTSB INTV RIGHTSB                                      {printf("varName -> ID LEFTSB INTV RIGHTSB\n");}
              | ID LEFTSB INTV RIGHTSB LEFTSB INTV RIGHTSB                  {printf("varName -> ID LEFTSB INTV RIGHTSB LEFTSB INTV RIGHTSB\n");}
              ;
compdStmt     : LEFTB varDecList stmtList RIGHTB                            {printf("compdStmt -> LEFTB varDecList stmtList RIGHTB\n");}
              ;
varDecList    : varDecList varDec                                           {printf("varDecList -> varDecList varDec\n");}
              | %empty                                                      {printf("varDecList -> %%empty\n");}
              ;
varDec        : type varList SEMI                                           {printf("varDec -> type varList SEMI\n");}
              ;
varList       : varList COMMA varName                                       {printf("varList -> varList COMMA varName");}
              | varName                                                     {printf("varList -> varName");}
              ;
stmtList:       stmtList statement                                          {printf("stmtList -> stmtList statement\n");}
              | %empty                                                      {printf("stmtList -> %%empty\n");}
              ;
statement:      expStmt                                                     {printf("statement -> expStmt\n");}
              | compdStmt                                                   {printf("statement -> compdStmt\n");}
              | selStmt                                                     {printf("statement -> selStmt\n");}
              | iterStmt                                                    {printf("statement -> iterStmt\n");}
              | jmpStmt                                                     {printf("statement -> jmpStmt\n");}
              | retStmt                                                     {printf("statement -> retStmt\n");}
              ;
expStmt       : experssion SEMI                                             {printf("expStmt -> expression SEMI\n");}
              | SEMI                                                        {printf("expStmt -> SEMI\n");}
              ;
experssion    : experssion ASSIGN expr_or                                   {printf("experssion -> experssion ASSIGN expr_or\n");}
              | expr_or                                                     {printf("experssion -> expr_or\n");}
              ;
expr_or       : expr_or OR expr_and                                         {printf("expr_or -> expr_or OR expr_and\n");}
              | expr_and                                                    {printf("expr_or -> expr_and\n");}
              ;
expr_and      : expr_and AND expr_compare                                   {printf("expr_and -> expr_and AND expr_compare\n");}
              | expr_compare                                                {printf("expr_and -> expr_compare\n");}
              ;
expr_compare  : expr_compare GE expr_add                                    {printf("expr_compare -> expr_compare GE expr_add\n");}
              | expr_compare LE expr_add                                    {printf("expr_compare -> expr_compare LE expr_add\n");}
              | expr_compare GT expr_add                                    {printf("expr_compare -> expr_compare GT expr_add\n");}
              | expr_compare LT expr_add                                    {printf("expr_compare -> expr_compare LT expr_add\n");}
              | expr_compare EQUAL expr_add                                 {printf("expr_compare -> expr_compare EQUAL expr_add\n");}
              | expr_compare NE expr_add                                    {printf("expr_compare -> expr_compare NE expr_add\n");}
              | expr_add                                                    {printf("expr_compare -> expr_add\n");}
              ;
expr_add      : expr_add PLUS expr_mul                                      {printf("expr_add -> expr_add PLUS expr_mul\n");}
              | expr_add MINUS expr_mul                                     {printf("expr_add -> expr_add MINUS expr_mul\n");}
              | expr_mul                                                    {printf("expr_add -> expr_mul\n");}
              ;
expr_mul      : expr_mul STAR factor                                        {printf("expr_mul -> expr_mul STAR factor\n");}
              | expr_mul DIV factor                                         {printf("expr_mul -> expr_mul DIV factor\n");}
              | expr_mul REM factor                                         {printf("expr_mul -> expr_mul REM factor\n");}
              | LEFTP experssion RIGHTP                                     {printf("expr_mul -> LEFTP experssion RIGHTP\n");}
              | MINUS factor                                                {printf("expr_mul -> MINUS factor\n");}
              | NOT factor                                                  {printf("expr_mul -> NOT factor\n");}
              | factor                                                      {printf("expr_mul -> factor\n");}
              ;
factor        : var                                                         {printf("factor -> var\n");}
              | call                                                        {printf("factor -> call\n");}
              | CONF                                                        {printf("factor -> CONF\n");}
              ;
var           : ID                                                          {printf("var -> ID\n");}
              | ID LEFTSB experssion RIGHTSB                                {printf("var -> ID LEFTSB experssion RIGHTSB\n");}
              | ID LEFTSB experssion RIGHTSB LEFTSB experssion RIGHTSB      {printf("var -> ID LEFTSB experssion RIGHTSB LEFTSB experssion RIGHTSB\n");}
              ;
call          : ID LEFTP argList RIGHTP                                     {printf("call -> ID LEFTP argList RIGHTP\n");}
              | ID LEFTP RIGHTP                                             {printf("call -> ID LEFTP RIGHTP\n");}
              ;
argList       : argList COMMA experssion                                    {printf("argList -> argList COMMA experssion\n");}
              | experssion                                                  {printf("argList -> experssion\n");}
              ;
CONF          : INTV                                                        {printf("CONF -> INTV\n");}
              | FLOATV                                                      {printf("CONF -> FLOATV\n");}
              | CHARV                                                       {printf("CONF -> CHARV\n");}
              | STRINGV                                                     {printf("CONF -> STRINGV\n");}
              ;
selStmt       : IF LEFTP experssion RIGHTP statement eiList elseStmt SEMI   {printf("selStmt -> \n");}
              ;
eiList        : eiList ELSEIF LEFTP experssion RIGHTP statement             {printf("eiList -> eiList ELSEIF LEFTP experssion RIGHTP statement\n");}
              | %empty                                                      {printf("eiList -> %%empty\n");}
              ;
elseStmt      : ELSE statement                                              {printf("elseStmt -> ELSE statement\n");}
              | %empty                                                      {printf("elseStmt -> %%empty\n");}
              ;
iterStmt      : FOR LEFTP expStmt expStmt experssion RIGHTP statement       {printf("iterStmt -> FOR LEFTP expStmt expStmt experssion RIGHTP statement\n");}
              | WHILE LEFTP experssion RIGHTP statement                     {printf("iterStmt -> WHILE LEFTP experssion RIGHTP statement\n");}
              ;
jmpStmt       : BREAK SEMI                                                  {printf("jmpStmt -> BREAK SEMI\n");}
              | CONTINUE SEMI                                               {printf("jmpStmt -> CONTINUE SEMI\n");}
              ;
retStmt       : RETURN SEMI                                                 {printf("retStmt -> RETURN SEMI\n");}
              | RETURN experssion SEMI                                      {printf("retStmt -> RETURN experssion SEMI\n");}
              ;
%%
