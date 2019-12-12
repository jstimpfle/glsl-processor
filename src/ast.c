#include <glsl-compiler/memoryalloc.h>
#include <glsl-compiler/ast.h>

AstName create_astname(struct Ast *ast, const char *data)
{
        int length = strlen(data);
        AstName astString;
        ALLOC_MEMORY(&astString.data, length + 1);
        memcpy(astString.data, data, length + 1);
        return astString;
}

enum {
        POOL_TYPEEXPR,
        POOL_UNIFORMDECL,
        POOL_ATTRIBUTEDECL,
        POOL_FUNCDECL,
        POOL_FUNCDEFN,
        NUM_POOL_KINDS
};

static int poolObjectSize[NUM_POOL_KINDS] = {
#define MAKE(k, t) [k] = sizeof (t)
        MAKE(POOL_TYPEEXPR, struct TypeExpr),
        MAKE(POOL_UNIFORMDECL, struct UniformDecl),
        MAKE(POOL_FUNCDECL, struct FuncDecl),
        MAKE(POOL_FUNCDEFN, struct FuncDefn),
#undef MAKE
};

static void *allocate_pooled_object(struct Ast *ast, int poolKind)
{
        ENSURE(0 <= poolKind && poolKind < NUM_POOL_KINDS);
        int size = poolObjectSize[poolKind];
        void *ptr;
        alloc_memory(&ptr, 1, size);
        return ptr;
}

#define DEFINE_ALLOCATOR_FUNCTION(type, name, poolKind) type *name(struct Ast *ast) { return allocate_pooled_object(ast, (poolKind)); }

DEFINE_ALLOCATOR_FUNCTION(struct TypeExpr, create_typeexpr, POOL_TYPEEXPR)
DEFINE_ALLOCATOR_FUNCTION(struct UniformDecl, create_uniformdecl, POOL_UNIFORMDECL)
DEFINE_ALLOCATOR_FUNCTION(struct AttributeDecl, create_attributedecl, POOL_ATTRIBUTEDECL)
DEFINE_ALLOCATOR_FUNCTION(struct FuncDecl, create_funcdecl, POOL_FUNCDECL)
DEFINE_ALLOCATOR_FUNCTION(struct FuncDefn, create_funcdefn, POOL_FUNCDEFN)

struct ToplevelNode *add_new_toplevel_node(struct Ast *ast)
{
        ++ast->numToplevelNodes;
        REALLOC_MEMORY(&ast->toplevelNodes, ast->numToplevelNodes);

        struct ToplevelNode *toplevelNode;
        ALLOC_MEMORY(&toplevelNode, 1);
        ast->toplevelNodes[ast->numToplevelNodes - 1] = toplevelNode;
        return toplevelNode;
}
