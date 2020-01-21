#include <glsl-processor/logging.h>

void _gp_message_begin(struct GP_LogCtx logCtx)
{
        fprintf(stderr, "In %s:%d: ", logCtx.filename, logCtx.line);
}

void gp_message_write_fv(const char *fmt, va_list ap)
{
        vfprintf(stderr, fmt, ap);
}

void gp_message_write_f(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        gp_message_write_fv(fmt, ap);
        va_end(ap);
}

void gp_message_write(const char *data, int length)
{
        fwrite(data, length, 1, stderr);
}

void gp_message_end(void)
{
        fprintf(stderr, "\n");
}

void _gp_message_fv(struct GP_LogCtx logCtx, const char *fmt, va_list ap)
{
        _gp_message_begin(logCtx);
        gp_message_write_fv(fmt, ap);
        gp_message_end();
}

void _gp_message_f(struct GP_LogCtx logCtx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _gp_message_fv(logCtx, fmt, ap);
        va_end(ap);
}

void _gp_message_s(struct GP_LogCtx logCtx, const char *msg)
{
        _gp_message_f(logCtx, "%s", msg);
}

void _gp_fatal_begin(struct GP_LogCtx logCtx)
{
        _gp_message_begin(logCtx);
        gp_message_write_f("FATAL ERROR: ");
}

void gp_fatal_write_fv(const char *fmt, va_list ap)
{
        gp_message_write_fv(fmt, ap);
}

void gp_fatal_write_f(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        gp_fatal_write_fv(fmt, ap);
        va_end(ap);
}

void NORETURN gp_fatal_end(void)
{
        gp_message_end();
        abort();
}

void NORETURN _gp_fatal_fv(struct GP_LogCtx logCtx, const char *fmt, va_list ap)
{
        _gp_fatal_begin(logCtx);
        gp_fatal_write_fv(fmt, ap);
        gp_fatal_end();
}

void NORETURN _gp_fatal_f(struct GP_LogCtx logCtx, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        _gp_fatal_fv(logCtx, fmt, ap);
        va_end(ap);
};
