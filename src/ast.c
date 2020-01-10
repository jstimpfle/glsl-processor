#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/ast.h>

AstString create_aststring(struct Ast *ast, const char *data)
{
        int length = (int) strlen(data);
        AstString astString;
        ALLOC_MEMORY(&astString.data, length + 1);
        memcpy(astString.data, data, length + 1);
        return astString;
}

enum {
        POOL_PROGRAMDECL,
        POOL_SHADERDECL,
        POOL_LINKITEM,
        POOL_TYPEEXPR,
        POOL_UNIFORMDECL,
        POOL_VARIABLEDECL,
        POOL_FUNCDECL,
        POOL_FUNCDEFN,
        NUM_POOL_KINDS
};

static int poolObjectSize[NUM_POOL_KINDS] = {
#define MAKE(k, t) [k] = sizeof (t)
        MAKE(POOL_PROGRAMDECL, struct ProgramDecl),
        MAKE(POOL_SHADERDECL, struct ShaderDecl),
        MAKE(POOL_LINKITEM, struct LinkItem),
        MAKE(POOL_UNIFORMDECL, struct UniformDecl),
        MAKE(POOL_VARIABLEDECL, struct VariableDecl),
        MAKE(POOL_TYPEEXPR, struct TypeExpr),
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

#define DEFINE_ALLOCATOR_FUNCTION(type, name, poolKind) type *name(struct Ast *ast) \
{ \
        assert(poolObjectSize[poolKind] == sizeof(type)); \
        return allocate_pooled_object(ast, (poolKind)); \
}

#define DEFINE_ARRAY_ALLOCATOR_FUNCTION(type, name, structName, countMember, arrayMember) type *name(structName *container) \
{ \
        int idx = container->countMember++; \
        REALLOC_MEMORY(&container->arrayMember, container->countMember); \
        return &container->arrayMember[idx]; \
}

DEFINE_ARRAY_ALLOCATOR_FUNCTION(struct ProgramDecl, create_program_decl, struct Ast, numPrograms, programDecls)
DEFINE_ARRAY_ALLOCATOR_FUNCTION(struct ShaderDecl, create_shader_decl, struct Ast, numShaders, shaderDecls)
DEFINE_ARRAY_ALLOCATOR_FUNCTION(struct LinkItem, create_link_item, struct Ast, numLinkItems, linkItems)
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

struct ToplevelNode *add_new_toplevel_node(struct Ast *ast)
{
        return add_new_toplevel_node_to_shaderfileast(&ast->shaderfileAsts[ast->currentFileIndex]);
}

void add_file_to_ast_and_switch_to_it(struct Ast *ast, const char *filepath)
{
        int index = ast->numFiles ++;
        REALLOC_MEMORY(&ast->shaderfileAsts, ast->numFiles);
        struct ShaderfileAst *fa = &ast->shaderfileAsts[index];
        memset(fa, 0, sizeof *fa);
        int filepathLength = (int) strlen(filepath);
        ALLOC_MEMORY(&fa->filepath, filepathLength + 1);
        memcpy(fa->filepath, filepath, filepathLength + 1);
        // switch
        ast->currentFileIndex = index;
}

void setup_ast(struct Ast *ast)
{
        memset(ast, 0, sizeof *ast);
}

void teardown_ast(struct Ast *ast)
{
        for (int i = 0; i < ast->numFiles; i++)
                FREE_MEMORY(&ast->shaderfileAsts[i].filepath);
        FREE_MEMORY(&ast->shaderfileAsts);
        FREE_MEMORY(&ast->astStrings);
}
