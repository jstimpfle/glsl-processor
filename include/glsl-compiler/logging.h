#ifndef GLSLCOMPILER_LOGGING_H_INCLUDED
#define GLSLCOMPILER_LOGGING_H_INCLUDED

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define ENSURE assert
#define NORETURN __attribute__((noreturn))


struct LogCtx {
        const char *filename;
        int line;
};

void _message_begin(struct LogCtx logCtx);
void message_write_fv(const char *fmt, va_list ap);
void message_write_f(const char *fmt, ...);
void message_end(void);
void _message_fv(struct LogCtx logCtx, const char *fmt, va_list ap);
void _message_f(struct LogCtx logCtx, const char *fmt, ...);
void _message_s(struct LogCtx logCtx, const char *msg);
void NORETURN _fatal_fv(struct LogCtx logCtx, const char *fmt, va_list ap);
void NORETURN _fatal_f(struct LogCtx logCtx, const char *fmt, ...);


#define MAKE_LOGCTX() ((struct LogCtx) { __FILE__, __LINE__ })
#define message_begin() _message_begin(MAKE_LOGCTX())
#define message_fv(fmt, ap) _message_fv(MAKE_LOGCTX(), (fmt), (ap))
#define message_f(fmt, ...) _message_f(MAKE_LOGCTX(), (fmt), ##__VA_ARGS__)
#define message_s(msg) _message_s(MAKE_LOGCTX(), (msg))
#define fatal_fv(fmt, ap) _fatal_fv(MAKE_LOGCTX(), (fmt), (ap))
#define fatal_f(fmt, ...) _fatal_f(MAKE_LOGCTX(), (fmt), ##__VA_ARGS__)

#endif
