#ifndef GLSLPROCESSOR_LOGGING_H_INCLUDED
#define GLSLPROCESSOR_LOGGING_H_INCLUDED

#include <glsl-processor/defs.h>

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define ENSURE(x) do { if (!(x)) internalerror_f("Assertion failed: %s", #x); } while (0)


struct LogCtx {
        const char *filename;
        int line;
};

void _gp_message_begin(struct LogCtx logCtx);
void gp_message_write_fv(const char *fmt, va_list ap);
void gp_message_write_f(const char *fmt, ...);
void gp_message_write(const char *data, int length);
void gp_message_end(void);
void _gp_message_fv(struct LogCtx logCtx, const char *fmt, va_list ap);
void _gp_message_f(struct LogCtx logCtx, const char *fmt, ...);
void _gp_message_s(struct LogCtx logCtx, const char *msg);

void _gp_fatal_begin(struct LogCtx logCtx);
void gp_fatal_write_fv(const char *fmt, va_list ap);
void gp_fatal_write_f(const char *fmt, ...);
void NORETURN gp_fatal_end(void);
void NORETURN _gp_fatal_fv(struct LogCtx logCtx, const char *fmt, va_list ap);
void NORETURN _gp_fatal_f(struct LogCtx logCtx, const char *fmt, ...);

void _gp_internalerror_begin(struct LogCtx logCtx);
void gp_internalerror_write_fv(const char *fmt, va_list ap);
void gp_internalerror_write_f(const char *fmt, ...);
void NORETURN gp_internalerror_end(void);
void NORETURN _gp_internalerror_fv(struct LogCtx logCtx, const char *fmt, va_list ap);
void NORETURN _gp_internalerror_f(struct LogCtx logCtx, const char *fmt, ...);

#define MAKE_LOGCTX() ((struct LogCtx) { __FILE__, __LINE__ })
#define message_begin() _gp_message_begin(MAKE_LOGCTX())
#define message_fv(fmt, ap) _gp_message_fv(MAKE_LOGCTX(), (fmt), (ap))
#define message_f(fmt, ...) _gp_message_f(MAKE_LOGCTX(), (fmt), ##__VA_ARGS__)
#define message_s(msg) _gp_message_s(MAKE_LOGCTX(), (msg))
#define fatal_begin() _gp_fatal_begin(MAKE_LOGCTX())
#define fatal_fv(fmt, ap) _gp_fatal_fv(MAKE_LOGCTX(), (fmt), (ap))
#define fatal_f(fmt, ...) _gp_fatal_f(MAKE_LOGCTX(), (fmt), ##__VA_ARGS__)
#define internalerror_fv(fmt, ap) _gp_internalerror_fv(MAKE_LOGCTX(), (fmt), (ap))
#define internalerror_f(fmt, ...) _gp_internalerror_f(MAKE_LOGCTX(), (fmt), ##__VA_ARGS__)


#endif
