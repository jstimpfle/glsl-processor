#include <glsl-compiler/logging.h>
#include <glsl-compiler/memoryalloc.h>
#include <glsl-compiler/ast.h>
#include <stdio.h> //XXX

static void print_type(struct Ast *ast, struct TypeExpr *typeExpr)
{
        if (typeExpr == NULL)
                // we should not handle this but I need to figure out what's happening right now while I'm writing this comment.
                printf("(WARNING: type missing???)");
        else if (typeExpr->primtypeKind == -1) //XXX
                printf("void");
        else
                printf("%s", primtypeString[typeExpr->primtypeKind]);
}

void print_interface_of_shaderfile(struct Ast *ast, int shaderIndex)
{
        struct ShaderDecl *shaderDecl = &ast->shaderDecls[shaderIndex];
        struct ShaderfileAst *fa = &ast->shaderfileAsts[shaderIndex];
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
                else if (node->directiveKind == DIRECTIVE_VARIABLE) {
                        struct VariableDecl *vdecl = node->data.tVariable;
                        if (ast->shaderDecls[shaderIndex].shaderType != SHADERTYPE_VERTEX)
                                continue;  // attributes can occur only in vertex shaders.
                        if (vdecl->inOrOut != 0) /* TODO: enum for this. Only "in" variable in the vertex shaders are attributes. */
                                continue;
                        if (vdecl->typeExpr == NULL) // this should only happen if the type was an interface block.
                                fatal_f("File %s is a %s, so is not allowed to contain an interface block",
                                        get_aststring_buffer(ast, shaderDecl->shaderFilepath),
                                        shadertypeKindString[shaderDecl->shaderType]);
                        AstString name = vdecl->name;
                        struct TypeExpr *typeExpr = vdecl->typeExpr;
                        const char *nameBuffer = get_aststring_buffer(ast, name);
                        printf("%s: attribute ", fa->filepath);
                        printf("%s ", vdecl->inOrOut ? "out" : "in");
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

/*
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
*/

void process_ast(struct Ast *ast)
{
        struct PrintInterfacesCtx ctx;
        setup_printInterfaceCtx(&ctx, ast);
        for (int i = 0; i < ast->numFiles; i++) {
                print_interface_of_shaderfile(ast, i);
        }
        teardown_printInterfacesCtx(&ctx);
}
