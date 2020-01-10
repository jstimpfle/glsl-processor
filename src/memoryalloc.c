#include <glsl-processor/logging.h>
#include <glsl-processor/memoryalloc.h>
#include <stdlib.h>

void _alloc_memory(struct LogCtx logCtx, void **outPtr, int numElems, int elemSize)
{
        int numBytes = numElems * elemSize; /*XXX overflow*/
        void *ptr = malloc(numBytes);
        if (!ptr)
                _fatal_f(logCtx, "OOM!\n");
        *outPtr = ptr;
}


void _realloc_memory(struct LogCtx logCtx, void **inoutPtr, int numElems, int elemSize)
{
        int numBytes = numElems * elemSize; /*XXX overflow*/
        void *ptr = realloc(*inoutPtr, numBytes);
        if (!ptr)
                _fatal_f(logCtx, "OOM!\n");
        *inoutPtr = ptr;
}

void _free_memory(struct LogCtx logCtx, void **inoutPtr)
{
        free(*inoutPtr);
        *inoutPtr = NULL;
}
