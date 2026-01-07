/*
 * hair_strand_shading.c
 * Hair strand shading implementation (Marschner Model)
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "shading/hair/hair_strand_shading.h"
#include "include/math/vec3.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_HAIR_STRAND_SHADING_MAX_COUNT 64
#define SHADING_HAIR_STRAND_SHADING_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct hair_params {
    vec3_t base_color;
    float roughness_longitudinal;
    float roughness_azimuthal;
    float shift; // Longitudinal shift for cuticles (usually negative for R, positive for TRT)
    float ior;
} hair_params_t;

typedef struct shading_hair_strand_shading_internal {
    uint32_t id;
    uint32_t flags;
    hair_params_t params;
    bool initialized;
    bool dirty;
} shading_hair_strand_shading_internal_t;

typedef struct shading_hair_strand_shading_context {
    shading_hair_strand_shading_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_hair_strand_shading_context_t;

static shading_hair_strand_shading_context_t g_hair_strand_shading_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Approximate longitudinal scattering (Gaussian M)
static float mp_scattering(float sin_theta_h, float alpha) {
    float sigma_a = alpha; // Width parameter
    return expf(-(sin_theta_h * sin_theta_h) / (2.0f * sigma_a * sigma_a)) / (sqrtf(2.0f * PI) * sigma_a);
}

// Azimuthal scattering (Np) for R lobe
static float np_r(float phi, float eta) {
    // Simplified approximation for R lobe azimuthal distribution
    // In full Marschner, this involves solving cubic equations for ray paths
    // Using a cosine lobe approximation for real-time
    float cos_half_phi = cosf(phi * 0.5f);
    return cos_half_phi * cos_half_phi; // Very rough approx
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_hair_strand_shading_init(void) {
    if (g_hair_strand_shading_ctx.initialized) return 0;

    g_hair_strand_shading_ctx.capacity = SHADING_HAIR_STRAND_SHADING_DEFAULT_CAPACITY;
    g_hair_strand_shading_ctx.items = calloc(g_hair_strand_shading_ctx.capacity, sizeof(shading_hair_strand_shading_internal_t));
    
    if (!g_hair_strand_shading_ctx.items) return -1;
    
    g_hair_strand_shading_ctx.count = 0;
    g_hair_strand_shading_ctx.initialized = true;
    
    return 0;
}

void shading_hair_strand_shading_shutdown(void) {
    if (!g_hair_strand_shading_ctx.initialized) return;
    
    free(g_hair_strand_shading_ctx.items);
    g_hair_strand_shading_ctx.items = NULL;
    g_hair_strand_shading_ctx.count = 0;
    g_hair_strand_shading_ctx.capacity = 0;
    g_hair_strand_shading_ctx.initialized = false;
}

int shading_hair_strand_shading_create(shading_hair_strand_shading_handle_t* out_handle, const shading_hair_strand_shading_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_hair_strand_shading_ctx.initialized) return -2;
    
    if (g_hair_strand_shading_ctx.count >= g_hair_strand_shading_ctx.capacity) {
        uint32_t new_capacity = g_hair_strand_shading_ctx.capacity * 2;
        if (new_capacity > SHADING_HAIR_STRAND_SHADING_MAX_COUNT) new_capacity = SHADING_HAIR_STRAND_SHADING_MAX_COUNT;
        
        if (new_capacity == g_hair_strand_shading_ctx.capacity) return -3;
        
        void* new_items = realloc(g_hair_strand_shading_ctx.items, new_capacity * sizeof(shading_hair_strand_shading_internal_t));
        if (!new_items) return -4;
        
        g_hair_strand_shading_ctx.items = new_items;
        g_hair_strand_shading_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_hair_strand_shading_ctx.count++;
    shading_hair_strand_shading_internal_t* item = &g_hair_strand_shading_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default hair values
    item->params.base_color = vec3_set(0.2f, 0.15f, 0.1f);  // Brown hair
    item->params.roughness_longitudinal = 0.1f;
    item->params.roughness_azimuthal = 0.2f;
    item->params.shift = 0.035f; // ~2 degrees
    item->params.ior = 1.55f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_hair_strand_shading_destroy(shading_hair_strand_shading_handle_t handle) {
    if (handle.id >= g_hair_strand_shading_ctx.count) return;
    g_hair_strand_shading_ctx.items[handle.id].initialized = false;
}

int shading_hair_strand_shading_update(shading_hair_strand_shading_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_hair_strand_shading_ctx.count) return -1;
    
    shading_hair_strand_shading_internal_t* item = &g_hair_strand_shading_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(hair_params_t)) {
        memcpy(&item->params, data, sizeof(hair_params_t));
        item->dirty = true;
    }
    
    return 0;
}

bool shading_hair_strand_shading_is_valid(shading_hair_strand_shading_handle_t handle) {
    if (handle.id >= g_hair_strand_shading_ctx.count) return false;
    return g_hair_strand_shading_ctx.items[handle.id].initialized;
}

// NOTE: T is the tangent pointing from root to tip
void evaluate_hair_shading(shading_hair_strand_shading_handle_t handle,
                          vec3_t T, vec3_t V, vec3_t L,
                          vec3_t* out_color) {
    
    if (!g_hair_strand_shading_ctx.initialized || handle.id >= g_hair_strand_shading_ctx.count) return;
    shading_hair_strand_shading_internal_t* item = &g_hair_strand_shading_ctx.items[handle.id];
    
    // Angles for Marschner
    float theta_i = asin(CLAMP(vec3_dot(T, L), -1.0f, 1.0f));
    float theta_r = asin(CLAMP(vec3_dot(T, V), -1.0f, 1.0f));
    
    // R Lobe (Primary Specular)
    // Shifted towards root
    float shift_r = -item->params.shift; 
    float alpha_r = item->params.roughness_longitudinal;
    
    // Longitudinal scattering M_R
    // simplified: gaussian(theta_h - shift)
    float theta_h = (theta_i + theta_r) * 0.5f;
    float M_R = mp_scattering(sin(theta_h - shift_r), alpha_r);
    
    // Combine terms (Simplified R term)
    // Need full geometry for Np, using constant approx for now
    float Np_R = 0.1f; // approximation
    float R_term = M_R * Np_R;
    
    // TT Lobe (Transmission - backlight)
    // Shifted towards tip
    float shift_tt = item->params.shift; 
    float alpha_tt = item->params.roughness_longitudinal * 0.5f; // TT is usually sharper longitudinally
    float M_TT = mp_scattering(sin(theta_h - shift_tt), alpha_tt);
    
    // Use base color for transmission
    vec3_t TT_color = item->params.base_color;
    // Transmission is stronger when backlit (theta_i + theta_r near 0 in some frames, or just check dot)
    // For now simple addition
    float TT_term = M_TT * 0.4f; // strength factor
    
    // TRT (Secondary Specular - colored)
    // Stronger shift
    float shift_trt = item->params.shift * 2.0f;
    float M_TRT = mp_scattering(sin(theta_h - shift_trt), alpha_r);
    float TRT_term = M_TRT * 0.2f;
    
    // Combine
    // Final = White * R + Color * TT + Color_Secondary * TRT
    
    if (out_color) {
        // R is usually white (dielectric reflection)
        out_color->x = R_term + (TT_color.x * TT_term) + (TT_color.x * TRT_term);
        out_color->y = R_term + (TT_color.y * TT_term) + (TT_color.y * TRT_term);
        out_color->z = R_term + (TT_color.z * TT_term) + (TT_color.z * TRT_term);
    }
}

uint32_t shading_hair_strand_shading_get_count(void) {
    return g_hair_strand_shading_ctx.count;
}

size_t shading_hair_strand_shading_get_memory_usage(void) {
    size_t total = sizeof(g_hair_strand_shading_ctx);
    total += g_hair_strand_shading_ctx.capacity * sizeof(shading_hair_strand_shading_internal_t);
    return total;
}

void shading_hair_strand_shading_debug_print(void) {
    // Debug print
}
