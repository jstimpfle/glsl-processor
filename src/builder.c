#include <glsl-processor/memory.h>
#include <glsl-processor/builder.h>
#include <glsl-processor/parse.h>
#include <string.h>
#include <stdlib.h>

struct GP_Builder_File {
        char *fileID;
        char *contents;
        int size;
};

struct GP_Builder_Program {
        char *programID;
};

struct GP_Builder_Shader {
        char *shaderID;
        char *fileID;
        int shadertypeKind;
};

struct GP_Builder_Link {
        char *programID;
        char *shaderID;
};

static char *gp_builder_create_buffer(const char *data, int size)
{
        char *ptr;
        ALLOC_MEMORY(&ptr, size);
        memcpy(ptr, data, size);
        return ptr;
}

static void gp_builder_destroy_buffer(char *ptr)
{
        FREE_MEMORY(&ptr);
}

static char *gp_builder_create_string(const char *s)
{
        int length = (int) strlen(s);
        return gp_builder_create_buffer(s, length + 1);
}

static void gp_builder_destroy_string(char *ptr)
{
        gp_builder_destroy_buffer(ptr);
}

static inline void _gp_delete_from_array(char **ptr, int *numElems, int idx, int elemSize)
{
        int off1 = idx * elemSize;
        int off2 = (idx + 1) * elemSize;
        int length = (*numElems - (idx + 1)) * elemSize;
        memmove(ptr + off1, ptr + off2, length);
        *numElems -= 1;
        realloc_memory((void**)ptr, *numElems, elemSize);
}

#define GP_DELETE_FROM_ARRAY(pptr, pNumElems, idx) _gp_delete_from_array((char**)(pptr), (pNumElems), (idx), sizeof **(pptr))

static int gp_builder_find_file(struct GP_Builder *ctx, const char *fileID)
{
        for (int i = 0; i < ctx->numFiles; i++)
                if (!strcmp(ctx->files[i].fileID, fileID))
                        return i;
        return -1;
}

static int gp_builder_find_program(struct GP_Builder *ctx, const char *programID)
{
        for (int i = 0; i < ctx->numPrograms; i++)
                if (!strcmp(ctx->programs[i].programID, programID))
                        return i;
        return -1;
}

static int gp_builder_find_shader(struct GP_Builder *ctx, const char *shaderID)
{
        for (int i = 0; i < ctx->numShaders; i++)
                if (!strcmp(ctx->shaders[i].shaderID, shaderID))
                        return i;
        return -1;
}

static int gp_builder_find_link(struct GP_Builder *builder, const char *programID, const char *shaderID)
{
        for (int i = 0; i < builder->numLinks; i++)
                if (!strcmp(builder->links[i].programID, programID)
                    && !strcmp(builder->links[i].shaderID, shaderID))
                        return i;
        return -1;
}

void gp_builder_create_file(struct GP_Builder *builder, const char *fileID, const char *data, int size)
{
        int idx = builder->numFiles++;
        REALLOC_MEMORY(&builder->files, builder->numFiles);
        builder->files[idx].fileID = gp_builder_create_string(fileID);
        builder->files[idx].contents = gp_builder_create_buffer(data, size);
        builder->files[idx].size = size;
}

void gp_builder_create_program(struct GP_Builder *builder, const char *programID)
{
        int idx = builder->numPrograms++;
        REALLOC_MEMORY(&builder->programs, builder->numPrograms);
        builder->programs[idx].programID = gp_builder_create_string(programID);
}

void gp_builder_create_shader(struct GP_Builder *builder, const char *shaderID, const char *fileID, int shadertypeKind)
{
        int idx = builder->numShaders++;
        REALLOC_MEMORY(&builder->shaders, builder->numShaders);
        builder->shaders[idx].shaderID = gp_builder_create_string(shaderID);
        builder->shaders[idx].fileID = gp_builder_create_string(fileID);
        builder->shaders[idx].shadertypeKind = shadertypeKind;
}

void gp_builder_create_link(struct GP_Builder *builder, const char *programID, const char *shaderID)
{
        int idx = builder->numLinks++;
        REALLOC_MEMORY(&builder->links, builder->numLinks);
        builder->links[idx].programID = gp_builder_create_string(programID);
        builder->links[idx].shaderID = gp_builder_create_string(shaderID);
}

void gp_builder_destroy_file(struct GP_Builder *builder, const char *fileID)
{
        int idx = gp_builder_find_file(builder, fileID);
        if (idx != -1) {
                gp_builder_destroy_string(builder->files[idx].fileID);
                gp_builder_destroy_buffer(builder->files[idx].contents);
                GP_DELETE_FROM_ARRAY(&builder->files, &builder->numFiles, idx);
        }
}

void gp_builder_destroy_program(struct GP_Builder *builder, const char *programID)
{
        int idx = gp_builder_find_program(builder, programID);
        if (idx != -1) {
                gp_builder_destroy_string(builder->programs[idx].programID);
                GP_DELETE_FROM_ARRAY(&builder->programs, &builder->numPrograms, idx);
        }
}

void gp_builder_destroy_shader(struct GP_Builder *builder, const char *shaderID)
{
        int idx = gp_builder_find_shader(builder, shaderID);
        if (idx != -1) {
                gp_builder_destroy_string(builder->shaders[idx].shaderID);
                gp_builder_destroy_string(builder->shaders[idx].fileID);
                GP_DELETE_FROM_ARRAY(&builder->shaders, &builder->numShaders, idx);
        }
}

void gp_builder_destroy_link(struct GP_Builder *builder, const char *programID, const char *shaderID)
{
        int idx = gp_builder_find_link(builder, programID, shaderID);
        if (idx != -1) {
                gp_builder_destroy_string(builder->links[idx].programID);
                gp_builder_destroy_string(builder->links[idx].shaderID);
                GP_DELETE_FROM_ARRAY(&builder->links, &builder->numLinks, idx);
        }
}

static int compare_files(const void *a, const void *b)
{
        const struct GP_Builder_File *x = a;
        const struct GP_Builder_File *y = b;
        return strcmp(x->fileID, y->fileID);
}

static int compare_programs(const void *a, const void *b)
{
        const struct GP_Builder_Program *x = a;
        const struct GP_Builder_Program *y = b;
        return strcmp(x->programID, y->programID);
}

static int compare_shaders(const void *a, const void *b)
{
        const struct GP_Builder_Shader *x = a;
        const struct GP_Builder_Shader *y = b;
        return strcmp(x->shaderID, y->shaderID);
}

static int compare_links(const void *a, const void *b)
{
        const struct GP_Builder_Link *x = a;
        const struct GP_Builder_Link *y = b;
        return strcmp(x->programID, y->shaderID);
}

void gp_builder_process(struct GP_Builder *builder)
{
        qsort(builder->files, builder->numFiles, sizeof *builder->files, compare_files);
        qsort(builder->programs, builder->numPrograms, sizeof *builder->programs, compare_programs);
        qsort(builder->shaders, builder->numShaders, sizeof *builder->shaders, compare_shaders);
        qsort(builder->links, builder->numLinks, sizeof *builder->links, compare_links);
        for (int i = 1; i < builder->numFiles; i++)
                if (!strcmp(builder->files[i].fileID, builder->files[i-1].fileID))
                        gp_fatal_f("Multiple files '%s' given", builder->files[i].fileID);
        for (int i = 1; i < builder->numPrograms; i++)
                if (!strcmp(builder->programs[i].programID, builder->programs[i-1].programID))
                        gp_fatal_f("Multiple programs '%s' given", builder->programs[i].programID);
        for (int i = 1; i < builder->numShaders; i++)
                if (!strcmp(builder->shaders[i].shaderID, builder->shaders[i-1].shaderID))
                        gp_fatal_f("Multiple shaders '%s' given", builder->shaders[i].shaderID);
        for (int i = 1; i < builder->numLinks; i++)
                if (!strcmp(builder->links[i].programID, builder->links[i-1].programID)
                    && !strcmp(builder->links[i].shaderID, builder->links[i-1].shaderID))
                        gp_fatal_f("Multiple links '%s -> %s' given",
                                builder->links[i].programID, builder->links[i].shaderID);
        for (int i = 1; i < builder->numShaders; i++)
                if (gp_builder_find_file(builder, builder->shaders[i].fileID) == -1)
                        gp_fatal_f("Shader '%s' needs file '%s' but it doesn't exist",
                                   builder->shaders[i].shaderID,
                                   builder->shaders[i].fileID);
        for (int i = 0; i < builder->numLinks; i++) {
                if (gp_builder_find_program(builder, builder->links[i].programID) == -1)
                        gp_fatal_f("In Link '%s -> %s': No such program '%s'",
                                builder->links[i].programID,
                                builder->links[i].shaderID,
                                builder->links[i].programID);
                if (gp_builder_find_shader(builder, builder->links[i].shaderID) == -1)
                        gp_fatal_f("In Link '%s -> %s': No such shader '%s'",
                                builder->links[i].programID,
                                builder->links[i].shaderID,
                                builder->links[i].shaderID);
        }
}

void gp_builder_to_ctx(struct GP_Builder *sp, struct GP_Ctx *ctx)
{
        struct GP_Desc *desc = &ctx->desc;
        memset(desc, 0, sizeof *desc);
        desc->numFiles = sp->numFiles;
        desc->numPrograms = sp->numPrograms;
        desc->numShaders = sp->numShaders;
        desc->numLinks = sp->numLinks;
        REALLOC_MEMORY(&desc->fileInfo, desc->numFiles);
        REALLOC_MEMORY(&desc->programInfo, desc->numPrograms);
        REALLOC_MEMORY(&desc->shaderInfo, desc->numShaders);
        REALLOC_MEMORY(&desc->linkInfo, desc->numLinks);
        REALLOC_MEMORY(&ctx->shaderfileAsts, desc->numShaders); //!!!
        for (int i = 0; i < sp->numFiles; i++) {
                desc->fileInfo[i].fileID = sp->files[i].fileID;
                desc->fileInfo[i].contents = sp->files[i].contents;
                desc->fileInfo[i].size = sp->files[i].size;
        }
        for (int i = 0; i < sp->numPrograms; i++)
                desc->programInfo[i].programName = sp->programs[i].programID;
        for (int i = 0; i < sp->numShaders; i++) {
                desc->shaderInfo[i].shaderName = sp->shaders[i].shaderID;
                desc->shaderInfo[i].fileID = sp->shaders[i].fileID;
                desc->shaderInfo[i].shaderType = sp->shaders[i].shadertypeKind;
        }
        for (int i = 0; i < sp->numLinks; i++) {
                int programIndex = gp_builder_find_program(sp, sp->links[i].programID);
                int shaderIndex = gp_builder_find_shader(sp, sp->links[i].shaderID);
                GP_ENSURE(programIndex != -1);  //should have been caught earlier
                GP_ENSURE(shaderIndex != -1);  //should have been caught earlier
                desc->linkInfo[i].programIndex = programIndex;
                desc->linkInfo[i].shaderIndex = shaderIndex;
        }
}

void gp_builder_setup(struct GP_Builder *builder)
{
        memset(builder, 0, sizeof *builder);
}

void gp_builder_teardown(struct GP_Builder *builder)
{
        for (int i = 0; i < builder->numFiles; i++) {
                gp_builder_destroy_string(builder->files[i].fileID);
                gp_builder_destroy_buffer(builder->files[i].contents);
        }
        for (int i = 0; i < builder->numPrograms; i++) {
                gp_builder_destroy_string(builder->programs[i].programID);
        }
        for (int i = 0; i < builder->numShaders; i++) {
                gp_builder_destroy_string(builder->shaders[i].shaderID);
        }
        for (int i = 0; i < builder->numLinks; i++) {
                gp_builder_destroy_string(builder->links[i].programID);
                gp_builder_destroy_string(builder->links[i].shaderID);
        }
        FREE_MEMORY(&builder->files);
        FREE_MEMORY(&builder->programs);
        FREE_MEMORY(&builder->shaders);
        FREE_MEMORY(&builder->links);
        memset(builder, 0, sizeof *builder);
}
