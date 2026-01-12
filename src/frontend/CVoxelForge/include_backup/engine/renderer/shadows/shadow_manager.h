#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Shadow Manager
 * Central system for managing shadow maps across all light types
 */

typedef struct Light Light;

// Shadow map type
typedef enum {
    SHADOW_MAP_2D,           // Directional/spot lights
    SHADOW_MAP_CUBE,         // Point lights
    SHADOW_MAP_CASCADE       // Cascaded shadow maps
} ShadowMapType;

// Shadow map
typedef struct {
    uint32_t id;
    uint32_t texture_id;
    ShadowMapType type;
    uint32_t resolution;
    uint32_t array_layers;   // For cascades or cube faces
    bool is_allocated;
} ShadowMap;

// Shadow atlas for efficient memory usage
typedef struct {
    uint32_t texture_id;
    uint32_t width;
    uint32_t height;
    uint32_t tile_size;
    bool* tile_allocated;
    uint32_t tile_count;
} ShadowAtlas;

// Shadow manager
typedef struct ShadowManager ShadowManager;

// Initialization
ShadowManager* shadow_manager_create(uint32_t atlas_size);
void shadow_manager_destroy(ShadowManager* manager);

// Shadow map allocation
uint32_t shadow_manager_allocate_shadow_map(ShadowManager* manager,
                                            ShadowMapType type,
                                            uint32_t resolution);
void shadow_manager_free_shadow_map(ShadowManager* manager, uint32_t shadow_map_id);
ShadowMap* shadow_manager_get_shadow_map(ShadowManager* manager, uint32_t shadow_map_id);

// Shadow atlas management
bool shadow_atlas_allocate_tile(ShadowAtlas* atlas, uint32_t* out_x, uint32_t* out_y);
void shadow_atlas_free_tile(ShadowAtlas* atlas, uint32_t x, uint32_t y);

// Update priorities (for dynamic shadow updates)
void shadow_manager_set_priority(ShadowManager* manager, uint32_t shadow_map_id, float priority);
void shadow_manager_update_prioritization(ShadowManager* manager);

// LOD system
void shadow_manager_set_lod_distances(ShadowManager* manager, const float* distances, uint32_t count);
uint32_t shadow_manager_get_lod_for_distance(const ShadowManager* manager, float distance);

// Stats
uint32_t shadow_manager_get_allocated_count(const ShadowManager* manager);
uint32_t shadow_manager_get_memory_usage(const ShadowManager* manager);

#endif // SHADOW_MANAGER_H
