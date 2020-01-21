#ifndef GP_MEMORYALLOC_H_INCLUDED
#define GP_MEMORYALLOC_H_INCLUDED

#include <glsl-processor/logging.h>

void _gp_alloc_memory(struct GP_LogCtx logCtx, void **outPtr, int numElems, int elemSize);
void _gp_realloc_memory(struct GP_LogCtx logCtx, void **inoutPtr, int numElems, int elemSize);
void _gp_free_memory(struct GP_LogCtx logCtx, void **inoutPtr);

#define alloc_memory(outPtr, numElems, elemSize) _gp_alloc_memory(GP_MAKE_LOGCTX(), (outPtr), (numElems), (elemSize))
#define realloc_memory(inoutPtr, numElems, elemSize) _gp_realloc_memory(GP_MAKE_LOGCTX(), (inoutPtr), (numElems), (elemSize))
#define free_memory(inoutPtr) _gp_free_memory(GP_MAKE_LOGCTX(), (inoutPtr))

#define ALLOC_MEMORY(outPtr, numElems) alloc_memory((void**) (outPtr), (numElems), sizeof **(outPtr))
#define REALLOC_MEMORY(inoutPtr, numElems) realloc_memory((void**) (inoutPtr), (numElems), sizeof **(inoutPtr))
#define FREE_MEMORY(inoutPtr) free_memory((void**) (inoutPtr))

#endif
