/*
 * spot_light.c
 * Spotlight rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "spot_light.h"
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

#define LIGHTING_SPOT_LIGHT_MAX_COUNT 2048
#define LIGHTING_SPOT_LIGHT_DEFAULT_CAPACITY 128
#define LIGHTING_SPOT_LIGHT_ALIGNMENT 16

#ifndef EPSILON
#define EPSILON 0.0001f
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_spot_light_internal {
    uint32_t id;
    spot_light_t light_data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint32_t flags;
} lighting_spot_light_internal_t;

typedef struct lighting_spot_light_context {
    lighting_spot_light_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    uint32_t* free_indices;
    uint32_t free_count;
    bool initialized;
} lighting_spot_light_context_t;

static lighting_spot_light_context_t g_spot_light_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static inline float smooth_distance_attenuation(float distance_sq, float radius_sq, float inverse_radius_sq) {
    if (distance_sq >= radius_sq) {
        return 0.0f;
    }
    
    float distance = sqrtf(distance_sq);
    float x = distance * sqrtf(inverse_radius_sq);
    float x2 = x * x;
    float x4 = x2 * x2;
    
    float window = 1.0f - x4;
    window = window < 0.0f ? 0.0f : window;
    window = window * window;
    
    return window / (distance_sq + 1.0f);
}

static inline float spot_cone_attenuation(float dot_dir, float cone_scale, float cone_offset) {
    // scale * dot + offset => maps [outer, inner] to [0, 1]
    float att = dot_dir * cone_scale + cone_offset;
    // Clamp to [0, 1]
    if (att < 0.0f) att = 0.0f;
    if (att > 1.0f) att = 1.0f;
    // Square falloff for smoother edge
    return att * att;
}

static void spot_light_update_derived_data(spot_light_t* light) {
    light->inverse_radius_sq = (light->radius > EPSILON) ? (1.0f / (light->radius * light->radius)) : 0.0f;
    
    // Clamp angles to reasonable range (0 to PI)
    // Ensure inner <= outer
    if (light->inner_cone_angle > light->outer_cone_angle) {
        light->inner_cone_angle = light->outer_cone_angle;
    }
    
    light->inner_cone_cos = cosf(light->inner_cone_angle);
    light->outer_cone_cos = cosf(light->outer_cone_angle);
    
    // Precompute scale and offset for shader/evaluation
    float cos_diff = light->inner_cone_cos - light->outer_cone_cos;
    if (cos_diff < EPSILON) {
        // Hard edge case
        light->cone_scale = 1.0f; // Won't be used effectively or will be inf
        light->cone_offset = -light->outer_cone_cos; // This logic needs robustness for 0 diff
        // If equal, avoid div by zero
        light->cone_scale = 100000.0f; // Very sharp transition
    } else {
        light->cone_scale = 1.0f / cos_diff;
        light->cone_offset = -light->outer_cone_cos * light->cone_scale;
    }
}

static void lighting_spot_light_cleanup_internal(lighting_spot_light_internal_t* item) {
    if (!item) return;
    item->initialized = false;
    item->light_data.active = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_spot_light_init(void) {
    if (g_spot_light_ctx.initialized) {
        return 0; // Already initialized
    }

    g_spot_light_ctx.capacity = LIGHTING_SPOT_LIGHT_DEFAULT_CAPACITY;
    g_spot_light_ctx.items = calloc(g_spot_light_ctx.capacity, sizeof(lighting_spot_light_internal_t));
    if (!g_spot_light_ctx.items) {
        return -1;
    }
    
    g_spot_light_ctx.free_indices = calloc(g_spot_light_ctx.capacity, sizeof(uint32_t));
    if (!g_spot_light_ctx.free_indices) {
        free(g_spot_light_ctx.items);
        return -1;
    }

    g_spot_light_ctx.count = 0;
    g_spot_light_ctx.free_count = 0;
    g_spot_light_ctx.initialized = true;

    return 0;
}

void lighting_spot_light_shutdown(void) {
    if (!g_spot_light_ctx.initialized) {
        return;
    }

    free(g_spot_light_ctx.items);
    free(g_spot_light_ctx.free_indices);
    g_spot_light_ctx.items = NULL;
    g_spot_light_ctx.free_indices = NULL;
    g_spot_light_ctx.count = 0;
    g_spot_light_ctx.capacity = 0;
    g_spot_light_ctx.initialized = false;
}

int lighting_spot_light_create(lighting_spot_light_handle_t* out_handle, const lighting_spot_light_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_spot_light_ctx.initialized) {
        if (lighting_spot_light_init() != 0) return -2;
    }

    uint32_t index;
    if (g_spot_light_ctx.free_count > 0) {
        index = g_spot_light_ctx.free_indices[--g_spot_light_ctx.free_count];
    } else {
        if (g_spot_light_ctx.count >= g_spot_light_ctx.capacity) {
            // Resize logic omitted for brevity, same as point light
            if (g_spot_light_ctx.count >= LIGHTING_SPOT_LIGHT_MAX_COUNT) {
                return -3;
            }
             // Realloc
             uint32_t new_cap = g_spot_light_ctx.capacity * 2;
             void* new_items = realloc(g_spot_light_ctx.items, new_cap * sizeof(lighting_spot_light_internal_t));
             void* new_free = realloc(g_spot_light_ctx.free_indices, new_cap * sizeof(uint32_t));
             
             if (!new_items || !new_free) {
                 return -4;
             }
             
             // Clear new memory
             memset((char*)new_items + (g_spot_light_ctx.capacity * sizeof(lighting_spot_light_internal_t)), 
                    0, 
                    (new_cap - g_spot_light_ctx.capacity) * sizeof(lighting_spot_light_internal_t));
                    
             g_spot_light_ctx.items = new_items;
             g_spot_light_ctx.free_indices = new_free;
             g_spot_light_ctx.capacity = new_cap;
        }
        index = g_spot_light_ctx.count++;
    }

    lighting_spot_light_internal_t* item = &g_spot_light_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    item->light_data.position = desc->params.position;
    item->light_data.direction = vec3_normalize(desc->params.direction);
    item->light_data.color = desc->params.color;
    item->light_data.intensity = desc->params.intensity;
    item->light_data.radius = desc->params.radius;
    item->light_data.falloff_exponent = desc->params.falloff_exponent;
    item->light_data.inner_cone_angle = desc->params.inner_cone_angle;
    item->light_data.outer_cone_angle = desc->params.outer_cone_angle;
    item->light_data.cast_shadows = desc->params.cast_shadows;
    item->light_data.shadow_bias = desc->params.shadow_bias;
    item->light_data.shadow_map_index = 0xFFFFFFFF;
    item->light_data.active = true;
    item->light_data.flags = desc->flags;
    
    spot_light_update_derived_data(&item->light_data);

    out_handle->id = index;
    return 0;
}

void lighting_spot_light_destroy(lighting_spot_light_handle_t handle) {
    if (handle.id >= g_spot_light_ctx.count && handle.id >= g_spot_light_ctx.capacity) {
        return;
    }
    
    lighting_spot_light_internal_t* item = &g_spot_light_ctx.items[handle.id];
    if (item->initialized) {
        lighting_spot_light_cleanup_internal(item);
        if (g_spot_light_ctx.free_count < g_spot_light_ctx.capacity) {
            g_spot_light_ctx.free_indices[g_spot_light_ctx.free_count++] = handle.id;
        }
    }
}

int lighting_spot_light_update(lighting_spot_light_handle_t handle, const spot_light_params_t* params) {
    if (handle.id >= g_spot_light_ctx.capacity || !g_spot_light_ctx.items[handle.id].initialized) {
        return -1;
    }
    if (!params) return -1;

    lighting_spot_light_internal_t* item = &g_spot_light_ctx.items[handle.id];
    
    item->light_data.position = params->position;
    item->light_data.direction = vec3_normalize(params->direction);
    item->light_data.color = params->color;
    item->light_data.intensity = params->intensity;
    item->light_data.radius = params->radius;
    item->light_data.falloff_exponent = params->falloff_exponent;
    item->light_data.inner_cone_angle = params->inner_cone_angle;
    item->light_data.outer_cone_angle = params->outer_cone_angle;
    item->light_data.cast_shadows = params->cast_shadows;
    item->light_data.shadow_bias = params->shadow_bias;
    
    spot_light_update_derived_data(&item->light_data);
    
    item->dirty = true;
    return 0;
}

bool lighting_spot_light_is_valid(lighting_spot_light_handle_t handle) {
    if (!g_spot_light_ctx.initialized) return false;
    if (handle.id >= g_spot_light_ctx.capacity) return false;
    return g_spot_light_ctx.items[handle.id].initialized;
}

int lighting_spot_light_get_info(lighting_spot_light_handle_t handle, lighting_spot_light_info_t* out_info) {
    if (!out_info) return -1;
    if (!lighting_spot_light_is_valid(handle)) return -2;

    const lighting_spot_light_internal_t* item = &g_spot_light_ctx.items[handle.id];
    out_info->id = item->id;
    // out_info->flags = item->flags; // Flags removed from info API
    out_info->initialized = item->initialized;
    out_info->data = item->light_data;

    return 0;
}

void lighting_spot_light_mark_dirty(lighting_spot_light_handle_t handle) {
    if (lighting_spot_light_is_valid(handle)) {
        g_spot_light_ctx.items[handle.id].dirty = true;
    }
}

int lighting_spot_light_process_pending(void) {
    if (!g_spot_light_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_spot_light_ctx.capacity; i++) {
        lighting_spot_light_internal_t* item = &g_spot_light_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

int lighting_spot_light_evaluate(
    lighting_spot_light_handle_t handle,
    vec3_t position,
    vec3_t* out_color,
    vec3_t* out_direction,
    float* out_attenuation
) {
    if (!out_color || !out_direction || !out_attenuation) return -1;
    if (!lighting_spot_light_is_valid(handle)) return -2;

    const spot_light_t* light = &g_spot_light_ctx.items[handle.id].light_data;
    
    vec3_t L = vec3_sub(light->position, position);
    float dist_sq = vec3_dot(L, L);
    float dist = sqrtf(dist_sq);
    
    if (dist > EPSILON) {
        float inv_dist = 1.0f / dist;
        *out_direction = vec3_set(L.x * inv_dist, L.y * inv_dist, L.z * inv_dist);
    } else {
        *out_direction = vec3_set(0, 1, 0);
    }
    
    // Distance attenuation
    float dist_att = smooth_distance_attenuation(dist_sq, light->radius * light->radius, light->inverse_radius_sq);
    
    // Cone attenuation
    // L normalized points FROM point on surface TO light
    // Spot direction points FROM light
    // We need dot(-L, direction) or dot(P - LightPos, direction) if L is normalized
    // Actually standard is dot( -L_direction, SpotDirection )
    // out_direction = L_unified (surface to light)
    // So we want dot( -out_direction, light->direction )
    vec3_t negated_dir = vec3_set(-out_direction->x, -out_direction->y, -out_direction->z);
    float cos_angle = vec3_dot(negated_dir, light->direction);
    
    float cone_att = spot_cone_attenuation(cos_angle, light->cone_scale, light->cone_offset);
    
    float total_att = dist_att * cone_att;
    *out_attenuation = total_att;
    
    float final_intensity = light->intensity * total_att;
    *out_color = vec3_set(
        light->color.x * final_intensity,
        light->color.y * final_intensity,
        light->color.z * final_intensity
    );
    
    return 0;
}

int lighting_spot_light_get_bounds(
    lighting_spot_light_handle_t handle,
    vec3_t* out_center,
    float* out_radius
) {
    if (!out_center || !out_radius) return -1;
    if (!lighting_spot_light_is_valid(handle)) return -2;
    
    // Simple sphere bound - same as point light
    // Could tighten this to a cone-sphere bound, but sphere is safer/cheaper for now
    const spot_light_t* light = &g_spot_light_ctx.items[handle.id].light_data;
    *out_center = light->position;
    *out_radius = light->radius;
    
    return 0;
}

aabb_t lighting_spot_light_get_aabb(lighting_spot_light_handle_t handle) {
    if (!lighting_spot_light_is_valid(handle)) {
        aabb_t empty;
        memset(&empty, 0, sizeof(aabb_t));
        return empty;
    }
    
    const spot_light_t* light = &g_spot_light_ctx.items[handle.id].light_data;
    vec3_t r = vec3_set(light->radius, light->radius, light->radius);
    
    aabb_t bounds;
    bounds.min = vec3_sub(light->position, r);
    bounds.max = vec3_add(light->position, r);
    return bounds;
}

uint32_t lighting_spot_light_get_count(void) {
    if (!g_spot_light_ctx.initialized) return 0;
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < g_spot_light_ctx.capacity; i++) {
        if (g_spot_light_ctx.items[i].initialized) active_count++;
    }
    return active_count;
}

size_t lighting_spot_light_get_memory_usage(void) {
    if (!g_spot_light_ctx.initialized) return 0;
    size_t total = sizeof(g_spot_light_ctx);
    total += g_spot_light_ctx.capacity * sizeof(lighting_spot_light_internal_t);
    total += g_spot_light_ctx.capacity * sizeof(uint32_t);
    return total;
}

void lighting_spot_light_debug_print(void) {
    if (!g_spot_light_ctx.initialized) {
        printf("Spot Light System: Not initialized\n");
        return;
    }
    
    printf("Spot Light System Status:\n");
    printf("  Capacity: %u\n", g_spot_light_ctx.capacity);
    printf("  Active Count: %u\n", lighting_spot_light_get_count());
    printf("  Memory Usage: %zu bytes\n", lighting_spot_light_get_memory_usage());
}

uint32_t lighting_spot_light_write_gpu_data(spot_light_gpu_data_t* buffer, uint32_t max_count) {
    if (!g_spot_light_ctx.initialized || !buffer) return 0;
    
    uint32_t written = 0;
    for (uint32_t i = 0; i < g_spot_light_ctx.capacity && written < max_count; i++) {
        lighting_spot_light_internal_t* item = &g_spot_light_ctx.items[i];
        if (item->initialized) {
            spot_light_t* light = &item->light_data;
            spot_light_gpu_data_t* gpu = &buffer[written];
            
            gpu->position[0] = light->position.x;
            gpu->position[1] = light->position.y;
            gpu->position[2] = light->position.z;
            gpu->radius = light->radius;
            
            gpu->direction[0] = light->direction.x;
            gpu->direction[1] = light->direction.y;
            gpu->direction[2] = light->direction.z;
            gpu->intensity = light->intensity;
            
            gpu->color[0] = light->color.x;
            gpu->color[1] = light->color.y;
            gpu->color[2] = light->color.z;
            gpu->inverse_radius_sq = light->inverse_radius_sq;
            
            gpu->cone_scale = light->cone_scale;
            gpu->cone_offset = light->cone_offset;
            gpu->shadow_map_index = light->shadow_map_index;
            gpu->padding = 0.0f;
            
            written++;
        }
    }
    return written;
}

/* End of spot_light.c */
