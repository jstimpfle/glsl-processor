#include <glsl-compiler/logging.h>
#include <glsl-compiler/memoryalloc.h>
#include <glsl-compiler/ast.h>
#include <stdio.h> //XXX

static void print_type(struct Ast *ast, struct TypeExpr *typeExpr)
{
        int kind = typeExpr->primtypeKind;
        if (kind == -1) //XXX
                printf("void");
        else
                printf("%s", primtypeInfo[typeExpr->primtypeKind].name);
}

void print_interface_of_shaderfile(struct Ast *ast, struct ShaderfileAst *fa)
{
        for (int i = 0; i < fa->numToplevelNodes; i++) {
                struct ToplevelNode *node = fa->toplevelNodes[i];
                if (node->directiveKind == DIRECTIVE_UNIFORM) {
                        AstString name = node->data.tUniform->uniDeclName;
                        struct TypeExpr *typeExpr = node->data.tUniform->uniDeclTypeExpr;
                        const char *nameBuffer = get_aststring_buffer(ast, name);
                        printf("%s: uniform ", fa->filepath);
                        print_type(ast, typeExpr);
                        printf(" %s;\n", nameBuffer);
                }
                else if (node->directiveKind == DIRECTIVE_ATTRIBUTE) {
                        struct AttributeDecl *adecl = node->data.tAttribute;
                        AstString name = adecl->name;
                        struct TypeExpr *typeExpr = adecl->typeExpr;
                        const char *nameBuffer = get_aststring_buffer(ast, name);
                        printf("%s: attribute ", fa->filepath);
                        printf("%s ", adecl->inOrOut ? "out" : "in");
                        print_type(ast, typeExpr);
                        printf(" %s;\n", nameBuffer);
                }
                else if (node->directiveKind == DIRECTIVE_FUNCDECL) {
                        struct FuncDecl *fdecl = node->data.tFuncdecl;
                        AstString name = fdecl->name;
                        printf("%s: funcdecl ", fa->filepath);
                        print_type(ast, fdecl->returnTypeExpr);
                        printf(" %s();\n", get_aststring_buffer(ast, name));
                }
                else if (node->directiveKind == DIRECTIVE_FUNCDEFN) {
                        struct FuncDefn *fdef = node->data.tFuncdefn;
                        AstString name = fdef->name;
                        printf("%s: funcdefn ", fa->filepath);
                        print_type(ast, fdef->returnTypeExpr);
                        printf(" %s();\n", get_aststring_buffer(ast, name));
                }
        }
}

struct PrintInterfacesCtx {
        struct Ast *ast;
        // for each shader in 0..ast->numShaders, one preallocated "boolean" to indicate
        // visited state.
        int *visited;
        int *todo;
        int numTodo;
};

void setup_printInterfaceCtx(struct PrintInterfacesCtx *ctx, struct Ast *ast)
{
        ctx->ast = ast;
        ctx->numTodo = 0;
        ALLOC_MEMORY(&ctx->visited, ast->numShaders);
        ALLOC_MEMORY(&ctx->todo, ast->numShaders);
}

void teardown_printInterfacesCtx(struct PrintInterfacesCtx *ctx)
{
        FREE_MEMORY(&ctx->visited);
        FREE_MEMORY(&ctx->todo);
        memset(ctx, 0, sizeof *ctx);
}

static int find_program_index(struct Ast *ast, const char *programName)
{
        for (int i = 0; i < ast->numShaders; i++)
                if (!strcmp(programName, get_aststring_buffer(ast, ast->programDecls[i].programName)))
                        return i;
        fatal_f("Referenced Shader does not exist: %s", programName);
}

static int find_shader_index(struct Ast *ast, const char *shaderName)
{
        for (int i = 0; i < ast->numShaders; i++)
                if (!strcmp(shaderName, get_aststring_buffer(ast, ast->shaderDecls[i].shaderName)))
                        return i;
        fatal_f("Referenced Shader does not exist: %s", shaderName);
}

void print_necessary_interfaces_for_shaderfile(struct PrintInterfacesCtx *ctx, const char *shaderName)
{
        memset(ctx->visited, 0, ctx->ast->numShaders * sizeof *ctx->visited);
        ctx->numTodo = 0;
        {
                int idx = find_shader_index(ctx->ast, shaderName);
                ctx->todo[ctx->numTodo++] = idx;
                ctx->visited[idx] = 1;
        }
        for (int i = 0; i < ctx->numTodo; i++) {
        }
}

static void resolve_all_references(struct Ast *ast)
{
        for (int i = 0; i < ast->numLinkItems; i++) {
                AstString programName = ast->linkItems[i].programName;
                AstString shaderName = ast->linkItems[i].shaderName;
                ast->linkItems[i].resolvedProgramIndex = find_program_index(ast, get_aststring_buffer(ast, programName));
                ast->linkItems[i].resolvedShaderIndex = find_shader_index(ast, get_aststring_buffer(ast, shaderName));
        }
}

void process_ast(struct Ast *ast)
{
        struct PrintInterfacesCtx ctx;
        setup_printInterfaceCtx(&ctx, ast);
        for (int i = 0; i < ast->numFiles; i++) {
                print_interface_of_shaderfile(ast, &ast->shaderfileAsts[i]);
        }
        teardown_printInterfacesCtx(&ctx);
}
