#include <glsl-compiler/memoryalloc.h>
#include <glsl-compiler/logging.h>
#include <glsl-compiler/ast.h>
#include <glsl-compiler/parse.h>
#include <glsl-compiler/parselinkerfile.h>
#include <glsl-compiler/process.h>
#include <stdio.h>

void parse_file(struct Ctx *ctx, const char *filepath)
{
        char *fileContents = NULL;
        int fileSize;
        FILE *fileHandle = fopen(filepath, "rb");
        fseek(fileHandle, 0, SEEK_END);
        fileSize = ftell(fileHandle);
        message_f("File size is %d bytes", fileSize);
        fseek(fileHandle, 0, SEEK_SET);
        ALLOC_MEMORY(&fileContents, fileSize + 1);
        int r = fread(fileContents, fileSize, 1, fileHandle);
        if (r != 1)
                fatal_f("Error reading file %s", filepath);
        fclose(fileHandle);

        parse_next_file(ctx, filepath, fileContents, fileSize);
}

//XXX testing
static const char linkerfileString[] =
"program circle;\n"
"shader projections_vert VERTEX_SHADER \"projections.vert\";\n"
"shader circle_frag FRAGMENT_SHADER \"circle.frag\";\n"
"shader ellipse_frag FRAGMENT_SHADER \"ellipse.frag\";\n"
;

int main(int argc, const char **argv)
{
        if (argc < 2) {
                message_f("Usage: %s <shader-file> <shader-file>...", argv[0]);
                return 1;
        }

        struct Ctx ctx;
        setup_ctx(&ctx);

        parse_linker_file("shaders.link", linkerfileString,
                         sizeof linkerfileString - 1);

        for (int i = 1; i < argc; i++)
                parse_file(&ctx, argv[i]);

        process_ast(ctx.ast);

        teardown_ctx(&ctx);
        return 0;
}
