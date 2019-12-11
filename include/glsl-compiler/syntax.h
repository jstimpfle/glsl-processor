#include <glsl-compiler/parse.h>  // TODO: unify these two files
typedef char *String; // XXX

typedef int Expr;
typedef int Stmt;
typedef int TypeExpr;

enum {
        BINOP_EQ,
        BINOP_NE,
        BINOP_LT,
        BINOP_LE,
        BINOP_GT,
        BINOP_GE,
        BINOP_PLUS,
        BINOP_MINUS,
        BINOP_MUL,
        BINOP_DIV,
        BINOP_ASSIGN,
        NUM_BINOP_KINDS,
};

enum {
        EXPR_LIT_NUM,
        EXPR_BINOP,
};

enum {
        STMT_EXPR,
        STMT_RETURN,
        STMT_IF,
        STMT_IFELSE,
};

struct LitExpr {
        double floatingValue;  // TODO better representation
};

struct BinopExpr {
        Expr exprLeft;
        Expr exprRight;
};

struct ExprNode {
        int exprKind;
        union {
                struct LitExpr tLit;
                struct BinopExpr tBinop;
        } data;
};

struct ExprStmt {
        Expr expr;
};

struct ReturnStmt {
        Expr expr;
};

struct IfStmt {
        Expr condExpr;
        Stmt stmt;
};

struct IfElseStmt {
        Expr condExpr;
        Stmt ifBranchStmt;
        Stmt elseBranchStmt;
};

struct StmtNode {
        int stmtKind;
        union {
                struct ExprStmt tExpr;
                struct ReturnStmt tReturn;
                struct IfStmt tIf;
                struct IfElseStmt tIfElse;
        } data;
};

enum {
        DIRECTIVE_UNIFORM,
        DIRECTIVE_ATTRIBUTE,
        DIRECTIVE_FUNCDECL,
        DIRECTIVE_FUNCDEF,
};

struct UniformDirective {
        TypeExpr typeExpr;
        String name;
};

struct AttributeDirective {
        int inOrOut;
        TypeExpr typeExpr;
        String name;
};

struct FuncdeclDirective {
        String name;
        TypeExpr typeExpr;
        TypeExpr *argExprs;
        int numArgs;
};

struct FuncdefDirective {
        String name;
        TypeExpr typeExpr;
        TypeExpr *argExpr;
        int numArgs;
        Stmt bodyStmt;
};

struct DirectiveNode {
        int directiveKind;
        union {
                struct UniformDirective tUniform;
                struct AttributeDirective tAttribute;
                struct FuncdeclDirective tFuncdecl;
                struct FuncdefDirective tFuncdef;
        } data;
};

struct BinopInfo {
        char *text;
};

struct BinopTokenInfo {
        int tokenKind;
        int binopKind;
};

extern const char *const tokenKindString[NUM_TOKEN_KINDS];
extern const struct BinopInfo binopInfo[NUM_BINOP_KINDS];
extern const struct BinopTokenInfo binopTokenInfo[];
extern const int numBinopTokens;
