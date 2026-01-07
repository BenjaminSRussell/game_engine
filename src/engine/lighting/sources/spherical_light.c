/*
 * spherical_light.c
 * Spherical area light rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/sources/spherical_light.h"
#include <include/math/math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_SPHERICAL_LIGHT_MAX_COUNT 512
#define LIGHTING_SPHERICAL_LIGHT_DEFAULT_CAPACITY 64
#define LIGHTING_SPHERICAL_LIGHT_ALIGNMENT 16

#ifndef EPSILON
#define EPSILON 0.0001f
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_spherical_light_internal {
    uint32_t id;
    spherical_light_t light_data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint32_t flags;
} lighting_spherical_light_internal_t;

typedef struct lighting_spherical_light_context {
    lighting_spherical_light_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    uint32_t* free_indices;
    uint32_t free_count;
    bool initialized;
} lighting_spherical_light_context_t;

static lighting_spherical_light_context_t g_sphere_light_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void spherical_light_update_derived_data(spherical_light_t* light) {
    light->radius_sq = light->radius * light->radius;
    light->influence_radius_sq = light->influence_radius * light->influence_radius;
    
    if (light->influence_radius > EPSILON) {
        light->inverse_influence_radius_sq = 1.0f / light->influence_radius_sq;
    } else {
        light->inverse_influence_radius_sq = 0.0f;
    }
}

static void lighting_spherical_light_cleanup_internal(lighting_spherical_light_internal_t* item) {
    if (!item) return;
    item->initialized = false;
    item->light_data.active = false;
}

static inline float smooth_attenuation(float distance_sq, float radius_sq, float inverse_radius_sq) {
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

// Helper to solve quadratic equation for ray sphere intersection
// Returns true if hit, out_t is smallest positive t
// Actually we need specific logic for closest point on ray to sphere center
// Closest point on ray L(t) = P + V*t to center C is at t = dot(C-P, V)
static vec3_t closest_point_ray_point(vec3_t ray_origin, vec3_t ray_dir, vec3_t point) {
    vec3_t p_to_point = vec3_sub(point, ray_origin);
    float t = vec3_dot(p_to_point, ray_dir);
    // if t < 0, the closest point is the origin (behind)
    // but for reflection vector we assume it's forward
    if (t < 0.0f) t = 0.0f;
    
    // Result = O + D*t
    return vec3_add(ray_origin, vec3_scale(ray_dir, t));
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_spherical_light_init(void) {
    if (g_sphere_light_ctx.initialized) {
        return 0; // Already initialized
    }

    g_sphere_light_ctx.capacity = LIGHTING_SPHERICAL_LIGHT_DEFAULT_CAPACITY;
    g_sphere_light_ctx.items = calloc(g_sphere_light_ctx.capacity, sizeof(lighting_spherical_light_internal_t));
    if (!g_sphere_light_ctx.items) {
        return -1;
    }
    
    g_sphere_light_ctx.free_indices = calloc(g_sphere_light_ctx.capacity, sizeof(uint32_t));
    if (!g_sphere_light_ctx.free_indices) {
        free(g_sphere_light_ctx.items);
        return -1;
    }

    g_sphere_light_ctx.count = 0;
    g_sphere_light_ctx.free_count = 0;
    g_sphere_light_ctx.initialized = true;

    return 0;
}

void lighting_spherical_light_shutdown(void) {
    if (!g_sphere_light_ctx.initialized) {
        return;
    }

    free(g_sphere_light_ctx.items);
    free(g_sphere_light_ctx.free_indices);
    g_sphere_light_ctx.items = NULL;
    g_sphere_light_ctx.free_indices = NULL;
    g_sphere_light_ctx.count = 0;
    g_sphere_light_ctx.capacity = 0;
    g_sphere_light_ctx.initialized = false;
}

int lighting_spherical_light_create(lighting_spherical_light_handle_t* out_handle, const lighting_spherical_light_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_sphere_light_ctx.initialized) {
        if (lighting_spherical_light_init() != 0) return -2;
    }

    uint32_t index;
    if (g_sphere_light_ctx.free_count > 0) {
        index = g_sphere_light_ctx.free_indices[--g_sphere_light_ctx.free_count];
    } else {
        if (g_sphere_light_ctx.count >= g_sphere_light_ctx.capacity) {
            // Resize logic
            if (g_sphere_light_ctx.count >= LIGHTING_SPHERICAL_LIGHT_MAX_COUNT) {
                return -3;
            }
             // Realloc
             uint32_t new_cap = g_sphere_light_ctx.capacity * 2;
             void* new_items = realloc(g_sphere_light_ctx.items, new_cap * sizeof(lighting_spherical_light_internal_t));
             void* new_free = realloc(g_sphere_light_ctx.free_indices, new_cap * sizeof(uint32_t));
             
             if (!new_items || !new_free) {
                 return -4;
             }
             
             // Clear new memory
             memset((char*)new_items + (g_sphere_light_ctx.capacity * sizeof(lighting_spherical_light_internal_t)), 
                    0, 
                    (new_cap - g_sphere_light_ctx.capacity) * sizeof(lighting_spherical_light_internal_t));
                    
             g_sphere_light_ctx.items = new_items;
             g_sphere_light_ctx.free_indices = new_free;
             g_sphere_light_ctx.capacity = new_cap;
        }
        index = g_sphere_light_ctx.count++;
    }

    lighting_spherical_light_internal_t* item = &g_sphere_light_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    item->light_data.position = desc->params.position;
    item->light_data.color = desc->params.color;
    item->light_data.intensity = desc->params.intensity;
    item->light_data.radius = desc->params.radius;
    item->light_data.influence_radius = desc->params.influence_radius;
    item->light_data.falloff_exponent = desc->params.falloff_exponent;
    item->light_data.cast_shadows = desc->params.cast_shadows;
    item->light_data.shadow_bias = desc->params.shadow_bias;
    
    item->light_data.active = true;
    item->light_data.flags = desc->flags;
    
    spherical_light_update_derived_data(&item->light_data);

    out_handle->id = index;
    return 0;
}

void lighting_spherical_light_destroy(lighting_spherical_light_handle_t handle) {
    if (handle.id >= g_sphere_light_ctx.count && handle.id >= g_sphere_light_ctx.capacity) {
        return;
    }
    
    lighting_spherical_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    if (item->initialized) {
        lighting_spherical_light_cleanup_internal(item);
        if (g_sphere_light_ctx.free_count < g_sphere_light_ctx.capacity) {
            g_sphere_light_ctx.free_indices[g_sphere_light_ctx.free_count++] = handle.id;
        }
    }
}

int lighting_spherical_light_update(lighting_spherical_light_handle_t handle, const spherical_light_params_t* params) {
    if (handle.id >= g_sphere_light_ctx.capacity || !g_sphere_light_ctx.items[handle.id].initialized) {
        return -1;
    }
    if (!params) return -1;

    lighting_spherical_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    
    item->light_data.position = params->position;
    item->light_data.color = params->color;
    item->light_data.intensity = params->intensity;
    item->light_data.radius = params->radius;
    item->light_data.influence_radius = params->influence_radius;
    item->light_data.falloff_exponent = params->falloff_exponent;
    item->light_data.cast_shadows = params->cast_shadows;
    item->light_data.shadow_bias = params->shadow_bias;
    
    spherical_light_update_derived_data(&item->light_data);
    
    item->dirty = true;
    return 0;
}

bool lighting_spherical_light_is_valid(lighting_spherical_light_handle_t handle) {
    if (!g_sphere_light_ctx.initialized) return false;
    if (handle.id >= g_sphere_light_ctx.capacity) return false;
    return g_sphere_light_ctx.items[handle.id].initialized;
}

int lighting_spherical_light_get_info(lighting_spherical_light_handle_t handle, lighting_spherical_light_info_t* out_info) {
    if (!out_info) return -1;
    if (!lighting_spherical_light_is_valid(handle)) return -2;

    const lighting_spherical_light_internal_t* item = &g_sphere_light_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->initialized = item->initialized;
    out_info->data = item->light_data;

    return 0;
}

void lighting_spherical_light_mark_dirty(lighting_spherical_light_handle_t handle) {
    if (lighting_spherical_light_is_valid(handle)) {
        g_sphere_light_ctx.items[handle.id].dirty = true;
    }
}

int lighting_spherical_light_process_pending(void) {
    if (!g_sphere_light_ctx.initialized) return 0;
    int processed = 0;
    for (uint32_t i = 0; i < g_sphere_light_ctx.capacity; i++) {
        if (g_sphere_light_ctx.items[i].initialized && g_sphere_light_ctx.items[i].dirty) {
            g_sphere_light_ctx.items[i].dirty = false;
            processed++;
        }
    }
    return processed;
}

int lighting_spherical_light_evaluate(
    lighting_spherical_light_handle_t handle,
    vec3_t position,
    vec3_t normal,
    vec3_t view_dir,
    float roughness,
    vec3_t* out_color,
    vec3_t* out_direction, /* To representative point */
    float* out_attenuation
) {
    if (!out_color || !out_direction || !out_attenuation) return -1;
    if (!lighting_spherical_light_is_valid(handle)) return -2;

    const spherical_light_t* light = &g_sphere_light_ctx.items[handle.id].light_data;
    
    // Representative Point Method for Sphere Lights (Karis 2013)
    // 1. Calculate the reflection ray R
    // R = reflect(-V, N) = 2(N.V)N - V (assuming V is from eye to point)
    // Actually reflect takes incident vector, so reflect(-view_dir, normal) if view_dir is P->Eye ?
    // Standard R = reflect(I, N). I is view->P usually.
    // Let's assume view_dir is P->Eye (normalized). Then Incident I = -view_dir.
    vec3_t negated_view = vec3_set(-view_dir.x, -view_dir.y, -view_dir.z);
    
    // reflect(I, N) = I - 2.0 * dot(N, I) * N
    float dotNI = vec3_dot(normal, negated_view);
    vec3_t R = vec3_sub(negated_view, vec3_scale(normal, 2.0f * dotNI));
    R = vec3_normalize(R);
    
    // 2. Find point on ray R closest to sphere center L
    vec3_t center_to_surface = vec3_sub(position, light->position); // Vector C->P? No, L->P
    // We want closes point on ray starting at surface position P, direction R
    // Closest point on line P + t*R to light position C
    // t = dot(C - P, R)
    vec3_t P_to_Center = vec3_sub(light->position, position);
    float t = vec3_dot(P_to_Center, R);
    if (t < 0.0f) t = 0.0f;
    
    vec3_t closest_point = vec3_add(position, vec3_scale(R, t));
    
    // 3. Clamp point to be on sphere
    vec3_t center_to_closest = vec3_sub(closest_point, light->position);
    float dist_to_closest = vec3_length(center_to_closest);
    
    vec3_t representative_point;
    if (dist_to_closest <= light->radius) {
        // Ray passes through sphere, or closest point is inside
        representative_point = closest_point;
    } else {
        // Project onto sphere surface
        representative_point = vec3_add(light->position, vec3_scale(center_to_closest, light->radius / dist_to_closest));
    }
    
    // 4. Calculate lighting using this representative point as a point light
    vec3_t L = vec3_sub(representative_point, position);
    float dist_sq = vec3_dot(L, L);
    float dist = sqrtf(dist_sq);
    
    if (dist > EPSILON) {
        *out_direction = vec3_scale(L, 1.0f / dist);
    } else {
        *out_direction = normal; 
    }
    
    // Modification for energy conservation with roughness according to Karis
    // (Omitted for simplicity here, but would adjust dist/radius based on roughness)
    
    // Attenuation
    // Use center of sphere for distance attenuation to avoid singularity at surface
    vec3_t center_L = vec3_sub(light->position, position);
    float center_dist_sq = vec3_dot(center_L, center_L);
    
    float att = smooth_attenuation(center_dist_sq, light->influence_radius_sq, light->inverse_influence_radius_sq);
    
    *out_attenuation = att;
    
    // Normalization factor for area light (optional/model dependent)
    // The larger the sphere, the spread out the energy? Usually intensity is defined as total standard or flux
    
    *out_color = vec3_scale(light->color, light->intensity * att);
    
    return 0;
}

aabb_t lighting_spherical_light_get_aabb(lighting_spherical_light_handle_t handle) {
    if (!lighting_spherical_light_is_valid(handle)) {
        aabb_t empty;
        memset(&empty, 0, sizeof(aabb_t));
        return empty;
    }
    
    const spherical_light_t* light = &g_sphere_light_ctx.items[handle.id].light_data;
    float r = light->influence_radius; // Use influence radius for culling bounds
    vec3_t rv = vec3_set(r, r, r);
    
    aabb_t bounds;
    bounds.min = vec3_sub(light->position, rv);
    bounds.max = vec3_add(light->position, rv);
    return bounds;
}

uint32_t lighting_spherical_light_get_count(void) {
    if (!g_sphere_light_ctx.initialized) return 0;
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < g_sphere_light_ctx.capacity; i++) {
        if (g_sphere_light_ctx.items[i].initialized) active_count++;
    }
    return active_count;
}

size_t lighting_spherical_light_get_memory_usage(void) {
    if (!g_sphere_light_ctx.initialized) return 0;
    size_t total = sizeof(g_sphere_light_ctx);
    total += g_sphere_light_ctx.capacity * sizeof(lighting_spherical_light_internal_t);
    total += g_sphere_light_ctx.capacity * sizeof(uint32_t);
    return total;
}

void lighting_spherical_light_debug_print(void) {
    if (!g_sphere_light_ctx.initialized) {
        printf("Spherical Light System: Not initialized\n");
        return;
    }
    
    printf("Spherical Light System Status:\n");
    printf("  Capacity: %u\n", g_sphere_light_ctx.capacity);
    printf("  Active Count: %u\n", lighting_spherical_light_get_count());
    printf("  Memory Usage: %zu bytes\n", lighting_spherical_light_get_memory_usage());
}

uint32_t lighting_spherical_light_write_gpu_data(spherical_light_gpu_data_t* buffer, uint32_t max_count) {
    if (!g_sphere_light_ctx.initialized || !buffer) return 0;
    
    uint32_t written = 0;
    for (uint32_t i = 0; i < g_sphere_light_ctx.capacity && written < max_count; i++) {
        lighting_spherical_light_internal_t* item = &g_sphere_light_ctx.items[i];
        if (item->initialized) {
            spherical_light_t* light = &item->light_data;
            spherical_light_gpu_data_t* gpu = &buffer[written];
            
            gpu->position[0] = light->position.x;
            gpu->position[1] = light->position.y;
            gpu->position[2] = light->position.z;
            gpu->radius = light->radius;
            
            gpu->color[0] = light->color.x;
            gpu->color[1] = light->color.y;
            gpu->color[2] = light->color.z;
            gpu->intensity = light->intensity;
            
            gpu->influence_radius = light->influence_radius;
            gpu->inverse_influence_radius_sq = light->inverse_influence_radius_sq;
            
            written++;
        }
    }
    return written;
}

/* End of spherical_light.c */
