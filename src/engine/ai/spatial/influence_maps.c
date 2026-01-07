#include "ai/ai_types.h"
#include "core/core.h"
#include <string.h>
#include <math.h>

#define GRID_SIZE 64

typedef struct {
    float values[GRID_SIZE][GRID_SIZE];
    int width, height;
} InfluenceMap;

static InfluenceMap g_maps[4]; // Danger, Cover, etc.

void influence_map_init(InfluenceMap *map, int w, int h) {
    map->width = w;
    map->height = h;
    memset(map->values, 0, sizeof(map->values));
}

void influence_add(InfluenceMap *map, int x, int y, float radius, float value) {
    int r = (int)radius;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;
            
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist > radius) cont continue;
            
            // Gaussian falloff
            float falloff = expf(-(dist*dist) / (2.0f * radius*radius));
            map->values[ny][nx] += value * falloff;
        }
    }
}

void influence_propagate(InfluenceMap *map, float decay) {
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            map->values[y][x] *= decay;
        }
    }
}

void influence_combine(InfluenceMap *result, InfluenceMap *a, InfluenceMap *b, int op) {
    for (int y = 0; y < result->height; y++) {
        for (int x = 0; x < result->width; x++) {
            float va = a->values[y][x];
            float vb = b->values[y][x];
            switch(op) {
                case 0: result->values[y][x] = va + vb; break;  // ADD
                case 1: result->values[y][x] = va * vb; break;  // MULTIPLY
                case 2: result->values[y][x] = fmaxf(va, vb); break; // MAX
                case 3: result->values[y][x] = fminf(va, vb); break; // MIN
            }
        }
    }
}

void influence_get_gradient(InfluenceMap *map, int x, int y, float *gx, float *gy) {
    float dx = (x < map->width-1 ? map->values[y][x+1] : 0) - (x > 0 ? map->values[y][x-1] : 0);
    float dy = (y < map->height-1 ? map->values[y+1][x] : 0) - (y > 0 ? map->values[y-1][x] : 0);
    *gx = dx;
    *gy = dy;
}
