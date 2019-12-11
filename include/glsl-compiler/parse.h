#ifndef GLSLCOMPILER_PARSE_H_INCLUDED
#define GLSLCOMPILER_PARSE_H_INCLUDED

enum {
        TOKEN_EOF,
        TOKEN_LITERAL,
        TOKEN_NAME,
        TOKEN_STRING,
        TOKEN_LEFTPAREN,
        TOKEN_RIGHTPAREN,
        TOKEN_LEFTBRACE,
        TOKEN_RIGHTBRACE,
        TOKEN_COMMA,
        TOKEN_SEMICOLON,
        NUM_TOKEN_KINDS
};

struct Ctx {
        char *filepath;
        char *fileContents;
        int fileSize;
        int cursorPos;
        int savedCharacter;
        int haveSavedCharacter;
        /* this is backing storage for dynamically allocated token data. It is
         * valid only for the last token that was lexed using this context. */
        int haveSavedToken;
        int tokenKind; // this will always be valid, even if !haveSavedToken
        double tokenFloatingValue;
        char *tokenBuffer;
        int tokenBufferLength;
        int tokenBufferCapacity;
};

const char *const tokenKindString[NUM_TOKEN_KINDS];

void setup_ctx(struct Ctx *ctx);
void teardown_ctx(struct Ctx *ctx);

void parse(struct Ctx *ctx);

#endif
