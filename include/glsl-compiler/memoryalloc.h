#include <glsl-compiler/logging.h>

void _alloc_memory(struct LogCtx logCtx, void **outPtr, int numElems, int elemSize);
void _realloc_memory(struct LogCtx logCtx, void **inoutPtr, int numElems, int elemSize);
void _free_memory(struct LogCtx logCtx, void **inoutPtr);

#define alloc_memory(outPtr, numElems, elemSize) _alloc_memory(MAKE_LOGCTX(), (outPtr), (numElems), (elemSize))
#define realloc_memory(inoutPtr, numElems, elemSize) _realloc_memory(MAKE_LOGCTX(), (inoutPtr), (numElems), (elemSize))
#define free_memory(inoutPtr) _free_memory(MAKE_LOGCTX(), (inoutPtr))

#define ALLOC_MEMORY(outPtr, numElems) alloc_memory((void**) (outPtr), (numElems), sizeof **(outPtr))
#define REALLOC_MEMORY(inoutPtr, numElems) realloc_memory((void**) (inoutPtr), (numElems), sizeof **(inoutPtr))
#define FREE_MEMORY(inoutPtr) free_memory((void**) (inoutPtr))
