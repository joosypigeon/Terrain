#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

void perlin_init(int seed);
float perlin_noise2d(float x, float y);
float fractal_noise2d(float x, float y, int octaves, float persistence);

float fractal_noise3d(float x, float y, float z, int octaves, float persistence);

#endif // PERLIN_NOISE_H
