#include "torus.h"
#include "perlin_noise.h"

#include <stdlib.h>

#include <stdio.h>

#include <assert.h>

typedef struct TorusCoords {
    float theta, phi;
    float cosTheta, sinTheta;
    float cosPhi, sinPhi;
} TorusCoords;

static TorusCoords torusCoords = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};

static float R = -1.0f;
static float r = -1.0f;

void SetTorusDimensions(float major, float minor) {
    R = major;
    r = minor;
}

// Generates a torus mesh with the specified number of rings and sides.
Mesh MyGenTorusMesh(int rings, int sides) {
    int vertexCount = rings * sides;
    int indexCount = rings * sides * 6;  // 2 triangles per quad, 3 indices each

    Vector3 *vertices = MemAlloc(vertexCount * sizeof(Vector3));
    Vector3 *normals = MemAlloc(vertexCount * sizeof(Vector3));
    Vector2 *texcoords = MemAlloc(vertexCount * sizeof(Vector2));
    unsigned short *indices = MemAlloc(indexCount * sizeof(unsigned short));

    // Generate de-duplicated vertices
    for (int i = 0; i < rings; i++) {
        float theta = ((float)i / rings) * 2.0f * PI;
        float cosTheta = cosf(theta);
        float sinTheta = sinf(theta);

        for (int j = 0; j < sides; j++) {
            float phi = ((float)j / sides) * 2.0f * PI;
            float cosPhi = cosf(phi);
            float sinPhi = sinf(phi);

            float x = (R + r * cosPhi) * cosTheta;
            float y = r * sinPhi;
            float z = (R + r * cosPhi) * sinTheta;

            float nx = cosPhi * cosTheta;
            float ny = sinPhi;
            float nz = cosPhi * sinTheta;

            int idx = i * sides + j;
            vertices[idx] = (Vector3){ x, y, z };
            normals[idx] = (Vector3){ nx, ny, nz };
            texcoords[idx] = (Vector2){ (float)i / rings, (float)j / sides };
        }
    }

    // Generate indices with wrapping
    int k = 0;
    for (int i = 0; i < rings; i++) {
        int i1 = (i + 1) % rings;
        for (int j = 0; j < sides; j++) {
            int j1 = (j + 1) % sides;

            int v00 = i * sides + j;
            int v01 = i * sides + j1;
            int v10 = i1 * sides + j;
            int v11 = i1 * sides + j1;

            // Triangle 1
            indices[k++] = v00;
            indices[k++] = v01;
            indices[k++] = v10;

            // Triangle 2
            indices[k++] = v10;
            indices[k++] = v01;
            indices[k++] = v11;
        }
    }

    // Build mesh
    Mesh mesh = { 0 };
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = indexCount / 3;
    mesh.vertices = (float *)vertices;
    mesh.normals = (float *)normals;
    mesh.texcoords = (float *)texcoords;
    mesh.indices = indices;

    UploadMesh(&mesh, false);
    return mesh;
}

#define WRAP_MOD(a, m) (((a) % (m) + (m)) % (m))

Mesh MyGenFlatTorusMesh(int rings, int sides) {
    float **heightmap = malloc(SCREEN_HEIGHT * sizeof(float *));
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        heightmap[i] = malloc(SCREEN_WIDTH * sizeof(float));
    }

    unsigned char image[SCREEN_HEIGHT][SCREEN_WIDTH];

    perlin_init(42);  // consistent seed

    float scale = 0.05f;


    float min = 2.0f;
    float max = -1.0f;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            float nx = x * scale;
            float ny = y * scale;
            float noise = fractal_noise2d(nx, ny, 16, 0.5f);  // 6 octaves
            if (noise < min) min = noise;
            if (noise > max) max = noise;
            heightmap[y][x] = noise;
            image[y][x] = (unsigned char)(noise * 255.0f);
        }
    }
    printf("Heightmap min: %f, max: %f\n", min, max);

    FILE *f = fopen("heightmap.pgm", "wb");
    if (!f) {
        perror("Cannot write image");
        exit(1);
    }

    fprintf(f, "P5\n%d %d\n255\n", SCREEN_WIDTH, SCREEN_HEIGHT);  // P5 = binary greyscale
    fwrite(image, 1, SCREEN_WIDTH * SCREEN_HEIGHT, f);
    fclose(f);

    printf("Heightmap written to heightmap.pgm\n");

    float upper_bound = 50.0f;
    float lower_bound = 0.0f;
    float gradient = (upper_bound - lower_bound) / (max - min);
    printf("Gradient: %f\n", gradient);


    // 1. Allocate vertex and normal grids
    Vector3 **vertexGrid = MemAlloc(rings * sizeof(Vector3 *));
    Vector3 **normalGrid = MemAlloc(rings * sizeof(Vector3 *));
    for (int i = 0; i < rings; i++) {
        vertexGrid[i] = MemAlloc(sides * sizeof(Vector3));
        normalGrid[i] = MemAlloc(sides * sizeof(Vector3));
        for (int j = 0; j < sides; j++) {
            normalGrid[i][j] = (Vector3){0.0f, 0.0f, 0.0f};
        }
    }

    // 2. Fill vertexGrid with positions (and optionally sample height)
    for (int i = 0; i < rings; i++) {
        float theta = (float)i / rings * 2.0f * PI;
        for (int j = 0; j < sides; j++) {
            float phi = (float)j / sides * 2.0f * PI;

            float x = SCREEN_HEIGHT - phi * r;
            float z = R * theta;

            int sx = WRAP_MOD((int)z, SCREEN_WIDTH);
            int sy = WRAP_MOD((int)(SCREEN_HEIGHT - x), SCREEN_HEIGHT);

            float height = heightmap[sy][sx];
            float adjusted_height = lower_bound + (height - min) * gradient;

            vertexGrid[i][j] = (Vector3){ x, adjusted_height, z };
        }
    }

    for (int i = 0; i < rings; i++) {
        int i1 = (i + 1) % rings;
        for (int j = 0; j < sides; j++) {
            int j1 = (j + 1) % sides;

            Vector3 p00 = vertexGrid[i][j];
            Vector3 p01 = vertexGrid[i][j1];
            Vector3 p10 = vertexGrid[i1][j];
            Vector3 p11 = vertexGrid[i1][j1];

            // Triangle 1: p00, p01, p10
            Vector3 edge1 = Vector3Subtract(p01, p00);
            Vector3 edge2 = Vector3Subtract(p10, p00);
            Vector3 n1 = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

            normalGrid[i][j] = Vector3Add(normalGrid[i][j], n1);
            normalGrid[i][j1] = Vector3Add(normalGrid[i][j1], n1);
            normalGrid[i1][j] = Vector3Add(normalGrid[i1][j], n1);

            // Triangle 2: p10, p01, p11
            Vector3 edge3 = Vector3Subtract(p01, p10);
            Vector3 edge4 = Vector3Subtract(p11, p10);
            Vector3 n2 = Vector3Normalize(Vector3CrossProduct(edge3, edge4));

            normalGrid[i1][j] = Vector3Add(normalGrid[i1][j], n2);
            normalGrid[i][j1] = Vector3Add(normalGrid[i][j1], n2);
            normalGrid[i1][j1] = Vector3Add(normalGrid[i1][j1], n2);
        }
    }

    // 3. Generate indices (each quad = 2 triangles = 6 indices)
    int indexCount = rings * sides * 6;
    unsigned short *indices = MemAlloc(indexCount * sizeof(unsigned short)); // Use uint16 for Raylib
    int k = 0;
    for (int i = 0; i < rings - 1; i++) {
        int i1 = i + 1;
        for (int j = 0; j < sides - 1; j++) {
            int j1 = j + 1;

            int v00 = i * sides + j;
            int v01 = i * sides + j1;
            int v10 = i1 * sides + j;
            int v11 = i1 * sides + j1;

            // Triangle 1
            indices[k++] = v00;
            indices[k++] = v01;
            indices[k++] = v10;

            // Triangle 2
            indices[k++] = v10;
            indices[k++] = v01;
            indices[k++] = v11;
        }
    }

    int vertexCount = rings * sides;
    Vector3 *flatVertices = MemAlloc(vertexCount * sizeof(Vector3));
    Vector3 *flatNormals = MemAlloc(vertexCount * sizeof(Vector3));
    Vector2 *texcoords = MemAlloc(vertexCount * sizeof(Vector2));

    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < sides; j++) {
            int idx = i * sides + j;
            flatVertices[idx] = vertexGrid[i][j];
            flatNormals[idx] = Vector3Normalize(normalGrid[i][j]);
            texcoords[idx] = (Vector2){ (float)j / sides, (float)i / rings };
        }
    }

    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        free(heightmap[i]);
    }
    free(heightmap);

    Mesh mesh = { 0 };
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = indexCount / 3;
    mesh.vertices = (float *)flatVertices;
    mesh.normals = (float *)flatNormals;
    mesh.texcoords = (float *)texcoords;
    mesh.indices = (unsigned short *)indices;

    UploadMesh(&mesh, false);
    return mesh;
}



float get_theta(float u) {
        return 2 * PI * u / SCREEN_WIDTH;
}

float get_phi(float v) {
        return 2 * PI * v / SCREEN_HEIGHT;
}

Vector3 get_torus_normal(float u, float v) {
    float theta = get_theta(u);
    float phi = get_phi(v);

    float nx = cosf(phi) * cosf(theta);
    float ny = sinf(phi);
    float nz = cosf(phi) * sinf(theta);

    return (Vector3){ nx, ny, nz };
}

Vector3 get_phi_tangent(float u, float v) {
    float theta = get_theta(u);
    float phi = get_phi(v);

    float tx = -sinf(phi) * cosf(theta);
    float ty = cosf(phi);
    float tz = -sinf(phi) * sinf(theta);

    return (Vector3){ tx, ty, tz };
}

Vector3 get_theta_tangent(float u, float v) {
    float theta = get_theta(u);

    float tx = -sinf(theta);
    float ty = 0.0f;
    float tz =  cosf(theta);

    return (Vector3){ tx, ty, tz };
}

Vector3 get_torus_position(float u, float v) {
    float theta = get_theta(u);
    float phi = get_phi(v);

    float x = (R + r * cosf(phi)) * cosf(theta);
    float y = r * sinf(phi);
    float z = (R + r * cosf(phi)) * sinf(theta);

    return (Vector3){ x, y, z };
}

void set_torus_coords(float u, float v) {
    torusCoords.theta = get_theta(u);
    torusCoords.phi = get_phi(v);
    torusCoords.cosTheta = cosf(torusCoords.theta);
    torusCoords.sinTheta = sinf(torusCoords.theta);
    torusCoords.cosPhi = cosf(torusCoords.phi);
    torusCoords.sinPhi = sinf(torusCoords.phi);
}

Vector3 get_torus_position_fast() {
    return (Vector3){ (R + r * torusCoords.cosPhi) * torusCoords.cosTheta,
                      r * torusCoords.sinPhi,
                      (R + r * torusCoords.cosPhi) * torusCoords.sinTheta };
}

Vector3 get_torus_normal_fast() {
    return (Vector3){ torusCoords.cosPhi * torusCoords.cosTheta,
                      torusCoords.sinPhi,
                      torusCoords.cosPhi * torusCoords.sinTheta };
}
Vector3 get_theta_tangent_fast() {
    return (Vector3){ -torusCoords.sinTheta, 0.0f, torusCoords.cosTheta };
}
Vector3 get_phi_tangent_fast() {
    return (Vector3){ -torusCoords.sinPhi * torusCoords.cosTheta,
                      torusCoords.cosPhi,
                      -torusCoords.sinPhi * torusCoords.sinTheta };
}