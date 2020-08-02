//
#include "../AllocManager.h"
#include "../AllocManagerImpl.h"

#include <assert.h>

void TestFrag() {
  _Frag frag;
  _SetFree(&frag.pretag);
  _SetSize(&frag.pretag, 32);
  assert(_GetSize(frag.pretag) == 32);
  assert(!_InUse(frag.pretag));
  _SetInUse(&frag.pretag);
  assert(_InUse(frag.pretag));
}

typedef struct {
  _Frag frag1;
  unsigned char space1[8];
  _Frag frag2;
  unsigned char space2[32];
  _Frag frag3;
  // omit
} TestFragStruct1;

void TestNav() {
  TestFragStruct1 layout;
  _SetFree(&layout.frag1.pretag);
  _SetSize(&layout.frag1.pretag, 8);
  _SetFree(&layout.frag2.posttag);
  _SetSize(&layout.frag2.posttag, 8);
  _SetFree(&layout.frag2.pretag);
  _SetSize(&layout.frag2.pretag, 32);
  _SetFree(&layout.frag3.posttag);
  _SetSize(&layout.frag3.posttag, 32);
  _Frag *current = &layout.frag2;
  assert(_GetSmallerNeighbour(current) == &layout.frag1);
  assert(_GetLargerNeighbour(current) == &layout.frag3);
}

void TestIndexBin() {
  unsigned int last = 0;
  for (Size size = 8; size < 0x7fffffff; size += 8) {
    unsigned int current = _IndexBin(size);
    assert(current >= last);
    last = current;
  }
  assert(last == 127);
}

void TestInitFrag() {
  unsigned char mem[128];
  _Frag *frag = _InitFrag(mem, 32, 0, NULL, NULL);
  assert(!_InUse(frag->pretag));
  assert(_GetSize(frag->pretag) == 32);
  _Frag *next_frag = _GetLargerNeighbour(frag);
  assert(_GetSmallerNeighbour(next_frag) == frag);
  assert(!_InUse(next_frag->posttag));
  assert(_GetSize(next_frag->posttag) == 32);
}

typedef void (*TestFunc)();

const TestFunc TESTCASES[] = {
    TestFrag, TestNav, TestIndexBin, TestInitFrag, NULL,
};

int main(void) {
  for (int i = 0; TESTCASES[i] != NULL; i += 1) {
    TESTCASES[i]();
  }
}