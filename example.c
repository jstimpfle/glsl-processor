#include <glsl-processor/defs.h>
#include <glsl-processor/logging.h>
#include <glsl-processor/memory.h>
#include <glsl-processor/parse.h>
#include <glsl-processor/builder.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#include <Windows.h>
static void make_directory_if_not_exists(const char *dirpath)
{
        BOOL ret = CreateDirectory(dirpath, NULL);
        if (!ret && GetLastError() != ERROR_ALREADY_EXISTS)
                gp_fatal_f("Failed to create directory %s", dirpath);
}
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
static void make_directory_if_not_exists(const char *dirpath)
{
        int r = mkdir(dirpath, 0770);
        if (r == -1 && errno != EEXIST)
                gp_fatal_f("Failed to create directory %s: %s",
                           dirpath, strerror(errno));
}
#endif

#define INDENT "        "

struct MemoryBuffer {
        char *data;
        size_t length;
};

struct WriteCtx {
        struct GP_Ctx *ctx;
        struct MemoryBuffer hFilepath;
        struct MemoryBuffer cFilepath;
        struct MemoryBuffer hFileHandle;
        struct MemoryBuffer cFileHandle;
};

static void commit_to_file(struct MemoryBuffer *mb, const char *filepath)
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
                                gp_fatal_f("Failed to ftell() file '%s': %s", filepath, strerror(errno));
                        if (contentsLength != mb->length)
                                different = 1;
                        else {
                                fseek(f, 0, SEEK_SET);
                                char *contents;
                                ALLOC_MEMORY(&contents, contentsLength + 1);
                                size_t readBytes = fread(contents, 1, contentsLength + 1, f);
                                if (readBytes != contentsLength)
                                        gp_fatal_f("File '%s' seems to have changed during the commit");
                                if (memcmp(contents, mb->data, mb->length) != 0)
                                        different = 1;
                                FREE_MEMORY(&contents);
                        }
                        fclose(f);
                }
        }
        if (different) {
                FILE *f = fopen(filepath, "wb");
                if (f == NULL)
                        gp_fatal_f("Failed to open '%s' for writing", filepath);
                fwrite(mb->data, 1, mb->length, f);
                fflush(f);
                if (ferror(f))
                        gp_fatal_f("I/O error while writing '%s'", filepath);
                fclose(f);
        }
}

static void append_to_buffer_fv(struct MemoryBuffer *mb, const char *fmt, va_list ap)
{
        /* We're not allowed to use "ap" twice, and on gcc using it twice
        actually resulted in a segfault. So we make a backup copy before giving
        it to vsnprintf(). */
        va_list ap2;
        va_copy(ap2, ap);
        size_t need = vsnprintf(NULL, 0, fmt, ap);
        REALLOC_MEMORY(&mb->data, mb->length + need + 1);
        vsnprintf(mb->data + mb->length, need + 1, fmt, ap2);
        mb->data[mb->length + need] = '\0';
        mb->length += need;
}

static void append_to_buffer_f(struct MemoryBuffer *mb, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        append_to_buffer_fv(mb, fmt, ap);
        va_end(ap);
}

static void append_to_buffer(struct MemoryBuffer *mb, const char *data)
{
        append_to_buffer_f(mb, "%s", data);
}

static void append_filepath_component(struct MemoryBuffer *mb, const char *comp)
{
        // TODO: make code more correct, at least for Windows and Linux
        if (!(mb->length == 0 || (mb->data[mb->length - 1] == '/'
#ifdef _MSC_VER
            || mb->data[mb->length - 1] == '\\'
#endif
           ))) {
                append_to_buffer(mb, "/");
        }
        append_to_buffer(mb, comp);
}

static void teardown_buffer(struct MemoryBuffer *mb)
{
        FREE_MEMORY(&mb->data);
        memset(mb, 0, sizeof *mb);
}


static void begin_enum(struct WriteCtx *wc)
{
        append_to_buffer_f(&wc->hFileHandle, "enum {\n");
}

static void end_enum(struct WriteCtx *wc)
{
        append_to_buffer_f(&wc->hFileHandle, "};\n\n");
}

static void add_enum_item(struct WriteCtx *wc, const char *name1)
{
        append_to_buffer_f(&wc->hFileHandle, INDENT "%s,\n", name1);
}

static void add_enum_item_2(struct WriteCtx *wc, const char *name1, const char *name2)
{
        append_to_buffer_f(&wc->hFileHandle, INDENT "%s_%s,\n", name1, name2);
}

static void add_enum_item_3(struct WriteCtx *wc, const char *name1, const char *name2, const char *name3)
{
        append_to_buffer_f(&wc->hFileHandle, INDENT "%s_%s_%s,\n", name1, name2, name3);
}

static const char *const PRIMTYPE_to_GRAFIKATTRIBUTETYPE[GP_NUM_PRIMTYPE_KINDS] = {
        [GP_PRIMTYPE_BOOL] = "GRAFIKATTRTYPE_BOOL",
        [GP_PRIMTYPE_INT] = "GRAFIKATTRTYPE_INT",
        [GP_PRIMTYPE_UINT] = "GRAFIKATTRTYPE_UINT",
        [GP_PRIMTYPE_FLOAT] = "GRAFIKATTRTYPE_FLOAT",
        [GP_PRIMTYPE_DOUBLE] = "GRAFIKUNIFORMTYPE_DOUBLE",
        [GP_PRIMTYPE_VEC2] = "GRAFIKATTRTYPE_VEC2",
        [GP_PRIMTYPE_VEC3] = "GRAFIKATTRTYPE_VEC3",
        [GP_PRIMTYPE_VEC4] = "GRAFIKATTRTYPE_VEC4",
};

static const char *const PRIMTYPE_to_GRAFIKUNIFORMTYPE[GP_NUM_PRIMTYPE_KINDS] = {
        [GP_PRIMTYPE_BOOL] = "GRAFIKUNIFORMTYPE_BOOL",
        [GP_PRIMTYPE_INT] = "GRAFIKUNIFORMTYPE_INT",
        [GP_PRIMTYPE_UINT] = "GRAFIKUNIFORMTYPE_UINT",
        [GP_PRIMTYPE_FLOAT] = "GRAFIKUNIFORMTYPE_FLOAT",
        [GP_PRIMTYPE_DOUBLE] = "GRAFIKUNIFORMTYPE_DOUBLE",
        [GP_PRIMTYPE_VEC2] = "GRAFIKUNIFORMTYPE_VEC2",
        [GP_PRIMTYPE_VEC3] = "GRAFIKUNIFORMTYPE_VEC3",
        [GP_PRIMTYPE_VEC4] = "GRAFIKUNIFORMTYPE_VEC4",
        [GP_PRIMTYPE_MAT2] = "GRAFIKUNIFORMTYPE_MAT2",
        [GP_PRIMTYPE_MAT3] = "GRAFIKUNIFORMTYPE_MAT3",
        [GP_PRIMTYPE_MAT4] = "GRAFIKUNIFORMTYPE_MAT4",
};

void write_c_interface(struct GP_Ctx *ctx, const char *autogenDirpath)
{
        struct WriteCtx mtsCtx = { 0 };
        struct WriteCtx *wc = &mtsCtx;

        append_filepath_component(&wc->hFilepath, autogenDirpath);
        append_filepath_component(&wc->cFilepath, autogenDirpath);
        append_filepath_component(&wc->hFilepath, "shaders.h");
        append_filepath_component(&wc->cFilepath, "shaders.c");

        append_to_buffer_f(&wc->hFileHandle,
                "#ifndef AUTOGENERATED_SHADERS_H_INCLUDED\n"
                "#define AUTOGENERATED_SHADERS_H_INCLUDED\n"
                "\n"
                "#include <glsl-processor.h>\n"
                "\n"
                "#ifdef __cplusplus\n"
                "extern \"C\" {\n"
                "#endif\n"
                "\n");

        begin_enum(wc);
        for (int i = 0; i < ctx->desc.numPrograms; i++)
                add_enum_item_2(wc, "PROGRAM", ctx->desc.programInfo[i].programName);
        add_enum_item(wc, "NUM_PROGRAM_KINDS");
        end_enum(wc);

        begin_enum(wc);
        for (int i = 0; i < ctx->desc.numShaders; i++)
                add_enum_item_2(wc, "SHADER", ctx->desc.shaderInfo[i].shaderName);
        add_enum_item(wc, "NUM_SHADER_KINDS");
        end_enum(wc);

        begin_enum(wc);
        for (int i = 0; i < ctx->numProgramUniforms; i++) {
                int programIndex = ctx->programUniforms[i].programIndex;
                const char *programName = ctx->desc.programInfo[programIndex].programName;
                const char *uniformName = ctx->programUniforms[i].uniformName;
                add_enum_item_3(wc, "UNIFORM", programName, uniformName);
        }
        add_enum_item(wc, "NUM_UNIFORM_KINDS");
        end_enum(wc);

        begin_enum(wc);
        for (int i = 0; i < ctx->numProgramAttributes; i++) {
                int programIndex = ctx->programAttributes[i].programIndex;
                const char *programName = ctx->desc.programInfo[programIndex].programName;
                const char *attributeName = ctx->programAttributes[i].attributeName;
                add_enum_item_3(wc, "ATTRIBUTE", programName, attributeName);
        }
        add_enum_item(wc, "NUM_ATTRIBUTE_KINDS");
        end_enum(wc);

        append_to_buffer_f(&wc->hFileHandle,
                "extern const struct SM_ShaderInfo smShaderInfo[NUM_SHADER_KINDS];\n"
                "extern const struct SM_ProgramInfo smProgramInfo[NUM_PROGRAM_KINDS];\n"
                "extern const struct SM_LinkInfo smLinkInfo[];\n"
                "extern const int numLinkInfos;\n"
                "extern const struct SM_UniformInfo smUniformInfo[NUM_UNIFORM_KINDS];\n"
                "extern const struct SM_AttributeInfo smAttributeInfo[NUM_ATTRIBUTE_KINDS];\n"
                "extern const struct SM_Description smDescription;\n"
                "\n"
        );

        append_to_buffer_f(&wc->cFileHandle, "#include <shaders.h>\n\n");

        append_to_buffer_f(&wc->cFileHandle, "const struct SM_ProgramInfo smProgramInfo[NUM_PROGRAM_KINDS] = {\n");
        for (int i = 0; i < ctx->desc.numPrograms; i++) {
                const char *programName = ctx->desc.programInfo[i].programName;
                append_to_buffer_f(&wc->cFileHandle, INDENT "[PROGRAM_%s] = { \"%s\" },\n", programName, programName);
        }
        append_to_buffer_f(&wc->cFileHandle, "};\n\n");

        append_to_buffer_f(&wc->cFileHandle, "const struct SM_ShaderInfo smShaderInfo[NUM_SHADER_KINDS] = {\n");
        for (int i = 0; i < ctx->desc.numShaders; i++) {
                struct GP_ShaderInfo *info = &ctx->desc.shaderInfo[i];
                append_to_buffer_f(&wc->cFileHandle, INDENT "[SHADER_%s] = { %s, \"%s\", \"%s\" },\n",
                        info->shaderName, gp_shadertypeKindString[info->shaderType], info->shaderName, info->fileID);
        }
        append_to_buffer_f(&wc->cFileHandle, "};\n\n");
        
        append_to_buffer_f(&wc->cFileHandle, "const struct SM_LinkInfo smLinkInfo[] = {\n");
        for (int i = 0; i < ctx->desc.numLinks; i++) {
                int programIndex = ctx->desc.linkInfo[i].programIndex;
                int shaderIndex = ctx->desc.linkInfo[i].shaderIndex;
                const char *programName = ctx->desc.programInfo[programIndex].programName;
                const char *shaderName = ctx->desc.shaderInfo[shaderIndex].shaderName;
                append_to_buffer_f(&wc->cFileHandle, INDENT "{ PROGRAM_%s, SHADER_%s },\n", programName, shaderName);
        }
        append_to_buffer_f(&wc->cFileHandle, "};\n\n");
        append_to_buffer_f(&wc->cFileHandle, "const int numLinkInfos = sizeof smLinkInfo / sizeof smLinkInfo[0];\n\n");

        append_to_buffer_f(&wc->cFileHandle, "const struct SM_UniformInfo smUniformInfo[NUM_UNIFORM_KINDS] = {\n");
        for (int i = 0; i < ctx->numProgramUniforms; i++) {
                int programIndex = ctx->programUniforms[i].programIndex;
                int primtypeKind = ctx->programUniforms[i].typeKind;
                const char *programName = ctx->desc.programInfo[programIndex].programName;
                const char *uniformName = ctx->programUniforms[i].uniformName;
                const char *typeName = PRIMTYPE_to_GRAFIKUNIFORMTYPE[primtypeKind];
                GP_ENSURE(typeName != NULL);
                append_to_buffer_f(&wc->cFileHandle, INDENT "[UNIFORM_%s_%s] = { PROGRAM_%s, %s, \"%s\" },\n",
                        programName, uniformName, programName, typeName, uniformName);
        }
        append_to_buffer_f(&wc->cFileHandle, "};\n\n");

        append_to_buffer_f(&wc->cFileHandle, "const struct SM_AttributeInfo smAttributeInfo[NUM_ATTRIBUTE_KINDS] = {\n");
        for (int i = 0; i < ctx->numProgramAttributes; i++) {
                int programIndex = ctx->programAttributes[i].programIndex;
                int primtypeKind = ctx->programAttributes[i].typeKind;
                const char *programName = ctx->desc.programInfo[programIndex].programName;
                const char *attributeName = ctx->programAttributes[i].attributeName;
                const char *typeName = PRIMTYPE_to_GRAFIKATTRIBUTETYPE[primtypeKind];
                GP_ENSURE(typeName != NULL);
                append_to_buffer_f(&wc->cFileHandle, INDENT "[ATTRIBUTE_%s_%s] = { PROGRAM_%s, %s, \"%s\" },\n",
                        programName, attributeName, programName, typeName, attributeName);
        }
        append_to_buffer_f(&wc->cFileHandle, "};\n\n");

        append_to_buffer_f(&wc->hFileHandle,
                "extern GfxShader gfxShader[NUM_SHADER_KINDS];\n"
                "extern GfxProgram gfxProgram[NUM_PROGRAM_KINDS];\n"
                "extern GfxUniformLocation gfxUniformLocation[NUM_UNIFORM_KINDS];\n"
                "extern GfxAttributeLocation gfxAttributeLocation[NUM_ATTRIBUTE_KINDS];\n"
                "\n"
        );

        append_to_buffer_f(&wc->cFileHandle,
                "GfxProgram gfxProgram[NUM_PROGRAM_KINDS];\n"
                "GfxShader gfxShader[NUM_SHADER_KINDS];\n"
                "GfxUniformLocation gfxUniformLocation[NUM_UNIFORM_KINDS];\n"
                "GfxAttributeLocation gfxAttributeLocation[NUM_ATTRIBUTE_KINDS];\n"
                "\n"
        );

        append_to_buffer_f(&wc->cFileHandle,
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

        for (int i = 0; i < ctx->numProgramUniforms; i++) {
                int programIndex = ctx->programUniforms[i].programIndex;
                int primtypeKind = ctx->programUniforms[i].typeKind;
                const char *programName = ctx->desc.programInfo[programIndex].programName;
                if (i == 0 || programIndex != ctx->programUniforms[i - 1].programIndex) {
                        append_to_buffer_f(&wc->hFileHandle, "static inline void %sShader_render(GfxVAO vao, int firstVertice, int length) { render_with_GfxProgram(gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName, programName);
                        append_to_buffer_f(&wc->hFileHandle, "static inline void %sShader_render_primitive(int gfxPrimitiveKind, GfxVAO vao, int firstVertice, int length) { render_primitive_with_GfxProgram(gfxPrimitiveKind, gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName, programName);
                }
                const char *uniformName = ctx->programUniforms[i].uniformName;
                append_to_buffer_f(&wc->hFileHandle, "static inline void %sShader_set_%s", programName, uniformName);
                const char *fmt;
                switch (primtypeKind) {
                case GP_PRIMTYPE_FLOAT: fmt = "(float x) { set_GfxProgram_uniform_1f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x); }\n"; break;
                case GP_PRIMTYPE_VEC2: fmt = "(float x, float y) { set_GfxProgram_uniform_2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y); }\n"; break;
                case GP_PRIMTYPE_VEC3: fmt = "(float x, float y, float z) { set_GfxProgram_uniform_3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z); }\n"; break;
                case GP_PRIMTYPE_VEC4: fmt = "(float x, float y, float z, float w) { set_GfxProgram_uniform_4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z, w); }\n"; break;
                case GP_PRIMTYPE_MAT2: fmt = "(float *fourFloats) { set_GfxProgram_uniform_mat2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], fourFloats); }\n"; break;
                case GP_PRIMTYPE_MAT3: fmt = "(float *nineFloats) { set_GfxProgram_uniform_mat3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], nineFloats); }\n"; break;
                case GP_PRIMTYPE_MAT4: fmt = "(float *sixteenFloats) { set_GfxProgram_uniform_mat4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], sixteenFloats); }\n"; break;
                case GP_PRIMTYPE_SAMPLER2D: continue;  // cannot be set, can it?
                default: gp_fatal_f("Not implemented!");
                }
                append_to_buffer_f(&wc->hFileHandle, fmt, programName, programName, uniformName);
        }

        append_to_buffer_f(&wc->hFileHandle,
                "\n"
                "\n"
                "#ifdef __cplusplus\n\n");
        for (int i = 0; i < ctx->numProgramUniforms; i++) {
                int programIndex = ctx->programUniforms[i].programIndex;
                int primtypeKind = ctx->programUniforms[i].typeKind;
                const char *uniformName = ctx->programUniforms[i].uniformName;
                const char *programName = ctx->desc.programInfo[programIndex].programName;
                if (i == 0 || programIndex != ctx->programUniforms[i - 1].programIndex) {
                        append_to_buffer_f(&wc->hFileHandle, "static struct {\n");
                        append_to_buffer_f(&wc->hFileHandle, INDENT "static inline void render(GfxVAO vao, int firstVertice, int length) { render_with_GfxProgram(gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName);
                        append_to_buffer_f(&wc->hFileHandle, INDENT "static inline void render_primitive(int gfxPrimitiveKind, GfxVAO vao, int firstVertice, int length) { render_with_GfxProgram(int gfxPrimitiveKind, gfxProgram[PROGRAM_%s], vao, firstVertice, length); }\n", programName);
                }
                append_to_buffer_f(&wc->hFileHandle, INDENT "static inline void set_%s", uniformName);
                const char *fmt;
                switch (primtypeKind) {
                case GP_PRIMTYPE_FLOAT: fmt = "(float x) { set_GfxProgram_uniform_1f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x); }\n"; break;
                case GP_PRIMTYPE_VEC2: fmt = "(float x, float y) { set_GfxProgram_uniform_2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y); }\n"; break;
                case GP_PRIMTYPE_VEC3: fmt = "(float x, float y, float z) { set_GfxProgram_uniform_3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z); }\n"; break;
                case GP_PRIMTYPE_VEC4: fmt = "(float x, float y, float z, float w) { set_GfxProgram_uniform_4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], x, y, z, w); }\n"; break;
                case GP_PRIMTYPE_MAT2: fmt = "(float *fourFloats) { set_GfxProgram_uniform_mat2f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], fourFloats); }\n"; break;
                case GP_PRIMTYPE_MAT3: fmt = "(float *nineFloats) { set_GfxProgram_uniform_mat3f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], nineFloats); }\n"; break;
                case GP_PRIMTYPE_MAT4: fmt = "(float *sixteenFloats) { set_GfxProgram_uniform_mat4f(gfxProgram[PROGRAM_%s], gfxUniformLocation[UNIFORM_%s_%s], sixteenFloats); }\n"; break;
                default: gp_fatal_f("Not implemented!");
                }
                append_to_buffer_f(&wc->hFileHandle, fmt, programName, programName, uniformName);
                if (i + 1 == ctx->numProgramUniforms || programIndex != ctx->programUniforms[i + 1].programIndex)
                        append_to_buffer_f(&wc->hFileHandle, "} %sShader;\n\n", programName);
        }
        append_to_buffer_f(&wc->hFileHandle, "#endif // #ifdef __cplusplus\n\n");

        append_to_buffer_f(&wc->hFileHandle,
                "#ifdef __cplusplus\n"
                "} // extern \"C\" {\n"
                "#endif\n"
                "#endif\n");

        make_directory_if_not_exists(autogenDirpath);
        commit_to_file(&wc->hFileHandle, wc->hFilepath.data);
        commit_to_file(&wc->cFileHandle, wc->cFilepath.data);

        teardown_buffer(&wc->hFilepath);
        teardown_buffer(&wc->cFilepath);
        teardown_buffer(&wc->hFileHandle);
        teardown_buffer(&wc->cFileHandle);
}

static const struct {
        const char *shaderID;
        const char *fileID;
        int shadertypeKind;
} shaders[] = {
#define VERT(x) { x "_vert", "example-shaders/" x ".vert", GP_SHADERTYPE_VERTEX }
#define FRAG(x) { x "_frag", "example-shaders/" x ".frag", GP_SHADERTYPE_FRAGMENT }
        VERT("line"),
        FRAG("line"),
        VERT("circle"),
        FRAG("circle"),
        VERT("arc"),
        FRAG("arc"),
#undef VERT
#undef FRAG
};

static const char *const programs[] = {
        "line",
        "circle",
        "arc",
};

static const struct {
        const char *programID;
        const char *shaderID;
} links[] = {
        { "line", "line_vert" },
        { "line", "line_frag" },
        { "circle", "circle_vert" },
        { "circle", "circle_frag" },
        { "arc", "arc_vert" },
        { "arc", "arc_frag" },
};

int main(void)
{
        struct GP_Builder sp = {0};
        for (int i = 0; i < LENGTH(shaders); i++) {
                const char *filepath = shaders[i].fileID;
                FILE *f = fopen(filepath, "rb");
                if (f == NULL)
                        gp_fatal_f("Failed to open shader file '%s'", filepath);
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                char *data;
                ALLOC_MEMORY(&data, size + 1);
                fseek(f, 0, SEEK_SET);
                size_t nread = fread(data, 1, size + 1, f);
                if (nread != size)
                        gp_fatal_f("Expected to read %d bytes from '%s', but got %d",
                                size, filepath, nread);
                if (ferror(f))
                        gp_fatal_f("I/O error while reading from '%s'", filepath);
                fclose(f);
                gp_builder_create_file(&sp, shaders[i].fileID, data, size);
        }
        for (int i = 0; i < LENGTH(shaders); i++)
                gp_builder_create_shader(&sp, shaders[i].shaderID, shaders[i].fileID, shaders[i].shadertypeKind);
        for (int i = 0; i < LENGTH(programs); i++)
                gp_builder_create_program(&sp, programs[i]);
        for (int i = 0; i < LENGTH(links); i++)
                gp_builder_create_link(&sp, links[i].programID, links[i].shaderID);
        gp_builder_process(&sp);
        struct GP_Ctx ctx = {0};
        gp_builder_to_ctx(&sp, &ctx);
        gp_parse(&ctx);
        write_c_interface(&ctx, "autogenerated/");
        gp_teardown(&ctx);
        gp_builder_teardown(&sp);
        return 0;
}
