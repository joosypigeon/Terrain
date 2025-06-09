#include "perlin_noise.h"
#include <stdlib.h>
#include <math.h>

static unsigned char perm[512];

static float fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static float lerp(float t, float a, float b)
{
    return a + t * (b - a);
}

static float grad(int hash, float x, float y)
{
    switch (hash & 7)
    {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        default: return -y;
    }
}

void perlin_init(int seed)
{
    srand(seed);
    unsigned char p[256];
    for (int i = 0; i < 256; i++) p[i] = (unsigned char)i;
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        unsigned char tmp = p[i];
        p[i] = p[j];
        p[j] = tmp;
    }
    for (int i = 0; i < 512; i++) perm[i] = p[i & 255];
}

float perlin_noise2d(float x, float y)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    float xf = x - floorf(x);
    float yf = y - floorf(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = perm[xi    ] + yi;
    int ab = perm[xi    ] + yi + 1;
    int ba = perm[xi + 1] + yi;
    int bb = perm[xi + 1] + yi + 1;

    float x1 = lerp(u, grad(perm[aa], xf,     yf), grad(perm[ba], xf - 1, yf));
    float x2 = lerp(u, grad(perm[ab], xf, yf - 1), grad(perm[bb], xf - 1, yf - 1));

    return lerp(v, x1, x2);
}

float fractal_noise2d(float x, float y, int octaves, float persistence) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += perlin_noise2d(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return (total / maxValue + 1.0f) / 2.0f;  // Normalise to [0, 1]
}

float grad3D(int hash, float x, float y, float z) {
    int h = hash & 15;      // 16 possible values (0–15)
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin_noise3d(float x, float y, float z)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    // Nested permutation indexing
    int aaa = perm[perm[perm[xi    ] + yi    ] + zi    ];
    int aba = perm[perm[perm[xi    ] + yi + 1] + zi    ];
    int baa = perm[perm[perm[xi + 1] + yi    ] + zi    ];
    int bba = perm[perm[perm[xi + 1] + yi + 1] + zi    ];
    int aab = perm[perm[perm[xi    ] + yi    ] + zi + 1];
    int abb = perm[perm[perm[xi    ] + yi + 1] + zi + 1];
    int bab = perm[perm[perm[xi + 1] + yi    ] + zi + 1];
    int bbb = perm[perm[perm[xi + 1] + yi + 1] + zi + 1];

    float x1 = lerp(u, grad3D(aaa, xf,     yf,     zf),
                       grad3D(baa, xf - 1, yf,     zf));
    float x2 = lerp(u, grad3D(aba, xf,     yf - 1, zf),
                       grad3D(bba, xf - 1, yf - 1, zf));
    float y1 = lerp(v, x1, x2);

    float x3 = lerp(u, grad3D(aab, xf,     yf,     zf - 1),
                       grad3D(bab, xf - 1, yf,     zf - 1));
    float x4 = lerp(u, grad3D(abb, xf,     yf - 1, zf - 1),
                       grad3D(bbb, xf - 1, yf - 1, zf - 1));
    float y2 = lerp(v, x3, x4);

    return lerp(w, y1, y2);  // returns in [-1, 1]
}


float fractal_noise3d(float x, float y, float z, int octaves, float persistence) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += perlin_noise3d(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return (total / maxValue + 1.0f) / 2.0f;  // Normalise to [0, 1]
}