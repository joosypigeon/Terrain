#ifndef TERRAIN_H
#define TERRAIN_H

#include "raylib.h"



#define GRID_SIZE 5


Mesh GenPlaneStripMesh();
void DrawTriangleStripGrid(int rows, int cols, float cellSize, Color color);

#endif // TERRAIN_H