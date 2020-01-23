#include <glsl-processor/logging.h>
#include <glsl-processor/memory.h>
#include <stdlib.h>

void _gp_alloc_memory(struct GP_LogCtx logCtx, void **outPtr, size_t numElems, size_t elemSize)
{
        size_t numBytes = numElems * elemSize; /*XXX overflow*/
        void *ptr = malloc(numBytes);
        if (!ptr)
                _gp_fatal_f(logCtx, "OOM!\n");
        *outPtr = ptr;
}

void _gp_realloc_memory(struct GP_LogCtx logCtx, void **inoutPtr, size_t numElems, size_t elemSize)
{
        size_t numBytes = numElems * elemSize; /*XXX overflow*/
        void *ptr = realloc(*inoutPtr, numBytes);
        if (!ptr)
                _gp_fatal_f(logCtx, "OOM!\n");
        *inoutPtr = ptr;
}

void _gp_free_memory(struct GP_LogCtx logCtx, void **inoutPtr)
{
        free(*inoutPtr);
        *inoutPtr = NULL;
}
