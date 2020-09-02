// author: sgdxbc
// based on http://gee.cs.oswego.edu/dl/html/malloc.html
#include "AllocManager.h"
#include "AllocManagerImpl.h"

#include <signal.h>
#include <string.h>

void InitMemory(Memory memory, Size size) {
  if (size < sizeof(_MemoryImpl)) {
    raise(SIGABRT);
  }
  _MemoryImpl *mem = (_MemoryImpl *)memory;
  for (int i = 0; i < 128; i += 1) {
    mem->bins[i] = NULL;
  }
  Size totalAvailable = size - sizeof(_MemoryImpl) - sizeof(_Frag);
  _Frag *frag =
    _InitFrag((Memory)&mem->first_frag, totalAvailable, 0, NULL, NULL);
  _SetInUse(&frag->posttag);
  mem->bins[_IndexBin(totalAvailable)] = frag;
  _SetInUse(&_GetHigherNeighbour(frag)->pretag);
}

Object AllocObject(Memory memory, Size size) {
  _MemoryImpl *mem = (_MemoryImpl *)memory;
  _Frag *frag = _FindSmallestFrag(mem, size);
  if (frag == NULL) {
    return NULL;
  }
  _SetInUse(&frag->pretag);
  _SetInUse(&_GetHigherNeighbour(frag)->posttag);
  _RemoveFrag(mem, frag);
  _Frag *remain = _GetSize(frag->pretag) > size ? _SplitFrag(frag, size) : NULL;
  if (remain != NULL) {
    _SetFree(&remain->pretag);
    _SetFree(&_GetHigherNeighbour(remain)->posttag);
    _InsertFrag(mem, remain);
  }
  return frag;
}

void FreeObject(Memory memory, Object object) {
  _MemoryImpl *mem = (_MemoryImpl *)memory;
  _Frag *frag = _GetFrag(object);
  _SetFree(&frag->pretag);
  _SetFree(&_GetHigherNeighbour(frag)->posttag);
  _Frag *merged = _MergeFrag(frag);
  _InsertFrag(mem, merged);
}

Object ResizeObject(Memory memory, Object object, Size size) {
  _MemoryImpl *mem = (_MemoryImpl *)memory;
  _Frag *frag = _GetFrag(object);
  if (_GetSize(frag->pretag) >= size) {
    return object;
  }
  Size extendSize = size - _GetSize(frag->pretag);
  _Frag *followed = _GetHigherNeighbour(frag);
  if (!_InUse(followed->pretag) && _GetSize(followed->pretag) >= extendSize) {
    // todo: deplicated logic to `AllocObject`
    _SetInUse(&followed->pretag);
    _SetInUse(&_GetHigherNeighbour(followed)->posttag);
    _RemoveFrag(mem, followed);
    _Frag *remain =
      (_GetSize(followed->pretag) > extendSize
         ? _SplitFrag(followed, extendSize)
         : NULL);
    if (remain != NULL) {
      _SetFree(&remain->pretag);
      _SetFree(&_GetHigherNeighbour(remain)->posttag);
      _InsertFrag(mem, remain);
    }
    _SetSize(&frag->pretag, size);
    _SetSize(&_GetHigherNeighbour(frag)->posttag, size);
    return frag;
  } else {
    Object moved = AllocObject(memory, size);
    if (moved == NULL) {
      return NULL;
    }
    memcpy(moved, object, _GetSize(frag->pretag));
    FreeObject(memory, object);
    return moved;
  }
}