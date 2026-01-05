/*
 * marschner_model.c
 * Marschner hair model implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "marschner_model.h"
#include "../../math/vec3.h"
#include "../../../include/math/math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_MARSCHNER_MODEL_MAX_COUNT 64
#define SHADING_MARSCHNER_MODEL_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct marschner_params {
    vec3_t base_color;
    float melanin;        // 0.0 to 1.0 (Blonde to Black)
    float redness;        // Pheomelanin control
    float roughness;      // Longitudinal roughness
    float shift_r;        // Longitudinal shift for R lobe
    float shift_tt;       // Longitudinal shift for TT lobe
    float shift_trt;      // Longitudinal shift for TRT lobe
    float width_scale;    // Scale for azimuthal width
} marschner_params_t;

typedef struct shading_marschner_model_internal {
    uint32_t id;
    uint32_t flags;
    marschner_params_t params;
    bool initialized;
    bool dirty;
} shading_marschner_model_internal_t;

typedef struct shading_marschner_model_context {
    shading_marschner_model_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_marschner_model_context_t;

static shading_marschner_model_context_t g_marschner_model_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float longitudinal_scattering(float sin_theta_i, float sin_theta_r, float cos_theta_i, float cos_theta_r, float width, float shift) {
    // Gaussian M term
    // shift applies to theta_i or difference? Usually alpha in M(theta_h - alpha)
    // Simplified: exp(-(theta_h - alpha)^2 / (2 * width^2))
    
    // Using approx from sin/cos for efficiency in real shader, but here using angles for clarity
    float theta_i = asinf(sin_theta_i);
    float theta_r = asinf(sin_theta_r);
    float theta_h = (theta_i + theta_r) * 0.5f;
    
    float diff = theta_h - shift;
    return expf(-(diff * diff) / (2.0f * width * width));
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_marschner_model_init(void) {
    if (g_marschner_model_ctx.initialized) return 0;

    g_marschner_model_ctx.capacity = SHADING_MARSCHNER_MODEL_DEFAULT_CAPACITY;
    g_marschner_model_ctx.items = calloc(g_marschner_model_ctx.capacity, sizeof(shading_marschner_model_internal_t));
    if (!g_marschner_model_ctx.items) return -1;

    g_marschner_model_ctx.count = 0;
    g_marschner_model_ctx.initialized = true;

    return 0;
}

void shading_marschner_model_shutdown(void) {
    if (!g_marschner_model_ctx.initialized) return;

    free(g_marschner_model_ctx.items);
    g_marschner_model_ctx.items = NULL;
    g_marschner_model_ctx.count = 0;
    g_marschner_model_ctx.capacity = 0;
    g_marschner_model_ctx.initialized = false;
}

int shading_marschner_model_create(shading_marschner_model_handle_t* out_handle, const shading_marschner_model_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_marschner_model_ctx.initialized) return -2;

    if (g_marschner_model_ctx.count >= g_marschner_model_ctx.capacity) {
        uint32_t new_cap = g_marschner_model_ctx.capacity * 2;
        void* new_ptr = realloc(g_marschner_model_ctx.items, new_cap * sizeof(shading_marschner_model_internal_t));
        if (!new_ptr) return -3;
        
        g_marschner_model_ctx.items = new_ptr;
        g_marschner_model_ctx.capacity = new_cap;
    }

    uint32_t index = g_marschner_model_ctx.count++;
    shading_marschner_model_internal_t* item = &g_marschner_model_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Default brown hair
    item->params.base_color = vec3_set(0.1f, 0.05f, 0.01f);
    item->params.melanin = 0.5f;
    item->params.redness = 0.0f;
    item->params.roughness = 0.1f;
    
    // Typical shifts (in radians, roughly 5-10 degrees)
    item->params.shift_r = -0.08f;  // Primary specular shifted towards root
    item->params.shift_tt = 0.04f;  // Transmission shifted towards tip
    item->params.shift_trt = -0.12f; // Secondary specular shifted more towards root
    
    item->params.width_scale = 1.0f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_marschner_model_destroy(shading_marschner_model_handle_t handle) {
    if (handle.id < g_marschner_model_ctx.count) {
        g_marschner_model_ctx.items[handle.id].initialized = false;
    }
}

int shading_marschner_model_update(shading_marschner_model_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_marschner_model_ctx.count) return -1;
    shading_marschner_model_internal_t* item = &g_marschner_model_ctx.items[handle.id];
    
    if (size == sizeof(marschner_params_t)) {
        memcpy(&item->params, data, sizeof(marschner_params_t));
        item->dirty = true;
        return 0;
    }
    return -2;
}

bool shading_marschner_model_is_valid(shading_marschner_model_handle_t handle) {
    return handle.id < g_marschner_model_ctx.count && g_marschner_model_ctx.items[handle.id].initialized;
}

// Evaluate Marschner model
// Note: This is usually done on GPU; this function is for reference or CPU fallback
void evaluate_marschner_model(shading_marschner_model_handle_t handle,
                             vec3_t T, vec3_t V, vec3_t L,
                             vec3_t* out_color) {
    
    if (!g_marschner_model_ctx.initialized || handle.id >= g_marschner_model_ctx.count) return;
    shading_marschner_model_internal_t* item = &g_marschner_model_ctx.items[handle.id];
    
    // Need Tangent (u), Normal (v? no, u cross V?), Binormal(w) frame of the hair strand
    // T is tangent pointing from root to tip
    
    // 1. Longitudinal Scattering (M term) for R, TT, TRT
    // 2. Azimuthal Scattering (N term)
    
    // Simplified logic
    // Just setting color based on parameters for now
    if (out_color) {
        *out_color = item->params.base_color;
    }
}

int shading_marschner_model_get_info(shading_marschner_model_handle_t handle, shading_marschner_model_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_marschner_model_ctx.count) return -2;
    
    const shading_marschner_model_internal_t* item = &g_marschner_model_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void shading_marschner_model_mark_dirty(shading_marschner_model_handle_t handle) {
    if (handle.id < g_marschner_model_ctx.count) {
        g_marschner_model_ctx.items[handle.id].dirty = true;
    }
}

int shading_marschner_model_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_marschner_model_ctx.count; i++) {
        if (g_marschner_model_ctx.items[i].initialized && g_marschner_model_ctx.items[i].dirty) {
            g_marschner_model_ctx.items[i].dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t shading_marschner_model_get_count(void) {
    return g_marschner_model_ctx.count;
}

size_t shading_marschner_model_get_memory_usage(void) {
    size_t total = sizeof(g_marschner_model_ctx);
    total += g_marschner_model_ctx.capacity * sizeof(shading_marschner_model_internal_t);
    return total;
}

void shading_marschner_model_debug_print(void) {
    printf("Marschner Model Ctx: %u items\n", g_marschner_model_ctx.count);
}
