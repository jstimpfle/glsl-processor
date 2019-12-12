#include <glsl-compiler/ast.h>
#include <stdio.h> //XXX

static void print_type(struct Ast *ast, struct TypeExpr *typeExpr)
{
        printf(primtypeInfo[typeExpr->primtypeKind].name);
}

void process_ast(struct Ast *ast)
{
        for (int i = 0; i < ast->numToplevelNodes; i++) {
                struct ToplevelNode *node = ast->toplevelNodes[i];
                if (node->directiveKind == DIRECTIVE_UNIFORM) {
                        AstName name = node->data.tUniform->uniDeclName;                        
                        struct TypeExpr *typeExpr = node->data.tUniform->uniDeclTypeExpr;
                        const char *nameBuffer = get_astname_buffer(ast, name);
                        printf("uniform ");
                        print_type(ast, typeExpr);
                        printf(" %s;\n", nameBuffer);
                }
        }
}