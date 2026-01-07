// Shallow Water Simulation - Complete (7 TODOs)
#include "physics/fluids/shallow_water.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

typedef struct {
    float *height;        // Water column height
    float *velocity_x;    // Horizontal velocity
    float *velocity_z;    // Vertical velocity
    float *foam;          // Foam mask [0,1]
    uint32_t width, depth;
    float cell_size;
    float damping;
    float wave_speed;
} ShallowWaterGrid;

ShallowWaterGrid *shallow_water_init(uint32_t width, uint32_t depth, float cell_size) {
    ShallowWaterGrid *grid = malloc(sizeof(ShallowWaterGrid));
    uint32_t count = width * depth;
    grid->height = calloc(count, sizeof(float));
    grid->velocity_x = calloc(count, sizeof(float));
    grid->velocity_z = calloc(count, sizeof(float));
    grid->foam = calloc(count, sizeof(float));
    grid->width = width;
    grid->depth = depth;
    grid->cell_size = cell_size;
    grid->damping = 0.995f;
    grid->wave_speed = 5.0f;
    return grid;
}

void shallow_water_update(ShallowWaterGrid *grid, float dt) {
    // Wave propagation solver (simplified shallow water equations)
    for (uint32_t z = 1; z < grid->depth - 1; z++) {
        for (uint32_t x = 1; x < grid->width - 1; x++) {
            uint32_t idx = z * grid->width + x;
            
            // Height gradient drives velocity
            float dh_dx = (grid->height[idx+1] - grid->height[idx-1]) / (2.0f * grid->cell_size);
            float dh_dz = (grid->height[idx+grid->width] - grid->height[idx-grid->width]) / (2.0f * grid->cell_size);
            
            grid->velocity_x[idx] += -grid->wave_speed * dh_dx * dt;
            grid->velocity_z[idx] += -grid->wave_speed * dh_dz * dt;
            
            // Damping
            grid->velocity_x[idx] *= grid->damping;
            grid->velocity_z[idx] *= grid->damping;
        }
    }
    
    // Update height from velocity divergence
    for (uint32_t z = 1; z < grid->depth - 1; z++) {
        for (uint32_t x = 1; x < grid->width - 1; x++) {
            uint32_t idx = z * grid->width + x;
            
            float div = (grid->velocity_x[idx+1] - grid->velocity_x[idx-1]) / (2.0f * grid->cell_size) +
                       (grid->velocity_z[idx+grid->width] - grid->velocity_z[idx-grid->width]) / (2.0f * grid->cell_size);
            
            grid->height[idx] -= div * dt;
        }
    }
    
    // Boundary reflection
    for (uint32_t x = 0; x < grid->width; x++) {
        grid->velocity_z[x] = 0.0f; // Top
        grid->velocity_z[(grid->depth-1) * grid->width + x] = 0.0f; // Bottom
    }
    for (uint32_t z = 0; z < grid->depth; z++) {
        grid->velocity_x[z * grid->width] = 0.0f; // Left
        grid->velocity_x[z * grid->width + grid->width - 1] = 0.0f; // Right
    }
    
    // Foam generation (where velocity is high)
    for (uint32_t i = 0; i < grid->width * grid->depth; i++) {
        float speed = sqrtf(grid->velocity_x[i]*grid->velocity_x[i] + grid->velocity_z[i]*grid->velocity_z[i]);
        grid->foam[i] = fminf(1.0f, speed / 2.0f);
        grid->foam[i] *= 0.95f; // Decay
    }
}

void shallow_water_add_ripple(ShallowWaterGrid *grid, float x, float z, float strength) {
    // Object interaction
    int gx = (int)(x / grid->cell_size);
    int gz = (int)(z / grid->cell_size);
    if (gx >= 0 && gx < (int)grid->width && gz >= 0 && gz < (int)grid->depth) {
        grid->height[gz * grid->width + gx] += strength;
    }
}
