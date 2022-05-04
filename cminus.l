%{
    #include <stdio.h>
    #include "cminus.y.h"
    int yywrap(void)
    {
        return 1;
        
    }

    int yyerror(char *s)
    {
        printf("ERROR: %s\n", yytext);
        return 0;
    }
%}


COMMENT     \/\*([^\*]|(\*)*[^\*/])*(\*)*\*\/
ID          [a-zA-Z][a-zA-Z0-9]*

INTV        0|[1-9][0-9]*
FLOATV      [0-9]+\.[0-9]+
CHARV       \'.\'|\'\\.\'
STRINGV     \"(\\.|[^"\\])*\"


%%
{INTV}      {return INTV;} //remember to change llval
{FLOATV}    {return FLOATV;}
{CHARV}     {return CHARV;}
{STRINGV}   {return STRINGV;}

"if"        {return IF;}
"else"      {return ELSE;}
"elseif"    {return ELSEIF;}
"while"     {return WHILE;}
"for"       {return FOR;}
"break"     {return BREAK;}
"continue"  {return CONTINUE;}
"return"    {return RETURN;}
"int"       {return INT;}
"float"     {return FLOAT;}
"char"      {return CHAR;}
"void"      {return VOID;}

{ID}        {return ID;}

"="         {return ASSIGN;}
"+"         {return PLUS;}
"-"         {return MINUS;}
"*"         {return STAR;}
"/"         {return DIV;}
"%"         {return REM;}

">"         {return GT;}
"<"         {return LT;}
"=="        {return EQUAL;}
">="        {return GE;}
"<="        {return LE;}
"!="        {return NE;}

"("         {return LEFTP;}
")"         {return RIGHTP;}
"["         {return LEFTSB;}
"]"         {return RIGHTSB;}
"{"         {return LEFTB;}
"}"         {return RIGHTB;}

"&&"        {return AND;}
"||"        {return OR;}
"!"         {return NOT;}

","         {return COMMA;}
";"         {return SEMI;}

{COMMENT}   {;}
[ /n/r/t]   {;}
.           {printf("ERROR\n");}
%%
