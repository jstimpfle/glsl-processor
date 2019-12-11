#undef DATA
#define DATA
#include <glsl-compiler/lex.h>

#define ENUM_KIND_STRING(x) [x] = #x

const char *const tokenKindString[NUM_TOKEN_KINDS] = {
        ENUM_KIND_STRING( TOKEN_LITERAL ),
        ENUM_KIND_STRING( TOKEN_NAME ),
        ENUM_KIND_STRING( TOKEN_STRING ),
        ENUM_KIND_STRING( TOKEN_LEFTPAREN ),
        ENUM_KIND_STRING( TOKEN_RIGHTPAREN ),
        ENUM_KIND_STRING( TOKEN_LEFTBRACE ),
        ENUM_KIND_STRING( TOKEN_RIGHTBRACE ),
};
