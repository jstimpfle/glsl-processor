#include <glsl-compiler/logging.h>
#include <glsl-compiler/syntax.h>
#include <glsl-compiler/lex.h>
#include <glsl-compiler/str.h>
#include <stdio.h>

int main(void)
{
        message_s("Hello!");
        struct Ctx ctx;
        setup_ctx(&ctx);
        parse(&ctx);
        return 0;
}
