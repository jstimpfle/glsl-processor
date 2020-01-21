#ifndef GP_BUILDER_H_INCLUDED
#define GP_BUILDER_H_INCLUDED

struct GP_Builder_File;
struct GP_Builder_Program;
struct GP_Builder_Shader;
struct GP_Builder_Link;

struct GP_Builder {
        struct GP_Builder_File *files;
        struct GP_Builder_Program *programs;
        struct GP_Builder_Shader *shaders;
        struct GP_Builder_Link *links;
        int numFiles;
        int numPrograms;
        int numShaders;
        int numLinks;
};

void gp_builder_setup(struct GP_Builder *ctx);
void gp_builder_teardown(struct GP_Builder *ctx);

void gp_builder_process(struct GP_Builder *ctx);
void gp_builder_to_ctx(struct GP_Builder *sp, struct GP_Ctx *ctx);

void gp_builder_create_file(struct GP_Builder *ctx, const char *fileID, const char *data, int size);
void gp_builder_create_shader(struct GP_Builder *ctx, const char *shaderID, int shadertypeKind);
void gp_builder_create_program(struct GP_Builder *ctx, const char *programID);
void gp_builder_create_link(struct GP_Builder *ctx, const char *programID, const char *shaderID);

void gp_builder_destroy_file(struct GP_Builder *ctx, const char *fileID);
void gp_builder_destroy_program(struct GP_Builder *ctx, const char *programID);
void gp_builder_destroy_shader(struct GP_Builder *ctx, const char *shaderID);
void gp_builder_destroy_link(struct GP_Builder *ctx, const char *programID, const char *shaderID);

#endif