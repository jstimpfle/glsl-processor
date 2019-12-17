#include <glsl-compiler/memoryalloc.h>
#include <glsl-compiler/logging.h>
#include <glsl-compiler/ast.h>
#include <glsl-compiler/parse.h>
#include <glsl-compiler/parselinkerfile.h>
#include <glsl-compiler/process.h>
#include <stdio.h>

struct FileToRead {
        const char *fileContents;
        int fileSize;
};

static void read_file(const char *filepath, struct FileToRead *out)
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

        out->fileContents = fileContents;
        out->fileSize = fileSize;
}

void parse_file(struct Ctx *ctx, const char *filepath)
{
        struct FileToRead readtFile;
        read_file(filepath, &readtFile);
        parse_next_file(ctx, filepath, readtFile.fileContents, readtFile.fileSize);
        // TODO: when should we dispose the file buffer?
}

int main(int argc, const char **argv)
{
        if (argc != 2) {
                message_f("Usage: %s <linker-file>");
                return 1;
        }

        const char *linkerFilepath = argv[1];

        struct Ast ast;
        struct Ctx ctx;

        setup_ast(&ast);
        setup_ctx(&ctx, &ast);

        {
                struct FileToRead linkerFile;
                read_file(linkerFilepath, &linkerFile);
                parse_linker_file(linkerFilepath, linkerFile.fileContents, linkerFile.fileSize, &ast);
                // TODO: when to dispose file contents?
        }

        for (int i = 1; i < ctx.ast->numShaders; i++) {
                const char *filepath = get_astname_buffer(ctx.ast, ctx.ast->shaderDecls[i].shaderFilepath); //XXX
                message_f("filepath is: %s", filepath);
                parse_file(&ctx, filepath);
        }

        process_ast(&ast);

        teardown_ctx(&ctx);
        teardown_ast(&ast);

        return 0;
}
