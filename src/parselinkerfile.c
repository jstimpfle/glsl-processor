#include <glsl-compiler/defs.h>
#include <glsl-compiler/ast.h>
#include <glsl-compiler/logging.h>
#include <glsl-compiler/parselinkerfile.h>

enum {
        LINKTOKEN_NAME,
        LINKTOKEN_STRING,
        LINKTOKEN_SEMICOLON,
        NUM_LINKTOKEN_KINDS,
};

static const char *const linktokenKindString[NUM_LINKTOKEN_KINDS] = {
#define MAKE(x) [x] = #x
        MAKE( LINKTOKEN_NAME ),
        MAKE( LINKTOKEN_STRING ),
        MAKE( LINKTOKEN_SEMICOLON ),
#undef MAKE
};

/* XXX: should get rid of this... */
static const char *const shadertypeKeywords[NUM_SHADERTYPE_KINDS] = {
        [SHADERTYPE_VERTEX] = "VERTEX_SHADER",
        [SHADERTYPE_FRAGMENT] = "FRAGMENT_SHADER",
};

struct LinkerReadCtx {
        struct Ast *ast;  // TODO: initialize this
        const char *filepath;
        const char *fileContents;
        int fileSize;
        int fileCursor;
        int haveSavedCharacter;
        int savedCharacter;
        // current linker token
        int haveSavedToken;
        int tokenKind;
        char tokenBuffer[128];
        int tokenLength;
};

static void compute_line_and_column(struct LinkerReadCtx *ctx, int *outLine, int *outColumn)
{
        int line = 1;
        int column = 1;
        for (int i = 0; i < ctx->fileCursor; i++) {
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

void NORETURN fatal_linkerread_fv(struct LinkerReadCtx *ctx,
        const char *fmt, va_list ap)
{
        int line;
        int column;
        compute_line_and_column(ctx, &line, &column);
        fatal_begin();
        fatal_write_f("In file %s, line %d:%d: ", ctx->filepath, line, column);
        fatal_write_fv(fmt, ap);
        fatal_end();
}

void NORETURN fatal_linkerread_f(struct LinkerReadCtx *ctx,
        const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        fatal_linkerread_fv(ctx, fmt, ap);
        va_end(ap);
}


void reset_token_buffer(struct LinkerReadCtx *ctx)
{
        ctx->tokenLength = 0;
}

void append_character_to_token_buffer(struct LinkerReadCtx *ctx, int c)
{
        if (ctx->tokenLength + 1 == LENGTH(ctx->tokenBuffer))
                fatal_linkerread_f(ctx,
                        "Identifier too large. Have: '%s'",
                        ctx->tokenBuffer);
        ctx->tokenBuffer[ctx->tokenLength] = c;
        ctx->tokenBuffer[ctx->tokenLength + 1] = '\0';
        ctx->tokenLength++;
}

int look_linker_character(struct LinkerReadCtx *ctx)
{
        if (ctx->haveSavedCharacter)
                return ctx->savedCharacter;
        if (ctx->fileCursor == ctx->fileSize)
                return -1;
        int c = ctx->fileContents[ctx->fileCursor];
        ctx->fileCursor ++;
        ctx->haveSavedCharacter = 1;
        ctx->savedCharacter = c;
        return c;
}

void consume_linker_character(struct LinkerReadCtx *ctx)
{
        ENSURE(ctx->haveSavedCharacter);
        ctx->haveSavedCharacter = 0;
}

int look_linker_token(struct LinkerReadCtx *ctx)
{
        if (ctx->haveSavedToken)
                return 1;
        int c;
        for (;;) {
                c = look_linker_character(ctx);
                if (c == -1)
                        return 0;
                if (c > 32)
                        break;
                consume_linker_character(ctx);
        }
        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
                reset_token_buffer(ctx);
                for (;;) {
                        append_character_to_token_buffer(ctx, c);
                        consume_linker_character(ctx);
                        c = look_linker_character(ctx);
                        if (!(('a' <= c && c <= 'z')
                              || ('A' <= c && c <= 'Z')
                              || c == '_'))
                                break;
                }
                ctx->tokenKind = LINKTOKEN_NAME;
        }
        else if (c == '"') {
                consume_linker_character(ctx);
                reset_token_buffer(ctx);
                for (;;) {
                        c = look_linker_character(ctx);
                        if (c == -1)
                                fatal_linkerread_f(
                ctx, "EOF encountered while reading string literal");
                        consume_linker_character(ctx);
                        if (c == '"')
                                break;
                        append_character_to_token_buffer(ctx, c);
                }
                ctx->tokenKind = LINKTOKEN_STRING;
        }
        else if (c == ';') {
                consume_linker_character(ctx);
                ctx->tokenKind = LINKTOKEN_SEMICOLON;
        }
        else
                fatal_linkerread_f(ctx,
                           "Expected token, found character '%c'", c);
        ctx->tokenLength = 0;
        ctx->haveSavedToken = 1;
        return 1;
}

void consume_linker_token(struct LinkerReadCtx *ctx)
{
        ENSURE(ctx->haveSavedToken);
        ctx->haveSavedToken = 0;
}

static int is_keyword(struct LinkerReadCtx *ctx, const char *word)
{
        ENSURE(ctx->haveSavedToken);
        return ctx->tokenKind == LINKTOKEN_NAME
                && !strcmp(ctx->tokenBuffer, word);
}

static void expect_token_kind(struct LinkerReadCtx *ctx, int tokenKind)
{
        if (!look_linker_token(ctx))
                fatal_linkerread_f(ctx,
                        "Expected a '%s' token but found EOF.",
                        linktokenKindString[tokenKind]);
        if (ctx->tokenKind != tokenKind)
                fatal_linkerread_f(ctx,
                        "Expected a '%s' token but found a '%s' token.",
                        linktokenKindString[tokenKind],
                        linktokenKindString[ctx->tokenKind]);
}

static void parse_token_kind(struct LinkerReadCtx *ctx, int tokenKind)
{
        expect_token_kind(ctx, tokenKind);
        consume_linker_token(ctx);
}

static AstString parse_name(struct LinkerReadCtx *ctx)
{
        if (!look_linker_token(ctx))
                fatal_linkerread_f(ctx,
                        "Expected a name");
        AstString name = create_aststring(ctx->ast, ctx->tokenBuffer);
        consume_linker_token(ctx);
        return name;
}

// for now, string literals are represented as AstString's
static AstString parse_string(struct LinkerReadCtx *ctx)
{
        expect_token_kind(ctx, LINKTOKEN_STRING);
        AstString str = create_aststring(ctx->ast, ctx->tokenBuffer);
        consume_linker_token(ctx);
        return str;
}

static int parse_shader_type(struct LinkerReadCtx *ctx)
{
        if (!look_linker_token(ctx))
                fatal_linkerread_f(ctx, "Expected shader type.");
        for (int i = 0; i < NUM_SHADERTYPE_KINDS; i++) {
                if (is_keyword(ctx, shadertypeKeywords[i])) {
                        consume_linker_token(ctx);
                        return i;
                }
        }
        fatal_linkerread_f(ctx, "Expected shader type.");
}

static void parse_program_stmt(struct LinkerReadCtx *ctx)
{
        consume_linker_token(ctx); // "program"
        AstString programName = parse_name(ctx);
        parse_token_kind(ctx, LINKTOKEN_SEMICOLON);

        struct ProgramDecl *programDecl = create_program_decl(ctx->ast);
        programDecl->programName = programName;
}

static void parse_shader_stmt(struct LinkerReadCtx *ctx)
{
        consume_linker_token(ctx); // "shader"
        AstString shaderName = parse_name(ctx);
        int shaderType = parse_shader_type(ctx);
        AstString shaderFilepath = parse_string(ctx);
        parse_token_kind(ctx, LINKTOKEN_SEMICOLON);

        struct ShaderDecl *shaderDecl = create_shader_decl(ctx->ast);
        shaderDecl->shaderName = shaderName;
        shaderDecl->shaderType = shaderType;
        shaderDecl->shaderFilepath = shaderFilepath;
}

static void parse_link_stmt(struct LinkerReadCtx *ctx)
{
        consume_linker_token(ctx);  // "link"
        AstString programName = parse_name(ctx);
        AstString shaderName = parse_name(ctx);
        parse_token_kind(ctx, LINKTOKEN_SEMICOLON);

        struct LinkItem *linkItem = create_link_item(ctx->ast);
        linkItem->programName = programName;
        linkItem->shaderName = shaderName;
}

void parse_linker_file(
        const char *filepath, const char *fileContents,
        int fileSize, struct Ast *ast)
{
        struct LinkerReadCtx linkerReadCtx = {0};
        struct LinkerReadCtx *ctx = &linkerReadCtx;
        ctx->filepath = filepath;
        ctx->fileContents = fileContents;
        ctx->fileSize = fileSize;
        ctx->ast = ast;
        while (look_linker_token(ctx)) {
                if (ctx->tokenKind != LINKTOKEN_NAME)
                        fatal_linkerread_f(ctx,
                                "Expected a directive (a command) here");
                if (is_keyword(ctx, "program"))
                        parse_program_stmt(ctx);
                else if (is_keyword(ctx, "shader"))
                        parse_shader_stmt(ctx);
                else if (is_keyword(ctx, "link"))
                        parse_link_stmt(ctx);
                else
                        fatal_linkerread_f(ctx,
                                "Unknown directive: '%s'. "
                                "Valid directives are 'program', 'shader', and 'link'",
                                ctx->tokenBuffer);
        }
}
