#include <glsl-compiler/logging.h>
#include <glsl-compiler/str.h>
#include <string.h>
#include <stdlib.h>

void append_to_string(String *string, int c)
{
        int len = *string ? strlen(*string) : 0;
        char *s = realloc(*string, len + 2);
        if (s == NULL)
                fatal_f("OOM!");
        s[len] = c;
        s[len + 1] = 0;
        *string = s;
}

