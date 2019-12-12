#include <glsl-compiler/memoryalloc.h>
#include <glsl-compiler/logging.h>
#include <glsl-compiler/ast.h>
#include <glsl-compiler/parse.h>
#include <glsl-compiler/process.h>
#include <stdio.h>

int main(int argc, const char **argv)
{
        if (argc != 2) {
                message_f("Usage: %s <path-to-shader>", argv[0]);
                return 1;
        }

        const char *filepath = argv[1];

        char *fileContents = NULL;
        int fileSize;
        FILE *fileHandle = fopen(filepath, "rb");
        fseek(fileHandle, 0, SEEK_END);
        fileSize = ftell(fileHandle);
        message_f("File size is %d bytes");
        fseek(fileHandle, 0, SEEK_SET);
        ALLOC_MEMORY(&fileContents, fileSize + 1);
        int r = fread(fileContents, fileSize, 1, fileHandle);
        if (r != 1) {
                message_f("Error reading file %s", filepath);
                return 1;
        }
        fclose(fileHandle);

        struct Ctx ctx;
        setup_ctx(&ctx, filepath, fileContents, fileSize);
        parse(&ctx);

        process_ast(ctx.ast);

        teardown_ctx(&ctx);
        return 0;
}
