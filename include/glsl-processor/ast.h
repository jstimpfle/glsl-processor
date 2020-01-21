#ifndef GLSLPROCESSOR_AST_H_INCLUDED
#define GLSLPROCESSOR_AST_H_INCLUDED

#include <glsl-processor/defs.h>
#include <glsl-processor/parse.h>  // TODO: unify these two files

enum {
        GP_SHADERTYPE_VERTEX,
        GP_SHADERTYPE_FRAGMENT,
        GP_NUM_SHADERTYPE_KINDS
};

enum {
        TOKEN_EOF,
        TOKEN_LITERAL,
        TOKEN_NAME,
        TOKEN_STRING,
        TOKEN_LEFTPAREN,
        TOKEN_RIGHTPAREN,
        TOKEN_LEFTBRACE,
        TOKEN_RIGHTBRACE,
        TOKEN_DOT,
        TOKEN_COMMA,
        TOKEN_SEMICOLON,
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_STAR,
        TOKEN_SLASH,
        TOKEN_PERCENT,
        TOKEN_EQUALS,
        TOKEN_DOUBLEEQUALS,
        TOKEN_PLUSEQUALS,
        TOKEN_MINUSEQUALS,
        TOKEN_STAREQUALS,
        TOKEN_SLASHEQUALS,
        TOKEN_NE,
        TOKEN_LT,
        TOKEN_LE,
        TOKEN_GE,
        TOKEN_GT,
        TOKEN_NOT,
        TOKEN_AMPERSAND,
        TOKEN_DOUBLEAMPERSAND,
        TOKEN_PIPE,
        TOKEN_DOUBLEPIPE,
        GP_NUM_TOKEN_KINDS
};

enum {
        UNOP_NOT,
        UNOP_NEGATE,
        GP_NUM_UNOP_KINDS,
};

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
        BINOP_MOD,
        BINOP_ASSIGN,
        BINOP_PLUSASSIGN,
        BINOP_MINUSASSIGN,
        BINOP_MULASSIGN,
        BINOP_DIVASSIGN,
        BINOP_BITAND,
        BINOP_BITOR,
        BINOP_LOGICALAND,
        BINOP_LOGICALOR,
        GP_NUM_BINOP_KINDS,
};

enum {
        EXPR_LIT_NUM,
        EXPR_BINOP,
};

enum {
        PRIMTYPE_BOOL,
        PRIMTYPE_INT,
        PRIMTYPE_UINT,
        PRIMTYPE_FLOAT,
        PRIMTYPE_DOUBLE,
        PRIMTYPE_VEC2,
        PRIMTYPE_VEC3,
        PRIMTYPE_VEC4,
        PRIMTYPE_MAT2,
        PRIMTYPE_MAT3,
        PRIMTYPE_MAT4,
        PRIMTYPE_SAMPLER2D,
        GP_NUM_PRIMTYPE_KINDS
};

enum {
        STMT_EXPR,
        STMT_RETURN,
        STMT_IF,
        STMT_IFELSE,
};

typedef int Expr;
typedef int Stmt;
typedef int TypeExpr;

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

/* we probably should get rid of this type. GLSL has - to my knowledge - only
a fixed number of types (which are all built in) and some parts of the code
even rely on that fact. */
struct TypeExpr {
        int primtypeKind;
};

struct UniformDecl {
        char *uniDeclName;
        struct TypeExpr *uniDeclTypeExpr;
};

struct VariableDecl {
        int inOrOut;
        struct TypeExpr *typeExpr;
        char *name;
};

struct FuncDecl {
        char *name;
        struct TypeExpr *returnTypeExpr;
        struct TypeExpr **argTypeExprs;
        char **argNames;
        int numArgs;
};

struct FuncDefn {
        char *name;
        struct TypeExpr *returnTypeExpr;
        struct TypeExpr **argTypeExprs;
        char **argNames;
        int numArgs;
        Stmt bodyStmt;
};


enum {
        DIRECTIVE_UNIFORM,
        DIRECTIVE_VARIABLE,  // "in" or "out"
        DIRECTIVE_FUNCDECL,
        DIRECTIVE_FUNCDEFN,
};

struct ToplevelNode {
        int directiveKind;
        union {
                struct UniformDecl *tUniform;
                struct VariableDecl *tVariable;
                struct FuncDecl *tFuncdecl;
                struct FuncDefn *tFuncdefn;
        } data;
};

struct UnopInfo {
        char *text;
};

struct BinopInfo {
        char *text;
};

struct UnopTokenInfo {
        int tokenKind;
        int unopKind;
};

struct BinopTokenInfo {
        int tokenKind;
        int binopKind;
};

struct ShaderfileAst {
        char *filepath;

        // For now, for simplicity and pointer stability, an array of pointers...
        struct ToplevelNode **toplevelNodes;
        int numToplevelNodes;
};

struct FileInfo {
        char *fileID;
        char *contents;
        int size;
};

struct ProgramInfo {
        char *programName;
};

struct ShaderInfo {
        char *shaderName;
        int shaderType;
};

struct LinkInfo {
        int programIndex;
        int shaderIndex;
};

struct ProgramUniform {
        int programIndex;
        int typeKind;
        char *uniformName;
};

struct ProgramAttribute {
        int programIndex;
        int typeKind;
        char *attributeName;
};

extern const char *const gp_tokenKindString[GP_NUM_TOKEN_KINDS];
extern const char *const gp_primtypeKindString[GP_NUM_PRIMTYPE_KINDS];
extern const char *const gp_primtypeString[GP_NUM_PRIMTYPE_KINDS];
extern const char *const gp_shadertypeKindString[GP_NUM_SHADERTYPE_KINDS];
extern const struct UnopInfo gp_unopInfo[GP_NUM_UNOP_KINDS];
extern const struct UnopTokenInfo gp_unopTokenInfo[];
extern const struct BinopInfo gp_binopInfo[GP_NUM_BINOP_KINDS];
extern const struct BinopTokenInfo gp_binopTokenInfo[];
extern const int gp_numUnopToken;
extern const int gp_numBinopTokens;

char *alloc_string(struct GP_Ctx *ctx, const char *string);
struct TypeExpr *create_typeexpr(struct GP_Ctx *ctx);
struct UniformDecl *create_uniformdecl(struct GP_Ctx *ctx);
struct VariableDecl *create_variabledecl(struct GP_Ctx *ctx);
struct FuncDecl *create_funcdecl(struct GP_Ctx *ctx);
struct FuncDefn *create_funcdefn(struct GP_Ctx *ctx);

struct ToplevelNode *add_new_toplevel_node_to_fileast(struct ShaderfileAst *fileAst);
struct ToplevelNode *add_new_toplevel_node(struct GP_Ctx *ctx);

void gp_setup(struct GP_Ctx *ctx);
void gp_teardown(struct GP_Ctx *ctx);

#endif
