#include <glsl-compiler/logging.h>
#include <glsl-compiler/syntax.h>
#include <glsl-compiler/parse.h>
#include <stdio.h>

int main(void)
{
        message_s("Hello!");
        struct Ctx ctx;
        setup_ctx(&ctx);
        parse(&ctx);
        teardown_ctx(&ctx);
        return 0;
}
