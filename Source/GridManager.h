// Interface of first-level (grid) allocation

#ifndef MEMGRID_GRID_MANAGER_H
#define MEMGRID_GRID_MANAGER_H

#include "AllocManager.h"

// implementation should specify the detail structure of Grid and its manager
typedef struct _GridManager GridManager;
typedef struct _Grid Grid;

// call once on runtime setup/shutdown
// abort on error
void InitManager(GridManager *manager);
void FinalizeManager(GridManager *manager);  // notice: this function does not free memory

// runtime uses this interface to extract Memory field from Grid
// in order to call Object-allocation interfaces on it
Memory *GetMemory(Grid *grid);

// return NULL if allocating size is unacceptable
// abort on other error
Grid *AllocGrid(GridManager *manager, Size size);
void *FreeGrid(Grid *grid);

// indicate that the following processing exclusively writes to the specific grid
// notice that maybe more actions are required to implement full protection (such as modify .so)
// just do whatever need to do here
void SetCurrentGrid(GridManager *manager, Grid *grid);

#endif