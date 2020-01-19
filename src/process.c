#include <glsl-processor/logging.h>
#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/ast.h>
#include <stdio.h> //XXX

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

void process_ast(struct Ast *ast)
{
        for (int i = 0; i < ast->numLinkItems; i++) {
                AstString programName = ast->linkItems[i].programName;
                AstString shaderName = ast->linkItems[i].shaderName;
                ast->linkItems[i].resolvedProgramIndex = find_program_index(ast, get_aststring_buffer(ast, programName));
                ast->linkItems[i].resolvedShaderIndex = find_shader_index(ast, get_aststring_buffer(ast, shaderName));
        }
        for (int i = 0; i < ast->numFiles; i++) {
                struct ShaderfileAst *fa = &ast->shaderfileAsts[i];
                for (int j = 0; j < fa->numToplevelNodes; j++) {
                        struct ToplevelNode *node = fa->toplevelNodes[j];
                        if (node->directiveKind == DIRECTIVE_UNIFORM) {
                                struct UniformDecl *decl = node->data.tUniform;
                                for (int k = 0; k < ast->numLinkItems; k++) {
                                        struct LinkItem *linkItem = &ast->linkItems[k];
                                        if (linkItem->resolvedShaderIndex == i) {
                                                int programIndex = linkItem->resolvedProgramIndex;
                                                int uniformIndex = ast->numProgramUniforms++;
                                                REALLOC_MEMORY(&ast->programUniforms, ast->numProgramUniforms);
                                                ast->programUniforms[uniformIndex].programIndex = programIndex;
                                                ast->programUniforms[uniformIndex].typeKind = decl->uniDeclTypeExpr->primtypeKind;
                                                ast->programUniforms[uniformIndex].uniformName = get_aststring_buffer(ast, decl->uniDeclName);
                                        }
                                }
                        }
                        else if (node->directiveKind == DIRECTIVE_VARIABLE) {
                                struct VariableDecl *decl = node->data.tVariable;
                                // An attribute is an IN variable in a vertex shader
                                if (ast->shaderDecls[i].shaderType != SHADERTYPE_VERTEX)
                                        continue;
                                if (decl->inOrOut != 0 /* IN */)
                                        continue;
                                for (int k = 0; k < ast->numLinkItems; k++) {
                                        struct LinkItem *linkItem = &ast->linkItems[k];
                                        if (linkItem->resolvedShaderIndex == i) {
                                                int programIndex = linkItem->resolvedProgramIndex;
                                                int attributeIndex = ast->numProgramAttributes++;
                                                REALLOC_MEMORY(&ast->programAttributes, ast->numProgramAttributes);
                                                ast->programAttributes[attributeIndex].programIndex = programIndex;
                                                ast->programAttributes[attributeIndex].typeKind = decl->typeExpr->primtypeKind;
                                                ast->programAttributes[attributeIndex].attributeName = get_aststring_buffer(ast, decl->name);
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
                                const char *programName = get_aststring_buffer(ast, ast->programDecls[ast->programUniforms[i].programIndex].programName);
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
                                const char *programName = get_aststring_buffer(ast, ast->programDecls[ast->programAttributes[i].programIndex].programName);
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

#if 0
        for (int i = 0; i < ast->numProgramUniforms; i++) {
                const char *programName = get_aststring_buffer(ast, ast->programDecls[ast->programUniforms[i].programIndex].programName);
                const char *uniformName = ast->programUniforms[i].uniformName;
                const char *typeName = primtypeKindString[ast->programUniforms[i].typeKind];
                printf("UNIFORM %s %s %s\n", programName, uniformName, typeName);
        }

        for (int i = 0; i < ast->numProgramAttributes; i++) {
                const char *programName = get_aststring_buffer(ast, ast->programDecls[ast->programAttributes[i].programIndex].programName);
                const char *attributeName = ast->programAttributes[i].attributeName;
                const char *typeName = primtypeKindString[ast->programAttributes[i].typeKind];
                printf("ATTRIBUTE %s %s %s\n", programName, attributeName, typeName);
        }
#endif

#if 0
        struct PrintInterfacesCtx ctx;
        setup_printInterfaceCtx(&ctx, ast);
        for (int i = 0; i < ast->numFiles; i++) {
                print_interface_of_shaderfile(ast, i);
        }
        teardown_printInterfacesCtx(&ctx);
#endif
}
