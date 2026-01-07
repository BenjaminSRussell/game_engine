/*
 * iridescence.c
 * Thin-film iridescence implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "shading/special/iridescence.h"
#include "include/math/vec3.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_IRIDESCENCE_MAX_COUNT 64
#define SHADING_IRIDESCENCE_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct iridescence_params {
    float thickness; // in nanometers (e.g. 300-800)
    float ior;       // film index of refraction
    float k;         // extinction coefficient
} iridescence_params_t;

typedef struct shading_iridescence_internal {
    uint32_t id;
    uint32_t flags;
    iridescence_params_t params;
    bool initialized;
    bool dirty;
} shading_iridescence_internal_t;

typedef struct shading_iridescence_context {
    shading_iridescence_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_iridescence_context_t;

static shading_iridescence_context_t g_iridescence_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_iridescence_init(void) {
    if (g_iridescence_ctx.initialized) return 0;

    g_iridescence_ctx.capacity = SHADING_IRIDESCENCE_DEFAULT_CAPACITY;
    g_iridescence_ctx.items = calloc(g_iridescence_ctx.capacity, sizeof(shading_iridescence_internal_t));
    
    if (!g_iridescence_ctx.items) return -1;
    
    g_iridescence_ctx.count = 0;
    g_iridescence_ctx.initialized = true;
    
    return 0;
}

void shading_iridescence_shutdown(void) {
    if (!g_iridescence_ctx.initialized) return;
    
    free(g_iridescence_ctx.items);
    g_iridescence_ctx.items = NULL;
    g_iridescence_ctx.count = 0;
    g_iridescence_ctx.capacity = 0;
    g_iridescence_ctx.initialized = false;
}

int shading_iridescence_create(shading_iridescence_handle_t* out_handle, const shading_iridescence_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_iridescence_ctx.initialized) return -2;
    
    if (g_iridescence_ctx.count >= g_iridescence_ctx.capacity) {
        uint32_t new_capacity = g_iridescence_ctx.capacity * 2;
        if (new_capacity > SHADING_IRIDESCENCE_MAX_COUNT) new_capacity = SHADING_IRIDESCENCE_MAX_COUNT;
        
        if (new_capacity == g_iridescence_ctx.capacity) return -3;
        
        void* new_items = realloc(g_iridescence_ctx.items, new_capacity * sizeof(shading_iridescence_internal_t));
        if (!new_items) return -4;
        
        g_iridescence_ctx.items = new_items;
        g_iridescence_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_iridescence_ctx.count++;
    shading_iridescence_internal_t* item = &g_iridescence_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default thin film
    item->params.thickness = 500.0f; // nm
    item->params.ior = 1.33f;        // soap bubble
    item->params.k = 0.0f; 
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_iridescence_destroy(shading_iridescence_handle_t handle) {
    if (handle.id >= g_iridescence_ctx.count) return;
    g_iridescence_ctx.items[handle.id].initialized = false;
}

int shading_iridescence_update(shading_iridescence_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_iridescence_ctx.count) return -1;
    
    shading_iridescence_internal_t* item = &g_iridescence_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(iridescence_params_t)) {
        memcpy(&item->params, data, sizeof(iridescence_params_t));
        item->dirty = true;
    }
    
    return 0;
}

bool shading_iridescence_is_valid(shading_iridescence_handle_t handle) {
    if (handle.id >= g_iridescence_ctx.count) return false;
    return g_iridescence_ctx.items[handle.id].initialized;
}

// Evaluate Thin-Film Interference
// Returns factor to modulate F0
void evaluate_iridescence(shading_iridescence_handle_t handle,
                         float NdotV,
                         vec3_t* out_iridescence_f0) {
    
    if (!g_iridescence_ctx.initialized || handle.id >= g_iridescence_ctx.count) return;
    shading_iridescence_internal_t* item = &g_iridescence_ctx.items[handle.id];
    
    // Approximation of thin-film interference
    // Phase difference delta = 4 * PI * d * cos(theta_film) / lambda
    
    // Refraction angle in film (Snell's law)
    float eta_1 = 1.0f; // air
    float eta_2 = item->params.ior;
    float sin_theta_1 = sqrtf(MAX(0.0f, 1.0f - NdotV * NdotV));
    float sin_theta_2 = (eta_1 / eta_2) * sin_theta_1;
    float cos_theta_2 = sqrtf(MAX(0.0f, 1.0f - sin_theta_2 * sin_theta_2));
    
    float d = item->params.thickness;
    
    // Calculate interference for R, G, B wavelengths (approx 650, 510, 475 nm)
    float lambda_r = 650.0f;
    float lambda_g = 510.0f;
    float lambda_b = 475.0f;
    
    float path_diff = 2.0f * eta_2 * d * cos_theta_2;
    
    float phase_r = (2.0f * PI * path_diff) / lambda_r; // + shift if needed
    float phase_g = (2.0f * PI * path_diff) / lambda_g;
    float phase_b = (2.0f * PI * path_diff) / lambda_b;
    
    // Constructive/Destructive interference factor (0 to 1) or modulation
    // Intensity ~ cos(phase/2)^2
    float I_r = cosf(phase_r * 0.5f); I_r *= I_r;
    float I_g = cosf(phase_g * 0.5f); I_g *= I_g;
    float I_b = cosf(phase_b * 0.5f); I_b *= I_b;
    
    if (out_iridescence_f0) {
        out_iridescence_f0->x = I_r;
        out_iridescence_f0->y = I_g;
        out_iridescence_f0->z = I_b;
    }
}

uint32_t shading_iridescence_get_count(void) {
    return g_iridescence_ctx.count;
}

size_t shading_iridescence_get_memory_usage(void) {
    size_t total = sizeof(g_iridescence_ctx);
    total += g_iridescence_ctx.capacity * sizeof(shading_iridescence_internal_t);
    return total;
}

void shading_iridescence_debug_print(void) {
    // Debug print
}
