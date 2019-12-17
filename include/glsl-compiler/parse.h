#ifndef GLSLCOMPILER_PARSE_H_INCLUDED
#define GLSLCOMPILER_PARSE_H_INCLUDED

struct Ctx {
        struct Ast *ast; // This Ctx is all about creating this AST...

        const char *filepath;
        const char *fileContents;
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

void setup_ctx(struct Ctx *ctx, struct Ast *ast);
void teardown_ctx(struct Ctx *ctx);

void parse_next_file(struct Ctx *ctx, const char *filepath, const char *fileContents, int fileSize);

#endif
