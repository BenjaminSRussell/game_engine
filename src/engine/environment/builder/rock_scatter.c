#include "environment/builder/rock_scatter.h"
#include <stdlib.h>
#include <math.h>

typedef struct {
    float x, y, z;
} Vec3;

void rock_scatter_drop_simulation(void *entities, int count, void *terrain_collider) {
    // Physics simulation stub
    for (int i = 0; i < count; i++) {
        // 1. Raycast down to find ground
        // float ground_y = raycast(entity.pos, DOWN);
        
        // 2. Simulate physics step (bounce/roll)
        // Or just snap for simple scattering:
        // entity.pos.y = ground_y;
    }
}

void rock_scatter_randomize_scale(void *entity, float min_scale, float max_scale) {
    float s = min_scale + ((float)rand() / RAND_MAX) * (max_scale - min_scale);
    // Non-uniform option?
    // entity.scale = vec3(s, s * random(0.8, 1.2), s);
    (void)entity; // Stub
}

void rock_scatter_embed(void *entity, float depth) {
    // Sink object into ground to avoid floating artifacts
    // entity.pos.y -= depth;
    (void)entity; // Stub
}
