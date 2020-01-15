#include <glsl-processor/defs.h>
#include <glsl-processor/ast.h>
#include <glsl-processor/parse.h>
#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/logging.h>

static void compute_line_and_column(struct Ctx *ctx, int *outLine, int *outColumn)
{
        int line = 1;
        int column = 1;
        for (int i = 0; i < ctx->cursorPos; i++) {
                if (ctx->fileContents[i] == '\n') {
                        line ++;
                        column = 0;
                }
                else {
                        column ++;
                }
        }
        *outLine = line;
        *outColumn = column;
}

void NORETURN _fatal_parse_error_fv(
                struct LogCtx logCtx, struct Ctx *ctx, const char *fmt, va_list ap)
{
        int line;
        int column;
        compute_line_and_column(ctx, &line, &column);
        _fatal_begin(logCtx);
        fatal_write_f("while parsing '%s' at %d:%d: ",
                      ctx->filepath, line, column);
        fatal_write_fv(fmt, ap);
        fatal_end();
}

void NORETURN _fatal_parse_error_f(
                struct LogCtx logCtx, struct Ctx *ctx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _fatal_parse_error_fv(logCtx, ctx, fmt, ap);
        va_end(ap);
}

#define fatal_parse_error_fv(ctx, fmt, ap) \
        _fatal_parse_error_fv(MAKE_LOGCTX(), (ctx), (fmt), (ap))
#define fatal_parse_error_f(ctx, fmt, ...) \
        _fatal_parse_error_f(MAKE_LOGCTX(), (ctx), (fmt), ##__VA_ARGS__)


static int look_character(struct Ctx *ctx)
{
        if (ctx->haveSavedCharacter)
                return ctx->savedCharacter;
        if (ctx->cursorPos == ctx->fileSize)
                return -1;
        int c = ctx->fileContents[ctx->cursorPos];
        ctx->cursorPos++;
        ctx->savedCharacter = c;
        ctx->haveSavedCharacter = 1;
        return c;
}

static void consume_character(struct Ctx *ctx)
{
        ENSURE(ctx->haveSavedCharacter);
        ctx->haveSavedCharacter = 0;
}

static void reset_tokenbuffer(struct Ctx *ctx)
{
        ctx->tokenBufferLength = 0;
}

static void append_to_tokenbuffer(struct Ctx *ctx, int character)
{
        if (ctx->tokenBufferLength + 1 >= ctx->tokenBufferCapacity) {
                int capacity;
                if (ctx->tokenBufferCapacity == 0)
                        capacity = 32;
                else
                        capacity = 2 * ctx->tokenBufferCapacity;
                REALLOC_MEMORY(&ctx->tokenBuffer, capacity);
                ctx->tokenBufferCapacity = capacity;
        }
        int pos = ctx->tokenBufferLength ++;
        ctx->tokenBuffer[pos] = character;
        ctx->tokenBuffer[pos + 1] = '\0';
}

static const struct {
        int character;
        int tokenKind;
} tokInfo1[] = {
        { '(', TOKEN_LEFTPAREN },
        { ')', TOKEN_RIGHTPAREN },
        { '{', TOKEN_LEFTBRACE },
        { '}', TOKEN_RIGHTBRACE },
        { '.', TOKEN_DOT },
        { ',', TOKEN_COMMA },
        { ';', TOKEN_SEMICOLON },
        { '+', TOKEN_PLUS },
        { '-', TOKEN_MINUS },
        { '*', TOKEN_STAR },
        { '/', TOKEN_SLASH },
        { '%', TOKEN_PERCENT },
        { '!', TOKEN_NOT },
};

static const struct {
        int character1;
        int character2;
        int tokenKind1;
        int tokenKind2;
} tokInfo2[] = {
        { '<', '=', TOKEN_LT, TOKEN_LE },
        { '>', '=', TOKEN_GT, TOKEN_GE },
        { '=', '=', TOKEN_EQUALS, TOKEN_DOUBLEEQUALS },
        { '&', '&', TOKEN_AMPERSAND, TOKEN_DOUBLEAMPERSAND },
        { '|', '|', TOKEN_PIPE, TOKEN_DOUBLEPIPE },
};

int look_token(struct Ctx *ctx)
{
        if (ctx->haveSavedToken)
                return 1;
        int c;
skipwhitespace:
        for (;;) {
                c = look_character(ctx);
                if (c > 32)
                        break;
                if (c == -1) {
                        ctx->tokenKind = TOKEN_EOF;
                        return 0;
                }
                consume_character(ctx);
        }
        /* currently we have very crude support for preprocess directives: Just ignoring them, like comments. */
        if (c == '#') {
                consume_character(ctx);
                for (;;) {
                        c = look_character(ctx);
                        if (c == -1)
                                fatal_parse_error_f(ctx,
                                        "EOF encountered while expecting end of preprocessor directive");
                        consume_character(ctx);
                        if (c == '\n')
                                break;
                }
                goto skipwhitespace;
        }
        /* skip comments... */
        if (c == '/') {
                consume_character(ctx);
                c = look_character(ctx);
                if (c == '*') {
                        consume_character(ctx);
                        for (;;) {
                                c = look_character(ctx);
                                if (c == -1) {
                                        fatal_parse_error_f(ctx,
                        "EOF encountered while expecting end of comment");
                                }
                                consume_character(ctx);
                                if (c == '*' && look_character(ctx) == '/') {
                                        consume_character(ctx);
                                        break;
                                }
                        }
                        goto skipwhitespace;
                }
                else if (c == '/') {
                        consume_character(ctx);
                        for (;;) {
                                c = look_character(ctx);
                                if (c == -1) {
                                        // let's not consider that a failure
                                        // here... just back out
                                        goto skipwhitespace;
                                }
                                consume_character(ctx);
                                if (c == '\n') {
                                        goto skipwhitespace;
                                }
                        }
                }
                else {
                        ctx->tokenKind = TOKEN_SLASH;
                }
        }
        else if (('a' <= c && c <= 'z')
            || ('A' <= c && c <= 'Z')
            || (c == '_')) {
                ctx->tokenKind = TOKEN_NAME;
                reset_tokenbuffer(ctx);
                for (;;) {
                        append_to_tokenbuffer(ctx, c);
                        consume_character(ctx);
                        c = look_character(ctx);
                        if (!(('a' <= c && c <= 'z')
                              || ('A' <= c && c <= 'Z')
                              || (c == '_')
                              || ('0' <= c && c <= '9')))
                                break;
                }
        }
        else if ('0' <= c && c <= '9') {
                ctx->tokenKind = TOKEN_LITERAL;
                long long d = c - '0';
                int haveDot = 0;
                int nAfter = 0;
                for (;;) {
                        consume_character(ctx);
                        c = look_character(ctx);
                        if ('0' <= c && c <= '9') {
                                d = 10 * d + c - '0';
                                if (haveDot)
                                        nAfter++;
                        }
                        else if (!haveDot && c == '.')
                                haveDot = 1;
                        else
                                break;
                }
                double floatingValue = (double) d;
                while (nAfter > 0) {
                        nAfter--;
                        floatingValue /= 10.0;
                }
                //message_f("read floating value: %f", floatingValue);
        }
        else if (c == '"') {
                ctx->tokenKind = TOKEN_STRING;
                /* I believe there are no strings in GLSL, but we will
                 * have a use for them... */
                consume_character(ctx);
                reset_tokenbuffer(ctx);
                for (;;) {
                        c = look_character(ctx);
                        if (c == '"') {
                                consume_character(ctx);
                                break;
                        }
                        else if (c == -1) {
                                //message_s("EOF");
                                break;
                        }
                        else {
                                consume_character(ctx);
                                append_to_tokenbuffer(ctx, c);
                        }
                }
        }
        else {
                for (int i = 0; i < LENGTH(tokInfo1); i++) {
                        if (c == tokInfo1[i].character) {
                                consume_character(ctx);
                                ctx->tokenKind = tokInfo1[i].tokenKind;
                                goto ok;
                        }
                }
                for (int i = 0; i < LENGTH(tokInfo2); i++) {
                        if (c == tokInfo2[i].character1) {
                                consume_character(ctx);
                                if (look_character(ctx)
                                    == tokInfo2[i].character2) {
                                        consume_character(ctx);
                                        ctx->tokenKind = tokInfo2[i].tokenKind2;
                                }
                                else {
                                        ctx->tokenKind = tokInfo2[i].tokenKind1;
                                }
                                goto ok;
                        }
                }
                fatal_parse_error_f(ctx,
                                "Failed to lex; initial character: '%c'", c);
ok:
                ;
        }
        ctx->haveSavedToken = 1;
        return 1;
}

void consume_token(struct Ctx *ctx)
{
        ENSURE(ctx->haveSavedToken);
        ctx->haveSavedToken = 0;
}

int is_keyword(struct Ctx *ctx, const char *keyword)
{
        ENSURE(ctx->haveSavedToken);
        if (ctx->tokenKind != TOKEN_NAME)
                return 0;
        return !strcmp(ctx->tokenBuffer, keyword);
}

int is_known_type_name(struct Ctx *ctx)
{
        for (int i = 0; i < NUM_PRIMTYPE_KINDS; i++)
                if (!strcmp(ctx->tokenBuffer, primtypeString[i]))
                        return 1;
        return 0;
}

int is_binop_token(struct Ctx *ctx, int *binopKind)
{
        ENSURE(ctx->haveSavedToken);
        for (int i = 0; i < numBinopTokens; i++) {
                if (ctx->tokenKind == binopTokenInfo[i].tokenKind) {
                        *binopKind = binopTokenInfo[i].binopKind;
                        return 1;
                }
        }
        return 0;
}

static int look_token_kind(struct Ctx *ctx, int tokenKind)
{
        return look_token(ctx) && ctx->tokenKind == tokenKind;
}

static void expect_token_kind(struct Ctx *ctx, int tokenKind)
{
        if (!look_token_kind(ctx, tokenKind))
                fatal_parse_error_f(ctx, "expected '%s' token, found: '%s'",
                                    tokenKindString[tokenKind],
                                    tokenKindString[ctx->tokenKind]);
}

static void parse_simple_token(struct Ctx *ctx, int tokenKind)
{
        expect_token_kind(ctx, tokenKind);
        consume_token(ctx);
}

static void parse_semicolon(struct Ctx *ctx)
{
        parse_simple_token(ctx, TOKEN_SEMICOLON);
}

static AstString parse_name(struct Ctx *ctx)
{
        expect_token_kind(ctx, TOKEN_NAME);
        //message_f("name is '%s'", ctx->tokenBuffer);
        AstString name = create_aststring(ctx->ast, ctx->tokenBuffer);
        consume_token(ctx);
        return name;
}

//XXX: if we detect that this is an interface block, we'll return NULL
static struct TypeExpr *parse_typeexpr(struct Ctx *ctx)
{
        expect_token_kind(ctx, TOKEN_NAME);
        while (is_keyword(ctx, "flat")) {
                // XXX ignoring "flat" specifier for now. Not interesting to us.
                consume_token(ctx);
                expect_token_kind(ctx, TOKEN_NAME);
        }
        for (int i = 0; i < NUM_PRIMTYPE_KINDS; i++) {
                if (is_keyword(ctx, primtypeString[i])) {
                        consume_token(ctx);
                        //message_f("parsed type %s", primtypeInfo[i].name);
                        struct TypeExpr *typeExpr = create_typeexpr(ctx->ast);
                        typeExpr->primtypeKind = i;
                        return typeExpr;
                }
        }
        // maybe this is an interface block...
        consume_token(ctx);
        if (look_token_kind(ctx, TOKEN_LEFTBRACE)) {
                // this is an interface block. Parse it and ignore the contents (for now)
                consume_token(ctx);
                while (!look_token_kind(ctx, TOKEN_RIGHTBRACE)) {
                        //XXX ignoreing stuff for now
                        parse_typeexpr(ctx);
                        parse_name(ctx);
                        parse_semicolon(ctx);
                }
                consume_token(ctx);
                return NULL;
        }
        fatal_parse_error_f(ctx, "type expected or interface block was expected...");
}

static struct TypeExpr *parse_type_or_void(struct Ctx *ctx)
{
        expect_token_kind(ctx, TOKEN_NAME);
        if (is_keyword(ctx, "void")) {
                consume_token(ctx);
                struct TypeExpr *typeExpr = create_typeexpr(ctx->ast);
                typeExpr->primtypeKind = -1;
                return typeExpr;
        }
        return parse_typeexpr(ctx);
}

static struct VariableDecl *parse_variable(struct Ctx *ctx, int inOrOut)
{
        consume_token(ctx); // "in" or "out"
        struct TypeExpr *typeExpr = parse_typeexpr(ctx);
        // XXX WARNING currently parse_typeexpr() may return NULL, which means that this was an interface block. Is it safe to proceed?
        AstString name = parse_name(ctx);
        parse_semicolon(ctx);
        struct VariableDecl *variableDecl = create_variabledecl(ctx->ast);
        variableDecl->inOrOut = inOrOut;
        variableDecl->name = name;
        variableDecl->typeExpr = typeExpr;
        return variableDecl;
}

static struct UniformDecl *parse_uniform(struct Ctx *ctx)
{
        consume_token(ctx); // "uniform"
        struct TypeExpr *typeExpr = parse_typeexpr(ctx);
        // currently parse_typeexpr may return NULL, but this is not valid for uniforms.
        if (typeExpr == NULL)
                fatal_parse_error_f(ctx, "Can't use an interface block as a type for a uniform.");
        AstString name = parse_name(ctx);
        parse_semicolon(ctx);
        struct UniformDecl *uniformDecl = create_uniformdecl(ctx->ast);
        uniformDecl->uniDeclName = name;
        uniformDecl->uniDeclTypeExpr = typeExpr;
        //printf("parse uniform (%s) %s %s\n", ctx->filepath, get_aststring_buffer(ctx->ast, name), primtypeKindString[typeExpr->primtypeKind]);
        return uniformDecl;
}

static void parse_expression(struct Ctx *ctx)
{
        if (!look_token(ctx))
                fatal_parse_error_f(ctx, "Expected expression");
        if (ctx->tokenKind == TOKEN_NAME) {
                consume_token(ctx);
        }
        else if (ctx->tokenKind == TOKEN_LITERAL) {
                consume_token(ctx);
        }
        else if (ctx->tokenKind == TOKEN_LEFTPAREN) {
                consume_token(ctx);
                parse_expression(ctx);
                parse_simple_token(ctx, TOKEN_RIGHTPAREN);
        }
        else if (ctx->tokenKind == TOKEN_NOT) {
                consume_token(ctx);
                parse_expression(ctx);
        }
        else {
                fatal_parse_error_f(ctx, "Expected expression");
        }
        for (;;) {
                // function call?
                if (look_token_kind(ctx, TOKEN_LEFTPAREN)) {
                        consume_token(ctx);
                        if (!look_token_kind(ctx, TOKEN_RIGHTPAREN)) {
                                for (;;) {
                                        parse_expression(ctx);
                                        if (!look_token_kind(ctx, TOKEN_COMMA))
                                                break;
                                        consume_token(ctx);
                                }
                        }
                        parse_simple_token(ctx, TOKEN_RIGHTPAREN);
                }
                // member descend?
                else if (look_token_kind(ctx, TOKEN_DOT)) {
                        consume_token(ctx);
                        parse_name(ctx);
                }
                else {
                        break;
                }
        }
        // binary operators
        int binopKind;
        if (look_token(ctx) && is_binop_token(ctx, &binopKind)) {
                consume_token(ctx);
                //message_f("Found '%s' binop", binopInfo[binopKind].text);
                parse_expression(ctx);
        }
}

static void parse_stmt(struct Ctx *ctx); // forward declare: recursion

static void parse_compound_stmt(struct Ctx *ctx)
{
        parse_simple_token(ctx, TOKEN_LEFTBRACE);
        while (!look_token_kind(ctx, TOKEN_RIGHTBRACE))
                parse_stmt(ctx);
        parse_simple_token(ctx, TOKEN_RIGHTBRACE);
}

static void parse_variable_declaration_stmt(struct Ctx *ctx)
{
        parse_typeexpr(ctx);
        parse_name(ctx);
        if (look_token_kind(ctx, TOKEN_EQUALS)) {
                consume_token(ctx);
                parse_expression(ctx);
                parse_semicolon(ctx);
        }
}

static void parse_if_stmt(struct Ctx *ctx)
{
        consume_token(ctx); // "if"
        parse_simple_token(ctx, TOKEN_LEFTPAREN);
        parse_expression(ctx);
        parse_simple_token(ctx, TOKEN_RIGHTPAREN);
        parse_stmt(ctx);
        if (look_token_kind(ctx, TOKEN_NAME) && is_keyword(ctx, "else")) {
                consume_token(ctx); // "if"
                parse_stmt(ctx);
        }
}

static void parse_return_stmt(struct Ctx *ctx)
{
        consume_token(ctx); // "return"
        parse_expression(ctx);
        parse_semicolon(ctx);
}

static void parse_discard_stmt(struct Ctx *ctx)
{
        consume_token(ctx); // "discard"
        parse_semicolon(ctx);
}

static void parse_expression_stmt(struct Ctx *ctx)
{
        parse_expression(ctx);
        parse_semicolon(ctx);
}

static void parse_stmt(struct Ctx *ctx)
{
        if (!look_token(ctx))
                fatal_parse_error_f(ctx, "Expected statement");
        if (ctx->tokenKind == TOKEN_LEFTBRACE)
                parse_compound_stmt(ctx);
        else if (is_known_type_name(ctx))
                parse_variable_declaration_stmt(ctx);
        else if (is_keyword(ctx, "if"))
                parse_if_stmt(ctx);
        else if (is_keyword(ctx, "return"))
                parse_return_stmt(ctx);
        else if (is_keyword(ctx, "discard"))
                parse_discard_stmt(ctx);
        else
                parse_expression_stmt(ctx);
}

static void parse_FuncDefn_or_FuncDecl(struct Ctx *ctx)
{
        //message_f("Function!", ctx->tokenBufferLength);
        struct TypeExpr *returnTypeExpr = parse_type_or_void(ctx);
        AstString name = parse_name(ctx);
        parse_simple_token(ctx, TOKEN_LEFTPAREN);
        int numArgs = 0;
        AstString *argNames = NULL;
        struct TypeExpr **argTypeExprs = NULL;
        if (!look_token_kind(ctx, TOKEN_RIGHTPAREN)) {
                for (;;) {
                        numArgs++;
                        REALLOC_MEMORY(&argTypeExprs, numArgs);
                        REALLOC_MEMORY(&argNames, numArgs);
                        argTypeExprs[numArgs - 1] = parse_typeexpr(ctx);
                        argNames[numArgs - 1] = parse_name(ctx);
                        if (!look_token_kind(ctx, TOKEN_COMMA))
                                break;
                        consume_token(ctx);
                }
        }
        parse_simple_token(ctx, TOKEN_RIGHTPAREN);
        if (look_token_kind(ctx, TOKEN_SEMICOLON)) {
                // it's only a decl
                consume_token(ctx);
                struct FuncDecl *funcDecl = create_funcdecl(ctx->ast);
                funcDecl->name = name;
                funcDecl->returnTypeExpr = returnTypeExpr;
                funcDecl->argTypeExprs = argTypeExprs;
                funcDecl->argNames = argNames;
                funcDecl->numArgs = numArgs;
                struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                node->directiveKind = DIRECTIVE_FUNCDECL;
                node->data.tFuncdecl = funcDecl;
        }
        else {
                parse_simple_token(ctx, TOKEN_LEFTBRACE);
                while (!look_token_kind(ctx, TOKEN_RIGHTBRACE))
                        parse_stmt(ctx);
                parse_simple_token(ctx, TOKEN_RIGHTBRACE);

                struct FuncDefn *funcDefn = create_funcdefn(ctx->ast);
                funcDefn->name = name;
                funcDefn->returnTypeExpr = returnTypeExpr;
                funcDefn->argTypeExprs = argTypeExprs;
                funcDefn->argNames = argNames;
                funcDefn->numArgs = numArgs;
                struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                node->directiveKind = DIRECTIVE_FUNCDEFN;
                node->data.tFuncdefn = funcDefn;
                /*
                */
        }
}

static void parse(struct Ctx *ctx)
{
        while (look_token(ctx)) {
                if (is_keyword(ctx, "uniform")) {
                        struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                        node->directiveKind = DIRECTIVE_UNIFORM;
                        node->data.tUniform = parse_uniform(ctx);
                }
                else if (is_keyword(ctx, "in")) {
                        struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                        node->directiveKind = DIRECTIVE_VARIABLE;
                        node->data.tVariable = parse_variable(ctx, 0);
                }
                else if (is_keyword(ctx, "out")) {
                        struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                        node->directiveKind = DIRECTIVE_VARIABLE;
                        node->data.tVariable = parse_variable(ctx, 1);
                }
                else if (ctx->tokenKind == TOKEN_NAME) {
                        parse_FuncDefn_or_FuncDecl(ctx);
                }
                else
                        message_f("Unexpected token type %s!",
                                  tokenKindString[ctx->tokenKind]);
        }
}

void parse_next_file(struct Ctx *ctx, const char *filepath, const char *fileContents, int fileSize)
{
        ctx->filepath = filepath;
        ctx->fileContents = fileContents;
        ctx->fileSize = fileSize;

        ctx->cursorPos = 0;
        ctx->savedCharacter = -1;
        ctx->haveSavedCharacter = 0;

        ctx->haveSavedToken = 0;
        ctx->tokenKind = TOKEN_EOF;  // this is always valid. That's nice for error printing
        ctx->tokenBuffer = NULL;
        ctx->tokenBufferLength = 0;
        ctx->tokenBufferCapacity = 0;

        add_file_to_ast_and_switch_to_it(ctx->ast, filepath);
        parse(ctx);
}

void setup_ctx(struct Ctx *ctx, struct Ast *ast)
{
        memset(ctx, 0, sizeof *ctx);
        ctx->ast = ast;
}

void teardown_ctx(struct Ctx *ctx)
{
        FREE_MEMORY(&ctx->tokenBuffer);
        memset(ctx, 0, sizeof *ctx);
}
