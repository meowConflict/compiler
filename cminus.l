%{
    #include <stdio.h>
    #include "ast.h"
    #include "cminus.y.h"
%}


%option noyywrap

COMMENT     \/\*([^\*]|(\*)*[^\*/])*(\*)*\*\/
ID          [a-zA-Z][a-zA-Z0-9]*

INTV        0|[1-9][0-9]*
FLOATV      [0-9]+\.[0-9]+
CHARV       \'.\'|\'\\.\'
STRINGV     \"(\\.|[^"\\])*\"


%%
{INTV}      {yylval.node = new Node(yytext, "INTV", 0);     return INTV;}
{FLOATV}    {yylval.node = new Node(yytext, "FLOATV", 0);   return FLOATV;}
{CHARV}     {yylval.node = new Node(yytext, "CHARV", 0);    return CHARV;}
{STRINGV}   {yylval.node = new Node(yytext, "STRINGV", 0);  return STRINGV;}

"if"        {yylval.node = new Node("", "IF", 0);           return IF;}
"else"      {yylval.node = new Node("", "ELSE", 0);         return ELSE;}
"elseif"    {yylval.node = new Node("", "ELSEIF", 0);       return ELSEIF;}
"while"     {yylval.node = new Node("", "WHILE", 0);        return WHILE;}
"for"       {yylval.node = new Node("", "FOR", 0);          return FOR;}
"break"     {yylval.node = new Node("", "BREAK", 0);        return BREAK;}
"continue"  {yylval.node = new Node("", "CONTINUE", 0);     return CONTINUE;}
"return"    {yylval.node = new Node("", "RETURN", 0);       return RETURN;}
"int"       {yylval.node = new Node("", "INT", 0);          return INT;}
"float"     {yylval.node = new Node("", "FLOAT", 0);        return FLOAT;}
"char"      {yylval.node = new Node("", "CHAR", 0);         return CHAR;}
"void"      {yylval.node = new Node("", "VOID", 0);         return VOID;}

{ID}        {yylval.node = new Node(yytext, "ID", 0);       return ID;}

"="         {yylval.node = new Node("", "ASSIGN", 0);       return ASSIGN;}
"+"         {yylval.node = new Node("", "PLUS", 0);         return PLUS;}
"-"         {yylval.node = new Node("", "MINUS", 0);        return MINUS;}
"*"         {yylval.node = new Node("", "STAR", 0);         return STAR;}
"/"         {yylval.node = new Node("", "DIV", 0);          return DIV;}
"%"         {yylval.node = new Node("", "REM", 0);          return REM;}

">"         {yylval.node = new Node("", "GT", 0);           return GT;}
"<"         {yylval.node = new Node("", "LT", 0);           return LT;}
"=="        {yylval.node = new Node("", "EQUAL", 0);        return EQUAL;}
">="        {yylval.node = new Node("", "GE", 0);           return GE;}
"<="        {yylval.node = new Node("", "LE", 0);           return LE;}
"!="        {yylval.node = new Node("", "NE", 0);           return NE;}

"("         {yylval.node = new Node("", "LEFTP", 0);        return LEFTP;}
")"         {yylval.node = new Node("", "RIGHTP", 0);       return RIGHTP;}
"["         {yylval.node = new Node("", "LEFTSB", 0);       return LEFTSB;}
"]"         {yylval.node = new Node("", "RIGHTSB", 0);      return RIGHTSB;}
"{"         {yylval.node = new Node("", "LEFTB", 0);        return LEFTB;}
"}"         {yylval.node = new Node("", "RIGHTB", 0);       return RIGHTB;}

"&"         {yylval.node = new Node("", "AMP", 0);          return AMP;}
"&&"        {yylval.node = new Node("", "AND", 0);          return AND;}
"||"        {yylval.node = new Node("", "OR", 0);           return OR;}
"!"         {yylval.node = new Node("", "NOT", 0);          return NOT;}

","         {yylval.node = new Node("", "COMMA", 0);        return COMMA;}
";"         {yylval.node = new Node("", "SEMI", 0);         return SEMI;}

{COMMENT}   {;}
[ \n\r\t]   {;}
.           {printf("ERROR: %s\n", yytext);}
%%
