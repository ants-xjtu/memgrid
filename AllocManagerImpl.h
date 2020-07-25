// author: sgdxbc
// based on http://gee.cs.oswego.edu/dl/html/malloc.html
// put these into header file for testing
#ifndef MEMGRID_ALLOC_MANAGER_IMPL_H
#define MEMGRID_ALLOC_MANAGER_IMPL_H

#include "AllocManager.h"

#include <assert.h>
#include <stdint.h>

// internal structs & helpers
typedef uint64_t _UsedAndSize;

#define IN_USE_FLAG 0x8000000000000000

static int _InUse(_UsedAndSize uas) { return (uas & IN_USE_FLAG) != 0; }

static void _SetInUse(_UsedAndSize *uas) { *uas |= IN_USE_FLAG; }

static void _SetFree(_UsedAndSize *uas) { *uas &= ~IN_USE_FLAG; }

static Size _GetSize(_UsedAndSize uas) { return uas & ~IN_USE_FLAG; }

static void _SetSize(_UsedAndSize *uas, Size size) {
  assert(!_InUse(*uas));
  assert((size & IN_USE_FLAG) == 0);
  *uas = (*uas & IN_USE_FLAG) | size;
}

#undef IN_USE_FLAG

typedef struct _tagFrag {
  _UsedAndSize pretag;
  // previous & next fragments in the same bin
  struct _tagFrag *prev;
  struct _tagFrag *next;
  // varlen user space according to pretag
  // the type of object_head is meaningless
  // choose _UsedAndSize so it take the same space with posttag
  // which makes it easier to implement _GetLargerNeighbour
  _UsedAndSize object_head;
  // then posttag here
} _Frag;

Object _GetObject(_Frag *frag) { return (Object)frag->object_head; }

// larger neighbour fragment has larger address than current one
// use 'larger' and 'small' to distinguish with frag->prev/next
_Frag *_GetLargerNeighbour(_Frag *frag) {
  Size offset = sizeof(_Frag) + _GetSize(frag->pretag);
  return (_Frag *)((Memory)frag + offset);
}

_UsedAndSize _GetSmallerNeighbourUAS(_Frag *frag) {
  return *(_UsedAndSize *)((Memory)frag - sizeof(_UsedAndSize));
}

_Frag *_GetSmallerNeighbour(_Frag *frag) {
  Size offset = sizeof(_Frag) + _GetSize(_GetSmallerNeighbourUAS(frag));
  return (_Frag *)((Memory)frag - offset);
}

typedef struct _tagMemoryImpl {
  _Frag *bins[128];
  _Frag first_frag;
} _MemoryImpl;

static unsigned int _IndexBin(Size fragSize) {
  unsigned offset = 0;

#define Check(sizeStep, binCount)                                              \
  if (fragSize <= sizeStep * binCount) {                                       \
    return fragSize / sizeStep + offset;                                       \
  }                                                                            \
  fragSize -= sizeStep * binCount;                                             \
  offset += binCount;

  Check(8, 64);
  Check(64, 32);
  Check(512, 16);
  Check(4096, 8);
  Check(32768, 4);
  Check(262144, 2);

#undef Check

  return 127;
}

static _Frag *_FindBin(Memory memory, Size minSize) {
  unsigned int index = _IndexBin(minSize);
  _MemoryImpl *mem = (_MemoryImpl *)memory;
  for (; mem->bins[index] == NULL && index < 128; index += 1) {
  }
  return index == 128 ? NULL : mem->bins[index];
}

#endif