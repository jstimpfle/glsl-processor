#ifndef GLSLCOMPILER_PARSELINKERFILE_H_INCLUDED
#define GLSLCOMPILER_PARSELINKERFILE_H_INCLUDED

#include <glsl-compiler/ast.h>

struct ShaderProgram {
        const char *programName;
};

struct ShaderLinkItem {
        const char *programName;
        const char *shaderFilepath;
};

void parse_linker_file(const char *filepath, const char *fileContents, int fileSize, struct Ast *ast);

#endif
