#ifndef LIGHT_CULLING_H
#define LIGHT_CULLING_H

#include "../../lighting/light_manager.h"
#include "../../include/math/mat4.h"

/*
 * Clustered Light Culling System
 * Divides the view frustum into a 3D grid (clusters) and assigns lights to them.
 */

#define CULLING_TILE_SIZE 16 // Pixels
#define CULLING_GRID_Z 24    // Slices

typedef struct {
    uint32_t x_count;
    uint32_t y_count;
    uint32_t z_count;
    
    // Frustum definition
    float near_plane;
    float far_plane;
    Mat4 inverse_projection;
    
    // Culling results
    // We could use a linear buffer for light indices per tile
    uint32_t* light_indices;
    uint32_t* light_counts; // Per tile
    
    uint32_t max_lights_per_tile;
} LightCullingSystem;

LightCullingSystem* LightCulling_Create(uint32_t screen_width, uint32_t screen_height);
void LightCulling_Destroy(LightCullingSystem* system);

// Update culling state, rebuild grid, and assign lights
void LightCulling_Update(LightCullingSystem* system, LightManager* light_manager, 
                        const Mat4* view, const Mat4* proj, 
                        float near_z, float far_z);

// GPU Data Access (for upload)
// Returns pointer to light index list
const uint32_t* LightCulling_GetLightIndices(LightCullingSystem* system);
// Returns pointer to grid/count buffer
const uint32_t* LightCulling_GetLightCounts(LightCullingSystem* system);

#endif // LIGHT_CULLING_H
