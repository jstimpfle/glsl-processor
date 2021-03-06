#include <glsl-processor/defs.h>
#include <glsl-processor/ast.h>
#include <glsl-processor/parse.h>
#include <glsl-processor/memory.h>
#include <glsl-processor/logging.h>

static const struct {
        int character;
        int tokenKind;
} tokInfo1[] = {
        { '#', GP_TOKEN_HASH },
        { '(', GP_TOKEN_LEFTPAREN },
        { ')', GP_TOKEN_RIGHTPAREN },
        { '{', GP_TOKEN_LEFTBRACE },
        { '}', GP_TOKEN_RIGHTBRACE },
        { '.', GP_TOKEN_DOT },
        { ',', GP_TOKEN_COMMA },
        { ';', GP_TOKEN_SEMICOLON },
        { '%', GP_TOKEN_PERCENT },
        { '!', GP_TOKEN_NOT },
};

static const struct {
        int character1;
        int character2;
        int tokenKind1;
        int tokenKind2;
} tokInfo2[] = {
        { '<', '=', GP_TOKEN_LT, GP_TOKEN_LE },
        { '>', '=', GP_TOKEN_GT, GP_TOKEN_GE },
        { '=', '=', GP_TOKEN_EQUALS, GP_TOKEN_DOUBLEEQUALS },
        { '&', '&', GP_TOKEN_AMPERSAND, GP_TOKEN_DOUBLEAMPERSAND },
        { '|', '|', GP_TOKEN_PIPE, GP_TOKEN_DOUBLEPIPE },
        { '+', '=', GP_TOKEN_PLUS, GP_TOKEN_PLUSEQUALS },
        { '-', '=', GP_TOKEN_MINUS, GP_TOKEN_MINUSEQUALS },
        { '*', '=', GP_TOKEN_STAR, GP_TOKEN_STAREQUALS },
        { '/', '=', GP_TOKEN_SLASH, GP_TOKEN_SLASHEQUALS },
};

static void compute_line_and_column(struct GP_Ctx *ctx, int *outLine, int *outColumn)
{
        int line = 1;
        int column = 1;
        for (int i = 0; i < ctx->file.cursorPos; i++) {
                if (ctx->file.contents[i] == '\n') {
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

static void NORETURN _gp_fatal_parse_error_fv(
                struct GP_LogCtx logCtx, struct GP_Ctx *ctx, const char *fmt, va_list ap)
{
        int line;
        int column;
        compute_line_and_column(ctx, &line, &column);
        _gp_fatal_begin(logCtx);
        gp_fatal_write_f("while parsing '%s' at %d:%d: ",
                      ctx->file.fileID, line, column);
        gp_fatal_write_fv(fmt, ap);
        gp_fatal_end();
}

static void NORETURN _gp_fatal_parse_error_f(
                struct GP_LogCtx logCtx, struct GP_Ctx *ctx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _gp_fatal_parse_error_fv(logCtx, ctx, fmt, ap);
        va_end(ap);
}

#define gp_fatal_parse_error_fv(ctx, fmt, ap) \
        _gp_fatal_parse_error_fv(GP_MAKE_LOGCTX(), (ctx), (fmt), (ap))
#define gp_fatal_parse_error_f(ctx, fmt, ...) \
        _gp_fatal_parse_error_f(GP_MAKE_LOGCTX(), (ctx), (fmt), ##__VA_ARGS__)

/* Move the cursor that indicates the currently processed file position.
 * If copying is not currently suspended, the (forward) range that is described
 * by the move will be copied to the output buffer. */
static void copy_remaining_bytes(struct GP_Ctx *ctx)
{
        if (!ctx->file.outputSuspended) {
                // output range from current file
                int startOffset = ctx->file.outputFilePosition;
                int endOffset = ctx->file.indexOfFirstUnconsumedToken;
                int size = endOffset - startOffset;
                struct GP_ShaderfileAst *fa = &ctx->shaderfileAsts[ctx->currentShaderIndex];
                int idx = fa->outputSize;
                fa->outputSize += size;
                REALLOC_MEMORY(&fa->output, fa->outputSize + 1);
                memcpy(fa->output + idx, ctx->file.contents + startOffset, size);
                fa->output[fa->outputSize] = '\0';
        }
        ctx->file.outputFilePosition = ctx->file.indexOfFirstUnconsumedToken;
}

static void suspend_copying(struct GP_Ctx *ctx)
{
        GP_ENSURE(!ctx->file.outputSuspended);
        copy_remaining_bytes(ctx);
        ctx->file.outputSuspended = 1;
}

static void resume_copying(struct GP_Ctx *ctx)
{
        GP_ENSURE(ctx->file.outputSuspended);
        copy_remaining_bytes(ctx); //XXX. this won't copy but just move the outputFilePosition
        ctx->file.outputSuspended = 0;
}

static void gp_push_file(struct GP_Ctx *ctx, int fileIndex)
{
        //gp_message_f("push file '%s'", ctx->desc.fileInfo[fileIndex].fileID);

        /* first save the copy to the actual stack */
        if (ctx->fileStackSize > 0)
                ctx->fileStack[ctx->fileStackSize - 1] = ctx->file;

        int i = ctx->fileStackSize++;
        REALLOC_MEMORY(&ctx->fileStack, ctx->fileStackSize);

        struct GP_FileInfo *fileInfo = &ctx->desc.fileInfo[fileIndex];
        struct GP_FileStackItem fileStackItem = {
                .fileID = fileInfo->fileID,
                .contents = fileInfo->contents,
                .size = fileInfo->size,
                .cursorPos = 0,
                .savedCharacter = -1,
                .haveSavedCharacter = 0,
                .outputFilePosition = 0,
                .indexOfFirstUnconsumedToken = 0,
                .outputSuspended = 0,
        };
        ctx->fileStack[i] = fileStackItem;
        ctx->file = fileStackItem;
}

static void gp_pop_file(struct GP_Ctx *ctx)
{
        GP_ENSURE(ctx->fileStackSize > 0);
        copy_remaining_bytes(ctx);
        //gp_message_f("pop file '%s'", ctx->desc.fileInfo[ctx->fileStackSize - 1].fileID);
        --ctx->fileStackSize;
        if (ctx->fileStackSize > 0)
                ctx->file = ctx->fileStack[ctx->fileStackSize - 1];
}

int gp_find_file_index_from_id_or_fatal_error(struct GP_Ctx *ctx, const char *fileID)
{
        for (int i = 0; i < ctx->desc.numFiles; i++)
                if (!strcmp(fileID, ctx->desc.fileInfo[i].fileID))
                        return i;
        gp_fatal_parse_error_f(ctx, "No file with this fileID available: '%s'", fileID);
}

static int look_character(struct GP_Ctx *ctx)
{
        if (ctx->file.haveSavedCharacter)
                return ctx->file.savedCharacter;
        while (ctx->file.cursorPos == ctx->file.size) {
                gp_pop_file(ctx);
                if (ctx->fileStackSize == 0)
                        return -1;
        }
        int c = ctx->file.contents[ctx->file.cursorPos];
        ctx->file.cursorPos++;
        ctx->file.savedCharacter = c;
        ctx->file.haveSavedCharacter = 1;
        return c;
}

static void consume_character(struct GP_Ctx *ctx)
{
        GP_ENSURE(ctx->file.haveSavedCharacter);
        ctx->file.haveSavedCharacter = 0;
}

static void reset_tokenbuffer(struct GP_Ctx *ctx)
{
        ctx->tokenBufferLength = 0;
}

static void append_to_tokenbuffer(struct GP_Ctx *ctx, int character)
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

static int look_token_no_preproc(struct GP_Ctx *ctx)
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
                        ctx->tokenKind = GP_TOKEN_EOF;
                        return 0;
                }
                consume_character(ctx);
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
                                        gp_fatal_parse_error_f(ctx,
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
                        ctx->tokenKind = GP_TOKEN_SLASH;
                }
        }
        else if (('a' <= c && c <= 'z')
            || ('A' <= c && c <= 'Z')
            || (c == '_')) {
                ctx->tokenKind = GP_TOKEN_NAME;
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
                ctx->tokenKind = GP_TOKEN_LITERAL;
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
                ctx->tokenKind = GP_TOKEN_STRING;
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
                gp_fatal_parse_error_f(ctx,
                                "Failed to lex; initial character: '%c'", c);
ok:
                ;
        }
        ctx->haveSavedToken = 1;
        return 1;
}

static void consume_token(struct GP_Ctx *ctx)
{
        GP_ENSURE(ctx->haveSavedToken);
        ctx->haveSavedToken = 0;
        ctx->file.indexOfFirstUnconsumedToken = ctx->file.cursorPos;
}

static int look_token(struct GP_Ctx *ctx)
{
        if (!look_token_no_preproc(ctx))
                return 0;
        if (ctx->tokenKind != GP_TOKEN_HASH)
                return 1;
        /* We have a TOKEN_HASH. Suspend copying input to output until the end
         * of this preprocessor directive. */
        suspend_copying(ctx);
        consume_token(ctx);
        if (!look_token_no_preproc(ctx)
            || ctx->tokenKind != GP_TOKEN_NAME)
                gp_fatal_parse_error_f(ctx,
                                "parse error while looking for name of preprocessing directive");
        if (!strcmp(ctx->tokenBuffer, "include")) {
                consume_token(ctx);
                if (!look_token_no_preproc(ctx)
                    || ctx->tokenKind != GP_TOKEN_STRING)
                        gp_fatal_parse_error_f(ctx,
                                        "Expected string literal giving file to #include");
                consume_token(ctx);
                /* TODO: make sure to resume _after end of line_ */
                resume_copying(ctx);
                /* TODO: make sure line is terminated after string literal token */
                int fileIndex = gp_find_file_index_from_id_or_fatal_error(ctx, ctx->tokenBuffer);
                gp_push_file(ctx, fileIndex);
        }
        else if (!strcmp(ctx->tokenBuffer, "version")) {
                consume_token(ctx);
                if (!look_token_no_preproc(ctx)
                    || ctx->tokenKind != GP_TOKEN_LITERAL)
                        gp_fatal_parse_error_f(ctx,
                                        "Expected version num in #version directive");
                consume_token(ctx);
                resume_copying(ctx);
        }
        else {
                gp_fatal_parse_error_f(ctx,
                                "Unknown preprocessing directive: #%s", ctx->tokenBuffer);
        }
        return look_token(ctx);
}

static int is_keyword(struct GP_Ctx *ctx, const char *keyword)
{
        GP_ENSURE(ctx->haveSavedToken);
        if (ctx->tokenKind != GP_TOKEN_NAME)
                return 0;
        return !strcmp(ctx->tokenBuffer, keyword);
}

static int is_known_type_name(struct GP_Ctx *ctx)
{
        for (int i = 0; i < GP_NUM_TYPE_KINDS; i++)
                if (!strcmp(ctx->tokenBuffer, gp_typeString[i]))
                        return 1;
        return 0;
}

static int is_binop_token(struct GP_Ctx *ctx, int *binopKind)
{
        GP_ENSURE(ctx->haveSavedToken);
        for (int i = 0; i < gp_numBinopTokens; i++) {
                if (ctx->tokenKind == gp_binopTokenInfo[i].tokenKind) {
                        *binopKind = gp_binopTokenInfo[i].binopKind;
                        return 1;
                }
        }
        return 0;
}

static int look_token_kind(struct GP_Ctx *ctx, int tokenKind)
{
        return look_token(ctx) && ctx->tokenKind == tokenKind;
}

static void expect_token_kind(struct GP_Ctx *ctx, int tokenKind)
{
        if (!look_token_kind(ctx, tokenKind))
                gp_fatal_parse_error_f(ctx, "expected '%s' token, found: '%s'",
                                    gp_tokenKindString[tokenKind],
                                    gp_tokenKindString[ctx->tokenKind]);
}

static char *alloc_string(struct GP_Ctx *ctx, const char *data)
{
        int length = (int) strlen(data);
        char *string;
        ALLOC_MEMORY(&string, length + 1);
        memcpy(string, data, length + 1);
        return string;
}

static void parse_simple_token(struct GP_Ctx *ctx, int tokenKind)
{
        expect_token_kind(ctx, tokenKind);
        consume_token(ctx);
}

static void parse_semicolon(struct GP_Ctx *ctx)
{
        parse_simple_token(ctx, GP_TOKEN_SEMICOLON);
}

static char *parse_name(struct GP_Ctx *ctx)
{
        expect_token_kind(ctx, GP_TOKEN_NAME);
        //message_f("name is '%s'", ctx->tokenBuffer);
        char *name = alloc_string(ctx, ctx->tokenBuffer);
        consume_token(ctx);
        return name;
}

// TODO: attach to context
#define DEFINE_ALLOCATOR_FUNCTION(type, name) type *name(struct GP_Ctx *ctx) \
{ \
        type *x; \
        ALLOC_MEMORY(&x, sizeof *x); \
        return x; \
}

DEFINE_ALLOCATOR_FUNCTION(struct GP_TypeExpr, create_typeexpr)
DEFINE_ALLOCATOR_FUNCTION(struct GP_UniformDecl, create_uniformdecl)
DEFINE_ALLOCATOR_FUNCTION(struct GP_VariableDecl, create_variabledecl)
DEFINE_ALLOCATOR_FUNCTION(struct GP_FuncDecl, create_funcdecl)
DEFINE_ALLOCATOR_FUNCTION(struct GP_FuncDefn, create_funcdefn)

struct GP_ToplevelNode *gp_add_new_toplevel_node(struct GP_Ctx *ctx)
{
        struct GP_ShaderfileAst *fa = &ctx->shaderfileAsts[ctx->currentShaderIndex];
        int idx = fa->numToplevelNodes ++;
        REALLOC_MEMORY(&fa->toplevelNodes, fa->numToplevelNodes);
        ALLOC_MEMORY(&fa->toplevelNodes[idx], 1);
        return fa->toplevelNodes[idx];
}

//XXX: if we detect that this is an interface block, we'll return NULL
static struct GP_TypeExpr *parse_typeexpr(struct GP_Ctx *ctx)
{
        expect_token_kind(ctx, GP_TOKEN_NAME);
        while (is_keyword(ctx, "flat")) {
                // XXX ignoring "flat" specifier for now. Not interesting to us.
                consume_token(ctx);
                expect_token_kind(ctx, GP_TOKEN_NAME);
        }
        for (int i = 0; i < GP_NUM_TYPE_KINDS; i++) {
                if (is_keyword(ctx, gp_typeString[i])) {
                        consume_token(ctx);
                        //message_f("parsed type %s", typeInfo[i].name);
                        struct GP_TypeExpr *typeExpr = create_typeexpr(ctx);
                        typeExpr->typeKind = i;
                        return typeExpr;
                }
        }
        // maybe this is an interface block...
        consume_token(ctx);
        if (look_token_kind(ctx, GP_TOKEN_LEFTBRACE)) {
                // this is an interface block. Parse it and ignore the contents (for now)
                consume_token(ctx);
                while (!look_token_kind(ctx, GP_TOKEN_RIGHTBRACE)) {
                        //XXX ignoreing stuff for now
                        parse_typeexpr(ctx);
                        parse_name(ctx);
                        parse_semicolon(ctx);
                }
                consume_token(ctx);
                return NULL;
        }
        gp_fatal_parse_error_f(ctx, "type expected or interface block was expected, got: %s", ctx->tokenBuffer);
}

static struct GP_TypeExpr *parse_type_or_void(struct GP_Ctx *ctx)
{
        expect_token_kind(ctx, GP_TOKEN_NAME);
        if (is_keyword(ctx, "void")) {
                consume_token(ctx);
                struct GP_TypeExpr *typeExpr = create_typeexpr(ctx);
                typeExpr->typeKind = -1;
                return typeExpr;
        }
        return parse_typeexpr(ctx);
}

static struct GP_VariableDecl *parse_variable(struct GP_Ctx *ctx)
{
        int inOrOut;
        if (is_keyword(ctx, "flat")) {
                consume_token(ctx);
                look_token(ctx);
        }
        if (is_keyword(ctx, "in")) {
                inOrOut = 0;
        }
        else if (is_keyword(ctx, "out")) {
                inOrOut = 1;
        }
        else {
                gp_fatal_parse_error_f(ctx,
                        "Invalid token %s, expected 'in' or 'out'",
                        ctx->tokenBuffer);
        }
        consume_token(ctx); // "in" or "out"
        struct GP_TypeExpr *typeExpr = parse_typeexpr(ctx);
        // XXX WARNING currently parse_typeexpr() may return NULL, which means that this was an interface block. Is it safe to proceed?
        char *name = parse_name(ctx);
        parse_semicolon(ctx);
        struct GP_VariableDecl *variableDecl = create_variabledecl(ctx);
        variableDecl->inOrOut = inOrOut;
        variableDecl->name = name;
        variableDecl->typeExpr = typeExpr;
        return variableDecl;
}

static struct GP_UniformDecl *parse_uniform(struct GP_Ctx *ctx)
{
        consume_token(ctx); // "uniform"
        struct GP_TypeExpr *typeExpr = parse_typeexpr(ctx);
        // currently parse_typeexpr may return NULL, but this is not valid for uniforms.
        if (typeExpr == NULL)
                gp_fatal_parse_error_f(ctx, "Can't use an interface block as a type for a uniform.");
        char *name = parse_name(ctx);
        parse_semicolon(ctx);
        struct GP_UniformDecl *uniformDecl = create_uniformdecl(ctx);
        uniformDecl->uniDeclName = name;
        uniformDecl->uniDeclTypeExpr = typeExpr;
        //printf("parse uniform (%s) %s %s\n", ctx->filepath, name, typeKindString[typeExpr->typeKind]);
        return uniformDecl;
}

static void parse_expression(struct GP_Ctx *ctx)
{
        if (!look_token(ctx))
                gp_fatal_parse_error_f(ctx, "Expected expression");
        if (ctx->tokenKind == GP_TOKEN_NAME) {
                consume_token(ctx);
        }
        else if (ctx->tokenKind == GP_TOKEN_LITERAL) {
                consume_token(ctx);
        }
        else if (ctx->tokenKind == GP_TOKEN_LEFTPAREN) {
                consume_token(ctx);
                parse_expression(ctx);
                parse_simple_token(ctx, GP_TOKEN_RIGHTPAREN);
        }
        else {
                /* unary operator? */
                for (int i = 0; i < GP_NUM_UNOP_KINDS; i++) {
                        if (gp_unopTokenInfo[i].tokenKind == ctx->tokenKind) {
                                consume_token(ctx);
                                parse_expression(ctx);
                                goto ok;
                        }
                }
                gp_fatal_parse_error_f(ctx, "Expected expression");
ok:
                ;
        }
        for (;;) {
                // function call?
                if (look_token_kind(ctx, GP_TOKEN_LEFTPAREN)) {
                        consume_token(ctx);
                        if (!look_token_kind(ctx, GP_TOKEN_RIGHTPAREN)) {
                                for (;;) {
                                        parse_expression(ctx);
                                        if (!look_token_kind(ctx, GP_TOKEN_COMMA))
                                                break;
                                        consume_token(ctx);
                                }
                        }
                        parse_simple_token(ctx, GP_TOKEN_RIGHTPAREN);
                }
                // member descend?
                else if (look_token_kind(ctx, GP_TOKEN_DOT)) {
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

static void parse_stmt(struct GP_Ctx *ctx); // forward declare: recursion

static void parse_compound_stmt(struct GP_Ctx *ctx)
{
        parse_simple_token(ctx, GP_TOKEN_LEFTBRACE);
        while (!look_token_kind(ctx, GP_TOKEN_RIGHTBRACE))
                parse_stmt(ctx);
        parse_simple_token(ctx, GP_TOKEN_RIGHTBRACE);
}

static void parse_variable_declaration_stmt(struct GP_Ctx *ctx)
{
        parse_typeexpr(ctx);
        parse_name(ctx);
        if (look_token_kind(ctx, GP_TOKEN_EQUALS)) {
                consume_token(ctx);
                parse_expression(ctx);
                parse_semicolon(ctx);
        }
}

static void parse_if_stmt(struct GP_Ctx *ctx)
{
        consume_token(ctx); // "if"
        parse_simple_token(ctx, GP_TOKEN_LEFTPAREN);
        parse_expression(ctx);
        parse_simple_token(ctx, GP_TOKEN_RIGHTPAREN);
        parse_stmt(ctx);
        if (look_token_kind(ctx, GP_TOKEN_NAME) && is_keyword(ctx, "else")) {
                consume_token(ctx); // "if"
                parse_stmt(ctx);
        }
}

static void parse_return_stmt(struct GP_Ctx *ctx)
{
        consume_token(ctx); // "return"
        parse_expression(ctx);
        parse_semicolon(ctx);
}

static void parse_discard_stmt(struct GP_Ctx *ctx)
{
        consume_token(ctx); // "discard"
        parse_semicolon(ctx);
}

static void parse_expression_stmt(struct GP_Ctx *ctx)
{
        parse_expression(ctx);
        parse_semicolon(ctx);
}

static void parse_stmt(struct GP_Ctx *ctx)
{
        if (!look_token(ctx))
                gp_fatal_parse_error_f(ctx, "Expected statement");
        if (ctx->tokenKind == GP_TOKEN_LEFTBRACE)
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

static void parse_FuncDefn_or_FuncDecl(struct GP_Ctx *ctx)
{
        struct GP_TypeExpr *returnTypeExpr = parse_type_or_void(ctx);
        char *name = parse_name(ctx);
        parse_simple_token(ctx, GP_TOKEN_LEFTPAREN);
        int numArgs = 0;
        char **argNames = NULL;
        struct GP_TypeExpr **argTypeExprs = NULL;
        if (!look_token_kind(ctx, GP_TOKEN_RIGHTPAREN)) {
                for (;;) {
                        numArgs++;
                        REALLOC_MEMORY(&argTypeExprs, numArgs);
                        REALLOC_MEMORY(&argNames, numArgs);
                        argTypeExprs[numArgs - 1] = parse_typeexpr(ctx);
                        argNames[numArgs - 1] = parse_name(ctx);
                        if (!look_token_kind(ctx, GP_TOKEN_COMMA))
                                break;
                        consume_token(ctx);
                }
        }
        parse_simple_token(ctx, GP_TOKEN_RIGHTPAREN);
        if (look_token_kind(ctx, GP_TOKEN_SEMICOLON)) {
                // it's only a decl
                consume_token(ctx);
                struct GP_FuncDecl *funcDecl = create_funcdecl(ctx);
                funcDecl->name = name;
                funcDecl->returnTypeExpr = returnTypeExpr;
                funcDecl->argTypeExprs = argTypeExprs;
                funcDecl->argNames = argNames;
                funcDecl->numArgs = numArgs;
                struct GP_ToplevelNode *node = gp_add_new_toplevel_node(ctx);
                node->directiveKind = GP_DIRECTIVE_FUNCDECL;
                node->data.tFuncdecl = funcDecl;
        }
        else {
                parse_simple_token(ctx, GP_TOKEN_LEFTBRACE);
                while (!look_token_kind(ctx, GP_TOKEN_RIGHTBRACE))
                        parse_stmt(ctx);
                parse_simple_token(ctx, GP_TOKEN_RIGHTBRACE);
                struct GP_FuncDefn *funcDefn = create_funcdefn(ctx);
                funcDefn->name = name;
                funcDefn->returnTypeExpr = returnTypeExpr;
                funcDefn->argTypeExprs = argTypeExprs;
                funcDefn->argNames = argNames;
                funcDefn->numArgs = numArgs;
                struct GP_ToplevelNode *node = gp_add_new_toplevel_node(ctx);
                node->directiveKind = GP_DIRECTIVE_FUNCDEFN;
                node->data.tFuncdefn = funcDefn;
        }
}

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

static void gp_parse_shader(struct GP_Ctx *ctx, int shaderIndex)
{
        const char *fileID = ctx->desc.shaderInfo[shaderIndex].fileID;
        int fileIndex = gp_find_file_index_from_id_or_fatal_error(ctx, fileID);

        gp_push_file(ctx, fileIndex);

        ctx->haveSavedToken = 0;
        ctx->tokenKind = GP_TOKEN_EOF;  // this is always valid. That's nice for error printing
        ctx->tokenBuffer = NULL;
        ctx->tokenBufferLength = 0;
        ctx->tokenBufferCapacity = 0;

        {
        struct GP_ShaderfileAst *fa = &ctx->shaderfileAsts[shaderIndex];
        memset(fa, 0, sizeof *fa);
        // switch
        ctx->currentShaderIndex = shaderIndex;
        }

        while (look_token(ctx)) {
                if (is_keyword(ctx, "uniform")) {
                        struct GP_ToplevelNode *node = gp_add_new_toplevel_node(ctx);
                        node->directiveKind = GP_DIRECTIVE_UNIFORM;
                        node->data.tUniform = parse_uniform(ctx);
                }
                else if (is_keyword(ctx, "in")
                         || is_keyword(ctx, "out")
                         || is_keyword(ctx, "flat")) {
                        struct GP_ToplevelNode *node = gp_add_new_toplevel_node(ctx);
                        node->directiveKind = GP_DIRECTIVE_VARIABLE;
                        node->data.tVariable = parse_variable(ctx);
                }
                else if (ctx->tokenKind == GP_TOKEN_NAME) {
                        parse_FuncDefn_or_FuncDecl(ctx);
                }
                else {
                        gp_fatal_parse_error_f(ctx,
                                "While expecting toplevel syntax item: Unexpected token type %s!",
                                  gp_tokenKindString[ctx->tokenKind]);
                }
        }
}

static void gp_postprocess(struct GP_Ctx *ctx)
{        
        /*
        for (int i = 0; i < ctx->desc.numShaders; i++) {
                gp_message_f("And here is the preprocessor output for shader %s: \"\"\"\n%s\"\"\"\n",
                             ctx->desc.shaderInfo[i].shaderName,
                             ctx->shaderfileAsts[i].output);
        }
        */

        for (int i = 0; i < ctx->desc.numShaders; i++) {
                struct GP_ShaderfileAst *fa = &ctx->shaderfileAsts[i];
                for (int j = 0; j < fa->numToplevelNodes; j++) {
                        struct GP_ToplevelNode *node = fa->toplevelNodes[j];
                        if (node->directiveKind == GP_DIRECTIVE_UNIFORM) {
                                struct GP_UniformDecl *decl = node->data.tUniform;
                                for (int k = 0; k < ctx->desc.numLinks; k++) {
                                        struct GP_LinkInfo *linkInfo = &ctx->desc.linkInfo[k];
                                        if (linkInfo->shaderIndex == i) {
                                                int programIndex = linkInfo->programIndex;
                                                int uniformIndex = ctx->numProgramUniforms++;
                                                REALLOC_MEMORY(&ctx->programUniforms, ctx->numProgramUniforms);
                                                ctx->programUniforms[uniformIndex].programIndex = programIndex;
                                                ctx->programUniforms[uniformIndex].typeKind = decl->uniDeclTypeExpr->typeKind;
                                                ctx->programUniforms[uniformIndex].uniformName = decl->uniDeclName;
                                        }
                                }
                        }
                        else if (node->directiveKind == GP_DIRECTIVE_VARIABLE) {
                                struct GP_VariableDecl *decl = node->data.tVariable;
                                // An attribute is an IN variable in a vertex shader
                                if (ctx->desc.shaderInfo[i].shaderType != GP_SHADERTYPE_VERTEX)
                                        continue;
                                if (decl->inOrOut != 0 /* IN */)
                                        continue;
                                for (int k = 0; k < ctx->desc.numLinks; k++) {
                                        struct GP_LinkInfo *linkInfo = &ctx->desc.linkInfo[k];
                                        if (linkInfo->shaderIndex == i) {
                                                int programIndex = linkInfo->programIndex;
                                                int attributeIndex = ctx->numProgramAttributes++;
                                                REALLOC_MEMORY(&ctx->programAttributes, ctx->numProgramAttributes);
                                                ctx->programAttributes[attributeIndex].programIndex = programIndex;
                                                ctx->programAttributes[attributeIndex].typeKind = decl->typeExpr->typeKind;
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
                                const char *programName = ctx->desc.programInfo[ctx->programUniforms[i].programIndex].programName;
                                const char *uniformName = ctx->programUniforms[i].uniformName;
                                gp_fatal_f("The shader program '%s' cannot be linked since there are multiple uniforms '%s' with incompatible types",
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
                                const char *programName = ctx->desc.programInfo[ctx->programAttributes[i].programIndex].programName;
                                const char *attributeName = ctx->programAttributes[i].attributeName;
                                gp_fatal_f("The shader program '%s' cannot be linked since there are multiple attributes '%s' with incompatible types.",
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

void gp_parse(struct GP_Ctx *ctx)
{
        for (int i = 0; i < ctx->desc.numShaders; i++)
                gp_parse_shader(ctx, i);
        gp_postprocess(ctx);
}

void gp_setup(struct GP_Ctx *ctx)
{
        memset(ctx, 0, sizeof *ctx);
}

void gp_teardown(struct GP_Ctx *ctx)
{
        FREE_MEMORY(&ctx->tokenBuffer);
        FREE_MEMORY(&ctx->shaderfileAsts);
        memset(ctx, 0, sizeof *ctx);
}
