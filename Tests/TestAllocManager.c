//
#include "../AllocManager.h"
#include "../AllocManagerImpl.h"

#include <assert.h>

typedef struct {
  _Frag frag1;
  unsigned char space1[8];
  _UsedAndSize posttag1;
  _Frag frag2;
  unsigned char space2[32];
  _UsedAndSize posttag2;
  _Frag frag3;
  // omit
} TestFragStruct1;

void TestFrag() {
  _Frag frag;
  _SetFree(&frag.pretag);
  _SetSize(&frag.pretag, 32);
  assert(_GetSize(frag.pretag) == 32);
  assert(!_InUse(frag.pretag));
  _SetInUse(&frag.pretag);
  assert(_InUse(frag.pretag));
}

void TestNav() {
  TestFragStruct1 layout;
  _SetFree(&layout.frag1.pretag);
  _SetSize(&layout.frag1.pretag, sizeof(_UsedAndSize) + 8);
  _SetFree(&layout.posttag1);
  _SetSize(&layout.posttag1, sizeof(_UsedAndSize) + 8);
  _SetFree(&layout.frag2.pretag);
  _SetSize(&layout.frag2.pretag, sizeof(_UsedAndSize) + 32);
  _SetFree(&layout.posttag2);
  _SetSize(&layout.posttag2, sizeof(_UsedAndSize) + 32);
  _Frag *current = &layout.frag2;
  assert(_GetSmallerNeighbour(current) == &layout.frag1);
  assert(_GetLargerNeighbour(current) == &layout.frag3);
}

typedef void (*TestFunc)();

const TestFunc TESTCASES[] = {
  TestFrag,
  TestNav,
  NULL,
};

int main(void) {
  for (int i = 0; TESTCASES[i] != NULL; i += 1) {
    TESTCASES[i]();
  }
}