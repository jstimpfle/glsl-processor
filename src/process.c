#include <glsl-compiler/logging.h>
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

void process_ast(struct Ast *ast)
{
        for (int i = 0; i < ast->numFiles; i++) {
                struct ShaderfileAst *fa = &ast->shaderfileAsts[i];
                for (int j = 0; j < fa->numToplevelNodes; j++) {
                        struct ToplevelNode *node = fa->toplevelNodes[j];
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
}
