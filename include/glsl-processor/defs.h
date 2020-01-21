#ifndef GP_DEFS_H_INCLUDED
#define GP_DEFS_H_INCLUDED

#if defined(_MSC_VER)
#define NORETURN _declspec(noreturn)
#elif defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#else
#warning Compiler unknown, so defining NORETURN as empty
#define NORETURN
#endif

#define UNUSED(x) ((void)(x))

#define LENGTH(a) (sizeof (a) / sizeof (a)[0])

#endif
