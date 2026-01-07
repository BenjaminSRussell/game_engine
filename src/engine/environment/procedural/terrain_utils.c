#include "environment/procedural/terrain_utils.h"
#include <include/math/math.h>

float terrain_utils_perlin_noise(float x, float y) {
    // Simple perlin noise stub
    return sinf(x * 0.1f) * cosf(y * 0.1f);
}

void terrain_utils_smooth(float *heightmap, int width, int height) {
    // Box blur smoothing
}

void terrain_utils_normalize(float *heightmap, int width, int height, float min, float max) {
    // Normalize heightmap to range
}
