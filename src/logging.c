#include <glsl-compiler/backtrace.h>
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

void _fatal_begin(struct LogCtx logCtx)
{
        _message_begin(logCtx);
        message_write_f("FATAL ERROR: ");
}

void fatal_write_fv(const char *fmt, va_list ap)
{
        message_write_fv(fmt, ap);
}

void fatal_write_f(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        fatal_write_fv(fmt, ap);
        va_end(ap);
}

void NORETURN fatal_end(void)
{
        message_end();
        abort();
}

void NORETURN _fatal_fv(struct LogCtx logCtx, const char *fmt, va_list ap)
{
        _fatal_begin(logCtx);
        fatal_write_fv(fmt, ap);
        fatal_end();
}

void NORETURN _fatal_f(struct LogCtx logCtx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _fatal_fv(logCtx, fmt, ap);
        va_end(ap);
};



void _internalerror_begin(struct LogCtx logCtx)
{
        _message_begin(logCtx);
        message_write_f("INTERNAL ERROR: ");
}

void internalerror_write_fv(const char *fmt, va_list ap)
{
        message_write_fv(fmt, ap);
}

void internalerror_write_f(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        internalerror_write_fv(fmt, ap);
        va_end(ap);
}

void NORETURN internalerror_end(void)
{
        message_end();
        print_backtrace();
        abort();
}

void NORETURN _internalerror_fv(struct LogCtx logCtx, const char *fmt, va_list ap)
{
        _internalerror_begin(logCtx);
        internalerror_write_fv(fmt, ap);
        internalerror_end();
}

void NORETURN _internalerror_f(struct LogCtx logCtx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _internalerror_fv(logCtx, fmt, ap);
        va_end(ap);
};
