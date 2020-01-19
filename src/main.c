#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/logging.h>
#include <glsl-processor/ast.h>
#include <glsl-processor/parse.h>
#include <glsl-processor/parselinkerfile.h>
#include <glsl-processor/process.h>
#include <stdio.h>
#include <string.h>

struct FileToRead {
        const char *fileContents;
        int fileSize;
};

static void read_file(const char *filepath, struct FileToRead *out)
{
        char *fileContents = NULL;
        int fileSize;
        FILE *fileHandle = fopen(filepath, "rb");
        if (fileHandle == NULL)
                fatal_f("Failed to open '%s'", filepath);
        fseek(fileHandle, 0, SEEK_END);
        fileSize = ftell(fileHandle);
        //message_f("File size is %d bytes", fileSize);
        fseek(fileHandle, 0, SEEK_SET);
        ALLOC_MEMORY(&fileContents, fileSize + 1);
        size_t r = fread(fileContents, 1, fileSize + 1, fileHandle);
        if (r != fileSize)
                fatal_f("Error reading file %s: %d", filepath);
        fclose(fileHandle);

        out->fileContents = fileContents;
        out->fileSize = fileSize;
}

int main(int argc, const char **argv)
{
        if (argc != 3) {
                message_f("Usage: glsl-processor <linker-file> <out-dir>");
                return 1;
        }

        const char *linkerFilepath = argv[1];
        const char *outDirpath = argv[2];

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

        ALLOC_MEMORY(&ctx.ast->shaderfileAsts, ctx.ast->numShaders);
        for (int i = 0; i < ctx.ast->numShaders; i++) {
                struct ShaderDecl *shaderDecl = &ctx.ast->shaderDecls[i];
                const char *filepath = get_aststring_buffer(ctx.ast, shaderDecl->shaderFilepath);
                struct FileToRead readtFile;
                read_file(filepath, &readtFile);
                parse_next_file(&ctx, filepath, readtFile.fileContents, readtFile.fileSize);
                // TODO: when should we dispose the file buffer?
        }

        process_ast(&ast);

        extern void write_c_interface(struct Ast *ast, const char *autogenDir);
        write_c_interface(&ast, outDirpath);

        teardown_ctx(&ctx);
        teardown_ast(&ast);

        return 0;
}
