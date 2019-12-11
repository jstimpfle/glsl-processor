#include <stdio.h>

enum {
        TOKEN_LITERAL,
        TOKEN_NAME,
        TOKEN_STRING,
        TOKEN_LEFTPAREN,
        TOKEN_RIGHTPAREN,
        TOKEN_LEFTBRACE,
        TOKEN_RIGHTBRACE,
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
        int tokenKind;
        double tokenFloatingValue;
        char *tokenBuffer;
        int tokenBufferLength;
        int tokenBufferCapacity;
};

const char *const tokenKindString[NUM_TOKEN_KINDS];

void setup_ctx(struct Ctx *ctx);
void teardown_ctx(struct Ctx *ctx);
int read_token(struct Ctx *ctx);


void parse(struct Ctx *ctx);
