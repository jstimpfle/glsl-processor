#undef DATA
#define DATA
#include <glsl-compiler/ast.h>
#include <glsl-compiler/parse.h>

#define ENUM_KIND_STRING(x) [x] = #x
#define ENUM_TO_STRING(x, s) [x] = s

const char *const tokenKindString[NUM_TOKEN_KINDS] = {
        ENUM_KIND_STRING( TOKEN_EOF ),
        ENUM_KIND_STRING( TOKEN_LITERAL ),
        ENUM_KIND_STRING( TOKEN_NAME ),
        ENUM_KIND_STRING( TOKEN_STRING ),
        ENUM_KIND_STRING( TOKEN_LEFTPAREN ),
        ENUM_KIND_STRING( TOKEN_RIGHTPAREN ),
        ENUM_KIND_STRING( TOKEN_LEFTBRACE ),
        ENUM_KIND_STRING( TOKEN_RIGHTBRACE ),
        ENUM_KIND_STRING( TOKEN_COMMA ),
        ENUM_KIND_STRING( TOKEN_SEMICOLON ),
        ENUM_KIND_STRING( TOKEN_PLUS ),
        ENUM_KIND_STRING( TOKEN_MINUS ),
        ENUM_KIND_STRING( TOKEN_SLASH ),
        ENUM_KIND_STRING( TOKEN_STAR ),
        ENUM_KIND_STRING( TOKEN_EQUALS ),
        ENUM_KIND_STRING( TOKEN_DOUBLEEQUALS ),
        ENUM_KIND_STRING( TOKEN_NE ),
        ENUM_KIND_STRING( TOKEN_LT ),
        ENUM_KIND_STRING( TOKEN_LE ),
        ENUM_KIND_STRING( TOKEN_GE ),
        ENUM_KIND_STRING( TOKEN_GT ),
};

const struct BinopInfo binopInfo[NUM_BINOP_KINDS] = {
#define MAKE(x, y) [x] = y
        MAKE( BINOP_EQ, "==" ),
        MAKE( BINOP_NE, "!=" ),
        MAKE( BINOP_LT, "<" ),
        MAKE( BINOP_LE, "<=" ),
        MAKE( BINOP_GE, ">=" ),
        MAKE( BINOP_GT, ">" ),
        MAKE( BINOP_PLUS, "+" ),
        MAKE( BINOP_MINUS, "-" ),
        MAKE( BINOP_DIV, "/" ),
        MAKE( BINOP_MUL, "*" ),
        MAKE( BINOP_ASSIGN, "=" ),
#undef MAKE
};

const struct BinopTokenInfo binopTokenInfo[] = {
        { TOKEN_PLUS, BINOP_PLUS },
        { TOKEN_MINUS, BINOP_MINUS },
        { TOKEN_SLASH, BINOP_DIV },
        { TOKEN_STAR, BINOP_MUL },
        { TOKEN_EQUALS, BINOP_ASSIGN },
        { TOKEN_DOUBLEEQUALS, BINOP_EQ },
        { TOKEN_NE, BINOP_NE },
        { TOKEN_LT, BINOP_LT },
        { TOKEN_LE, BINOP_LE },
        { TOKEN_GE, BINOP_GE },
        { TOKEN_GT, BINOP_GT },
};

const int numBinopTokens = LENGTH(binopTokenInfo);


const struct PrimtypeInfo primtypeInfo[NUM_PRIMTYPE_KINDS] = {
        ENUM_TO_STRING( PRIMTYPE_FLOAT, "float" ),
        ENUM_TO_STRING( PRIMTYPE_VEC2, "vec2" ),
        ENUM_TO_STRING( PRIMTYPE_VEC3, "vec3" ),
        ENUM_TO_STRING( PRIMTYPE_VEC4, "vec4" ),
        ENUM_TO_STRING( PRIMTYPE_MAT2, "mat2" ),
        ENUM_TO_STRING( PRIMTYPE_MAT3, "mat3" ),
        ENUM_TO_STRING( PRIMTYPE_MAT4, "mat4" ),
};
