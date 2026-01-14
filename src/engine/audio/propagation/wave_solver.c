/**
 * =================================================================================================
 *                      GPU-ACCELERATED WAVE-BASED ACOUSTICS
 * =================================================================================================
 * 
 * PURPOSE: Real-time acoustic propagation through 3D geometry
 * APPROACH: Solve wave equation on voxelized level geometry using compute shaders
 */

#include <include/math/math_all.h>
#include <stdlib.h>
#include <string.h>

#define GRID_SIZE 128
#define WAVE_SPEED 343.0f  // Speed of sound in air (m/s)

// =================================================================================================
// ACOUSTIC GRID
// =================================================================================================

typedef struct {
    float* pressure;      // Sound pressure at each voxel
    float* velocity_x;    // Particle velocity components
    float* velocity_y;
    float* velocity_z;
    
    uint8_t* material;    // Material ID per voxel (0=air, 1=wall, etc.)
    float* absorption;    // Absorption coefficient per material
    
    uint32_t grid_x, grid_y, grid_z;
    float cell_size;      // Meters per voxel
    
    // Sound sources
    float source_positions[16][3];
    float source_amplitudes[16];
    uint32_t source_count;
    
    // Listener
    float listener_position[3];
} AcousticGrid;

// =================================================================================================
// INITIALIZATION
// =================================================================================================

AcousticGrid* acoustics_create(uint32_t size_x, uint32_t size_y, uint32_t size_z, 
                               float cell_size) {
    AcousticGrid* grid = (AcousticGrid*)calloc(1, sizeof(AcousticGrid));
    
    grid->grid_x = size_x;
    grid->grid_y = size_y;
    grid->grid_z = size_z;
    grid->cell_size = cell_size;
    
    uint32_t total_cells = size_x * size_y * size_z;
    
    grid->pressure = (float*)calloc(total_cells, sizeof(float));
    grid->velocity_x = (float*)calloc(total_cells, sizeof(float));
    grid->velocity_y = (float*)calloc(total_cells, sizeof(float));
    grid->velocity_z = (float*)calloc(total_cells, sizeof(float));
    
    grid->material = (uint8_t*)calloc(total_cells, sizeof(uint8_t));
    grid->absorption = (float*)calloc(256, sizeof(float));  // 256 material types
    
    // Default absorption coefficients
    grid->absorption[0] = 0.01f;  // Air (minimal)
    grid->absorption[1] = 0.3f;   // Wood
    grid->absorption[2] = 0.05f;  // Stone
    grid->absorption[3] = 0.6f;   // Cloth/carpet
    
    return grid;
}

void acoustics_destroy(AcousticGrid* grid) {
    if (grid) {
        free(grid->pressure);
        free(grid->velocity_x);
        free(grid->velocity_y);
        free(grid->velocity_z);
        free(grid->material);
        free(grid->absorption);
        free(grid);
    }
}

// =================================================================================================
// GEOMETRY VOXELIZATION
// =================================================================================================

void acoustics_set_voxel_material(AcousticGrid* grid, uint32_t x, uint32_t y, uint32_t z, 
                                  uint8_t material_id) {
    if (x >= grid->grid_x || y >= grid->grid_y || z >= grid->grid_z) return;
    
    uint32_t idx = x + y * grid->grid_x + z * grid->grid_x * grid->grid_y;
    grid->material[idx] = material_id;
}

// =================================================================================================
// WAVE EQUATION SOLVER (CPU - would be GPU compute shader in production)
// =================================================================================================

static inline uint32_t grid_index(const AcousticGrid* grid, uint32_t x, uint32_t y, uint32_t z) {
    return x + y * grid->grid_x + z * grid->grid_x * grid->grid_y;
}

void acoustics_update(AcousticGrid* grid, float dt) {
    float dx = grid->cell_size;
    float c = WAVE_SPEED;
    float c_dt_dx = (c * dt) / dx;
    
    // Update particle velocities from pressure gradient
    for (uint32_t z = 1; z < grid->grid_z - 1; z++) {
        for (uint32_t y = 1; y < grid->grid_y - 1; y++) {
            for (uint32_t x = 1; x < grid->grid_x - 1; x++) {
                uint32_t idx = grid_index(grid, x, y, z);
                
                // Skip solid voxels
                if (grid->material[idx] != 0) continue;
                
                // Pressure gradient
                float dp_dx = grid->pressure[grid_index(grid, x+1, y, z)] - 
                             grid->pressure[grid_index(grid, x-1, y, z)];
                float dp_dy = grid->pressure[grid_index(grid, x, y+1, z)] - 
                             grid->pressure[grid_index(grid, x, y-1, z)];
                float dp_dz = grid->pressure[grid_index(grid, x, y, z+1)] - 
                             grid->pressure[grid_index(grid, x, y, z-1)];
                
                // Update velocities
                grid->velocity_x[idx] -= c_dt_dx * dp_dx * 0.5f;
                grid->velocity_y[idx] -= c_dt_dx * dp_dy * 0.5f;
                grid->velocity_z[idx] -= c_dt_dx * dp_dz * 0.5f;
            }
        }
    }
    
    // Update pressure from velocity divergence
    for (uint32_t z = 1; z < grid->grid_z - 1; z++) {
        for (uint32_t y = 1; y < grid->grid_y - 1; y++) {
            for (uint32_t x = 1; x < grid->grid_x - 1; x++) {
                uint32_t idx = grid_index(grid, x, y, z);
                
                if (grid->material[idx] != 0) {
                    // Boundary: reflect waves
                    grid->pressure[idx] = 0.0f;
                    continue;
                }
                
                // Velocity divergence
                float div_v = 
                    (grid->velocity_x[grid_index(grid, x+1, y, z)] - 
                     grid->velocity_x[grid_index(grid, x-1, y, z)]) +
                    (grid->velocity_y[grid_index(grid, x, y+1, z)] - 
                     grid->velocity_y[grid_index(grid, x, y-1, z)]) +
                    (grid->velocity_z[grid_index(grid, x, y, z+1)] - 
                     grid->velocity_z[grid_index(grid, x, y, z-1)]);
                
                // Update pressure
                grid->pressure[idx] -= c_dt_dx * div_v * 0.5f;
                
                // Apply absorption
                uint8_t mat = grid->material[idx];
                grid->pressure[idx] *= (1.0f - grid->absorption[mat] * dt);
            }
        }
    }
    
    // Add sound sources
    for (uint32_t i = 0; i < grid->source_count; i++) {
        uint32_t sx = (uint32_t)(grid->source_positions[i][0] / grid->cell_size);
        uint32_t sy = (uint32_t)(grid->source_positions[i][1] / grid->cell_size);
        uint32_t sz = (uint32_t)(grid->source_positions[i][2] / grid->cell_size);
        
        if (sx < grid->grid_x && sy < grid->grid_y && sz < grid->grid_z) {
            uint32_t idx = grid_index(grid, sx, sy, sz);
            grid->pressure[idx] += grid->source_amplitudes[i];
        }
    }
}

// =================================================================================================
// SOUND SOURCE MANAGEMENT
// =================================================================================================

void acoustics_add_source(AcousticGrid* grid, float x, float y, float z, float amplitude) {
    if (grid->source_count >= 16) return;
    
    uint32_t idx = grid->source_count++;
    grid->source_positions[idx][0] = x;
    grid->source_positions[idx][1] = y;
    grid->source_positions[idx][2] = z;
    grid->source_amplitudes[idx] = amplitude;
}

void acoustics_set_listener(AcousticGrid* grid, float x, float y, float z) {
    grid->listener_position[0] = x;
    grid->listener_position[1] = y;
    grid->listener_position[2] = z;
}

// =================================================================================================
// AUDIO OUTPUT
// =================================================================================================

float acoustics_sample_at_listener(AcousticGrid* grid) {
    uint32_t lx = (uint32_t)(grid->listener_position[0] / grid->cell_size);
    uint32_t ly = (uint32_t)(grid->listener_position[1] / grid->cell_size);
    uint32_t lz = (uint32_t)(grid->listener_position[2] / grid->cell_size);
    
    if (lx >= grid->grid_x || ly >= grid->grid_y || lz >= grid->grid_z) {
        return 0.0f;
    }
    
    uint32_t idx = grid_index(grid, lx, ly, lz);
    return grid->pressure[idx];
}
