#include <glsl-processor/logging.h>
#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/ast.h>
#include <stdio.h> //XXX

#if 0
static void print_type(struct Ast *ast, struct TypeExpr *typeExpr)
{
        if (typeExpr == NULL)
                // we should not handle this but I need to figure out what's happening right now while I'm writing this comment.
                printf("(WARNING: type missing?)");
        else if (typeExpr->primtypeKind == -1) //XXX
                printf("void");
        else
                printf("%s", primtypeString[typeExpr->primtypeKind]);
}
#endif

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

int compare_ProgramUniforms(const void *a, const void *b)
{
        const struct ProgramUniform *x = a;
        const struct ProgramUniform *y = b;
        if (x->programIndex != y->programIndex)
                return (x->programIndex > y->programIndex) - (x->programIndex < y->programIndex);
        return strcmp(x->uniformName, y->uniformName);
}

int compare_ProgramAttributes(const void *a, const void *b)
{
        const struct ProgramAttribute *x = a;
        const struct ProgramAttribute *y = b;
        if (x->programIndex != y->programIndex)
                return (x->programIndex > y->programIndex) - (x->programIndex < y->programIndex);
        return strcmp(x->attributeName, y->attributeName);
}

void process_ast(struct Ast *ast)
{
        for (int i = 0; i < ast->numFiles; i++) {
                struct ShaderfileAst *fa = &ast->shaderfileAsts[i];
                for (int j = 0; j < fa->numToplevelNodes; j++) {
                        struct ToplevelNode *node = fa->toplevelNodes[j];
                        if (node->directiveKind == DIRECTIVE_UNIFORM) {
                                struct UniformDecl *decl = node->data.tUniform;
                                for (int k = 0; k < ast->numLinks; k++) {
                                        struct LinkInfo *linkInfo = &ast->linkInfo[k];
                                        if (linkInfo->shaderIndex == i) {
                                                int programIndex = linkInfo->programIndex;
                                                int uniformIndex = ast->numProgramUniforms++;
                                                REALLOC_MEMORY(&ast->programUniforms, ast->numProgramUniforms);
                                                ast->programUniforms[uniformIndex].programIndex = programIndex;
                                                ast->programUniforms[uniformIndex].typeKind = decl->uniDeclTypeExpr->primtypeKind;
                                                ast->programUniforms[uniformIndex].uniformName = decl->uniDeclName;
                                        }
                                }
                        }
                        else if (node->directiveKind == DIRECTIVE_VARIABLE) {
                                struct VariableDecl *decl = node->data.tVariable;
                                // An attribute is an IN variable in a vertex shader
                                if (ast->shaderInfo[i].shaderType != SHADERTYPE_VERTEX)
                                        continue;
                                if (decl->inOrOut != 0 /* IN */)
                                        continue;
                                for (int k = 0; k < ast->numLinks; k++) {
                                        struct LinkInfo *linkInfo = &ast->linkInfo[k];
                                        if (linkInfo->shaderIndex == i) {
                                                int programIndex = linkInfo->programIndex;
                                                int attributeIndex = ast->numProgramAttributes++;
                                                REALLOC_MEMORY(&ast->programAttributes, ast->numProgramAttributes);
                                                ast->programAttributes[attributeIndex].programIndex = programIndex;
                                                ast->programAttributes[attributeIndex].typeKind = decl->typeExpr->primtypeKind;
                                                ast->programAttributes[attributeIndex].attributeName = decl->name;
                                        }
                                }
                        }
                }
        }

        qsort(ast->programUniforms, ast->numProgramUniforms, sizeof *ast->programUniforms, compare_ProgramUniforms);
        qsort(ast->programAttributes, ast->numProgramAttributes, sizeof *ast->programAttributes, compare_ProgramAttributes);

        int j = 0;
        for (int i = 0; i < ast->numProgramUniforms; i++) {
                if (j > 0
                        && ast->programUniforms[i].programIndex == ast->programUniforms[j-1].programIndex
                        && !strcmp(ast->programUniforms[i].uniformName, ast->programUniforms[j-1].uniformName)) {
                        if (ast->programUniforms[i].typeKind != ast->programUniforms[j-1].typeKind) {
                                const char *programName = ast->programInfo[ast->programUniforms[i].programIndex].programName;
                                const char *uniformName = ast->programUniforms[i].uniformName;
                                fatal_f("The shader program '%s' cannot be linked since there are multiple uniforms '%s' with incompatible types",
                                        programName, uniformName);
                        }
                }
                else {
                        ast->programUniforms[j] = ast->programUniforms[i];
                        j++;
                }
        }
        ast->numProgramUniforms = j;

        j = 0;
        for (int i = 0; i < ast->numProgramAttributes; i++) {
                if (j > 0
                        && ast->programAttributes[i].programIndex == ast->programAttributes[j-1].programIndex
                        && !strcmp(ast->programAttributes[i].attributeName, ast->programAttributes[j-1].attributeName)) {
                        if (ast->programAttributes[i].typeKind != ast->programAttributes[j-1].typeKind) {
                                const char *programName = ast->programInfo[ast->programAttributes[i].programIndex].programName;
                                const char *attributeName = ast->programAttributes[i].attributeName;
                                fatal_f("The shader program '%s' cannot be linked since there are multiple attributes '%s' with incompatible types.",
                                        programName, attributeName);
                        }
                }
                else {
                        ast->programAttributes[j] = ast->programAttributes[i];
                        j++;
                }
        }
        ast->numProgramAttributes = j;

        /* TODO: I guess it's not allowed to have a uniform and a variable by the same name? */
}
