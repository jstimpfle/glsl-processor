#include <glsl-processor/ast.h>
#include <glsl-processor/parse.h>

#define ENUM_KIND_STRING(x) [x] = #x
#define ENUM_TO_STRING(x, s) [x] = s

const char *const gp_tokenKindString[GP_NUM_TOKEN_KINDS] = {
        ENUM_KIND_STRING( GP_TOKEN_EOF ),
        ENUM_KIND_STRING( GP_TOKEN_LITERAL ),
        ENUM_KIND_STRING( GP_TOKEN_NAME ),
        ENUM_KIND_STRING( GP_TOKEN_STRING ),
        ENUM_KIND_STRING( GP_TOKEN_HASH ),
        ENUM_KIND_STRING( GP_TOKEN_LEFTPAREN ),
        ENUM_KIND_STRING( GP_TOKEN_RIGHTPAREN ),
        ENUM_KIND_STRING( GP_TOKEN_LEFTBRACE ),
        ENUM_KIND_STRING( GP_TOKEN_RIGHTBRACE ),
        ENUM_KIND_STRING( GP_TOKEN_COMMA ),
        ENUM_KIND_STRING( GP_TOKEN_SEMICOLON ),
        ENUM_KIND_STRING( GP_TOKEN_PLUS ),
        ENUM_KIND_STRING( GP_TOKEN_MINUS ),
        ENUM_KIND_STRING( GP_TOKEN_STAR ),
        ENUM_KIND_STRING( GP_TOKEN_SLASH ),
        ENUM_KIND_STRING( GP_TOKEN_PERCENT ),
        ENUM_KIND_STRING( GP_TOKEN_EQUALS ),
        ENUM_KIND_STRING( GP_TOKEN_DOUBLEEQUALS ),
        ENUM_KIND_STRING( GP_TOKEN_PLUSEQUALS ),
        ENUM_KIND_STRING( GP_TOKEN_MINUSEQUALS ),
        ENUM_KIND_STRING( GP_TOKEN_STAREQUALS ),
        ENUM_KIND_STRING( GP_TOKEN_SLASHEQUALS ),
        ENUM_KIND_STRING( GP_TOKEN_NE ),
        ENUM_KIND_STRING( GP_TOKEN_LT ),
        ENUM_KIND_STRING( GP_TOKEN_LE ),
        ENUM_KIND_STRING( GP_TOKEN_GE ),
        ENUM_KIND_STRING( GP_TOKEN_GT ),
        ENUM_KIND_STRING( GP_TOKEN_NOT ),
        ENUM_KIND_STRING( GP_TOKEN_AMPERSAND ),
        ENUM_KIND_STRING( GP_TOKEN_DOUBLEAMPERSAND ),
        ENUM_KIND_STRING( GP_TOKEN_PIPE ),
        ENUM_KIND_STRING( GP_TOKEN_DOUBLEPIPE ),
};

const struct GP_UnopInfo gp_unopInfo[GP_NUM_UNOP_KINDS] = {
        [GP_UNOP_NOT] = { "!" },
        [GP_UNOP_NEGATE] = { "-" },
};

const struct GP_UnopTokenInfo gp_unopTokenInfo[] = {
        { GP_TOKEN_NOT, GP_UNOP_NOT },
        { GP_TOKEN_MINUS, GP_UNOP_NEGATE },
};

const struct GP_BinopInfo gp_binopInfo[GP_NUM_BINOP_KINDS] = {
#define MAKE(x, y) [x] = { y }
        MAKE( GP_BINOP_EQ, "==" ),
        MAKE( GP_BINOP_NE, "!=" ),
        MAKE( GP_BINOP_LT, "<" ),
        MAKE( GP_BINOP_LE, "<=" ),
        MAKE( GP_BINOP_GE, ">=" ),
        MAKE( GP_BINOP_GT, ">" ),
        MAKE( GP_BINOP_PLUS, "+" ),
        MAKE( GP_BINOP_MINUS, "-" ),
        MAKE( GP_BINOP_MUL, "*" ),
        MAKE( GP_BINOP_DIV, "/" ),
        MAKE( GP_BINOP_MOD, "%" ),
        MAKE( GP_BINOP_ASSIGN, "=" ),
        MAKE( GP_BINOP_PLUSASSIGN, "+=" ),
        MAKE( GP_BINOP_MINUSASSIGN, "-=" ),
        MAKE( GP_BINOP_MULASSIGN, "*=" ),
        MAKE( GP_BINOP_DIVASSIGN, "/=" ),
        MAKE( GP_BINOP_BITAND, "&" ),
        MAKE( GP_BINOP_BITOR, "|" ),
        MAKE( GP_BINOP_LOGICALAND, "&&" ),
        MAKE( GP_BINOP_LOGICALOR, "||" ),
#undef MAKE
};

const struct GP_BinopTokenInfo gp_binopTokenInfo[] = {
        { GP_TOKEN_PLUS, GP_BINOP_PLUS },
        { GP_TOKEN_MINUS, GP_BINOP_MINUS },
        { GP_TOKEN_STAR, GP_BINOP_MUL },
        { GP_TOKEN_SLASH, GP_BINOP_DIV },
        { GP_TOKEN_PERCENT, GP_BINOP_MOD },
        { GP_TOKEN_EQUALS, GP_BINOP_ASSIGN },
        { GP_TOKEN_DOUBLEEQUALS, GP_BINOP_EQ },
        { GP_TOKEN_PLUSEQUALS, GP_BINOP_PLUSASSIGN },
        { GP_TOKEN_MINUSEQUALS, GP_BINOP_MINUSASSIGN },
        { GP_TOKEN_STAREQUALS, GP_BINOP_MULASSIGN },
        { GP_TOKEN_SLASHEQUALS, GP_BINOP_DIVASSIGN },
        { GP_TOKEN_NE, GP_BINOP_NE },
        { GP_TOKEN_LT, GP_BINOP_LT },
        { GP_TOKEN_LE, GP_BINOP_LE },
        { GP_TOKEN_GE, GP_BINOP_GE },
        { GP_TOKEN_GT, GP_BINOP_GT },
        { GP_TOKEN_AMPERSAND, GP_BINOP_BITAND },
        { GP_TOKEN_PIPE, GP_BINOP_BITOR },
        { GP_TOKEN_DOUBLEAMPERSAND, GP_BINOP_LOGICALAND },
        { GP_TOKEN_DOUBLEPIPE, GP_BINOP_LOGICALOR },
};

const int gp_numUnopToken = LENGTH(gp_unopTokenInfo);
const int gp_numBinopTokens = LENGTH(gp_binopTokenInfo);


const char *const gp_typeString[GP_NUM_TYPE_KINDS] = {
        ENUM_TO_STRING( GP_TYPE_BOOL, "bool" ),
        ENUM_TO_STRING( GP_TYPE_INT, "int" ),
        ENUM_TO_STRING( GP_TYPE_UINT, "uint" ),
        ENUM_TO_STRING( GP_TYPE_FLOAT, "float" ),
        ENUM_TO_STRING( GP_TYPE_DOUBLE, "double" ),
        ENUM_TO_STRING( GP_TYPE_VEC2, "vec2" ),
        ENUM_TO_STRING( GP_TYPE_VEC3, "vec3" ),
        ENUM_TO_STRING( GP_TYPE_VEC4, "vec4" ),
        ENUM_TO_STRING( GP_TYPE_MAT2, "mat2" ),
        ENUM_TO_STRING( GP_TYPE_MAT3, "mat3" ),
        ENUM_TO_STRING( GP_TYPE_MAT4, "mat4" ),
        ENUM_TO_STRING( GP_TYPE_SAMPLER2D, "sampler2D" ),
};

const char *const gp_typeKindString[GP_NUM_TYPE_KINDS] = {
        ENUM_KIND_STRING(GP_TYPE_BOOL),
        ENUM_KIND_STRING(GP_TYPE_INT),
        ENUM_KIND_STRING(GP_TYPE_UINT),
        ENUM_KIND_STRING(GP_TYPE_FLOAT),
        ENUM_KIND_STRING(GP_TYPE_DOUBLE),
        ENUM_KIND_STRING(GP_TYPE_VEC2),
        ENUM_KIND_STRING(GP_TYPE_VEC3),
        ENUM_KIND_STRING(GP_TYPE_VEC4),
        ENUM_KIND_STRING(GP_TYPE_MAT2),
        ENUM_KIND_STRING(GP_TYPE_MAT3),
        ENUM_KIND_STRING(GP_TYPE_MAT4),
        ENUM_KIND_STRING(GP_TYPE_SAMPLER2D),
};

const char *const gp_shadertypeKindString[GP_NUM_SHADERTYPE_KINDS] = {
        [GP_SHADERTYPE_VERTEX] = "SHADERTYPE_VERTEX",
        [GP_SHADERTYPE_FRAGMENT] = "SHADERTYPE_FRAGMENT",
};
