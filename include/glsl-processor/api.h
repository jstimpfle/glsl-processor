

struct SP_File;
struct SP_Program;
struct SP_Shader;
struct SP_Link;

struct SP_Ctx {
        struct SP_File *files;
        struct SP_Program *programs;
        struct SP_Shader *shaders;
        struct SP_Link *links;

        int numFiles;
        int numPrograms;
        int numShaders;
        int numLinks;
};

void sp_setup(struct SP_Ctx *ctx);
void sp_teardown(struct SP_Ctx *ctx);

void sp_process(struct SP_Ctx *ctx);
void sp_to_gp(struct SP_Ctx *sp, struct GP_Ctx *ctx);


void sp_create_file(struct SP_Ctx *ctx, const char *fileID, const char *data, int size);
void sp_destroy_file(struct SP_Ctx *ctx, const char *fileID);

void sp_create_program(struct SP_Ctx *ctx, const char *programID);
void sp_destroy_program(struct SP_Ctx *ctx, const char *programID);

/* fileID must be an existing file. shaderID and fileID can be (and typically
 * are) the same. */
void sp_create_shader(struct SP_Ctx *ctx, const char *shaderID, int shadertypeKind);
void sp_destroy_shader(struct SP_Ctx *ctx, const char *shaderID);

void sp_create_link(struct SP_Ctx *ctx, const char *programID, const char *shaderID);
void sp_destroy_link(struct SP_Ctx *ctx, const char *programID, const char *shaderID);

void sp_attach_shader(struct SP_Ctx *ctx, const char *programName, const char *filepath);
void sp_unattach_shader(struct SP_Ctx *ctx, const char *programName, const char *filepath);
