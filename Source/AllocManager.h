#ifndef MEMGRID_ALLOC_MANAGER_H
#define MEMGRID_ALLOC_MANAGER_H

#include <stddef.h>
#include <string.h>

typedef unsigned char *Memory;
typedef void *Object;
typedef size_t Size;

// initialize memory object for allocation
// sigabrt if size is too small
void InitMemory(Memory memory, Size size);

// return an object at address x, where [x, x + size) is contained by memory
// return null on failure
Object AllocObject(Memory memory, Size size);

// free an object, raise segfault when parameter is not allocated
void FreeObject(Memory memory, Object object);

// resize an object, keep it inplace if possible
// return null if target size is too large
Object ResizeObject(Memory memory, Object object, Size size);

// helper function to fulfill spec
static Object AllocObjectArray(Memory memory, Size count, Size size) {
  Object object = AllocObject(memory, count * size);
  if (object != NULL) {
    memset(object, 0, count * size);
  }
  return object;
}

#endif