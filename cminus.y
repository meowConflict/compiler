%{
    #include "ast.hpp"

    int yylex(void);
    int yyparse(void);
    int yywrap(void);
    int yyerror(std::string str)
    { 
        fprintf(stderr, "[ERROR] %s\n", str.c_str());
        return 0;
    }
    #define YYSTYPE std::shared_ptr<lyf::ASTNode>
    extern std::stack<std::string> lex_texts;
    std::stringstream ss;
    std::shared_ptr<lyf::ASTNode> ROOT = nullptr;
%}

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
%token NULLPTR

%right ASSIGN
%left  OR
%left  AND
%left  GT LT EQUAL GE LE NE
%left  PLUS MINUS
%left  STAR DIV REM
%right NOT USTAR UMINUS AMP
%left  LEFTP RIGHTP LEFTSB RIGHTSB

%%
program     : funcList 
{
    ROOT=$1;
}
            ;
// funcList: ProgramNode
funcList    : funcList func 
{
    auto tmp = std::static_pointer_cast<lyf::ProgramNode>($1);
    tmp->push_node($2);
    $$ = tmp;
}
            | %empty
{
    $$=std::make_shared<lyf::ProgramNode>();
}
            ;
// func: StmtNode
func        : funcProto SEMI 
{
    $$ = std::move($1);
}
            | funcProto compdStmt
{
    auto tmp = std::static_pointer_cast<lyf::CompdStmtNode>($2);
    tmp->setFunc();
    $$ = std::make_shared<lyf::FunctionNode>(
        std::static_pointer_cast<lyf::PrototypeNode>($1),
        tmp
    );
}
            ;
// funcProto: PrototypeNode
funcProto   : retType ID LEFTP paramList RIGHTP
{
    $$ = std::make_shared<lyf::PrototypeNode>(
        lex_texts.top(),
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
        lex_texts.top(),
        nullptr
    );
    lex_texts.pop();
}
            ;
// arrayBrackets: VarType
arrayBrackets: arrayBrackets LEFTSB INTV RIGHTSB
{
    ss.clear();
    ss << lex_texts.top();
    lex_texts.pop();
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
    $$ = std::make_shared<lyf::ArrayInitExpr>(
        std::vector<std::shared_ptr<lyf::ExprNode>>{}
    );
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
    $$ = std::make_shared<lyf::ArrayInitExpr>(
        std::vector<std::shared_ptr<lyf::ExprNode>>{tmp}
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
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "ASSIGN",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}                                    
            | exprOr
{
    $$ =std::move($1);  
}                                                        
            ;
exprOr      : exprOr OR exprAnd 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "OR",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}                                        
            | exprAnd
{
    $$ =std::move($1);  
}                                                       
            ;
exprAnd     : exprAnd AND exprCmp
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "AND",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprCmp
{
    $$ =std::move($1);  
}  
            ;
exprCmp     : exprCmp GE exprAdd
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "GE",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprCmp LE exprAdd 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "LE",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprCmp GT exprAdd
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "GT",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprCmp LT exprAdd 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "LT",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprCmp EQUAL exprAdd 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "EQUAL",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprCmp NE exprAdd 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "NE",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprAdd
{
    $$ =std::move($1);  
}                                                       
            ;
exprAdd     : exprAdd PLUS exprMul 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "PLUS",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprAdd MINUS exprMul                              
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "MINUS",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprMul
{
    $$ =std::move($1);  
}                                                       
            ;
exprMul     : exprMul STAR exprUnaryOp 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "STAR",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprMul DIV exprUnaryOp 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "DIV",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprMul REM exprUnaryOp 
{
    $$ = std::make_shared<lyf::BinaryExprNode>(
        "REM",
        std::static_pointer_cast<lyf::ExprNode>($1),
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | exprUnaryOp
{
    $$ =std::move($1);  
}                 
// exprUnaryOp: UnaryExprNode
exprUnaryOp : MINUS exprUnaryOp %prec UMINUS
{
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "UMINUS", 
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}                            
            | NOT exprUnaryOp
{
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "NOT", 
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}                               
            | STAR exprUnaryOp %prec USTAR
{
    $$ =std::make_shared<lyf::UnaryExprNode>(
        "USTAR",
        std::static_pointer_cast<lyf::ExprNode>($2)
    );
}                             
            | AMP exprUnaryOp 
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
{
    auto tmp = std::static_pointer_cast<lyf::ExprNode>($1);
    $$ = std::make_shared<lyf::SubscriptExprNode>(
        tmp,
        std::static_pointer_cast<lyf::ExprNode>($3)
    );
}
            | ID
{
    $$ = std::make_shared<lyf::VarExprNode>(lex_texts.top());
    lex_texts.pop();
}
            ;
// call: CallExprNode
call        : ID LEFTP argList RIGHTP 
{
    auto tmp = std::static_pointer_cast<lyf::CallExprNode>($3);
    tmp->setCallee(lex_texts.top());
    lex_texts.pop();
    $$ = std::move(tmp);
}
            | ID LEFTP RIGHTP 
{
    $$ = std::make_shared<lyf::CallExprNode>(
        lex_texts.top(),
        std::vector<std::shared_ptr<lyf::ExprNode>>()
    );
    lex_texts.pop();
}
            ;
// argList: CallExprNode
argList     : argList COMMA expression
{
    auto tmp = std::static_pointer_cast<lyf::CallExprNode>($1);
    tmp->push_arg(std::static_pointer_cast<lyf::ExprNode>($3));
    $$=std::move(tmp);
}
            | expression 
{
    auto tmp = std::static_pointer_cast<lyf::ExprNode>($1);
    $$=std::make_shared<lyf::CallExprNode>(
        std::string(),
        std::vector<std::shared_ptr<lyf::ExprNode>>{tmp}
    );
}
            ;
// CONF: ConstExprNode
CONF        : INTV
{
    ss.clear();
    ss << lex_texts.top();
    lex_texts.pop();
    int val;
    ss >> val;
    $$=std::make_shared<lyf::ConstIntNode>(val);
}                                                    
            | FLOATV
{
    ss.clear();
    ss << lex_texts.top();
    lex_texts.pop();
    double val;
    ss >> val;
    $$=std::make_shared<lyf::ConstFloatNode>(val);
}                                                      
            | CHARV   
{
    char c;
    std::string s = lex_texts.top();
    lex_texts.pop();
    if (s.length() == 3) {
        c = s[1];
    } else {
        switch (s[2]) {
            case 'n': {
                c = '\n';
                break;
            }
            case 't': {
                c = '\t';
                break;
            }
            case '\'': {
                c = '\'';
                break;
            }
            case '\\': {
                c = '\\';
                break;
            }
        }
    }
    $$=std::make_shared<lyf::ConstCharNode>(c);
}                                                    
            | STRINGV 
{
    std::string s;
    int i=1;
    std::string lex_text = lex_texts.top();
    lex_texts.pop();
    for (; i<lex_text.length()-1; i++) {
        if (lex_text[i] == '\\') {
            switch (lex_text[++i]) {
                case 'n': {
                    s += '\n';
                    break;
                }
                case 't': {
                    s += '\t';
                    break;
                }
                case '\"': {
                    s += '"';
                    break;
                }
                case '\\': {
                    s += '\\';
                    break;
                }
            }
        } else {
            s += lex_text[i];
        }
    }
    $$=std::make_shared<lyf::ConstStringNode>(s);
} 
            | NULLPTR
{
    $$=std::make_shared<lyf::ConstNPtrNode>();
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
preStmt     : expStmt
{
    $$=std::move($1);
}
            | varDec
{
    $$=std::move($1);
}
            ;
iterStmt    : FOR LEFTP preStmt expStmt stepStmt RIGHTP compdStmt
{
    auto cpdStmt = std::static_pointer_cast<lyf::CompdStmtNode>($7);
    $$=std::make_shared<lyf::ForStmtNode>(
        std::static_pointer_cast<lyf::StmtNode>($3),
        std::static_pointer_cast<lyf::ExprNode>($4),
        std::static_pointer_cast<lyf::CompdStmtNode>($5),
        cpdStmt
    );
}    
            | WHILE LEFTP expression RIGHTP compdStmt
{
    $$=std::make_shared<lyf::WhileStmtNode>(
        std::static_pointer_cast<lyf::ExprNode>($3), 
        std::static_pointer_cast<lyf::CompdStmtNode>($5)
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