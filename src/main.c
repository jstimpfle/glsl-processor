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
        if (fileHandle == NULL)
                fatal_f("Failed to open '%s'", filepath);
        fseek(fileHandle, 0, SEEK_END);
        fileSize = ftell(fileHandle);
        //message_f("File size is %d bytes", fileSize);
        fseek(fileHandle, 0, SEEK_SET);
        ALLOC_MEMORY(&fileContents, fileSize + 1);
        size_t r = fread(fileContents, fileSize, 1, fileHandle);
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

static int find_program_index(struct Ast *ast, const char *programName)
{
        for (int i = 0; i < ast->numShaders; i++)
                if (!strcmp(programName, get_aststring_buffer(ast, ast->programDecls[i].programName)))
                        return i;
        fatal_f("Referenced Shader does not exist: %s", programName);
}

static int find_shader_index(struct Ast *ast, const char *shaderName)
{
        for (int i = 0; i < ast->numShaders; i++)
                if (!strcmp(shaderName, get_aststring_buffer(ast, ast->shaderDecls[i].shaderName)))
                        return i;
        fatal_f("Referenced Shader does not exist: %s", shaderName);
}

int main(int argc, const char **argv)
{
        if (argc != 2) {
                message_f("Usage: glsl-processor <linker-file>");
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

        ALLOC_MEMORY(&ctx.ast->shaderfileAsts, ctx.ast->numShaders);
        for (int i = 0; i < ctx.ast->numShaders; i++) {
                struct ShaderDecl *shaderDecl = &ctx.ast->shaderDecls[i];
                const char *filepath = get_aststring_buffer(ctx.ast, shaderDecl->shaderFilepath);
                parse_file(&ctx, filepath);
        }

        /* resolve all references */
        for (int i = 0; i < ast.numLinkItems; i++) {
                AstString programName = ast.linkItems[i].programName;
                AstString shaderName = ast.linkItems[i].shaderName;
                ast.linkItems[i].resolvedProgramIndex = find_program_index(&ast, get_aststring_buffer(&ast, programName));
                ast.linkItems[i].resolvedShaderIndex = find_shader_index(&ast, get_aststring_buffer(&ast, shaderName));
        }

        process_ast(&ast);

        teardown_ctx(&ctx);
        teardown_ast(&ast);

        return 0;
}
