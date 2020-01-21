#include <glsl-processor/memoryalloc.h>
#include <glsl-processor/ast.h>
#include <glsl-processor/api.h>
#include <string.h>
#include <stdlib.h>

struct SP_File {
        char *fileID;
        char *contents;
        int size;
};

struct SP_Program {
        char *programID;
};

struct SP_Shader {
        char *shaderID;
        int shadertypeKind;
};

struct SP_Link {
        char *programID;
        char *shaderID;
};

static char *sp_create_buffer(struct SP_Ctx *ctx, const char *data, int size)
{
        char *ptr;
        ALLOC_MEMORY(&ptr, size);
        memcpy(ptr, data, size);
        return ptr;
}

static void sp_destroy_buffer(struct SP_Ctx *ctx, char *ptr)
{
        FREE_MEMORY(&ptr);
}

static char *sp_create_string(struct SP_Ctx *ctx, const char *s)
{
        int length = (int) strlen(s);
        return sp_create_buffer(ctx, s, length + 1);
}

static void sp_destroy_string(struct SP_Ctx *ctx, char *ptr)
{
        sp_destroy_buffer(ctx, ptr);
}

static inline void _sp_delete_from_array(char **ptr, int *numElems, int idx, int elemSize)
{
        int off1 = idx * elemSize;
        int off2 = (idx + 1) * elemSize;
        int length = (*numElems - (idx + 1)) * elemSize;
        memmove(ptr + off1, ptr + off2, length);
        *numElems -= 1;
        realloc_memory(ptr, *numElems, elemSize);
}

#define SP_DELETE_FROM_ARRAY(pptr, pNumElems, idx) _sp_delete_from_array((char**)(pptr), (pNumElems), (idx), sizeof **(pptr))

static int sp_find_file(struct SP_Ctx *ctx, const char *fileID)
{
        for (int i = 0; i < ctx->numFiles; i++)
                if (!strcmp(ctx->files[i].fileID, fileID))
                        return i;
        return -1;
}

static int sp_find_program(struct SP_Ctx *ctx, const char *programID)
{
        for (int i = 0; i < ctx->numPrograms; i++)
                if (!strcmp(ctx->programs[i].programID, programID))
                        return i;
        return -1;
}

static int sp_find_shader(struct SP_Ctx *ctx, const char *shaderID)
{
        for (int i = 0; i < ctx->numShaders; i++)
                if (!strcmp(ctx->shaders[i].shaderID, shaderID))
                        return i;
        return -1;
}

static int sp_find_link(struct SP_Ctx *ctx, const char *programID, const char *shaderID)
{
        for (int i = 0; i < ctx->numLinks; i++)
                if (!strcmp(ctx->links[i].programID, programID)
                    && !strcmp(ctx->links[i].shaderID, shaderID))
                        return i;
        return -1;
}

void sp_create_file(struct SP_Ctx *ctx, const char *fileID, const char *data, int size)
{
        int idx = ctx->numFiles++;
        REALLOC_MEMORY(&ctx->files, ctx->numFiles);
        ctx->files[idx].fileID = sp_create_string(ctx, fileID);
        ctx->files[idx].contents = sp_create_buffer(ctx, data, size);
        ctx->files[idx].size = size;
}

void sp_create_program(struct SP_Ctx *ctx, const char *programID)
{
        int idx = ctx->numPrograms++;
        REALLOC_MEMORY(&ctx->programs, ctx->numPrograms);
        ctx->programs[idx].programID = sp_create_string(ctx, programID);
}

void sp_create_shader(struct SP_Ctx *ctx, const char *shaderID, int shadertypeKind)
{
        int idx = ctx->numShaders++;
        REALLOC_MEMORY(&ctx->shaders, ctx->numShaders);
        ctx->shaders[idx].shaderID = sp_create_string(ctx, shaderID);
        ctx->shaders[idx].shadertypeKind = shadertypeKind;
}

void sp_create_link(struct SP_Ctx *ctx, const char *programID, const char *shaderID)
{
        int idx = ctx->numLinks++;
        REALLOC_MEMORY(&ctx->links, ctx->numLinks);
        ctx->links[idx].programID = sp_create_string(ctx, programID);
        ctx->links[idx].shaderID = sp_create_string(ctx, shaderID);
}

void sp_destroy_file(struct SP_Ctx *ctx, const char *fileID)
{
        int idx = sp_find_file(ctx, fileID);
        if (idx != -1) {
                sp_destroy_string(ctx, ctx->files[idx].fileID);
                sp_destroy_buffer(ctx, ctx->files[idx].contents);
                SP_DELETE_FROM_ARRAY(&ctx->files, &ctx->numFiles, idx);
        }
}

void sp_destroy_program(struct SP_Ctx *ctx, const char *programID)
{
        int idx = sp_find_program(ctx, programID);
        if (idx != -1) {
                sp_destroy_string(ctx, ctx->programs[idx].programID);
                SP_DELETE_FROM_ARRAY(&ctx->programs, &ctx->numPrograms, idx);
        }
}

void sp_destroy_shader(struct SP_Ctx *ctx, const char *shaderID)
{
        int idx = sp_find_shader(ctx, shaderID);
        if (idx != -1) {
                sp_destroy_string(ctx, ctx->shaders[idx].shaderID);
                SP_DELETE_FROM_ARRAY(&ctx->shaders, &ctx->numShaders, idx);
        }
}

void sp_destroy_link(struct SP_Ctx *ctx, const char *programID, const char *shaderID)
{
        int idx = sp_find_link(ctx, programID, shaderID);
        if (idx != -1) {
                sp_destroy_string(ctx, ctx->links[idx].programID);
                sp_destroy_string(ctx, ctx->links[idx].shaderID);
                SP_DELETE_FROM_ARRAY(&ctx->links, &ctx->numLinks, idx);
        }
}

static int compare_files(const void *a, const void *b)
{
        const struct SP_File *x = a;
        const struct SP_File *y = b;
        return strcmp(x->fileID, y->fileID);
}

static int compare_programs(const void *a, const void *b)
{
        const struct SP_Program *x = a;
        const struct SP_Program *y = b;
        return strcmp(x->programID, y->programID);
}

static int compare_shaders(const void *a, const void *b)
{
        const struct SP_Shader *x = a;
        const struct SP_Shader *y = b;
        return strcmp(x->shaderID, y->shaderID);
}

static int compare_links(const void *a, const void *b)
{
        const struct SP_Link *x = a;
        const struct SP_Link *y = b;
        return strcmp(x->programID, y->shaderID);
}

void sp_process(struct SP_Ctx *ctx)
{
        qsort(ctx->files, ctx->numFiles, sizeof *ctx->files, compare_files);
        qsort(ctx->programs, ctx->numPrograms, sizeof *ctx->programs, compare_programs);
        qsort(ctx->shaders, ctx->numShaders, sizeof *ctx->shaders, compare_shaders);
        qsort(ctx->links, ctx->numLinks, sizeof *ctx->links, compare_links);
        for (int i = 1; i < ctx->numFiles; i++)
                if (!strcmp(ctx->files[i].fileID, ctx->files[i-1].fileID))
                        fatal_f("Multiple files '%s' given", ctx->files[i].fileID);
        for (int i = 1; i < ctx->numPrograms; i++)
                if (!strcmp(ctx->programs[i].programID, ctx->programs[i-1].programID))
                        fatal_f("Multiple programs '%s' given", ctx->programs[i].programID);
        for (int i = 1; i < ctx->numShaders; i++)
                if (!strcmp(ctx->shaders[i].shaderID, ctx->shaders[i-1].shaderID))
                        fatal_f("Multiple shaders '%s' given", ctx->shaders[i].shaderID);
        for (int i = 1; i < ctx->numLinks; i++)
                if (!strcmp(ctx->links[i].programID, ctx->links[i-1].programID)
                    && !strcmp(ctx->links[i].shaderID, ctx->links[i-1].shaderID))
                        fatal_f("Multiple links '%s -> %s' given",
                                ctx->links[i].programID, ctx->links[i].shaderID);
        for (int i = 0; i < ctx->numLinks; i++) {
                if (sp_find_program(ctx, ctx->links[i].programID) == -1)
                        fatal_f("In Link '%s -> %s': No such program '%s'",
                                ctx->links[i].programID,
                                ctx->links[i].shaderID,
                                ctx->links[i].programID);
                if (sp_find_shader(ctx, ctx->links[i].shaderID) == -1)
                        fatal_f("In Link '%s -> %s': No such shader '%s'",
                                ctx->links[i].programID,
                                ctx->links[i].shaderID,
                                ctx->links[i].shaderID);
        }
}

void sp_to_gp(struct SP_Ctx *sp, struct GP_Ctx *ctx)
{
        memset(ctx, 0, sizeof *ctx);
        ctx->numFiles = sp->numFiles;
        ctx->numPrograms = sp->numPrograms;
        ctx->numShaders = sp->numShaders;
        ctx->numLinks = sp->numLinks;
        REALLOC_MEMORY(&ctx->fileInfo, ctx->numFiles);
        REALLOC_MEMORY(&ctx->programInfo, ctx->numPrograms);
        REALLOC_MEMORY(&ctx->shaderInfo, ctx->numShaders);
        REALLOC_MEMORY(&ctx->linkInfo, ctx->numLinks);
        for (int i = 0; i < sp->numFiles; i++) {
                ctx->fileInfo[i].fileID = sp->files[i].fileID;
                ctx->fileInfo[i].contents = sp->files[i].contents;
                ctx->fileInfo[i].size = sp->files[i].size;
        }
        for (int i = 0; i < sp->numPrograms; i++)
                ctx->programInfo[i].programName = sp->programs[i].programID;
        for (int i = 0; i < sp->numShaders; i++) {
                ctx->shaderInfo[i].shaderName = sp->shaders[i].shaderID;
                ctx->shaderInfo[i].shaderType = sp->shaders[i].shadertypeKind;
        }
        for (int i = 0; i < sp->numLinks; i++) {
                int programIndex = sp_find_program(sp, sp->links[i].programID);
                int shaderIndex = sp_find_shader(sp, sp->links[i].shaderID);
                ENSURE(programIndex != -1);  //should have been caught earlier
                ENSURE(shaderIndex != -1);  //should have been caught earlier
                ctx->linkInfo[i].programIndex = programIndex;
                ctx->linkInfo[i].shaderIndex = shaderIndex;
        }
        //XXX!!
        REALLOC_MEMORY(&ctx->shaderfileAsts, ctx->numShaders);
}
