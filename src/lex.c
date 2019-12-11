#include <glsl-compiler/str.h>
#include <glsl-compiler/lex.h>
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

void setup_ctx(struct Ctx *ctx)
{
        ctx->filepath = "TEST";
        ctx->fileContents = "void main() {}";
        ctx->fileSize = 14;

        ctx->cursorPos = 0;
        ctx->savedCharacter = -1;
        ctx->haveSavedCharacter = 0;

        ctx->haveSavedToken = 0;
        ctx->tokenKind = -1;
        ctx->tokenBuffer = NULL;
        ctx->tokenBufferLength = 0;
        ctx->tokenBufferCapacity = 0;
}

void release_ctx(struct Ctx *ctx)
{
        FREE_MEMORY(&ctx->tokenBuffer);
}

int look_token(struct Ctx *ctx)
{
        if (ctx->haveSavedToken)
                return 1;
        int c;
        for (;;) {
                c = look_character(ctx);
                if (c > 32)
                        break;
                if (c == -1)
                        return 0;
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
                        if (!('a' <= c && c <= 'z')
                             || ('A' <= c && c <= 'Z')
                             || (c == '_')
                             || ('0' <= c && c <= '9'))
                                break;
                }
        }
        else if (c == '(') {
                consume_character(ctx);
                ctx->tokenKind = TOKEN_LEFTPAREN;
        }
        else if (c == ')') {
                consume_character(ctx);
                ctx->tokenKind = TOKEN_RIGHTPAREN;
        }
        else if (c == '{') {
                consume_character(ctx);
                ctx->tokenKind = TOKEN_LEFTBRACE;
        }
        else if (c == '}') {
                consume_character(ctx);
                ctx->tokenKind = TOKEN_RIGHTBRACE;
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
                fatal_f("Failed to lex; initial character: '%c'", c);
        }
        ctx->haveSavedToken = 1;
        return 1;
}

static void consume_token(struct Ctx *ctx)
{
        ENSURE(ctx->haveSavedToken);
        ctx->haveSavedToken = 0;
}

static int is_keyword(struct Ctx *ctx, const char *keyword)
{
        ENSURE(ctx->haveSavedToken);
        return ctx->tokenKind == TOKEN_NAME &&
                !strcmp(ctx->tokenBuffer, keyword);
}

void parse(struct Ctx *ctx)
{
        while (look_token(ctx)) {
                if (is_keyword(ctx, "uniform"))
                        message_f("Uniform!");
                else if (is_keyword(ctx, "in"))
                        message_f("In!");
                else if (ctx->tokenKind == TOKEN_NAME)
                        message_f("name: %s", ctx->tokenBuffer);
                else
                        message_f("Unexpected token type %s!",
                                  tokenKindString[ctx->tokenKind]);
                consume_token(ctx);
        }
}
