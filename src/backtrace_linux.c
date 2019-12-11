#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#define LENGTH(a) ((int)(sizeof (a) / sizeof (a)[0]))

void print_backtrace(void)
{
        void *buffer[32];
        int size = backtrace(buffer, LENGTH(buffer));

        char **symbols = backtrace_symbols(buffer, size);

        if (symbols != NULL) {
                for (int i = 0; i < size; i++)
                        fprintf(stderr, " %s\n", symbols[i]);
                free(symbols);
        }
}
