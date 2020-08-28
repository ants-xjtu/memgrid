// author: sgdxbc
// based on http://gee.cs.oswego.edu/dl/html/malloc.html
// put these into header file for testing
#ifndef MEMGRID_ALLOC_MANAGER_IMPL_H
#define MEMGRID_ALLOC_MANAGER_IMPL_H

#include <assert.h>
#include <stdint.h>

#include "AllocManager.h"
#include "BinIndex.h"

// internal structs & helpers
typedef uint32_t _UsedAndSize;

#define IN_USE_FLAG 0x80000000

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
  // posttag for smaller neighbour
  _UsedAndSize posttag;
  _UsedAndSize pretag;
  // previous & next fragments in the same bin
  struct _tagFrag *prev;
  struct _tagFrag *next;
} _Frag;

static Object _GetObject(_Frag *frag) {
  return (Object)((Memory)frag + sizeof(_Frag));
}

// larger neighbour fragment has larger address than current one
// use 'larger' and 'small' to distinguish with frag->prev/next
static _Frag *_GetLargerNeighbour(_Frag *frag) {
  Size offset = sizeof(_Frag) + _GetSize(frag->pretag);
  return (_Frag *)((Memory)frag + offset);
}

static _Frag *_GetSmallerNeighbour(_Frag *frag) {
  Size offset = sizeof(_Frag) + _GetSize(frag->posttag);
  return (_Frag *)((Memory)frag - offset);
}

static _Frag *
_InitFrag(Memory addr, Size size, int used, _Frag *prev, _Frag *next) {
  _UsedAndSize uas;
  _SetSize(&uas, size);
  if (used) {
    _SetInUse(&uas);
  } else {
    _SetFree(&uas);
  }

  _Frag *frag = (_Frag *)addr;
  frag->pretag = uas;
  _GetLargerNeighbour(frag)->posttag = uas;
  frag->prev = prev;
  frag->next = next;
  return frag;
}

static _Frag *_SplitFrag(_Frag *frag, Size pos) {
  assert(!_InUse(frag->pretag));
  assert(_GetSize(frag->pretag) > pos);
  // split into a smaller fragment and 0-sized fragment is meaningless
  if (_GetSize(frag->pretag) <= pos + sizeof(_Frag)) {
    return NULL;
  }
  Size nextSize = _GetSize(frag->pretag) - pos;

  _SetSize(&frag->pretag, pos);
  _Frag *next = _GetLargerNeighbour(frag);
  _UsedAndSize nextUAS;
  _SetFree(&nextUAS);
  _SetSize(&nextUAS, nextSize);
  next->pretag = nextUAS;
  next->posttag = frag->pretag;
  return next;
}

static _Frag *_MergeFrag(_Frag *frag) {
  assert(!_InUse(frag->pretag));
  Size sum = _GetSize(frag->pretag);
  _Frag *start = frag;
  while (!_InUse(start->posttag)) {
    start = _GetSmallerNeighbour(start);
    sum += _GetSize(start->pretag) + sizeof(_Frag);
  }
  _SetSize(&start->pretag, sum);
  while (!_InUse(_GetLargerNeighbour(start)->pretag)) {
    _SetSize(
      &start->pretag,
      _GetSize(start->pretag) + _GetSize(_GetLargerNeighbour(start)->pretag) +
        sizeof(_Frag));
  }
  _GetLargerNeighbour(start)->posttag = start->pretag;
  return start;
}

typedef struct _tagMemoryImpl {
  _Frag *bins[128];
  _Frag *last;
  uint8_t binLeft, binRight;
  _Frag first_frag;
} _MemoryImpl;

static unsigned int _IndexBin(Size size) { return bin_index(size); }

static void _RemoveFrag(_Frag *frag, _MemoryImpl *mem) {
  if (frag->prev == NULL) {
    assert(_IndexBin(_GetSize(frag->pretag)) == mem->binLeft);
    assert(frag->next != NULL);
    frag->next->prev = NULL;
    mem->binLeft = _IndexBin(_GetSize(frag->next->pretag));
  } else {
    frag->next->prev = frag->prev;
  }
  if (frag->next == NULL) {
    assert(frag == mem->last);
    assert(_IndexBin(_GetSize(frag->pretag)) + 1 == mem->binRight);
    assert(frag->prev != NULL);
    frag->prev->next = NULL;
    mem->binRight = _IndexBin(_GetSize(frag->prev->pretag));
    mem->last = frag->prev;
  } else {
    frag->prev->next = frag->next;
  }
}

#endif