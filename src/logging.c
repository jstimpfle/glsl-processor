#include <glsl-compiler/logging.h>

void _message_begin(struct LogCtx logCtx)
{
        fprintf(stderr, "In %s:%d: ", logCtx.filename, logCtx.line);
}

void message_write_fv(const char *fmt, va_list ap)
{
        vfprintf(stderr, fmt, ap);
}

void message_write_f(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        message_write_fv(fmt, ap);
        va_end(ap);
}

void message_end(void)
{
        fprintf(stderr, "\n");
}

void _message_fv(struct LogCtx logCtx, const char *fmt, va_list ap)
{
        _message_begin(logCtx);
        message_write_fv(fmt, ap);
        message_end();
}

void _message_f(struct LogCtx logCtx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _message_fv(logCtx, fmt, ap);
        va_end(ap);
}

void _message_s(struct LogCtx logCtx, const char *msg)
{
        _message_f(logCtx, "%s", msg);
}

void NORETURN _fatal_fv(struct LogCtx logCtx, const char *fmt, va_list ap)
{
        _message_fv(logCtx, fmt, ap);
        abort();
}

void NORETURN _fatal_f(struct LogCtx logCtx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _fatal_fv(logCtx, fmt, ap);
        va_end(ap);
};
