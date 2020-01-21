#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/ast.h>

char *alloc_string(struct GP_Ctx *ctx, const char *data)
{
        int length = (int) strlen(data);
        char *string;
        ALLOC_MEMORY(&string, length + 1);
        memcpy(string, data, length + 1);
        return string;
}

enum {
        POOL_TYPEEXPR,
        POOL_UNIFORMDECL,
        POOL_VARIABLEDECL,
        POOL_FUNCDECL,
        POOL_FUNCDEFN,
        NUM_POOL_KINDS
};

static int poolObjectSize[NUM_POOL_KINDS] = {
#define MAKE(k, t) [k] = sizeof (t)
        MAKE(POOL_UNIFORMDECL, struct UniformDecl),
        MAKE(POOL_VARIABLEDECL, struct VariableDecl),
        MAKE(POOL_TYPEEXPR, struct TypeExpr),
        MAKE(POOL_FUNCDECL, struct FuncDecl),
        MAKE(POOL_FUNCDEFN, struct FuncDefn),
#undef MAKE
};

static void *allocate_pooled_object(struct GP_Ctx *ctx, int poolKind)
{
        ENSURE(0 <= poolKind && poolKind < NUM_POOL_KINDS);
        int size = poolObjectSize[poolKind];
        void *ptr;
        alloc_memory(&ptr, 1, size);
        return ptr;
}

#define DEFINE_ALLOCATOR_FUNCTION(type, name, poolKind) type *name(struct GP_Ctx *ctx) \
{ \
        assert(poolObjectSize[poolKind] == sizeof(type)); \
        return allocate_pooled_object(ctx, (poolKind)); \
}

#define DEFINE_ARRAY_ALLOCATOR_FUNCTION(type, name, structName, countMember, arrayMember) type *name(structName *container) \
{ \
        int idx = container->countMember++; \
        REALLOC_MEMORY(&container->arrayMember, container->countMember); \
        return &container->arrayMember[idx]; \
}

DEFINE_ALLOCATOR_FUNCTION(struct TypeExpr, create_typeexpr, POOL_TYPEEXPR)
DEFINE_ALLOCATOR_FUNCTION(struct UniformDecl, create_uniformdecl, POOL_UNIFORMDECL)
DEFINE_ALLOCATOR_FUNCTION(struct VariableDecl, create_variabledecl, POOL_VARIABLEDECL)
DEFINE_ALLOCATOR_FUNCTION(struct FuncDecl, create_funcdecl, POOL_FUNCDECL)
DEFINE_ALLOCATOR_FUNCTION(struct FuncDefn, create_funcdefn, POOL_FUNCDEFN)

struct ToplevelNode *add_new_toplevel_node_to_shaderfileast(struct ShaderfileAst *fa)
{
        int idx = fa->numToplevelNodes ++;
        REALLOC_MEMORY(&fa->toplevelNodes, fa->numToplevelNodes);
        ALLOC_MEMORY(&fa->toplevelNodes[idx], 1);
        return fa->toplevelNodes[idx];
}

struct ToplevelNode *add_new_toplevel_node(struct GP_Ctx *ctx)
{
        return add_new_toplevel_node_to_shaderfileast(&ctx->shaderfileAsts[ctx->currentFileIndex]);
}
