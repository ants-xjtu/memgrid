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

static _Frag *_GetFrag(Object object) {
  return (_Frag *)((Memory)object - sizeof(_Frag));
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
  _Frag *head, *last;
  _Frag first_frag;
} _MemoryImpl;

static unsigned int _IndexBin(Size size) { return bin_index(size); }

static _Frag *_FindSmallestFrag(_MemoryImpl *mem, Size size) {
  unsigned int index;
  for (index = _IndexBin(size); index < NBINS && mem->bins[index] == NULL;
       index += 1) {
  }
  if (index == NBINS) {
    return NULL;
  }
  _Frag *current;
  for (current = mem->bins[index];
       current != NULL && _GetSize(current->pretag) < size;
       current = current->next) {
  }
  return current;
}

static void _AdjustBins(_MemoryImpl *mem, _Frag *frag, int is_free) {
  unsigned int index = _IndexBin(_GetSize(frag->pretag));
  if (!is_free && mem->bins[index] == frag) {
    mem->bins[index] =
      _IndexBin(_GetSize(frag->next->pretag)) == index ? frag->next : NULL;
  }
  // compare size with less or equal operator
  // because we always insert fragment in front of all other fragment with same
  // size
  if (is_free && _GetSize(frag->pretag) <= _GetSize(mem->bins[index]->pretag)) {
    mem->bins[index] = frag;
  }
}

static void _RemoveFrag(_MemoryImpl *mem, _Frag *frag) {
  if (frag == mem->head) {
    if (frag == mem->last) {
      mem->head = mem->last = NULL;
    } else {
      assert(frag->next != NULL);
      mem->head = frag->next;
      frag->next->prev = NULL;
    }
  } else if (frag == mem->last) {
    assert(frag->prev != NULL);
    mem->last = frag->prev;
    frag->prev->next = NULL;
  } else {
    frag->prev->next = frag->next;
    frag->next->prev = frag->prev;
  }
  _AdjustBins(mem, frag, 0);
}

static void _InsertFrag(_MemoryImpl *mem, _Frag *frag) {
  if (mem->head == NULL) {
    assert(mem->last == NULL);
    mem->head = mem->last = frag;
  } else {
    Size size = _GetSize(frag->pretag);
    if (size <= _GetSize(mem->head->pretag)) {
      mem->head->prev = frag;
      frag->next = mem->head;
      frag->prev = NULL;
      mem->head = frag;
    } else if (size > _GetSize(mem->last->pretag)) {
      mem->last->next = frag;
      frag->prev = mem->last;
      frag->next = NULL;
      mem->last = frag;
    } else {
      _Frag *followed = _FindSmallestFrag(mem, size);
      assert(followed != NULL);
      assert(followed->prev != NULL);
      frag->prev = followed->prev;
      frag->next = followed;
      followed->prev->next = frag;
      followed->prev = frag;
    }
  }
  _AdjustBins(mem, frag, 1);
}

#endif