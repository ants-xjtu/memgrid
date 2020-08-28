// author: sgdxbc
// based on http://gee.cs.oswego.edu/dl/html/malloc.html
// put these into header file for testing
#ifndef MEMGRID_ALLOC_MANAGER_IMPL_H
#define MEMGRID_ALLOC_MANAGER_IMPL_H

#include <assert.h>
#include <stdint.h>

#include "AllocManager.h"
#include "BinIndex.h"

typedef uint32_t _FragInfo;

#define IN_USE_FLAG 0x00000001

static int _InUse(_FragInfo info) { return (info & IN_USE_FLAG) != 0; }

static void _SetInUse(_FragInfo *info) { *info |= IN_USE_FLAG; }

static void _SetFree(_FragInfo *info) { *info &= ~IN_USE_FLAG; }

// Frag size is always multiplier of 8, so the lowest 1 (3 actually) bit(s) are
// always 0
// not including overhead introduced by _Frag struct
static Size _GetSize(_FragInfo info) { return info & ~IN_USE_FLAG; }

static void _SetSize(_FragInfo *info, Size size) {
  assert(!_InUse(*info));
  assert((size & IN_USE_FLAG) == 0);
  *info = (*info & IN_USE_FLAG) | size;
}

#undef IN_USE_FLAG

typedef struct _tagFrag {
  // posttag for lower neighbour
  _FragInfo posttag;
  _FragInfo pretag;
  // previous & next fragments in the same bin
  struct _tagFrag *prev;
  struct _tagFrag *next;
} _Frag;

static Object _GetObject(_Frag *frag) {
  return (Object)((Memory)frag + sizeof(_Frag));
}

// Higher neighbour fragment has higher address than current one
// use 'high' and 'low' to distinguish with frag->prev/next
static _Frag *_GetHigherNeighbour(_Frag *frag) {
  Size offset = sizeof(_Frag) + _GetSize(frag->pretag);
  return (_Frag *)((Memory)frag + offset);
}

static _Frag *_GetLowerNeighbour(_Frag *frag) {
  Size offset = sizeof(_Frag) + _GetSize(frag->posttag);
  return (_Frag *)((Memory)frag - offset);
}

static _Frag *
_InitFrag(Memory addr, Size size, int used, _Frag *prev, _Frag *next) {
  _FragInfo info;
  _SetFree(&info);
  _SetSize(&info, size);
  if (used) {
    _SetInUse(&info);
  } else {
    _SetFree(&info);
  }

  _Frag *frag = (_Frag *)addr;
  frag->pretag = info;
  _GetHigherNeighbour(frag)->posttag = info;
  frag->prev = prev;
  frag->next = next;
  return frag;
}

// return the second frag
static _Frag *_SplitFrag(_Frag *frag, Size pos) {
  assert(!_InUse(frag->pretag));
  assert(_GetSize(frag->pretag) > pos);
  // split into a smaller fragment and 0-sized fragment is meaningless
  if (_GetSize(frag->pretag) <= pos + sizeof(_Frag)) {
    return NULL;
  }
  Size nextSize = _GetSize(frag->pretag) - pos;

  _SetSize(&frag->pretag, pos);
  _Frag *remain = _GetHigherNeighbour(frag);
  _FragInfo remainTag;
  _SetFree(&remainTag);
  _SetSize(&remainTag, nextSize);
  remain->pretag = remainTag;
  _GetHigherNeighbour(remain)->posttag = remainTag;
  remain->posttag = frag->pretag;
  return remain;
}

static _Frag *_MergeFrag(_Frag *frag) {
  assert(!_InUse(frag->pretag));
  Size sum = _GetSize(frag->pretag);
  _Frag *start = frag;
  while (!_InUse(start->posttag)) {
    start = _GetLowerNeighbour(start);
    sum += _GetSize(start->pretag) + sizeof(_Frag);
  }
  _SetSize(&start->pretag, sum);
  while (!_InUse(_GetHigherNeighbour(start)->pretag)) {
    _SetSize(
      &start->pretag,
      _GetSize(start->pretag) + _GetSize(_GetHigherNeighbour(start)->pretag) +
        sizeof(_Frag));
  }
  _GetHigherNeighbour(start)->posttag = start->pretag;
  return start;
}

typedef struct _tagMemoryImpl {
  _Frag *bins[128];
  _Frag *last;
  uint8_t binLeft, binRight;
  _Frag first_frag;
} _MemoryImpl;

static unsigned int _IndexBin(Size size) { return bin_index(size); }

#endif