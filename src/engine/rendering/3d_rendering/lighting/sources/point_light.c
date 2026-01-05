/*
 * point_light.c
 * Point light rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "point_light.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_POINT_LIGHT_MAX_COUNT 4096
#define LIGHTING_POINT_LIGHT_DEFAULT_CAPACITY 256
#define LIGHTING_POINT_LIGHT_ALIGNMENT 16
#ifndef EPSILON
#define EPSILON 0.0001f
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_point_light_internal {
    uint32_t id;
    point_light_t light_data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint32_t flags;
} lighting_point_light_internal_t;

typedef struct lighting_point_light_context {
    lighting_point_light_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    // Simple free list for ID recycling
    uint32_t* free_indices;
    uint32_t free_count;
    bool initialized;
} lighting_point_light_context_t;

static lighting_point_light_context_t g_point_light_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static inline float smooth_attenuation(float distance_sq, float radius_sq, float inverse_radius_sq) {
    if (distance_sq >= radius_sq) {
        return 0.0f;
    }
    
    // Inverse square falloff: 1 / (d^2 + 1)
    // We use a modified version that goes to 0 at radius
    // Based on Karis 2013, "Real Shading in Unreal Engine 4"
    
    float distance = sqrtf(distance_sq);
    float x = distance * sqrtf(inverse_radius_sq); // distance / radius
    float x2 = x * x;
    float x4 = x2 * x2;
    
    // Window function to force falloff to 0 at radius
    float window = 1.0f - x4;
    window = window < 0.0f ? 0.0f : window;
    window = window * window;
    
    return window / (distance_sq + 1.0f);
}

static bool lighting_point_light_validate(const lighting_point_light_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void lighting_point_light_cleanup_internal(lighting_point_light_internal_t* item) {
    if (!item) return;
    item->initialized = false;
    item->light_data.active = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_point_light_init(void) {
    if (g_point_light_ctx.initialized) {
        return 0; // Already initialized
    }

    g_point_light_ctx.capacity = LIGHTING_POINT_LIGHT_DEFAULT_CAPACITY;
    g_point_light_ctx.items = calloc(g_point_light_ctx.capacity, sizeof(lighting_point_light_internal_t));
    if (!g_point_light_ctx.items) {
        return -1;
    }
    
    g_point_light_ctx.free_indices = calloc(g_point_light_ctx.capacity, sizeof(uint32_t));
    if (!g_point_light_ctx.free_indices) {
        free(g_point_light_ctx.items);
        return -1;
    }
    
    g_point_light_ctx.free_count = 0;
    g_point_light_ctx.count = 0;
    g_point_light_ctx.initialized = true;

    return 0;
}

void lighting_point_light_shutdown(void) {
    if (!g_point_light_ctx.initialized) {
        return;
    }

    free(g_point_light_ctx.items);
    free(g_point_light_ctx.free_indices);
    g_point_light_ctx.items = NULL;
    g_point_light_ctx.free_indices = NULL;
    g_point_light_ctx.count = 0;
    g_point_light_ctx.capacity = 0;
    g_point_light_ctx.initialized = false;
}

int lighting_point_light_create(lighting_point_light_handle_t* out_handle, const lighting_point_light_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_point_light_ctx.initialized) {
        // Auto-init if needed
        if (lighting_point_light_init() != 0) {
            return -2;
        }
    }

    uint32_t index;
    if (g_point_light_ctx.free_count > 0) {
        index = g_point_light_ctx.free_indices[--g_point_light_ctx.free_count];
    } else {
        if (g_point_light_ctx.count >= g_point_light_ctx.capacity) {
            // Resize logic could go here, but for now fixed cap
            // Or just return error
            if (g_point_light_ctx.count >= LIGHTING_POINT_LIGHT_MAX_COUNT) {
                 return -3;
            }
            // Expand capacity logic would be here
            // For now assume we stay within initial capacity until sophisticated resize is added
            if (g_point_light_ctx.count >= g_point_light_ctx.capacity) {
                 // Realloc
                 uint32_t new_cap = g_point_light_ctx.capacity * 2;
                 void* new_items = realloc(g_point_light_ctx.items, new_cap * sizeof(lighting_point_light_internal_t));
                 void* new_free = realloc(g_point_light_ctx.free_indices, new_cap * sizeof(uint32_t));
                 
                 if (!new_items || !new_free) {
                     // Alloc failed
                     return -4;
                 }
                 
                 // Clear new memory
                 memset((char*)new_items + (g_point_light_ctx.capacity * sizeof(lighting_point_light_internal_t)), 
                        0, 
                        (new_cap - g_point_light_ctx.capacity) * sizeof(lighting_point_light_internal_t));
                        
                 g_point_light_ctx.items = new_items;
                 g_point_light_ctx.free_indices = new_free;
                 g_point_light_ctx.capacity = new_cap;
            }
        }
        index = g_point_light_ctx.count++;
    }

    lighting_point_light_internal_t* item = &g_point_light_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    // Copy params
    item->light_data.position = desc->params.position;
    item->light_data.color = desc->params.color;
    item->light_data.intensity = desc->params.intensity;
    item->light_data.radius = desc->params.radius;
    item->light_data.inverse_radius_sq = (desc->params.radius > EPSILON) ? (1.0f / (desc->params.radius * desc->params.radius)) : 0.0f;
    item->light_data.falloff_exponent = desc->params.falloff_exponent;
    item->light_data.cast_shadows = desc->params.cast_shadows;
    item->light_data.shadow_bias = desc->params.shadow_bias;
    item->light_data.shadow_map_index = 0xFFFFFFFF; // Invalid by default
    item->light_data.active = true;
    item->light_data.flags = desc->flags;

    out_handle->id = index;
    return 0;
}

void lighting_point_light_destroy(lighting_point_light_handle_t handle) {
    if (handle.id >= g_point_light_ctx.count && handle.id >= g_point_light_ctx.capacity) {
        return;
    }
    
    // Bounds check against capacity/items array validity
    // Since we use free list, higher IDs might exist
    
    lighting_point_light_internal_t* item = &g_point_light_ctx.items[handle.id];
    if (item->initialized) {
        lighting_point_light_cleanup_internal(item);
        
        // Add to free list
        if (g_point_light_ctx.free_count < g_point_light_ctx.capacity) {
            g_point_light_ctx.free_indices[g_point_light_ctx.free_count++] = handle.id;
        }
    }
}

int lighting_point_light_update(lighting_point_light_handle_t handle, const point_light_params_t* params) {
    if (handle.id >= g_point_light_ctx.capacity || !g_point_light_ctx.items[handle.id].initialized) {
        return -1;
    }

    if (!params) {
        return -1;
    }

    lighting_point_light_internal_t* item = &g_point_light_ctx.items[handle.id];
    
    item->light_data.position = params->position;
    item->light_data.color = params->color;
    item->light_data.intensity = params->intensity;
    item->light_data.radius = params->radius;
    item->light_data.inverse_radius_sq = (params->radius > EPSILON) ? (1.0f / (params->radius * params->radius)) : 0.0f;
    item->light_data.falloff_exponent = params->falloff_exponent;
    item->light_data.cast_shadows = params->cast_shadows;
    item->light_data.shadow_bias = params->shadow_bias;
    
    item->dirty = true;
    return 0;
}

bool lighting_point_light_is_valid(lighting_point_light_handle_t handle) {
    if (!g_point_light_ctx.initialized) return false;
    if (handle.id >= g_point_light_ctx.capacity) return false;
    return g_point_light_ctx.items[handle.id].initialized;
}

int lighting_point_light_get_info(lighting_point_light_handle_t handle, lighting_point_light_info_t* out_info) {
    if (!out_info) return -1;
    if (!lighting_point_light_is_valid(handle)) return -2;

    const lighting_point_light_internal_t* item = &g_point_light_ctx.items[handle.id];
    out_info->id = item->id;
    // out_info->flags was removed from header
    out_info->initialized = item->initialized;
    out_info->data = item->light_data;

    return 0;
}

void lighting_point_light_mark_dirty(lighting_point_light_handle_t handle) {
    if (lighting_point_light_is_valid(handle)) {
        g_point_light_ctx.items[handle.id].dirty = true;
    }
}

int lighting_point_light_process_pending(void) {
    if (!g_point_light_ctx.initialized) return 0;

    int processed = 0;
    // Iterate through all potentially active items
    // Since it's an array with holes (free list), we check initialized flag
    // Optimized loop could skip using free list knowledge but simple loop is fine for now
    for (uint32_t i = 0; i < g_point_light_ctx.capacity; i++) {
        lighting_point_light_internal_t* item = &g_point_light_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item: e.g. update derived data, notify light grid, etc.
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

int lighting_point_light_evaluate(
    lighting_point_light_handle_t handle,
    vec3_t position,
    vec3_t* out_color,
    vec3_t* out_direction,
    float* out_attenuation
) {
    if (!out_color || !out_direction || !out_attenuation) return -1;
    if (!lighting_point_light_is_valid(handle)) return -2;

    const point_light_t* light = &g_point_light_ctx.items[handle.id].light_data;
    
    // Vector from position to light
    vec3_t L = vec3_sub(light->position, position);
    float dist_sq = vec3_dot(L, L);
    
    // Normalize L
    float dist = sqrtf(dist_sq);
    if (dist > EPSILON) {
        float inv_dist = 1.0f / dist;
        // out_direction is direction to light
        *out_direction = vec3_set(L.x * inv_dist, L.y * inv_dist, L.z * inv_dist);
    } else {
        *out_direction = vec3_set(0, 1, 0); // Default up
    }
    
    // Attenuation
    float att = smooth_attenuation(dist_sq, light->radius * light->radius, light->inverse_radius_sq);
    *out_attenuation = att;
    
    // Final color: color * intensity * attenuation
    float final_intensity = light->intensity * att;
    *out_color = vec3_set(
        light->color.x * final_intensity,
        light->color.y * final_intensity,
        light->color.z * final_intensity
    );
    
    return 0;
}

int lighting_point_light_get_bounds(
    lighting_point_light_handle_t handle,
    vec3_t* out_center,
    float* out_radius
) {
    if (!out_center || !out_radius) return -1;
    if (!lighting_point_light_is_valid(handle)) return -2;
    
    const point_light_t* light = &g_point_light_ctx.items[handle.id].light_data;
    *out_center = light->position;
    *out_radius = light->radius;
    
    return 0;
}

aabb_t lighting_point_light_get_aabb(lighting_point_light_handle_t handle) {
    if (!lighting_point_light_is_valid(handle)) {
        aabb_t empty;
        memset(&empty, 0, sizeof(aabb_t));
        return empty;
    }
    
    const point_light_t* light = &g_point_light_ctx.items[handle.id].light_data;
    vec3_t r = vec3_set(light->radius, light->radius, light->radius);
    
    aabb_t bounds;
    bounds.min = vec3_sub(light->position, r);
    bounds.max = vec3_add(light->position, r);
    return bounds;
}

uint32_t lighting_point_light_get_count(void) {
    if (!g_point_light_ctx.initialized) return 0;
    // Count actual active lights
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < g_point_light_ctx.capacity; i++) {
        if (g_point_light_ctx.items[i].initialized) {
            active_count++;
        }
    }
    return active_count;
}

size_t lighting_point_light_get_memory_usage(void) {
    if (!g_point_light_ctx.initialized) return 0;
    size_t total = sizeof(g_point_light_ctx);
    total += g_point_light_ctx.capacity * sizeof(lighting_point_light_internal_t);
    total += g_point_light_ctx.capacity * sizeof(uint32_t); // free indices
    return total;
}

void lighting_point_light_debug_print(void) {
    if (!g_point_light_ctx.initialized) {
        printf("Point Light System: Not initialized\n");
        return;
    }
    
    printf("Point Light System Status:\n");
    printf("  Capacity: %u\n", g_point_light_ctx.capacity);
    printf("  Active Count: %u\n", lighting_point_light_get_count());
    printf("  Memory Usage: %zu bytes\n", lighting_point_light_get_memory_usage());
}

uint32_t lighting_point_light_write_gpu_data(point_light_gpu_data_t* buffer, uint32_t max_count) {
    if (!g_point_light_ctx.initialized || !buffer) return 0;
    
    uint32_t written = 0;
    for (uint32_t i = 0; i < g_point_light_ctx.capacity && written < max_count; i++) {
        lighting_point_light_internal_t* item = &g_point_light_ctx.items[i];
        if (item->initialized) {
            point_light_t* light = &item->light_data;
            point_light_gpu_data_t* gpu = &buffer[written];
            
            gpu->position[0] = light->position.x;
            gpu->position[1] = light->position.y;
            gpu->position[2] = light->position.z;
            gpu->radius = light->radius;
            
            gpu->color[0] = light->color.x;
            gpu->color[1] = light->color.y;
            gpu->color[2] = light->color.z;
            
            gpu->intensity = light->intensity;
            gpu->inverse_radius_sq = light->inverse_radius_sq;
            gpu->shadow_map_index = light->shadow_map_index;
            
            written++;
        }
    }
    return written;
}

/* End of point_light.c */
