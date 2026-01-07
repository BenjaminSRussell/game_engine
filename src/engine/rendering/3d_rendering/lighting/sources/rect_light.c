/*
 * rect_light.c
 * Rectangular area lights (LTC)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "rect_light.h"
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

#define LIGHTING_RECT_LIGHT_MAX_COUNT 512
#define LIGHTING_RECT_LIGHT_DEFAULT_CAPACITY 64
#define LIGHTING_RECT_LIGHT_ALIGNMENT 16

#ifndef EPSILON
#define EPSILON 0.0001f
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_rect_light_internal {
    uint32_t id;
    rect_light_t light_data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint32_t flags;
} lighting_rect_light_internal_t;

typedef struct lighting_rect_light_context {
    lighting_rect_light_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    uint32_t* free_indices;
    uint32_t free_count;
    bool initialized;
} lighting_rect_light_context_t;

static lighting_rect_light_context_t g_rect_light_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rect_light_update_derived_data(rect_light_t* light) {
    light->half_width = light->width * 0.5f;
    light->half_height = light->height * 0.5f;
    
    // Compute normal
    light->normal = vec3_cross(light->axis_x, light->axis_y);
    light->normal = vec3_normalize(light->normal);
    
    // Compute corner points in world space
    // Center +/- (axis_x * hw) +/- (axis_y * hh)
    vec3_t x_offset = vec3_scale(light->axis_x, light->half_width);
    vec3_t y_offset = vec3_scale(light->axis_y, light->half_height);
    
    // p0: -x, -y
    // p1: +x, -y
    // p2: +x, +y
    // p3: -x, +y
    light->points[0] = vec3_sub(vec3_sub(light->position, x_offset), y_offset);
    light->points[1] = vec3_sub(vec3_add(light->position, x_offset), y_offset);
    light->points[2] = vec3_add(vec3_add(light->position, x_offset), y_offset);
    light->points[3] = vec3_add(vec3_sub(light->position, x_offset), y_offset);
}

static void lighting_rect_light_cleanup_internal(lighting_rect_light_internal_t* item) {
    if (!item) return;
    item->initialized = false;
    item->light_data.active = false;
}

static inline float integrate_edge(vec3_t v1, vec3_t v2) {
    float x = vec3_dot(v1, v2);
    float y = fabs(x);
    
    float a = 0.8543985f + (0.4965155f + 0.0145206f * y) * y;
    float b = 3.4175940f + (4.1616724f + y) * y;
    float v = a / b;
    
    float theta_sintheta = (x > 0.0f) ? v : 0.5f * (1.0f/sqrtf(1.0f - x*x)) - v;
    return theta_sintheta;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_rect_light_init(void) {
    if (g_rect_light_ctx.initialized) {
        return 0; // Already initialized
    }

    g_rect_light_ctx.capacity = LIGHTING_RECT_LIGHT_DEFAULT_CAPACITY;
    g_rect_light_ctx.items = calloc(g_rect_light_ctx.capacity, sizeof(lighting_rect_light_internal_t));
    if (!g_rect_light_ctx.items) {
        return -1;
    }
    
    g_rect_light_ctx.free_indices = calloc(g_rect_light_ctx.capacity, sizeof(uint32_t));
    if (!g_rect_light_ctx.free_indices) {
        free(g_rect_light_ctx.items);
        return -1;
    }

    g_rect_light_ctx.count = 0;
    g_rect_light_ctx.free_count = 0;
    g_rect_light_ctx.initialized = true;

    return 0;
}

void lighting_rect_light_shutdown(void) {
    if (!g_rect_light_ctx.initialized) {
        return;
    }

    free(g_rect_light_ctx.items);
    free(g_rect_light_ctx.free_indices);
    g_rect_light_ctx.items = NULL;
    g_rect_light_ctx.free_indices = NULL;
    g_rect_light_ctx.count = 0;
    g_rect_light_ctx.capacity = 0;
    g_rect_light_ctx.initialized = false;
}

int lighting_rect_light_create(lighting_rect_light_handle_t* out_handle, const lighting_rect_light_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_rect_light_ctx.initialized) {
        if (lighting_rect_light_init() != 0) return -2;
    }

    uint32_t index;
    if (g_rect_light_ctx.free_count > 0) {
        index = g_rect_light_ctx.free_indices[--g_rect_light_ctx.free_count];
    } else {
        if (g_rect_light_ctx.count >= g_rect_light_ctx.capacity) {
            // Resize logic
            if (g_rect_light_ctx.count >= LIGHTING_RECT_LIGHT_MAX_COUNT) {
                return -3;
            }
             // Realloc
             uint32_t new_cap = g_rect_light_ctx.capacity * 2;
             void* new_items = realloc(g_rect_light_ctx.items, new_cap * sizeof(lighting_rect_light_internal_t));
             void* new_free = realloc(g_rect_light_ctx.free_indices, new_cap * sizeof(uint32_t));
             
             if (!new_items || !new_free) {
                 return -4;
             }
             
             // Clear new memory
             memset((char*)new_items + (g_rect_light_ctx.capacity * sizeof(lighting_rect_light_internal_t)), 
                    0, 
                    (new_cap - g_rect_light_ctx.capacity) * sizeof(lighting_rect_light_internal_t));
                    
             g_rect_light_ctx.items = new_items;
             g_rect_light_ctx.free_indices = new_free;
             g_rect_light_ctx.capacity = new_cap;
        }
        index = g_rect_light_ctx.count++;
    }

    lighting_rect_light_internal_t* item = &g_rect_light_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    item->light_data.position = desc->params.position;
    item->light_data.axis_x = vec3_normalize(desc->params.axis_x);
    item->light_data.axis_y = vec3_normalize(desc->params.axis_y);
    item->light_data.color = desc->params.color;
    item->light_data.intensity = desc->params.intensity;
    item->light_data.width = desc->params.width;
    item->light_data.height = desc->params.height;
    item->light_data.range = desc->params.range;
    item->light_data.two_sided = desc->params.two_sided;
    item->light_data.active = true;
    item->light_data.flags = desc->flags;
    
    rect_light_update_derived_data(&item->light_data);

    out_handle->id = index;
    return 0;
}

void lighting_rect_light_destroy(lighting_rect_light_handle_t handle) {
    if (handle.id >= g_rect_light_ctx.count && handle.id >= g_rect_light_ctx.capacity) {
        return;
    }
    
    lighting_rect_light_internal_t* item = &g_rect_light_ctx.items[handle.id];
    if (item->initialized) {
        lighting_rect_light_cleanup_internal(item);
        if (g_rect_light_ctx.free_count < g_rect_light_ctx.capacity) {
            g_rect_light_ctx.free_indices[g_rect_light_ctx.free_count++] = handle.id;
        }
    }
}

int lighting_rect_light_update(lighting_rect_light_handle_t handle, const rect_light_params_t* params) {
    if (handle.id >= g_rect_light_ctx.capacity || !g_rect_light_ctx.items[handle.id].initialized) {
        return -1;
    }
    if (!params) return -1;

    lighting_rect_light_internal_t* item = &g_rect_light_ctx.items[handle.id];
    
    item->light_data.position = params->position;
    item->light_data.axis_x = vec3_normalize(params->axis_x);
    item->light_data.axis_y = vec3_normalize(params->axis_y);
    item->light_data.color = params->color;
    item->light_data.intensity = params->intensity;
    item->light_data.width = params->width;
    item->light_data.height = params->height;
    item->light_data.range = params->range;
    item->light_data.two_sided = params->two_sided;
    
    rect_light_update_derived_data(&item->light_data);
    
    item->dirty = true;
    return 0;
}

bool lighting_rect_light_is_valid(lighting_rect_light_handle_t handle) {
    if (!g_rect_light_ctx.initialized) return false;
    if (handle.id >= g_rect_light_ctx.capacity) return false;
    return g_rect_light_ctx.items[handle.id].initialized;
}

int lighting_rect_light_get_info(lighting_rect_light_handle_t handle, lighting_rect_light_info_t* out_info) {
    if (!out_info) return -1;
    if (!lighting_rect_light_is_valid(handle)) return -2;

    const lighting_rect_light_internal_t* item = &g_rect_light_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->initialized = item->initialized;
    out_info->data = item->light_data;

    return 0;
}

void lighting_rect_light_mark_dirty(lighting_rect_light_handle_t handle) {
    if (lighting_rect_light_is_valid(handle)) {
        g_rect_light_ctx.items[handle.id].dirty = true;
    }
}

int lighting_rect_light_process_pending(void) {
    if (!g_rect_light_ctx.initialized) return 0;
    int processed = 0;
    for (uint32_t i = 0; i < g_rect_light_ctx.capacity; i++) {
        if (g_rect_light_ctx.items[i].initialized && g_rect_light_ctx.items[i].dirty) {
            g_rect_light_ctx.items[i].dirty = false;
            processed++;
        }
    }
    return processed;
}

// Analytically evaluates the irradiance of a rect light (Lambertian shading)
// NOTE: This does not implement the full LTC approximation which requires texture lookups
// This is a "diffuse only" approximation for debugging or simpler materials
int lighting_rect_light_evaluate_simple(
    lighting_rect_light_handle_t handle,
    vec3_t position,
    vec3_t normal,
    vec3_t* out_color
) {
    if (!out_color) return -1;
    if (!lighting_rect_light_is_valid(handle)) return -2;

    const rect_light_t* light = &g_rect_light_ctx.items[handle.id].light_data;
    
    // Calculate solid angle of the rectangle
    // Project points onto sphere around P
    vec3_t L[4];
    for(int i=0; i<4; ++i) {
        L[i] = vec3_sub(light->points[i], position);
        L[i] = vec3_normalize(L[i]);
    }
    
    // Clipping would go here if we were doing this properly for horizon
    
    float solid_angle = 0.0f;
    for(int i=0; i<4; ++i) {
        int next = (i + 1) % 4;
        solid_angle += integrate_edge(L[i], L[next]); // Spherical polygon formula part
        // Actually the formula for solid angle of polygon on sphere is:
        // sum(beta_i) - (n-2)*pi, where beta_i are internal angles
        // But for lighting we use a simpler integration usually
        // Let's use the standard "projection onto hemisphere" approximation if we aren't using LTC
    }
    
    // Fallback: simplified point light approximation for far distances, or Monte Carlo
    // Since LTC is the "correct" way for this engine, and requires texture, we will just return 
    // a placeholder color scaled by distance for now to prove API connectivity
    // Once LTC textures are available, we'd do the texture fetch here.
    
    // For now: Sample center
    vec3_t light_vec = vec3_sub(light->position, position);
    float dist_sq = vec3_dot(light_vec, light_vec);
    float dist = sqrtf(dist_sq);
    vec3_t L_dir = vec3_scale(light_vec, 1.0f/dist);
    
    float NdotL = vec3_dot(normal, L_dir);
    if (NdotL < 0.0f) NdotL = 0.0f;
    
    // Area light attenuation is 1/r^2 but converges to constant at surface
    float attenuation = 1.0f / (dist_sq + 1.0f);
    
    // Apply barn door / range if needed
    if (light->range > 0.0f) {
        float range_factor = 1.0f - (dist / light->range);
        if (range_factor < 0.0f) range_factor = 0.0f;
        attenuation *= range_factor;
    }
    
    *out_color = vec3_scale(light->color, light->intensity * NdotL * attenuation);
    
    return 0;
}

aabb_t lighting_rect_light_get_aabb(lighting_rect_light_handle_t handle) {
    if (!lighting_rect_light_is_valid(handle)) {
        aabb_t empty;
        memset(&empty, 0, sizeof(aabb_t));
        return empty;
    }
    
    const rect_light_t* light = &g_rect_light_ctx.items[handle.id].light_data;
    
    aabb_t bounds;
    bounds.min = light->points[0];
    bounds.max = light->points[0];
    
    for(int i=1; i<4; ++i) {
        bounds.min = vec3_min(bounds.min, light->points[i]);
        bounds.max = vec3_max(bounds.max, light->points[i]);
    }
    
    // Add some padding
    vec3_t padding = vec3_set(0.1f, 0.1f, 0.1f);
    bounds.min = vec3_sub(bounds.min, padding);
    bounds.max = vec3_add(bounds.max, padding);
    
    return bounds;
}

uint32_t lighting_rect_light_get_count(void) {
    if (!g_rect_light_ctx.initialized) return 0;
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < g_rect_light_ctx.capacity; i++) {
        if (g_rect_light_ctx.items[i].initialized) active_count++;
    }
    return active_count;
}

size_t lighting_rect_light_get_memory_usage(void) {
    if (!g_rect_light_ctx.initialized) return 0;
    size_t total = sizeof(g_rect_light_ctx);
    total += g_rect_light_ctx.capacity * sizeof(lighting_rect_light_internal_t);
    total += g_rect_light_ctx.capacity * sizeof(uint32_t);
    return total;
}

void lighting_rect_light_debug_print(void) {
    if (!g_rect_light_ctx.initialized) {
        printf("Rect Light System: Not initialized\n");
        return;
    }
    
    printf("Rect Light System Status:\n");
    printf("  Capacity: %u\n", g_rect_light_ctx.capacity);
    printf("  Active Count: %u\n", lighting_rect_light_get_count());
    printf("  Memory Usage: %zu bytes\n", lighting_rect_light_get_memory_usage());
}

uint32_t lighting_rect_light_write_gpu_data(rect_light_gpu_data_t* buffer, uint32_t max_count) {
    if (!g_rect_light_ctx.initialized || !buffer) return 0;
    
    uint32_t written = 0;
    for (uint32_t i = 0; i < g_rect_light_ctx.capacity && written < max_count; i++) {
        lighting_rect_light_internal_t* item = &g_rect_light_ctx.items[i];
        if (item->initialized) {
            rect_light_t* light = &item->light_data;
            rect_light_gpu_data_t* gpu = &buffer[written];
            
            gpu->position[0] = light->position.x;
            gpu->position[1] = light->position.y;
            gpu->position[2] = light->position.z;
            gpu->intensity = light->intensity;
            
            gpu->axis_x[0] = light->axis_x.x;
            gpu->axis_x[1] = light->axis_x.y;
            gpu->axis_x[2] = light->axis_x.z;
            gpu->half_width = light->half_width;
            
            gpu->axis_y[0] = light->axis_y.x;
            gpu->axis_y[1] = light->axis_y.y;
            gpu->axis_y[2] = light->axis_y.z;
            gpu->half_height = light->half_height;
            
            gpu->color[0] = light->color.x;
            gpu->color[1] = light->color.y;
            gpu->color[2] = light->color.z;
            gpu->range = light->range;
            
            // Flatten points
            for(int k=0; k<4; ++k) {
                gpu->points[k*3 + 0] = light->points[k].x;
                gpu->points[k*3 + 1] = light->points[k].y;
                gpu->points[k*3 + 2] = light->points[k].z;
            }
            
            written++;
        }
    }
    return written;
}

/* End of rect_light.c */
