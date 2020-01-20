#include <glsl-processor/logging.h>
#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/ast.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>



#ifdef _MSC_VER
#include <Windows.h>
static void make_directory_if_not_exists(const char *dirpath)
{
        BOOL ret = CreateDirectory(dirpath, NULL);
        if (!ret && GetLastError() != ERROR_ALREADY_EXISTS)
                fatal_f("Failed to create directory %s", dirpath);
}
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
static void make_directory_if_not_exists(const char *dirpath)
{
        int r = mkdir(dirpath, 0770);
        if (r == -1 && errno != EEXIST)
                fatal_f("Failed to create directory %s: %s",
                        dirpath, strerror(errno));
}
#endif

// TODO
#define AUTOGENERATED_DIR "autogenerated/"
#define IFACE_C_FILEPATH AUTOGENERATED_DIR "shaders.c"
#define IFACE_H_FILEPATH AUTOGENERATED_DIR "shaders.h"

#define INDENT "        "

struct MemoryBuffer {
        char *data;
        size_t length;
};

struct MtsCtx {
        struct Ast *ast;
        struct MemoryBuffer hFilepath;
        struct MemoryBuffer cFilepath;
        struct MemoryBuffer hFileHandle;
        struct MemoryBuffer cFileHandle;
};

static void commit_to_file(struct MemoryBuffer *handle, const char *filepath)
{
        // writing only if file is different. This is to avoid the IDE unnecessary noticing File changes.
        int different = 0;
        {
                FILE *f = fopen(filepath, "rb");
                if (f == NULL)
                        different = 1;
                else {
                        fseek(f, 0, SEEK_END);
                        long contentsLength = ftell(f);
                        if (contentsLength == -1)
                                fatal_f("Failed to ftell() file '%s': %s", filepath, strerror(errno));
                        if (contentsLength != handle->length)
                                different = 1;
                        else {
                                fseek(f, 0, SEEK_SET);
                                char *contents;
                                ALLOC_MEMORY(&contents, contentsLength + 1);
                                size_t readBytes = fread(contents, 1, contentsLength + 1, f);
                                if (readBytes != contentsLength)
                                        fatal_f("File '%s' seems to have changed during the commit");
                                if (memcmp(contents, handle->data, handle->length) != 0)
                                        different = 1;
                                FREE_MEMORY(&contents);
                        }
                        fclose(f);
                }
        }
        if (different) {
                FILE *f = fopen(filepath, "wb");
                if (f == NULL)
                        fatal_f("Failed to open '%s' for writing", filepath);
                fwrite(handle->data, 1, handle->length, f);
                fflush(f);
                if (ferror(f))
                        fatal_f("I/O error while writing '%s'", filepath);
                fclose(f);
        }
}

static void append_to_buffer_fv(struct MemoryBuffer *handle, const char *fmt, va_list ap)
{
        /* We're not allowed to use "ap" twice, and on gcc using it twice
        actually resulted in a segfault. So we make a backup copy before giving
        it to vsnprintf(). */
        va_list ap2;
        va_copy(ap2, ap);
        size_t need = vsnprintf(NULL, 0, fmt, ap);
        REALLOC_MEMORY(&handle->data, handle->length + need + 1);
        vsnprintf(handle->data + handle->length, need + 1, fmt, ap2);
        handle->data[handle->length + need] = '\0';
        handle->length += need;
}

static void append_to_buffer_f(struct MemoryBuffer *handle, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        append_to_buffer_fv(handle, fmt, ap);
        va_end(ap);
}

static void append_to_buffer(struct MemoryBuffer *handle, const char *data)
{
        append_to_buffer_f(handle, "%s", data);
}

static void append_filepath_component(struct MemoryBuffer *buf, const char *comp)
{
        // TODO: make code more correct, at least for Windows and Linux
        if (!(buf->length == 0 || (buf->data[buf->length - 1] == '/'
#ifdef _MSC_VER
            || buf->data[buf->length - 1] == '\\'
#endif
           ))) {
                append_to_buffer(buf, "/");
        }
        append_to_buffer(buf, comp);
}

static void teardown_buffer(struct MemoryBuffer *handle)
{
        FREE_MEMORY(&handle->data);
        memset(handle, 0, sizeof *handle);
}


static void begin_enum(struct MtsCtx *ctx)
{
        append_to_buffer_f(&ctx->hFileHandle, "enum {\n");
}

static void end_enum(struct MtsCtx *ctx)
{
        append_to_buffer_f(&ctx->hFileHandle, "};\n\n");
}

static void add_enum_item(struct MtsCtx *ctx, const char *name1)
{
        append_to_buffer_f(&ctx->hFileHandle, INDENT "%s,\n", name1);
}

static void add_enum_item_2(struct MtsCtx *ctx, const char *name1, const char *name2)
{
        append_to_buffer_f(&ctx->hFileHandle, INDENT "%s_%s,\n", name1, name2);
}

static void add_enum_item_3(struct MtsCtx *ctx, const char *name1, const char *name2, const char *name3)
{
        append_to_buffer_f(&ctx->hFileHandle, INDENT "%s_%s_%s,\n", name1, name2, name3);
}

static const char *const PRIMTYPE_to_GRAFIKATTRIBUTETYPE[NUM_PRIMTYPE_KINDS] = {
        [PRIMTYPE_BOOL] = "GRAFIKATTRTYPE_BOOL",
        [PRIMTYPE_INT] = "GRAFIKATTRTYPE_INT",
        [PRIMTYPE_UINT] = "GRAFIKATTRTYPE_UINT",
        [PRIMTYPE_FLOAT] = "GRAFIKATTRTYPE_FLOAT",
        [PRIMTYPE_DOUBLE] = "GRAFIKUNIFORMTYPE_DOUBLE",
        [PRIMTYPE_VEC2] = "GRAFIKATTRTYPE_VEC2",
        [PRIMTYPE_VEC3] = "GRAFIKATTRTYPE_VEC3",
        [PRIMTYPE_VEC4] = "GRAFIKATTRTYPE_VEC4",
};

static const char *const PRIMTYPE_to_GRAFIKUNIFORMTYPE[NUM_PRIMTYPE_KINDS] = {
        [PRIMTYPE_BOOL] = "GRAFIKUNIFORMTYPE_BOOL",
        [PRIMTYPE_INT] = "GRAFIKUNIFORMTYPE_INT",
        [PRIMTYPE_UINT] = "GRAFIKUNIFORMTYPE_UINT",
        [PRIMTYPE_FLOAT] = "GRAFIKUNIFORMTYPE_FLOAT",
        [PRIMTYPE_DOUBLE] = "GRAFIKUNIFORMTYPE_DOUBLE",
        [PRIMTYPE_VEC2] = "GRAFIKUNIFORMTYPE_VEC2",
        [PRIMTYPE_VEC3] = "GRAFIKUNIFORMTYPE_VEC3",
        [PRIMTYPE_VEC4] = "GRAFIKUNIFORMTYPE_VEC4",
        [PRIMTYPE_MAT2] = "GRAFIKUNIFORMTYPE_MAT2",
        [PRIMTYPE_MAT3] = "GRAFIKUNIFORMTYPE_MAT3",
        [PRIMTYPE_MAT4] = "GRAFIKUNIFORMTYPE_MAT4",
};


void write_c_interface(struct Ast *ast, const char *autogenDirpath)
{
        struct MtsCtx mtsCtx = { 0 };
        struct MtsCtx *ctx = &mtsCtx;

        append_filepath_component(&ctx->hFilepath, autogenDirpath);
        append_filepath_component(&ctx->cFilepath, autogenDirpath);
        append_filepath_component(&ctx->hFilepath, "shaders.h");
        append_filepath_component(&ctx->cFilepath, "shaders.c");

        append_to_buffer_f(&ctx->hFileHandle,
                "#ifndef AUTOGENERATED_SHADERS_H_INCLUDED\n"
                "#define AUTOGENERATED_SHADERS_H_INCLUDED\n"
                "\n"
                "#include <glsl-processor.h>\n"
                "\n"
                "#ifdef __cplusplus\n"
                "extern \"C\" {\n"
                "#endif\n"
                "\n");

        begin_enum(ctx);
        for (int i = 0; i < ast->numPrograms; i++)
                add_enum_item_2(ctx, "PROGRAM", ast->programInfo[i].programName);
        add_enum_item(ctx, "NUM_PROGRAM_KINDS");
        end_enum(ctx);

        begin_enum(ctx);
        for (int i = 0; i < ast->numShaders; i++)
                add_enum_item_2(ctx, "SHADER", ast->shaderInfo[i].shaderName);
        add_enum_item(ctx, "NUM_SHADER_KINDS");
        end_enum(ctx);

        begin_enum(ctx);
        for (int i = 0; i < ast->numProgramUniforms; i++) {
                int programIndex = ast->programUniforms[i].programIndex;
                const char *programName = ast->programInfo[programIndex].programName;
                const char *uniformName = ast->programUniforms[i].uniformName;
                add_enum_item_3(ctx, "UNIFORM", programName, uniformName);
        }
        add_enum_item(ctx, "NUM_UNIFORM_KINDS");
        end_enum(ctx);

        begin_enum(ctx);
        for (int i = 0; i < ast->numProgramAttributes; i++) {
                int programIndex = ast->programAttributes[i].programIndex;
                const char *programName = ast->programInfo[programIndex].programName;
                const char *attributeName = ast->programAttributes[i].attributeName;
                add_enum_item_3(ctx, "ATTRIBUTE", programName, attributeName);
        }
        add_enum_item(ctx, "NUM_ATTRIBUTE_KINDS");
        end_enum(ctx);

        append_to_buffer_f(&ctx->hFileHandle,
                "extern const struct SM_ShaderInfo smShaderInfo[NUM_SHADER_KINDS];\n"
                "extern const struct SM_ProgramInfo smProgramInfo[NUM_PROGRAM_KINDS];\n"
                "extern const struct SM_LinkInfo smLinkInfo[];\n"
                "extern const int numLinkInfos;\n"
                "extern const struct SM_UniformInfo smUniformInfo[NUM_UNIFORM_KINDS];\n"
                "extern const struct SM_AttributeInfo smAttributeInfo[NUM_ATTRIBUTE_KINDS];\n"
                "extern const struct SM_Description smDescription;\n"
                "\n"
        );

        append_to_buffer_f(&ctx->cFileHandle, "#include <shaders.h>\n\n");

        append_to_buffer_f(&ctx->cFileHandle, "const struct SM_ProgramInfo smProgramInfo[NUM_PROGRAM_KINDS] = {\n");
        for (int i = 0; i < ast->numPrograms; i++) {
                const char *programName = ast->programInfo[i].programName;
                append_to_buffer_f(&ctx->cFileHandle, INDENT "[PROGRAM_%s] = { \"%s\" },\n", programName, programName);
        }
        append_to_buffer_f(&ctx->cFileHandle, "};\n\n");

        append_to_buffer_f(&ctx->cFileHandle, "const struct SM_ShaderInfo smShaderInfo[NUM_SHADER_KINDS] = {\n");


#if 0 // TODO: shadef filepath is no longer available. Reorganize file reading!
        for (int i = 0; i < ast->numShaders; i++) {
                struct ShaderInfo *info = &ast->shaderInfo[i];
                char *shaderName = info->shaderName;
                char *shaderFilepath = info->shaderFilepath;
                static const char *const basePath = ""; //XXX
                char *fullyQualifiedFilepath;
                ALLOC_MEMORY(&fullyQualifiedFilepath, strlen(basePath) + strlen(shaderFilepath) + 1);
                memcpy(fullyQualifiedFilepath, basePath, strlen(basePath));
                memcpy(fullyQualifiedFilepath + strlen(basePath), shaderFilepath, strlen(shaderFilepath) + 1);
                int shaderTypeKind = info->shaderType;
                append_to_buffer_f(&ctx->cFileHandle, INDENT "[SHADER_%s] = { %s, \"%s\", \"%s\" },\n",
                        shaderName, shadertypeKindString[shaderTypeKind], shaderName, fullyQualifiedFilepath);
                FREE_MEMORY(&fullyQualifiedFilepath);
        }
#endif
        append_to_buffer_f(&ctx->cFileHandle, "};\n\n");
        
        append_to_buffer_f(&ctx->cFileHandle, "const struct SM_LinkInfo smLinkInfo[] = {\n");
        for (int i = 0; i < ast->numLinks; i++) {
                //int programIndex = ast->linkItems[i].resolvedProgramIndex;
                //int shaderIndex = ast->linkItems[i].resolvedShaderIndex;
                const char *programName = ast->programInfo[ast->linkInfo[i].programIndex].programName;
                const char *shaderName = ast->shaderInfo[ast->linkInfo[i].shaderIndex].shaderName;
                append_to_buffer_f(&ctx->cFileHandle, INDENT "{ PROGRAM_%s, SHADER_%s },\n", programName, shaderName);
        }
        append_to_buffer_f(&ctx->cFileHandle, "};\n\n");
        append_to_buffer_f(&ctx->cFileHandle, "const int numLinkInfos = sizeof smLinkInfo / sizeof smLinkInfo[0];\n\n");

        append_to_buffer_f(&ctx->cFileHandle, "const struct SM_UniformInfo smUniformInfo[NUM_UNIFORM_KINDS] = {\n");

        for (int i = 0; i < ast->numProgramUniforms; i++) {
                int programIndex = ast->programUniforms[i].programIndex;
                int primtypeKind = ast->programUniforms[i].typeKind;
                const char *programName = ast->programInfo[programIndex].programName;
                const char *uniformName = ast->programUniforms[i].uniformName;
                const char *typeName = PRIMTYPE_to_GRAFIKUNIFORMTYPE[primtypeKind];
                ENSURE(typeName != NULL);
                append_to_buffer_f(&ctx->cFileHandle, INDENT "[UNIFORM_%s_%s] = { PROGRAM_%s, %s, \"%s\" },\n",
                        programName, uniformName, programName, typeName, uniformName);
        }
        append_to_buffer_f(&ctx->cFileHandle, "};\n\n");

        append_to_buffer_f(&ctx->cFileHandle, "const struct SM_AttributeInfo smAttributeInfo[NUM_ATTRIBUTE_KINDS] = {\n");
        for (int i = 0; i < ast->numProgramAttributes; i++) {
                int programIndex = ast->programAttributes[i].programIndex;
                int primtypeKind = ast->programAttributes[i].typeKind;
                const char *programName = ast->programInfo[programIndex].programName;
                const char *attributeName = ast->programAttributes[i].attributeName;
                const char *typeName = PRIMTYPE_to_GRAFIKATTRIBUTETYPE[primtypeKind];
                ENSURE(typeName != NULL);
                append_to_buffer_f(&ctx->cFileHandle, INDENT "[ATTRIBUTE_%s_%s] = { PROGRAM_%s, %s, \"%s\" },\n",
                        programName, attributeName, programName, typeName, attributeName);
        }
        append_to_buffer_f(&ctx->cFileHandle, "};\n\n");

        append_to_buffer_f(&ctx->hFileHandle,
                "extern GfxShader gfxShader[NUM_SHADER_KINDS];\n"
                "extern GfxProgram gfxProgram[NUM_PROGRAM_KINDS];\n"
                "extern GfxUniformLocation gfxUniformLocation[NUM_UNIFORM_KINDS];\n"
                "extern GfxAttributeLocation gfxAttributeLocation[NUM_ATTRIBUTE_KINDS];\n"
                "\n"
        );

        append_to_buffer_f(&ctx->cFileHandle,
                "GfxProgram gfxProgram[NUM_PROGRAM_KINDS];\n"
                "GfxShader gfxShader[NUM_SHADER_KINDS];\n"
                "GfxUniformLocation gfxUniformLocation[NUM_UNIFORM_KINDS];\n"
                "GfxAttributeLocation gfxAttributeLocation[NUM_ATTRIBUTE_KINDS];\n"
                "\n"
        );

        append_to_buffer_f(&ctx->cFileHandle,
                "const struct SM_Description smDescription = {\n"
                INDENT ".programInfo = smProgramInfo,\n"
                INDENT ".shaderInfo = smShaderInfo,\n"
                INDENT ".linkInfo = smLinkInfo,\n"
                INDENT ".uniformInfo = smUniformInfo,\n"
                INDENT ".attributeInfo = smAttributeInfo,\n"
                "\n"
                INDENT ".gfxProgram = gfxProgram,\n"
                INDENT ".gfxShader = gfxShader,\n"
                INDENT ".gfxUniformLocation = gfxUniformLocation,\n"
                INDENT ".gfxAttributeLocation = gfxAttributeLocation,\n"
                "\n"
                INDENT ".numPrograms = NUM_PROGRAM_KINDS,\n"
                INDENT ".numShaders = NUM_SHADER_KINDS,\n"
                INDENT ".numUniforms = NUM_UNIFORM_KINDS,\n"
                INDENT ".numAttributes = NUM_ATTRIBUTE_KINDS,\n"
                INDENT ".numLinks = sizeof smLinkInfo / sizeof smLinkInfo[0],\n"
                "};\n\n"
        );

        for (int i = 0; i < ast->numProgramUniforms; i++) {
                int programIndex = ast->programUniforms[i].programIndex;
                int primtypeKind = ast->programUniforms[i].typeKind;
                const char *programName = ast->programInfo[programIndex].programName;
                if (i == 0 || programIndex != ast->programUniforms[i - 1].programIndex) {
                        append_to_buffer_f(&ctx->hFileHandle, "static inline void %sShader_render(GfxVAO vao, int firstVertice, int length) { render_with_GfxProgram(gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName, programName);
                        append_to_buffer_f(&ctx->hFileHandle, "static inline void %sShader_render_primitive(int gfxPrimitiveKind, GfxVAO vao, int firstVertice, int length) { render_primitive_with_GfxProgram(gfxPrimitiveKind, gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName, programName);
                }
                const char *uniformName = ast->programUniforms[i].uniformName;
                append_to_buffer_f(&ctx->hFileHandle, "static inline void %sShader_set_%s", programName, uniformName);
                const char *fmt;
                switch (primtypeKind) {
                case PRIMTYPE_FLOAT: fmt = "(float x) { set_GfxProgram_uniform_1f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x); }\n"; break;
                case PRIMTYPE_VEC2: fmt = "(float x, float y) { set_GfxProgram_uniform_2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y); }\n"; break;
                case PRIMTYPE_VEC3: fmt = "(float x, float y, float z) { set_GfxProgram_uniform_3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z); }\n"; break;
                case PRIMTYPE_VEC4: fmt = "(float x, float y, float z, float w) { set_GfxProgram_uniform_4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z, w); }\n"; break;
                case PRIMTYPE_MAT2: fmt = "(float *fourFloats) { set_GfxProgram_uniform_mat2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], fourFloats); }\n"; break;
                case PRIMTYPE_MAT3: fmt = "(float *nineFloats) { set_GfxProgram_uniform_mat3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], nineFloats); }\n"; break;
                case PRIMTYPE_MAT4: fmt = "(float *sixteenFloats) { set_GfxProgram_uniform_mat4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], sixteenFloats); }\n"; break;
                case PRIMTYPE_SAMPLER2D: continue;  // cannot be set, can it?
                default: fatal_f("Not implemented!");
                }
                append_to_buffer_f(&ctx->hFileHandle, fmt, programName, programName, uniformName);
        }

        append_to_buffer_f(&ctx->hFileHandle,
                "\n"
                "\n"
                "#ifdef __cplusplus\n\n");
        for (int i = 0; i < ast->numProgramUniforms; i++) {
                int programIndex = ast->programUniforms[i].programIndex;
                int primtypeKind = ast->programUniforms[i].typeKind;
                const char *programName = ast->programInfo[programIndex].programName;
                const char *uniformName = ast->programUniforms[i].uniformName;
                if (i == 0 || programIndex != ast->programUniforms[i - 1].programIndex) {
                        append_to_buffer_f(&ctx->hFileHandle, "static struct {\n");
                        append_to_buffer_f(&ctx->hFileHandle, INDENT "static inline void render(GfxVAO vao, int firstVertice, int length) { render_with_GfxProgram(gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName);
                        append_to_buffer_f(&ctx->hFileHandle, INDENT "static inline void render_primitive(int gfxPrimitiveKind, GfxVAO vao, int firstVertice, int length) { render_with_GfxProgram(int gfxPrimitiveKind, gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName);
                }
                append_to_buffer_f(&ctx->hFileHandle, INDENT "static inline void set_%s", uniformName);
                const char *fmt;
                switch (primtypeKind) {
                case PRIMTYPE_FLOAT: fmt = "(float x) { set_GfxProgram_uniform_1f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x); }\n"; break;
                case PRIMTYPE_VEC2: fmt = "(float x, float y) { set_GfxProgram_uniform_2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y); }\n"; break;
                case PRIMTYPE_VEC3: fmt = "(float x, float y, float z) { set_GfxProgram_uniform_3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z); }\n"; break;
                case PRIMTYPE_VEC4: fmt = "(float x, float y, float z, float w) { set_GfxProgram_uniform_4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z, w); }\n"; break;
                case PRIMTYPE_MAT2: fmt = "(float *fourFloats) { set_GfxProgram_uniform_mat2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], fourFloats); }\n"; break;
                case PRIMTYPE_MAT3: fmt = "(float *nineFloats) { set_GfxProgram_uniform_mat3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], nineFloats); }\n"; break;
                case PRIMTYPE_MAT4: fmt = "(float *sixteenFloats) { set_GfxProgram_uniform_mat4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], sixteenFloats); }\n"; break;
                default: fatal_f("Not implemented!");
                }
                append_to_buffer_f(&ctx->hFileHandle, fmt, programName, programName, uniformName);
                if (i + 1 == ast->numProgramUniforms || programIndex != ast->programUniforms[i + 1].programIndex)
                        append_to_buffer_f(&ctx->hFileHandle, "} %sShader;\n\n", programName);
        }
        append_to_buffer_f(&ctx->hFileHandle, "#endif // #ifdef __cplusplus\n\n");


#if 0
        /* XXX this is really specific to the current setup. Separate stuff out! */
        fprintf(ctx->cFileHandle, "const struct SM_AttributeInfo smAttributeInfo[NUM_ATTRIBUTE_KINDS] = {\n");
        for (int programIndex = 0; programIndex < ast->numPrograms; programIndex++) {
                const char *programName = ast->programInfo[programIndex].programName;
                struct VariableIterator variableIter;
                for (begin_iterating_variables(&variableIter, ast, programIndex);
                        have_variable(&variableIter); go_to_next_variable(&variableIter))
                {
                        struct VariableDecl *decl = get_variable(&variableIter);
                        if (decl->inOrOut != 0) /* TODO: enum for this. 0 == IN direction */
                                continue;
                        { // this code is too complex. It should be easier to determine the shader type.
                                int shaderIndex = ast->linkItems[variableIter.shaderIter.linkIndex].resolvedShaderIndex;
                                int shaderType = ast->shaderInfo[shaderIndex].shaderType;
                                if (shaderType != SHADERTYPE_VERTEX)
                                        continue;  // only vertex shaders can contain attributes. Attributes are "in" variables of the vertex shader.
                        }
                        const char *attribName = decl->name;
                        int primtypeKind = decl->typeExpr->primtypeKind;
                        const char *typeName = primtypeKindString[primtypeKind];

                        const char *structName = 

                        fprintf(ctx->cFileHandle, "[ATTRIBUTE_%s_%s] = { PROGRAM_%s, ATTRIBUTE_%s_%s, VAO_%s, VBO_THE_ONE_AND_ONLY, %s, %d, %d }\n",
                                programName, attribName, programName, programName, attribName, programName, typeName, 
                        );
                }
        }
#endif

        append_to_buffer_f(&ctx->hFileHandle,
                "#ifdef __cplusplus\n"
                "} // extern \"C\" {\n"
                "#endif\n"
                "#endif\n");


        make_directory_if_not_exists(autogenDirpath);
        commit_to_file(&ctx->hFileHandle, ctx->hFilepath.data);
        commit_to_file(&ctx->cFileHandle, ctx->cFilepath.data);

        teardown_buffer(&ctx->hFilepath);
        teardown_buffer(&ctx->cFilepath);
        teardown_buffer(&ctx->hFileHandle);
        teardown_buffer(&ctx->cFileHandle);
}
