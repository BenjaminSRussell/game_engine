/*
 * sphere_light.c
 * Sphere/point light source implementation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "sphere_light.h"
#include "include/math/vec3.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SPHERE_LIGHT_DEFAULT_CAPACITY 256
#define MAX_SPHERE_LIGHTS 4096

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct sphere_light_data {
    vec3_t position;
    float radius;
    vec3_t color;
    float intensity;
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
    bool cast_shadows;
    uint32_t shadow_map_index;
} sphere_light_data_t;

typedef struct lighting_sphere_light_internal {
    uint32_t id;
    uint32_t flags;
    sphere_light_data_t data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} lighting_sphere_light_internal_t;

typedef struct lighting_sphere_light_context {
    lighting_sphere_light_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} lighting_sphere_light_context_t;

static lighting_sphere_light_context_t g_sphere_light_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline float calculate_attenuation(const sphere_light_data_t* light, float distance) {
    float denom = light->attenuation_constant +
                  light->attenuation_linear * distance +
                  light->attenuation_quadratic * distance * distance;
    return 1.0f / fmaxf(denom, 0.0001f);
}

static inline float calculate_light_radius(const sphere_light_data_t* light, float threshold) {
    // Calculate radius where attenuation drops below threshold
    // Solve: threshold = intensity / (c + l*r + q*r^2)
    float a = light->attenuation_quadratic;
    float b = light->attenuation_linear;
    float c = light->attenuation_constant - (light->intensity / threshold);
    
    if (a < 1e-6f) {
        // Linear attenuation
        return (light->intensity / threshold - c) / b;
    }
    
    // Quadratic formula
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return 100.0f;  // Default large radius
    }
    
    return (-b + sqrtf(discriminant)) / (2.0f * a);
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool lighting_sphere_light_validate(const lighting_sphere_light_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void lighting_sphere_light_cleanup_internal(lighting_sphere_light_internal_t* item) {
    if (!item) return;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_sphere_light_init(void) {
    if (g_sphere_light_ctx.initialized) {
        return 0;
    }
    
    g_sphere_light_ctx.capacity = SPHERE_LIGHT_DEFAULT_CAPACITY;
    g_sphere_light_ctx.items = calloc(g_sphere_light_ctx.capacity,
                                      sizeof(lighting_sphere_light_internal_t));
    if (!g_sphere_light_ctx.items) {
        return -1;
    }
    
    g_sphere_light_ctx.count = 0;
    g_sphere_light_ctx.initialized = true;
    
    return 0;
}

void lighting_sphere_light_shutdown(void) {
    if (!g_sphere_light_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_sphere_light_ctx.count; i++) {
        lighting_sphere_light_cleanup_internal(&g_sphere_light_ctx.items[i]);
    }
    
    free(g_sphere_light_ctx.items);
    g_sphere_light_ctx.items = NULL;
    g_sphere_light_ctx.count = 0;
    g_sphere_light_ctx.capacity = 0;
    g_sphere_light_ctx.initialized = false;
}

int lighting_sphere_light_create(lighting_sphere_light_handle_t* out_handle,
                                  const lighting_sphere_light_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_sphere_light_ctx.initialized) {
        return -2;
    }
    
    if (g_sphere_light_ctx.count >= g_sphere_light_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_sphere_light_ctx.count++;
    lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->data.position = desc->position;
    item->data.color = desc->color;
    item->data.intensity = desc->intensity;
    item->data.attenuation_constant = desc->attenuation_constant > 0.0f ? desc->attenuation_constant : 1.0f;
    item->data.attenuation_linear = desc->attenuation_linear;
    item->data.attenuation_quadratic = desc->attenuation_quadratic;
    item->data.cast_shadows = desc->cast_shadows;
    item->data.shadow_map_index = 0;
    
    // Calculate effective radius
    item->data.radius = calculate_light_radius(&item->data, 0.01f);  // 1% threshold
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    out_handle->id = index;
    return 0;
}

void lighting_sphere_light_destroy(lighting_sphere_light_handle_t handle) {
    if (handle.id >= g_sphere_light_ctx.count) {
        return;
    }
    
    lighting_sphere_light_cleanup_internal(&g_sphere_light_ctx.items[handle.id]);
}

int lighting_sphere_light_set_position(lighting_sphere_light_handle_t handle, vec3_t position) {
    if (handle.id >= g_sphere_light_ctx.count) {
        return -1;
    }
    
    lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->data.position = position;
    item->dirty = true;
    
    return 0;
}

int lighting_sphere_light_set_color(lighting_sphere_light_handle_t handle, vec3_t color, float intensity) {
    if (handle.id >= g_sphere_light_ctx.count) {
        return -1;
    }
    
    lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->data.color = color;
    item->data.intensity = intensity;
    item->data.radius = calculate_light_radius(&item->data, 0.01f);
    item->dirty = true;
    
    return 0;
}

int lighting_sphere_light_set_attenuation(lighting_sphere_light_handle_t handle,
                                           float constant, float linear, float quadratic) {
    if (handle.id >= g_sphere_light_ctx.count) {
        return -1;
    }
    
    lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->data.attenuation_constant = constant;
    item->data.attenuation_linear = linear;
    item->data.attenuation_quadratic = quadratic;
    item->data.radius = calculate_light_radius(&item->data, 0.01f);
    item->dirty = true;
    
    return 0;
}

int lighting_sphere_light_get_data(lighting_sphere_light_handle_t handle,
                                    sphere_light_data_t* out_data) {
    if (!out_data) {
        return -1;
    }
    
    if (handle.id >= g_sphere_light_ctx.count) {
        return -2;
    }
    
    const lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }
    
    *out_data = item->data;
    return 0;
}

bool lighting_sphere_light_is_valid(lighting_sphere_light_handle_t handle) {
    if (handle.id >= g_sphere_light_ctx.count) {
        return false;
    }
    return g_sphere_light_ctx.items[handle.id].initialized;
}

int lighting_sphere_light_update(lighting_sphere_light_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_sphere_light_ctx.count) {
        return -1;
    }
    
    lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->dirty = true;
    return 0;
}

void lighting_sphere_light_mark_dirty(lighting_sphere_light_handle_t handle) {
    if (handle.id < g_sphere_light_ctx.count) {
        g_sphere_light_ctx.items[handle.id].dirty = true;
    }
}

int lighting_sphere_light_process_pending(void) {
    int processed = 0;
    
    for (uint32_t i = 0; i < g_sphere_light_ctx.count; i++) {
        lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[i];
        if (item->initialized && item->dirty) {
            // TODO: Upload to GPU light buffer
            item->dirty = false;
            processed++;
        }
    }
    
    return processed;
}

uint32_t lighting_sphere_light_get_count(void) {
    return g_sphere_light_ctx.count;
}

size_t lighting_sphere_light_get_memory_usage(void) {
    return sizeof(g_sphere_light_ctx) +
           g_sphere_light_ctx.capacity * sizeof(lighting_sphere_light_internal_t);
}

void lighting_sphere_light_debug_print(void) {
    printf("[Sphere Lights] Total lights: %u\n", g_sphere_light_ctx.count);
    
    for (uint32_t i = 0; i < g_sphere_light_ctx.count; i++) {
        const lighting_sphere_light_internal_t* item = &g_sphere_light_ctx.items[i];
        if (item->initialized) {
            printf("  Light %u: pos=(%.1f,%.1f,%.1f) radius=%.1f intensity=%.2f\n",
                   i, item->data.position.x, item->data.position.y, item->data.position.z,
                   item->data.radius, item->data.intensity);
        }
    }
}

/* End of sphere_light.c */
