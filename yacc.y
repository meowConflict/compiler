%{
#include<stdio.h>
#include<string.h>
extern FILE *yyin;
extern int yylineno;	// defined and maintained in lex
extern char *yytext;	// defined and maintained in lex
%}

%token INTV FLOATV CHARV STRINGV
%token IF ELSE WHILE FOR BREAK CONTINUE RETURN INT FLOAT CHAR VOID
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
%left STAR DIV
%right NOT
%left LEFTP RIGHTP LEFTSB RIGHTSB

//some grammar production
%%
program:      decList 
{
    printf("program ");
}
decList:   funDecList {printf("fundecList ");}
varDecList:     varDecList varDec {printf("varDecList ");}
|               varDec {printf("varDecList ");}
|
varDec:         type varList SEMI{}
type:           INT {}
    |           FLOAT {}
    |           CHAR {}
    |           VOID {}
varList:        varList COMMA varName{}
       |        varName {}
varName:        ID {}
       |        ID LEFTSB INTV RIGHTSB {}
       |        ID LEFTSB INTV RIGHTSB LEFTSB INTV RIGHTSB {}
funDecList:     funDecList funcDec {}
          |     funcDec {}
funcDec:        type ID LEFTP paramList RIGHTP compdStmt {} /**/
        |
paramList:      paramList COMMA param {}
        |       param {}
param:          type varName {}
|
              ;
compdStmt:      LEFTB varDecList stmtList RIGHTB {}
stmtList:       stmtList statement {}
        |       statement {}
statement:      expStmt {}
         |      compdStmt {}
         |      selStmt {}
         |      iterStmt {}
         |      retStmt {}
         |      BREAK SEMI {}
         |      CONTINUE SEMI {}
         |
expStmt:        experssion SEMI {}
       |        SEMI {}
experssion:     experssion ASSIGN expr_or {}
          |     expr_or {}
          ;     
          
expr_or   :     expr_or OR expr_and {}
          |     expr_and{}
          ;
expr_and  :    expr_and AND expr_compare {}
          |     expr_compare{}
          ;          
expr_compare  :     expr_compare GE expr_add {}
              |     expr_compare LE expr_add {}
              |     expr_compare GT expr_add {}
              |     expr_compare LT expr_add {}
              |     expr_compare EQUAL expr_add {}
              |     expr_compare NE expr_add {}
              |     expr_add{}
              ;     
expr_add      :     expr_add PLUS expr_mul {}
              |     expr_add MINUS expr_mul {}
              |     expr_mul {}
              ;
expr_mul      :     expr_mul STAR factor {}
              |     expr_mul DIV factor {}
              |     expr_mul REM factor {}
              |     LEFTP factor RIGHTP {}
              |     MINUS factor {}
              |     NOT experssion {}
              |     factor {}
factor:         var {}
      |         call {}
      |         CONF {}
var:            ID {}
   |            ID LEFTSB experssion RIGHTSB {}
   |            ID LEFTSB experssion RIGHTSB LEFTSB experssion RIGHTSB {}
call:           ID LEFTP argList RIGHTP {}
argList:        argList COMMA experssion {}
       |        experssion {}
CONF:           INTV {}
    |           FLOATV {}
    |           CHARV {}
    |           STRINGV {}
selStmt:        IF LEFTP experssion RIGHTP statement {}
       |        IF LEFTP experssion RIGHTP statement ELSE statement {}
iterStmt:       FOR LEFTP expStmt expStmt experssion RIGHTP statement {
    printf("111");
}
        |       WHILE LEFTP experssion RIGHTP statement {}
retStmt:        RETURN SEMI {}
       |        RETURN experssion SEMI {}

//some function
%%
int main() {
    yyin=fopen("test.c","r");
    return yyparse();
}
void yyerror(char *s)
{
	int len=strlen(yytext);
	int i;
	char buf[512]={0};
	printf("*%s*", yytext);
}