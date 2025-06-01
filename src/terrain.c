#include "terrain.h"
#include <math.h>
#include "raymath.h"

Mesh GenGridStripMeshWithNormals(int rows, int cols, float width, float height) {
    int stripsPerRow = 2 * cols + 2;
    int rawVertexCount = (rows - 1) * stripsPerRow - 2;

    // Allocate space
    Vector3 *vertices = MemAlloc(rawVertexCount * sizeof(Vector3));
    Vector3 *normals = MemAlloc(rawVertexCount * sizeof(Vector3));
    Vector2 *texcoords = MemAlloc(rawVertexCount * sizeof(Vector2));

    // Store all positions for reference
    Vector3 **grid = MemAlloc(rows * sizeof(Vector3 *));
    for (int i = 0; i < rows; i++) {
        grid[i] = MemAlloc(cols * sizeof(Vector3));
    }

    float dx = width / (cols - 1);
    float dy = height / (rows - 1);

    // First create the vertex grid (positions only)
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float px = x * dx - width / 2;
            float py = 0;
            float pz = y * dy - height / 2;
            grid[y][x] = (Vector3){ px, py, pz };
        }
    }

    // Compute per-vertex normals by averaging triangle face normals
    Vector3 **normalGrid = MemAlloc(rows * sizeof(Vector3 *));
    for (int i = 0; i < rows; i++) {
        normalGrid[i] = MemAlloc(cols * sizeof(Vector3));
        for (int j = 0; j < cols; j++) normalGrid[i][j] = (Vector3){ 0 };
    }

    // Accumulate normals from triangles
    for (int y = 0; y < rows - 1; y++) {
        for (int x = 0; x < cols - 1; x++) {
            Vector3 p00 = grid[y][x];
            Vector3 p10 = grid[y][x + 1];
            Vector3 p01 = grid[y + 1][x];
            Vector3 p11 = grid[y + 1][x + 1];

            // Triangle 1: p00, p10, p11
            Vector3 edge1 = Vector3Subtract(p10, p00);
            Vector3 edge2 = Vector3Subtract(p11, p00);
            Vector3 n1 = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

            // Triangle 2: p00, p11, p01
            edge1 = Vector3Subtract(p11, p00);
            edge2 = Vector3Subtract(p01, p00);
            Vector3 n2 = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

            // Accumulate
            normalGrid[y][x] = Vector3Add(normalGrid[y][x], n1);
            normalGrid[y][x + 1] = Vector3Add(normalGrid[y][x + 1], n1);
            normalGrid[y + 1][x + 1] = Vector3Add(normalGrid[y + 1][x + 1], n1);

            normalGrid[y][x] = Vector3Add(normalGrid[y][x], n2);
            normalGrid[y + 1][x + 1] = Vector3Add(normalGrid[y + 1][x + 1], n2);
            normalGrid[y + 1][x] = Vector3Add(normalGrid[y + 1][x], n2);
        }
    }

    // Build strip vertex data
    int index = 0;
    for (int y = 0; y < rows - 1; y++) {
        for (int x = 0; x <= cols; x++) {
            // Lower row vertex
            Vector3 p0 = grid[y][x];
            Vector3 n0 = Vector3Normalize(normalGrid[y][x]);
            Vector2 t0 = (Vector2){ (float)x / (cols - 1), (float)y / (rows - 1) };

            vertices[index] = p0;
            normals[index] = n0;
            texcoords[index++] = t0;

            // Upper row vertex
            Vector3 p1 = grid[y + 1][x];
            Vector3 n1 = Vector3Normalize(normalGrid[y + 1][x]);
            Vector2 t1 = (Vector2){ (float)x / (cols - 1), (float)(y + 1) / (rows - 1) };

            vertices[index] = p1;
            normals[index] = n1;
            texcoords[index++] = t1;
        }

        // Add degenerate triangle to bridge rows
        if (y < rows - 2) {
            vertices[index] = vertices[index - 1];
            normals[index] = normals[index - 1];
            texcoords[index] = texcoords[index - 1];
            index++;

            vertices[index] = grid[y + 1][0];
            normals[index] = Vector3Normalize(normalGrid[y + 1][0]);
            texcoords[index++] = (Vector2){ 0, (float)(y + 1) / (rows - 1) };
        }
    }

    // Cleanup grid memory
    for (int i = 0; i < rows; i++) {
        MemFree(grid[i]);
        MemFree(normalGrid[i]);
    }
    MemFree(grid);
    MemFree(normalGrid);

    Mesh mesh = { 0 };
    mesh.vertexCount = index;
    mesh.triangleCount = index - 2;

    mesh.vertices = (float *)vertices;
    mesh.normals = (float *)normals;
    mesh.texcoords = (float *)texcoords;

    UploadMesh(&mesh, false);
    return mesh;
}
