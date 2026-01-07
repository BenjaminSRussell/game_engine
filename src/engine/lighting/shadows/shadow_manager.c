#include "lighting/shadows/shadow_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <include/math/math.h>

/**
 * Shadow Manager Implementation
 * Manages shadow map allocation, atlasing, and LOD
 */

#define MAX_SHADOW_MAPS 128
#define MAX_LOD_LEVELS 4

struct ShadowManager {
    ShadowMap shadow_maps[MAX_SHADOW_MAPS];
    uint32_t shadow_map_count;
    uint32_t next_id;
    
    ShadowAtlas atlas;
    
    // LOD distances
    float lod_distances[MAX_LOD_LEVELS];
    uint32_t lod_count;
    
    // Priority queue for updates
    float priorities[MAX_SHADOW_MAPS];
};

// Create shadow manager
ShadowManager* shadow_manager_create(uint32_t atlas_size) {
    ShadowManager* manager = calloc(1, sizeof(ShadowManager));
    if (!manager) return NULL;
    
    manager->shadow_map_count = 0;
    manager->next_id = 1;
    
    // Initialize shadow atlas
    manager->atlas.width = atlas_size;
    manager->atlas.height = atlas_size;
    manager->atlas.tile_size = 512; // Default tile size
    
    uint32_t tiles_x = atlas_size / manager->atlas.tile_size;
    uint32_t tiles_y = atlas_size / manager->atlas.tile_size;
    manager->atlas.tile_count = tiles_x * tiles_y;
    
    manager->atlas.tile_allocated = calloc(manager->atlas.tile_count, sizeof(bool));
    
    // Default LOD distances
    manager->lod_distances[0] = 50.0f;
    manager->lod_distances[1] = 100.0f;
    manager->lod_distances[2] = 200.0f;
    manager->lod_distances[3] = 500.0f;
    manager->lod_count = 4;
    
    printf("[ShadowManager] Created with %ux%u atlas (%u tiles)\n",
           atlas_size, atlas_size, manager->atlas.tile_count);
    
    return manager;
}

// Destroy shadow manager
void shadow_manager_destroy(ShadowManager* manager) {
    if (!manager) return;
    
    if (manager->atlas.tile_allocated) {
        free(manager->atlas.tile_allocated);
    }
    
    free(manager);
    printf("[ShadowManager] Destroyed\n");
}

// Allocate shadow map
uint32_t shadow_manager_allocate_shadow_map(ShadowManager* manager,
                                            ShadowMapType type,
                                            uint32_t resolution) {
    if (!manager || manager->shadow_map_count >= MAX_SHADOW_MAPS) {
        printf("[ShadowManager] ERROR: Cannot allocate more shadow maps\n");
        return 0;
    }
    
    ShadowMap* map = &manager->shadow_maps[manager->shadow_map_count];
    map->id = manager->next_id++;
    map->type = type;
    map->resolution = resolution;
    map->is_allocated = true;
    
    // Set array layers based on type
    switch (type) {
        case SHADOW_MAP_2D:
            map->array_layers = 1;
            break;
        case SHADOW_MAP_CUBE:
            map->array_layers = 6; // 6 cube faces
            break;
        case SHADOW_MAP_CASCADE:
            map->array_layers = 4; // 4 cascades (typical)
            break;
    }
    
    // Initialize priority to medium
    manager->priorities[manager->shadow_map_count] = 0.5f;
    
    manager->shadow_map_count++;
    
    printf("[ShadowManager] Allocated %s shadow map (ID: %u, %ux%u, %u layers)\n",
           type == SHADOW_MAP_2D ? "2D" :
           type == SHADOW_MAP_CUBE ? "Cube" : "Cascade",
           map->id, resolution, resolution, map->array_layers);
    
    return map->id;
}

// Free shadow map
void shadow_manager_free_shadow_map(ShadowManager* manager, uint32_t shadow_map_id) {
    if (!manager) return;
    
    for (uint32_t i = 0; i < manager->shadow_map_count; i++) {
        if (manager->shadow_maps[i].id == shadow_map_id) {
            // Mark as not allocated
            manager->shadow_maps[i].is_allocated = false;
            
            // Shift remaining maps
            memmove(&manager->shadow_maps[i], &manager->shadow_maps[i + 1],
                   (manager->shadow_map_count - i - 1) * sizeof(ShadowMap));
            memmove(&manager->priorities[i], &manager->priorities[i + 1],
                   (manager->shadow_map_count - i - 1) * sizeof(float));
            
            manager->shadow_map_count--;
            
            printf("[ShadowManager] Freed shadow map ID: %u\n", shadow_map_id);
            return;
        }
    }
}

// Get shadow map
ShadowMap* shadow_manager_get_shadow_map(ShadowManager* manager, uint32_t shadow_map_id) {
    if (!manager) return NULL;
    
    for (uint32_t i = 0; i < manager->shadow_map_count; i++) {
        if (manager->shadow_maps[i].id == shadow_map_id) {
            return &manager->shadow_maps[i];
        }
    }
    
    return NULL;
}

// Shadow atlas allocation
bool shadow_atlas_allocate_tile(ShadowAtlas* atlas, uint32_t* out_x, uint32_t* out_y) {
    if (!atlas) return false;
    
    uint32_t tiles_x = atlas->width / atlas->tile_size;
    
    for (uint32_t i = 0; i < atlas->tile_count; i++) {
        if (!atlas->tile_allocated[i]) {
            atlas->tile_allocated[i] = true;
            
            *out_x = (i % tiles_x) * atlas->tile_size;
            *out_y = (i / tiles_x) * atlas->tile_size;
            
            return true;
        }
    }
    
    printf("[ShadowAtlas] ERROR: No free tiles\n");
    return false;
}

void shadow_atlas_free_tile(ShadowAtlas* atlas, uint32_t x, uint32_t y) {
    if (!atlas) return;
    
    uint32_t tiles_x = atlas->width / atlas->tile_size;
    uint32_t tile_x = x / atlas->tile_size;
    uint32_t tile_y = y / atlas->tile_size;
    uint32_t index = tile_y * tiles_x + tile_x;
    
    if (index < atlas->tile_count) {
        atlas->tile_allocated[index] = false;
    }
}

// Priority management
void shadow_manager_set_priority(ShadowManager* manager, uint32_t shadow_map_id, float priority) {
    if (!manager) return;
    
    for (uint32_t i = 0; i < manager->shadow_map_count; i++) {
        if (manager->shadow_maps[i].id == shadow_map_id) {
            manager->priorities[i] = priority;
            return;
        }
    }
}

void shadow_manager_update_prioritization(ShadowManager* manager) {
    if (!manager) return;
    
    // TODO: Sort shadow maps by priority for update scheduling
    // This would determine which shadows get updated each frame
}

// LOD system
void shadow_manager_set_lod_distances(ShadowManager* manager, const float* distances, uint32_t count) {
    if (!manager || count > MAX_LOD_LEVELS) return;
    
    memcpy(manager->lod_distances, distances, count * sizeof(float));
    manager->lod_count = count;
    
    printf("[ShadowManager] Set %u LOD distances\n", count);
}

uint32_t shadow_manager_get_lod_for_distance(const ShadowManager* manager, float distance) {
    if (!manager) return 0;
    
    for (uint32_t i = 0; i < manager->lod_count; i++) {
        if (distance < manager->lod_distances[i]) {
            return i;
        }
    }
    
    return manager->lod_count - 1;
}

// Stats
uint32_t shadow_manager_get_allocated_count(const ShadowManager* manager) {
    return manager ? manager->shadow_map_count : 0;
}

uint32_t shadow_manager_get_memory_usage(const ShadowManager* manager) {
    if (!manager) return 0;
    
    uint32_t total = 0;
    
    for (uint32_t i = 0; i < manager->shadow_map_count; i++) {
        const ShadowMap* map = &manager->shadow_maps[i];
        // Assume 16 bits per texel for depth
        uint32_t bytes_per_layer = map->resolution * map->resolution * 2;
        total += bytes_per_layer * map->array_layers;
    }
    
    return total;
}
