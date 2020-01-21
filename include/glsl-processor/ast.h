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
        GP_TOKEN_EOF,
        GP_TOKEN_LITERAL,
        GP_TOKEN_NAME,
        GP_TOKEN_STRING,
        GP_TOKEN_LEFTPAREN,
        GP_TOKEN_RIGHTPAREN,
        GP_TOKEN_LEFTBRACE,
        GP_TOKEN_RIGHTBRACE,
        GP_TOKEN_DOT,
        GP_TOKEN_COMMA,
        GP_TOKEN_SEMICOLON,
        GP_TOKEN_PLUS,
        GP_TOKEN_MINUS,
        GP_TOKEN_STAR,
        GP_TOKEN_SLASH,
        GP_TOKEN_PERCENT,
        GP_TOKEN_EQUALS,
        GP_TOKEN_DOUBLEEQUALS,
        GP_TOKEN_PLUSEQUALS,
        GP_TOKEN_MINUSEQUALS,
        GP_TOKEN_STAREQUALS,
        GP_TOKEN_SLASHEQUALS,
        GP_TOKEN_NE,
        GP_TOKEN_LT,
        GP_TOKEN_LE,
        GP_TOKEN_GE,
        GP_TOKEN_GT,
        GP_TOKEN_NOT,
        GP_TOKEN_AMPERSAND,
        GP_TOKEN_DOUBLEAMPERSAND,
        GP_TOKEN_PIPE,
        GP_TOKEN_DOUBLEPIPE,
        GP_NUM_TOKEN_KINDS
};

enum {
        GP_UNOP_NOT,
        GP_UNOP_NEGATE,
        GP_NUM_UNOP_KINDS,
};

enum {
        GP_BINOP_EQ,
        GP_BINOP_NE,
        GP_BINOP_LT,
        GP_BINOP_LE,
        GP_BINOP_GT,
        GP_BINOP_GE,
        GP_BINOP_PLUS,
        GP_BINOP_MINUS,
        GP_BINOP_MUL,
        GP_BINOP_DIV,
        GP_BINOP_MOD,
        GP_BINOP_ASSIGN,
        GP_BINOP_PLUSASSIGN,
        GP_BINOP_MINUSASSIGN,
        GP_BINOP_MULASSIGN,
        GP_BINOP_DIVASSIGN,
        GP_BINOP_BITAND,
        GP_BINOP_BITOR,
        GP_BINOP_LOGICALAND,
        GP_BINOP_LOGICALOR,
        GP_NUM_BINOP_KINDS,
};

enum {
        GP_PRIMTYPE_BOOL,
        GP_PRIMTYPE_INT,
        GP_PRIMTYPE_UINT,
        GP_PRIMTYPE_FLOAT,
        GP_PRIMTYPE_DOUBLE,
        GP_PRIMTYPE_VEC2,
        GP_PRIMTYPE_VEC3,
        GP_PRIMTYPE_VEC4,
        GP_PRIMTYPE_MAT2,
        GP_PRIMTYPE_MAT3,
        GP_PRIMTYPE_MAT4,
        GP_PRIMTYPE_SAMPLER2D,
        GP_NUM_PRIMTYPE_KINDS
};


typedef int GP_Expr;
typedef int GP_Stmt;
typedef int GP_TypeExpr;

struct GP_LitExpr {
        double floatingValue;  // TODO better representation
};

struct GP_BinopExpr {
        GP_Expr exprLeft;
        GP_Expr exprRight;
};

struct GP_ExprNode {
        int exprKind;
        union {
                struct GP_LitExpr tLit;
                struct GP_BinopExpr tBinop;
        } data;
};

struct GP_ExprStmt {
        GP_Expr expr;
};

struct GP_ReturnStmt {
        GP_Expr expr;
};

struct GP_IfStmt {
        GP_Expr condExpr;
        GP_Stmt stmt;
};

struct GP_IfElseStmt {
        GP_Expr condExpr;
        GP_Stmt ifBranchStmt;
        GP_Stmt elseBranchStmt;
};

struct GP_StmtNode {
        int stmtKind;
        union {
                struct GP_ExprStmt tExpr;
                struct GP_ReturnStmt tReturn;
                struct GP_IfStmt tIf;
                struct GP_IfElseStmt tIfElse;
        } data;
};

/* we probably should get rid of this type. GLSL has - to my knowledge - only
a fixed number of types (which are all built in) and some parts of the code
even rely on that fact. */
struct GP_TypeExpr {
        int primtypeKind;
};

struct GP_UniformDecl {
        char *uniDeclName;
        struct GP_TypeExpr *uniDeclTypeExpr;
};

struct GP_VariableDecl {
        int inOrOut;
        struct GP_TypeExpr *typeExpr;
        char *name;
};

struct GP_FuncDecl {
        char *name;
        struct GP_TypeExpr *returnTypeExpr;
        struct GP_TypeExpr **argTypeExprs;
        char **argNames;
        int numArgs;
};

struct GP_FuncDefn {
        char *name;
        struct GP_TypeExpr *returnTypeExpr;
        struct GP_TypeExpr **argTypeExprs;
        char **argNames;
        int numArgs;
        GP_Stmt bodyStmt;
};


enum {
        GP_DIRECTIVE_UNIFORM,
        GP_DIRECTIVE_VARIABLE,  // "in" or "out"
        GP_DIRECTIVE_FUNCDECL,
        GP_DIRECTIVE_FUNCDEFN,
};

struct GP_ToplevelNode {
        int directiveKind;
        union {
                struct GP_UniformDecl *tUniform;
                struct GP_VariableDecl *tVariable;
                struct GP_FuncDecl *tFuncdecl;
                struct GP_FuncDefn *tFuncdefn;
        } data;
};

struct GP_UnopInfo {
        char *text;
};

struct GP_BinopInfo {
        char *text;
};

struct GP_UnopTokenInfo {
        int tokenKind;
        int unopKind;
};

struct GP_BinopTokenInfo {
        int tokenKind;
        int binopKind;
};

struct GP_ShaderfileAst {
        char *filepath;

        // For now, for simplicity and pointer stability, an array of pointers...
        struct GP_ToplevelNode **toplevelNodes;
        int numToplevelNodes;
};

struct GP_FileInfo {
        char *fileID;
        char *contents;
        int size;
};

struct GP_ProgramInfo {
        char *programName;
};

struct GP_ShaderInfo {
        char *shaderName;
        int shaderType;
};

struct GP_LinkInfo {
        int programIndex;
        int shaderIndex;
};

struct GP_ProgramUniform {
        int programIndex;
        int typeKind;
        char *uniformName;
};

struct GP_ProgramAttribute {
        int programIndex;
        int typeKind;
        char *attributeName;
};

extern const char *const gp_tokenKindString[GP_NUM_TOKEN_KINDS];
extern const char *const gp_primtypeKindString[GP_NUM_PRIMTYPE_KINDS];
extern const char *const gp_primtypeString[GP_NUM_PRIMTYPE_KINDS];
extern const char *const gp_shadertypeKindString[GP_NUM_SHADERTYPE_KINDS];
extern const struct GP_UnopInfo gp_unopInfo[GP_NUM_UNOP_KINDS];
extern const struct GP_UnopTokenInfo gp_unopTokenInfo[];
extern const struct GP_BinopInfo gp_binopInfo[GP_NUM_BINOP_KINDS];
extern const struct GP_BinopTokenInfo gp_binopTokenInfo[];
extern const int gp_numUnopToken;
extern const int gp_numBinopTokens;

char *alloc_string(struct GP_Ctx *ctx, const char *string);
struct GP_TypeExpr *create_typeexpr(struct GP_Ctx *ctx);
struct GP_UniformDecl *create_uniformdecl(struct GP_Ctx *ctx);
struct GP_VariableDecl *create_variabledecl(struct GP_Ctx *ctx);
struct GP_FuncDecl *create_funcdecl(struct GP_Ctx *ctx);
struct GP_FuncDefn *create_funcdefn(struct GP_Ctx *ctx);

struct GP_ToplevelNode *add_new_toplevel_node_to_fileast(struct GP_ShaderfileAst *fileAst);
struct GP_ToplevelNode *add_new_toplevel_node(struct GP_Ctx *ctx);

void gp_setup(struct GP_Ctx *ctx);
void gp_teardown(struct GP_Ctx *ctx);

#endif
