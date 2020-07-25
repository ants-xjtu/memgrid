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
  Size totalAvailable = size - sizeof(_MemoryImpl);
  _Frag *frag =
      _InitFrag((Memory)&mem->first_frag, totalAvailable, 0, NULL, NULL);
  mem->bins[_IndexBin(totalAvailable)] = frag;
}