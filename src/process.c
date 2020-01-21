#include <glsl-processor/logging.h>
#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/ast.h>
#include <stdio.h> //XXX

static int gp_compare_ProgramUniforms(const void *a, const void *b)
{
        const struct GP_ProgramUniform *x = a;
        const struct GP_ProgramUniform *y = b;
        if (x->programIndex != y->programIndex)
                return (x->programIndex > y->programIndex) - (x->programIndex < y->programIndex);
        return strcmp(x->uniformName, y->uniformName);
}

static int gp_compare_ProgramAttributes(const void *a, const void *b)
{
        const struct GP_ProgramAttribute *x = a;
        const struct GP_ProgramAttribute *y = b;
        if (x->programIndex != y->programIndex)
                return (x->programIndex > y->programIndex) - (x->programIndex < y->programIndex);
        return strcmp(x->attributeName, y->attributeName);
}

void gp_process(struct GP_Ctx *ctx)
{
        for (int i = 0; i < ctx->numFiles; i++) {
                struct GP_ShaderfileAst *fa = &ctx->shaderfileAsts[i];
                for (int j = 0; j < fa->numToplevelNodes; j++) {
                        struct GP_ToplevelNode *node = fa->toplevelNodes[j];
                        if (node->directiveKind == GP_DIRECTIVE_UNIFORM) {
                                struct GP_UniformDecl *decl = node->data.tUniform;
                                for (int k = 0; k < ctx->numLinks; k++) {
                                        struct GP_LinkInfo *linkInfo = &ctx->linkInfo[k];
                                        if (linkInfo->shaderIndex == i) {
                                                int programIndex = linkInfo->programIndex;
                                                int uniformIndex = ctx->numProgramUniforms++;
                                                REALLOC_MEMORY(&ctx->programUniforms, ctx->numProgramUniforms);
                                                ctx->programUniforms[uniformIndex].programIndex = programIndex;
                                                ctx->programUniforms[uniformIndex].typeKind = decl->uniDeclTypeExpr->primtypeKind;
                                                ctx->programUniforms[uniformIndex].uniformName = decl->uniDeclName;
                                        }
                                }
                        }
                        else if (node->directiveKind == GP_DIRECTIVE_VARIABLE) {
                                struct GP_VariableDecl *decl = node->data.tVariable;
                                // An attribute is an IN variable in a vertex shader
                                if (ctx->shaderInfo[i].shaderType != GP_SHADERTYPE_VERTEX)
                                        continue;
                                if (decl->inOrOut != 0 /* IN */)
                                        continue;
                                for (int k = 0; k < ctx->numLinks; k++) {
                                        struct GP_LinkInfo *linkInfo = &ctx->linkInfo[k];
                                        if (linkInfo->shaderIndex == i) {
                                                int programIndex = linkInfo->programIndex;
                                                int attributeIndex = ctx->numProgramAttributes++;
                                                REALLOC_MEMORY(&ctx->programAttributes, ctx->numProgramAttributes);
                                                ctx->programAttributes[attributeIndex].programIndex = programIndex;
                                                ctx->programAttributes[attributeIndex].typeKind = decl->typeExpr->primtypeKind;
                                                ctx->programAttributes[attributeIndex].attributeName = decl->name;
                                        }
                                }
                        }
                }
        }

        qsort(ctx->programUniforms, ctx->numProgramUniforms, sizeof *ctx->programUniforms, gp_compare_ProgramUniforms);
        qsort(ctx->programAttributes, ctx->numProgramAttributes, sizeof *ctx->programAttributes, gp_compare_ProgramAttributes);

        int j = 0;
        for (int i = 0; i < ctx->numProgramUniforms; i++) {
                if (j > 0
                        && ctx->programUniforms[i].programIndex == ctx->programUniforms[j-1].programIndex
                        && !strcmp(ctx->programUniforms[i].uniformName, ctx->programUniforms[j-1].uniformName)) {
                        if (ctx->programUniforms[i].typeKind != ctx->programUniforms[j-1].typeKind) {
                                const char *programName = ctx->programInfo[ctx->programUniforms[i].programIndex].programName;
                                const char *uniformName = ctx->programUniforms[i].uniformName;
                                fatal_f("The shader program '%s' cannot be linked since there are multiple uniforms '%s' with incompatible types",
                                        programName, uniformName);
                        }
                }
                else {
                        ctx->programUniforms[j] = ctx->programUniforms[i];
                        j++;
                }
        }
        ctx->numProgramUniforms = j;

        j = 0;
        for (int i = 0; i < ctx->numProgramAttributes; i++) {
                if (j > 0
                        && ctx->programAttributes[i].programIndex == ctx->programAttributes[j-1].programIndex
                        && !strcmp(ctx->programAttributes[i].attributeName, ctx->programAttributes[j-1].attributeName)) {
                        if (ctx->programAttributes[i].typeKind != ctx->programAttributes[j-1].typeKind) {
                                const char *programName = ctx->programInfo[ctx->programAttributes[i].programIndex].programName;
                                const char *attributeName = ctx->programAttributes[i].attributeName;
                                fatal_f("The shader program '%s' cannot be linked since there are multiple attributes '%s' with incompatible types.",
                                        programName, attributeName);
                        }
                }
                else {
                        ctx->programAttributes[j] = ctx->programAttributes[i];
                        j++;
                }
        }
        ctx->numProgramAttributes = j;

        /* TODO: I guess it's not allowed to have a uniform and a variable by the same name? */
}
