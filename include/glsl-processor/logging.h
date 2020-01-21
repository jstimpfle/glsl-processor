#ifndef GP_LOGGING_H_INCLUDED
#define GP_LOGGING_H_INCLUDED

#include <glsl-processor/defs.h>

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define GP_ENSURE(x) do { if (!(x)) gp_fatal_f("Assertion failed: %s", #x); } while (0)

struct GP_LogCtx {
        const char *filename;
        int line;
};

void _gp_message_begin(struct GP_LogCtx logCtx);
void gp_message_write_fv(const char *fmt, va_list ap);
void gp_message_write_f(const char *fmt, ...);
void gp_message_write(const char *data, int length);
void gp_message_end(void);
void _gp_message_fv(struct GP_LogCtx logCtx, const char *fmt, va_list ap);
void _gp_message_f(struct GP_LogCtx logCtx, const char *fmt, ...);
void _gp_message_s(struct GP_LogCtx logCtx, const char *msg);

void _gp_fatal_begin(struct GP_LogCtx logCtx);
void gp_fatal_write_fv(const char *fmt, va_list ap);
void gp_fatal_write_f(const char *fmt, ...);
void NORETURN gp_fatal_end(void);
void NORETURN _gp_fatal_fv(struct GP_LogCtx logCtx, const char *fmt, va_list ap);
void NORETURN _gp_fatal_f(struct GP_LogCtx logCtx, const char *fmt, ...);

#define GP_MAKE_LOGCTX() ((struct GP_LogCtx) { __FILE__, __LINE__ })
#define gp_message_begin() _gp_message_begin(GP_MAKE_LOGCTX())
#define gp_message_fv(fmt, ap) _gp_message_fv(GP_MAKE_LOGCTX(), (fmt), (ap))
#define gp_message_f(fmt, ...) _gp_message_f(GP_MAKE_LOGCTX(), (fmt), ##__VA_ARGS__)
#define gp_message_s(msg) _gp_message_s(GP_MAKE_LOGCTX(), (msg))
#define gp_fatal_begin() _gp_fatal_begin(GP_MAKE_LOGCTX())
#define gp_fatal_fv(fmt, ap) _gp_fatal_fv(GP_MAKE_LOGCTX(), (fmt), (ap))
#define gp_fatal_f(fmt, ...) _gp_fatal_f(GP_MAKE_LOGCTX(), (fmt), ##__VA_ARGS__)

#endif
