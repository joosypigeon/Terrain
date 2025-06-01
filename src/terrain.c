#include "terrain.h"
#include <stdio.h>
#include "raymath.h"
#include "rlgl.h"
#include <GL/gl.h> 
#define MAX_MESH_VERTEX_BUFFERS 7

Mesh GenPlaneStripMesh() {

    // 5 x 5 grid → 4 strips of 5*2 = 10 verts each, plus degenerate verts between rows
    // Total = 4*(10) + 3*(2) = 52 vertices

    Vector3 vertices[] = {
        // First row
        { -2, 0, -2 }, { -2, 0, -1 },
        { -1, 0, -2 }, { -1, 0, -1 },
        {  0, 0, -2 }, {  0, 0, -1 },
        {  1, 0, -2 }, {  1, 0, -1 },
        {  2, 0, -2 }, {  2, 0, -1 },

        // Degenerate
        {  2, 0, -1 }, { -2, 0, -1 },

        // Second row
        { -2, 0, -1 }, { -2, 0,  0 },
        { -1, 0, -1 }, { -1, 0,  0 },
        {  0, 0, -1 }, {  0, 0,  0 },
        {  1, 0, -1 }, {  1, 0,  0 },
        {  2, 0, -1 }, {  2, 0,  0 },

        // Degenerate
        {  2, 0,  0 }, { -2, 0,  0 },

        // Third row
        { -2, 0,  0 }, { -2, 0,  1 },
        { -1, 0,  0 }, { -1, 0,  1 },
        {  0, 0,  0 }, {  0, 0,  1 },
        {  1, 0,  0 }, {  1, 0,  1 },
        {  2, 0,  0 }, {  2, 0,  1 },

        // Degenerate
        {  2, 0,  1 }, { -2, 0,  1 },

        // Fourth row
        { -2, 0,  1 }, { -2, 0,  2 },
        { -1, 0,  1 }, { -1, 0,  2 },
        {  0, 0,  1 }, {  0, 0,  2 },
        {  1, 0,  1 }, {  1, 0,  2 },
        {  2, 0,  1 }, {  2, 0,  2 },
    };

    int vertexCount = sizeof(vertices) / sizeof(vertices[0]);

    Vector3 normals[52];
    Vector2 texcoords[52];
    for (int i = 0; i < vertexCount; i++) {
        normals[i] = (Vector3){ 0, 1, 0 };
        texcoords[i] = (Vector2){ 0, 0 }; // Flat for now
    }

    Mesh mesh = { 0 };
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = vertexCount - 2;

    mesh.vertices = (float *)vertices;
    mesh.normals = (float *)normals;
    mesh.texcoords = (float *)texcoords;
    mesh.vboId = (unsigned int *)MemAlloc(sizeof(unsigned int) * MAX_MESH_VERTEX_BUFFERS);
    UploadMesh(&mesh, false);

    return mesh;
}

void DrawTriangleStripGrid(int rows, int cols, float spacing, Color color) {
    static bool printed = false;
    rlColor3f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);

    for (int z = 0; z < rows - 1; z++) {
        if(!printed) {
            printf("Drawing row %d of %d\n", z, rows - 1);
        }
        rlBegin(GL_TRIANGLE_STRIP);

        bool leftToRight = (z % 2 == 0);

        for (int i = 0; i < cols; i++) {
            if(!printed) {
                printf("Drawing column %d of %d\n", i, cols);
            }
            int x = leftToRight ? i : (cols - 1 - i);

            float xPos = (x - (cols - 1) / 2.0f) * spacing;
            float z0   = (z - (rows - 1) / 2.0f) * spacing;
            float z1   = ((z + 1) - (rows - 1) / 2.0f) * spacing;

            rlVertex3f(xPos, 0.0f, z0); // bottom
            rlVertex3f(xPos, 0.0f, z1); // top

            if (!printed) {
                printf("bottom: (%f, %f, %f)\n", xPos, 0.0f, z0);
                printf("top: (%f, %f, %f)\n", xPos, 0.0f, z1);
            }
        }
        rlEnd();
    }
        if(!printed) {
            printf("DrawTriangleStripGrid: %d rows, %d cols, spacing %f\n", rows, cols, spacing);
            printed = true;
        }
}