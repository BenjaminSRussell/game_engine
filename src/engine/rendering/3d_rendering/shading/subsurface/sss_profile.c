/*
 * sss_profile.c
 * Subsurface scatter profile implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "sss_profile.h"
#include "../../math/vec3.h"
#include "../../../include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_SSS_PROFILE_MAX_COUNT 64
#define SHADING_SSS_PROFILE_DEFAULT_CAPACITY 16
#define MAX_SSS_SAMPLES 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct sss_profile_data {
    vec3_t scatter_distance;    // RGB falloff distances in mm
    vec3_t scatter_color;       // Albedo of the scattering
    float scale;                // Global scale factor for scattering
    float ior;                  // Index of refraction
    
    // Precomputed kernel for separability
    float kernel[MAX_SSS_SAMPLES];
    vec3_t kernel_weights[MAX_SSS_SAMPLES];
    uint32_t sample_count;
} sss_profile_data_t;

typedef struct shading_sss_profile_internal {
    uint32_t id;
    uint32_t flags;
    sss_profile_data_t data;
    bool initialized;
    bool dirty;
} shading_sss_profile_internal_t;

typedef struct shading_sss_profile_context {
    shading_sss_profile_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_sss_profile_context_t;

static shading_sss_profile_context_t g_sss_profile_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Burley's normalized diffusion profile
// R(r) = (e^(-r/d) + e^(-r/(3d))) / (8*pi*d*r)
static float burley_profile(float r, float d) {
    if (d < EPSILON) return 0.0f;
    float r_d = r / d;
    return (expf(-r_d) + expf(-r_d / 3.0f)) / (8.0f * PI * d * r);
}

// Gaussian profile for separable approximation
// G(r, v) = (1 / (2*pi*v)) * e^(-r^2 / (2*v))
static float gaussian_profile(float r, float v) {
    if (v < EPSILON) return 0.0f;
    return (1.0f / (2.0f * PI * v)) * expf(-(r * r) / (2.0f * v));
}

static void compute_sss_kernel(shading_sss_profile_internal_t* item) {
    // Determine number of samples based on quality settings (flags)
    // For now, use a fixed high quality sample count
    item->data.sample_count = 11; // Must be odd for separable kernel
    
    // Calculate variance based on scatter distance
    // This is a simplified Gaussian approximation of the BSSRDF
    // Real implementation would precompute weighted sum of Gaussians
    
    float w_sum_r = 0.0f, w_sum_g = 0.0f, w_sum_b = 0.0f;
    
    int center = item->data.sample_count / 2;
    for (uint32_t i = 0; i < item->data.sample_count; i++) {
        float x = (float)i - center;
        // Spatially varying width based on scatter distance
        float distance = fabsf(x * item->data.scale); 
        
        // Compute weights per channel
        // Using simplified falloff for demonstration
        vec3_t weight;
        weight.x = gaussian_profile(distance, item->data.scatter_distance.x);
        weight.y = gaussian_profile(distance, item->data.scatter_distance.y);
        weight.z = gaussian_profile(distance, item->data.scatter_distance.z);
        
        item->data.kernel_weights[i] = weight;
        
        w_sum_r += weight.x;
        w_sum_g += weight.y;
        w_sum_b += weight.z;
    }
    
    // Normalize weights
    for (uint32_t i = 0; i < item->data.sample_count; i++) {
        item->data.kernel_weights[i].x /= w_sum_r;
        item->data.kernel_weights[i].y /= w_sum_g;
        item->data.kernel_weights[i].z /= w_sum_b;
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_sss_profile_init(void) {
    if (g_sss_profile_ctx.initialized) return 0;

    g_sss_profile_ctx.capacity = SHADING_SSS_PROFILE_DEFAULT_CAPACITY;
    g_sss_profile_ctx.items = calloc(g_sss_profile_ctx.capacity, sizeof(shading_sss_profile_internal_t));
    
    if (!g_sss_profile_ctx.items) return -1;
    
    g_sss_profile_ctx.count = 0;
    g_sss_profile_ctx.initialized = true;
    
    return 0;
}

void shading_sss_profile_shutdown(void) {
    if (!g_sss_profile_ctx.initialized) return;
    
    free(g_sss_profile_ctx.items);
    g_sss_profile_ctx.items = NULL;
    g_sss_profile_ctx.count = 0;
    g_sss_profile_ctx.capacity = 0;
    g_sss_profile_ctx.initialized = false;
}

int shading_sss_profile_create(shading_sss_profile_handle_t* out_handle, const shading_sss_profile_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_sss_profile_ctx.initialized) return -2;
    
    if (g_sss_profile_ctx.count >= g_sss_profile_ctx.capacity) {
        // Simple expansion
        uint32_t new_capacity = g_sss_profile_ctx.capacity * 2;
        if (new_capacity > SHADING_SSS_PROFILE_MAX_COUNT) new_capacity = SHADING_SSS_PROFILE_MAX_COUNT;
        
        if (new_capacity == g_sss_profile_ctx.capacity) return -3;
        
        void* new_items = realloc(g_sss_profile_ctx.items, new_capacity * sizeof(shading_sss_profile_internal_t));
        if (!new_items) return -4;
        
        g_sss_profile_ctx.items = new_items;
        g_sss_profile_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_sss_profile_ctx.count++;
    shading_sss_profile_internal_t* item = &g_sss_profile_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default skin values
    item->data.scatter_distance = vec3_set(1.0f, 0.5f, 0.25f); // Red scatter further
    item->data.scatter_color = vec3_set(0.44f, 0.22f, 0.13f);
    item->data.scale = 1.0f;
    item->data.ior = 1.4f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_sss_profile_destroy(shading_sss_profile_handle_t handle) {
    if (handle.id >= g_sss_profile_ctx.count) return;
    
    // In a real system with slots, we'd mark as free or swap-remove
    // For this simple implementation, just uninitialize
    g_sss_profile_ctx.items[handle.id].initialized = false;
}

int shading_sss_profile_update(shading_sss_profile_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_sss_profile_ctx.count) return -1;
    
    shading_sss_profile_internal_t* item = &g_sss_profile_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    // Expect data to be sss_profile_data_t compatible struct
    // In a real engine, we'd have a safe casting mechanism
    if (size == sizeof(vec3_t)) {
        // Just updating distance
        memcpy(&item->data.scatter_distance, data, sizeof(vec3_t));
    } else if (size >= sizeof(float) && size <= sizeof(float)*2) {
        // Just updating scale
        memcpy(&item->data.scale, data, sizeof(float));
    }
    
    item->dirty = true;
    return 0;
}

bool shading_sss_profile_is_valid(shading_sss_profile_handle_t handle) {
    if (handle.id >= g_sss_profile_ctx.count) return false;
    return g_sss_profile_ctx.items[handle.id].initialized;
}

int shading_sss_profile_get_info(shading_sss_profile_handle_t handle, shading_sss_profile_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_sss_profile_ctx.count) return -2;
    
    const shading_sss_profile_internal_t* item = &g_sss_profile_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return 0;
}

void shading_sss_profile_mark_dirty(shading_sss_profile_handle_t handle) {
    if (handle.id < g_sss_profile_ctx.count) {
        g_sss_profile_ctx.items[handle.id].dirty = true;
    }
}

int shading_sss_profile_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_sss_profile_ctx.count; i++) {
        shading_sss_profile_internal_t* item = &g_sss_profile_ctx.items[i];
        if (item->initialized && item->dirty) {
            compute_sss_kernel(item);
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t shading_sss_profile_get_count(void) {
    return g_sss_profile_ctx.count;
}

size_t shading_sss_profile_get_memory_usage(void) {
    size_t total = sizeof(g_sss_profile_ctx);
    total += g_sss_profile_ctx.capacity * sizeof(shading_sss_profile_internal_t);
    return total;
}

void shading_sss_profile_debug_print(void) {
    // Debug implementation
}
