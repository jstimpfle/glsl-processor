#ifndef GLSLCOMPILER_AST_H_INCLUDED
#define GLSLCOMPILER_AST_H_INCLUDED

#include <glsl-compiler/defs.h>
#include <glsl-compiler/parse.h>  // TODO: unify these two files

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
        TOKEN_SLASH,
        TOKEN_STAR,
        TOKEN_EQUALS,
        TOKEN_DOUBLEEQUALS,
        TOKEN_NE,
        TOKEN_LT,
        TOKEN_LE,
        TOKEN_GE,
        TOKEN_GT,
        TOKEN_AMPERSAND,
        TOKEN_DOUBLEAMPERSAND,
        TOKEN_PIPE,
        TOKEN_DOUBLEPIPE,
        NUM_TOKEN_KINDS
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
        BINOP_ASSIGN,
        BINOP_BITAND,
        BINOP_BITOR,
        BINOP_LOGICALAND,
        BINOP_LOGICALOR,
        NUM_BINOP_KINDS,
};

enum {
        EXPR_LIT_NUM,
        EXPR_BINOP,
};

enum {
        PRIMTYPE_FLOAT,
        PRIMTYPE_VEC2,
        PRIMTYPE_VEC3,
        PRIMTYPE_VEC4,
        PRIMTYPE_MAT2,
        PRIMTYPE_MAT3,
        PRIMTYPE_MAT4,
        NUM_PRIMTYPE_KINDS
};

struct PrimtypeInfo {
        const char *name;
};

extern const struct PrimtypeInfo primtypeInfo[NUM_PRIMTYPE_KINDS];

enum {
        STMT_EXPR,
        STMT_RETURN,
        STMT_IF,
        STMT_IFELSE,
};

typedef int Expr;
typedef int Stmt;
typedef int TypeExpr;

typedef struct {
        char *data; // zero-terminated
} AstName;

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

struct TypeExpr {
        int primtypeKind;
};

struct UniformDecl {
        AstName uniDeclName;
        struct TypeExpr *uniDeclTypeExpr;
};

struct AttributeDecl {
        int inOrOut;
        struct TypeExpr *typeExpr;
        AstName name;
};

struct FuncDecl {
        AstName name;
        struct TypeExpr *returnTypeExpr;
        struct TypeExpr **argTypeExprs;
        AstName *argNames;
        int numArgs;
};

struct FuncDefn {
        AstName name;
        struct TypeExpr *returnTypeExpr;
        struct TypeExpr **argTypeExprs;
        AstName *argNames;
        int numArgs;
        Stmt bodyStmt;
};


enum {
        DIRECTIVE_UNIFORM,
        DIRECTIVE_ATTRIBUTE,
        DIRECTIVE_FUNCDECL,
        DIRECTIVE_FUNCDEFN,
};

struct ToplevelNode {
        int directiveKind;
        union {
                struct UniformDecl *tUniform;
                struct AttributeDecl *tAttribute;
                struct FuncDecl *tFuncdecl;
                struct FuncDefn *tFuncdefn;
        } data;
};

struct BinopInfo {
        char *text;
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

struct ProgramDecl {
        AstName programName;
};

struct ShaderDecl {
        AstName shaderName;
        int shaderType;  //SHADERTYPE_??
        AstName shaderFilepath;
};

struct LinkItem {
        AstName programName;
        AstName shaderName;
};

struct Ast {
        struct ProgramDecl *programDecls;
        struct ShaderDecl *shaderDecls;
        struct LinkItem *linkItems;

        int numPrograms;
        int numShaders;
        int numLinkItems;

        struct ShaderfileAst *shaderfileAsts;
        int numFiles;
        int currentFileIndex;  // global state for simpler code

        // TODO: think about allocation strategy...
        struct AstName *astStrings;
        struct UniformNode *uniforms;
        struct AttributeNode *attributes;
        struct FuncDeclNode *funcDecls;
};


extern const char *const tokenKindString[NUM_TOKEN_KINDS];
extern const char *const primtypeKindString[NUM_PRIMTYPE_KINDS];
extern const char *const primtypeString[NUM_PRIMTYPE_KINDS];
extern const struct BinopInfo binopInfo[NUM_BINOP_KINDS];
extern const struct BinopTokenInfo binopTokenInfo[];
extern const int numBinopTokens;

AstName create_astname(struct Ast *ast, const char *string);
struct TypeExpr *create_typeexpr(struct Ast *ast);
struct UniformDecl *create_uniformdecl(struct Ast *ast);
struct AttributeDecl *create_attributedecl(struct Ast *ast);
struct FuncDecl *create_funcdecl(struct Ast *ast);
struct FuncDefn *create_funcdefn(struct Ast *ast);

struct ToplevelNode *add_new_toplevel_node_to_fileast(struct ShaderfileAst *fileAst);
struct ToplevelNode *add_new_toplevel_node(struct Ast *ast);

struct ProgramDecl *create_program_decl(struct Ast *ast);
struct ShaderDecl *create_shader_decl(struct Ast *ast);
struct LinkItem *create_link_item(struct Ast *ast);

void add_file_to_ast_and_switch_to_it(struct Ast *ast, const char *filepath);

void setup_ast(struct Ast *ast);
void teardown_ast(struct Ast *ast);

static inline const char *get_astname_buffer(struct Ast *ast, AstName name)
{
        UNUSED(ast);
        return name.data;
}

#endif
