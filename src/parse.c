#include <glsl-compiler/defs.h>
#include <glsl-compiler/ast.h>
#include <glsl-compiler/parse.h>
#include <glsl-compiler/memoryalloc.h>
#include <glsl-compiler/logging.h>

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

static struct {
        int character;
        int tokenKind;
} tokInfo1[] = {
        { '(', TOKEN_LEFTPAREN },
        { ')', TOKEN_RIGHTPAREN },
        { '{', TOKEN_LEFTBRACE },
        { '}', TOKEN_RIGHTBRACE },
        { ',', TOKEN_COMMA },
        { ';', TOKEN_SEMICOLON },
        { '+', TOKEN_PLUS },
        { '-', TOKEN_MINUS },
        { '/', TOKEN_SLASH },
        { '*', TOKEN_STAR },
        { '=', TOKEN_EQUALS },
};

static int look_token(struct Ctx *ctx)
{
        if (ctx->haveSavedToken)
                return 1;
        int c;
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
        if (('a' <= c && c <= 'z')
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
                message_f("read floating value: %f", floatingValue);
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
                                message_s("EOF");
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
                fatal_f("Failed to lex; initial character: '%c'", c);
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

void compute_line_and_column(struct Ctx *ctx, int *outLine, int *outColumn)
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

void _expect_name(struct LogCtx logCtx, struct Ctx *ctx)
{
        if (!look_token(ctx) || ctx->tokenKind != TOKEN_NAME)
                _fatal_parse_error_f(logCtx, ctx, "expected name token");
}

#define expect_name(ctx) _expect_name(MAKE_LOGCTX(), (ctx))

static int look_simple_token(struct Ctx *ctx, int tokenKind)
{
        return look_token(ctx) && ctx->tokenKind == tokenKind;
}

static void parse_simple_token(struct Ctx *ctx, int tokenKind)
{
        if (!look_token(ctx) || ctx->tokenKind != tokenKind)
                fatal_parse_error_f(ctx, "Expected %s token, got: %s",
                                    tokenKindString[tokenKind],
                                    tokenKindString[ctx->tokenKind]);
        consume_token(ctx);
}

static void parse_comma(struct Ctx *ctx)
{
        parse_simple_token(ctx, TOKEN_COMMA);
}

static void parse_semicolon(struct Ctx *ctx)
{
        parse_simple_token(ctx, TOKEN_SEMICOLON);
}

struct TypeExpr *parse_typeexpr(struct Ctx *ctx)
{
        expect_name(ctx);
        for (int i = 0; i < NUM_PRIMTYPE_KINDS; i++) {
                if (is_keyword(ctx, primtypeInfo[i].name)) {
                        consume_token(ctx);
                        message_f("parsed type %s", primtypeInfo[i].name);
                        struct TypeExpr *typeExpr = create_typeexpr(ctx->ast);
                        typeExpr->primtypeKind = i;
                        return typeExpr;
                }
        }
        fatal_parse_error_f(ctx, "type expected, but found '%s'",
                            ctx->tokenBuffer);
}

struct TypeExpr *parse_type_or_void(struct Ctx *ctx)
{
        expect_name(ctx);
        if (is_keyword(ctx, "void")) {
                consume_token(ctx);
                struct TypeExpr *typeExpr = create_typeexpr(ctx->ast);
                typeExpr->primtypeKind = -1;
                return typeExpr;
        }
        return parse_typeexpr(ctx);
}

static AstName parse_name(struct Ctx *ctx)
{
        if (!look_token(ctx) || ctx->tokenKind != TOKEN_NAME)
                fatal_parse_error_f(ctx, "a name was expected");
        message_f("name is '%s'", ctx->tokenBuffer);
        AstName name = create_astname(ctx->ast, ctx->tokenBuffer);
        consume_token(ctx);
        return name;
}

static struct AttributeDecl *parse_varying(struct Ctx *ctx, int inOrOut)
{
        consume_token(ctx); // "in" or "out"
        struct TypeExpr *typeExpr = parse_typeexpr(ctx);
        AstName name = parse_name(ctx);
        parse_semicolon(ctx);
        struct AttributeDecl *attributeDecl = create_attributedecl(ctx->ast);
        attributeDecl->inOrOut = inOrOut;
        attributeDecl->name = name;
        attributeDecl->typeExpr = typeExpr;
        return attributeDecl;
}

static struct UniformDecl *parse_uniform(struct Ctx *ctx)
{
        consume_token(ctx); // uniform
        int startPosition = ctx->cursorPos;
        struct TypeExpr *typeExpr = parse_typeexpr(ctx);
        AstName name = parse_name(ctx);
        parse_semicolon(ctx);
        int endPosition = ctx->cursorPos;
        /*
        message_begin();
        message_write_f("uniform ");
        message_write(ctx->fileContents + startPosition,
                      endPosition - startPosition);
        message_end();
        */
        struct UniformDecl *uniformDecl = create_uniformdecl(ctx->ast);
        uniformDecl->uniDeclName = name;
        uniformDecl->uniDeclTypeExpr = typeExpr;
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
        // binary operators
        int binopKind;
        if (look_token(ctx) && is_binop_token(ctx, &binopKind)) {
                consume_token(ctx);
                message_f("Found '%s' binop", binopInfo[binopKind].text);
                parse_expression(ctx);
        }
}

static void parse_stmt(struct Ctx *ctx); // forward declare: recursion

static void parse_if_stmt(struct Ctx *ctx)
{
        consume_token(ctx); // "if"
        parse_simple_token(ctx, TOKEN_LEFTPAREN);
        parse_expression(ctx);
        parse_simple_token(ctx, TOKEN_RIGHTPAREN);
        parse_stmt(ctx);
        if (look_simple_token(ctx, TOKEN_NAME) && is_keyword(ctx, "else")) {
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
        if (is_keyword(ctx, "if"))
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
        int startCursorPos = ctx->cursorPos - ctx->tokenBufferLength - 1; // hack
        message_f("Function!", ctx->tokenBufferLength);
        struct TypeExpr *returnTypeExpr = parse_type_or_void(ctx);
        AstName name = parse_name(ctx);
        parse_simple_token(ctx, TOKEN_LEFTPAREN);
        int numArgs = 0;
        AstName *argNames = NULL;
        struct TypeExpr **argTypeExprs = NULL;
        if (!look_simple_token(ctx, TOKEN_RIGHTPAREN)) {
                for (;;) {
                        numArgs++;
                        REALLOC_MEMORY(&argTypeExprs, numArgs);
                        REALLOC_MEMORY(&argNames, numArgs);
                        argTypeExprs[numArgs - 1] = parse_typeexpr(ctx);
                        argNames[numArgs - 1] = parse_name(ctx);
                        if (!look_simple_token(ctx, TOKEN_COMMA))
                                break;
                        consume_token(ctx);
                }
        }
        parse_simple_token(ctx, TOKEN_RIGHTPAREN);
        int endCursorPos = ctx->cursorPos;
        if (look_simple_token(ctx, TOKEN_SEMICOLON)) {
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
                // HACK: try two write out prototype without much fuss
                message_begin();
                message_write_f("FUNCTION ");
                message_write(ctx->fileContents + startCursorPos,
                              endCursorPos - startCursorPos);
                message_write_f(";");
                message_end();

                parse_simple_token(ctx, TOKEN_LEFTBRACE);
                while (!look_simple_token(ctx, TOKEN_RIGHTBRACE))
                        parse_stmt(ctx);
                parse_simple_token(ctx, TOKEN_RIGHTBRACE);
        }
}

void parse(struct Ctx *ctx)
{
        while (look_token(ctx)) {
                if (is_keyword(ctx, "uniform")) {
                        struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                        node->directiveKind = DIRECTIVE_UNIFORM;
                        node->data.tUniform = parse_uniform(ctx);
                }
                else if (is_keyword(ctx, "in")) {
                        struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                        node->directiveKind = DIRECTIVE_ATTRIBUTE;
                        node->data.tAttribute = parse_varying(ctx, 0);
                }
                else if (is_keyword(ctx, "out")) {
                        struct ToplevelNode *node = add_new_toplevel_node(ctx->ast);
                        node->directiveKind = DIRECTIVE_ATTRIBUTE;
                        node->data.tAttribute = parse_varying(ctx, 1);
                }
                else if (ctx->tokenKind == TOKEN_NAME) {
                        parse_FuncDefn_or_FuncDecl(ctx);                        
                }
                else
                        message_f("Unexpected token type %s!",
                                  tokenKindString[ctx->tokenKind]);
        }
}

void setup_ctx(struct Ctx *ctx, const char *filepath, const char *fileContents, int fileSize)
{
        ALLOC_MEMORY(&ctx->ast, 1);
        memset(ctx->ast, 0, sizeof *ctx->ast);

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
}

void teardown_ctx(struct Ctx *ctx)
{
        FREE_MEMORY(&ctx->tokenBuffer);
}
