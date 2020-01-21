#ifndef GLSLPROCESSOR_PARSE_H_INCLUDED
#define GLSLPROCESSOR_PARSE_H_INCLUDED


struct GP_Ctx {
        struct GP_FileInfo *fileInfo;
        struct GP_ProgramInfo *programInfo;
        struct GP_ShaderInfo *shaderInfo;
        struct GP_LinkInfo *linkInfo;
        int numFiles;
        int numPrograms;
        int numShaders;
        int numLinks;

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

        const char *filepath;
        const char *fileContents;
        int fileSize;
        int cursorPos;
        int savedCharacter;
        int haveSavedCharacter;
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

void gp_parse_shader(struct GP_Ctx *ctx, int shaderIndex);

#endif
