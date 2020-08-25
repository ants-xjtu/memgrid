# MemGrid
### Allocation objects within grids.

```c
#include <GridManager.h>
#include <AllocManager.h>

void demo() {
    GridManager manager;
    InitManager(&manager);
    Grid *grid1 = AllocGrid(&manager, 2 << 30);  // 2GB
    Grid *grid2 = AllocGrid(&manager, 4 << 30);
    SetCurrentGrid(&manager, grid1);
    MyType *object = AllocObject(GetMemory(grid1), sizeof(MyType));
    
    // error
    // MyType *object2 = AllocObject(GetMemory(grid2), sizeof(MyType));
    
    MyType *objectList = ResizeObject(GetMemory(grid1), object, sizeof(MyType) * 8);
    MyType *objectList2 = AllocObjectArray(GetMemory(grid1), 8, sizeof(MyType));
    FreeObject(GetMemory(grid1), objectList);
    FreeObject(GetMemory(grid1), objectList2);
    FreeGrid(&manager, grid1);
    FreeGrid(&manager, grid2);
    FinalizeManager(&manager);
}
```
