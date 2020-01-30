#ifndef GP_PARSE_H_INCLUDED
#define GP_PARSE_H_INCLUDED

#include <glsl-processor/ast.h>

struct GP_FileInfo {
        char *fileID;
        char *contents;
        int size;
};

struct GP_ProgramInfo {
        char *programName;
};

struct GP_ShaderInfo {
        char *shaderName;
        char *fileID;
        int shaderType;
};

struct GP_LinkInfo {
        int programIndex;
        int shaderIndex;
};

struct GP_Desc {
        struct GP_FileInfo *fileInfo;
        struct GP_ProgramInfo *programInfo;
        struct GP_ShaderInfo *shaderInfo;
        struct GP_LinkInfo *linkInfo;
        int numFiles;
        int numPrograms;
        int numShaders;
        int numLinks;
};

struct GP_ProgramUniform {
        int programIndex;
        int typeKind;
        char *uniformName;
};

struct GP_ProgramAttribute {
        int programIndex;
        int typeKind;
        char *attributeName;
};

/* for parsing state */
struct GP_FileStackItem {
        const char *fileID;
        const char *contents;
        int size;
        int cursorPos;
        int savedCharacter;
        int haveSavedCharacter;

        // we need to remember the file position from which we would start the
        // next copy-input-to-output operation. Note that sometimes we skip a
        // section.
        int outputFilePosition;
        int indexOfFirstUnconsumedToken;
        // we can temporarily suppress copying input to output.
        int outputSuspended;
};

struct GP_Ctx {
        // copy of input data
        struct GP_Desc desc;

        // allocated and written in parsing stage
        struct GP_ShaderfileAst *shaderfileAsts;
        int currentShaderIndex;  // global state for simpler code

        /* This stuff here is completely computed from the parsed data. */
        struct GP_ProgramUniform *programUniforms;
        struct GP_ProgramAttribute *programAttributes;
        int numProgramUniforms;
        int numProgramAttributes;

        /*
         * PARSING STATE
         */

        /* the file stack is used for processing #include directives -
         * in general the parser reads from multiple files. */
        struct GP_FileStackItem *fileStack;
        int fileStackSize;
        /* the current fileInfo is duplicated here, to simplify the code. */
        struct GP_FileStackItem file;

        /* this is backing storage for dynamically allocated token data. It is
         * valid only for the last token that was lexed using this context. */
        int haveSavedToken;
        int tokenKind; // this will always be valid, even if !haveSavedToken
        double tokenFloatingValue;
        char *tokenBuffer;
        int tokenBufferLength;
        int tokenBufferCapacity;
};

void gp_setup(struct GP_Ctx *ctx);
void gp_teardown(struct GP_Ctx *ctx);
void gp_parse(struct GP_Ctx *ctx);


#endif
