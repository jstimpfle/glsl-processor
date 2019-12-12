#ifndef GLSL_COMPILER_DEFS_H_INCLUDED
#define GLSL_COMPILER_DEFS_H_INCLUDED

#if defined(_MSC_VER)
#define NORETURN _declspec(noreturn)
#elif defined(_GNUC)
#define NORETURN __attribute__((noreturn))
#endif

#define UNUSED(x) ((void)(x))

#define LENGTH(a) (sizeof (a) / sizeof (a)[0])

#endif
