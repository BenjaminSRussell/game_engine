/*
 * cascade_splits.c
 * CSM cascade split calculation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/cascades/cascade_splits.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_CASCADE_COUNT 8
#define DEFAULT_CASCADE_COUNT 4

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cascade_splits_context {
    float splits[MAX_CASCADE_COUNT + 1];  // +1 for far plane
    uint32_t cascade_count;
    float lambda;  // Practical split scheme parameter
    bool initialized;
} cascade_splits_context_t;

static cascade_splits_context_t g_cascade_ctx = {0};

/* ============================================================================
 * CASCADE SPLIT CALCULATION
 * ============================================================================ */

void lighting_cascade_calculate_splits(float near, float far, float lambda, 
                                       float* splits, uint32_t count) {
    if (!splits || count == 0) return;
    
    splits[0] = near;
    
    for (uint32_t i = 1; i < count; i++) {
        float p = (float)i / (float)count;
        
        // Logarithmic split
        float log_split = near * powf(far / near, p);
        
        // Uniform split
        float uniform_split = near + (far - near) * p;
        
        // Practical split scheme (blend between log and uniform)
        splits[i] = lambda * log_split + (1.0f - lambda) * uniform_split;
    }
    
    splits[count] = far;
}

void lighting_cascade_calculate_splits_logarithmic(float near, float far, 
                                                   float* splits, uint32_t count) {
    if (!splits || count == 0) return;
    
    splits[0] = near;
    
    for (uint32_t i = 1; i < count; i++) {
        float p = (float)i / (float)count;
        splits[i] = near * powf(far / near, p);
    }
    
    splits[count] = far;
}

void lighting_cascade_calculate_splits_uniform(float near, float far, 
                                               float* splits, uint32_t count) {
    if (!splits || count == 0) return;
    
    splits[0] = near;
    
    for (uint32_t i = 1; i < count; i++) {
        float p = (float)i / (float)count;
        splits[i] = near + (far - near) * p;
    }
    
    splits[count] = far;
}

void lighting_cascade_set_split_lambda(float lambda) {
    if (g_cascade_ctx.initialized) {
        g_cascade_ctx.lambda = fmaxf(0.0f, fminf(1.0f, lambda));
    }
}

float lighting_cascade_get_split_lambda(void) {
    return g_cascade_ctx.lambda;
}

void lighting_cascade_set_count(uint32_t count) {
    if (g_cascade_ctx.initialized && count > 0 && count <= MAX_CASCADE_COUNT) {
        g_cascade_ctx.cascade_count = count;
    }
}

uint32_t lighting_cascade_get_cascade_count(void) {
    return g_cascade_ctx.cascade_count;
}

const float* lighting_cascade_get_splits(void) {
    return g_cascade_ctx.splits;
}

void lighting_cascade_update_splits(float near, float far) {
    if (!g_cascade_ctx.initialized) return;
    
    lighting_cascade_calculate_splits(near, far, g_cascade_ctx.lambda, 
                                      g_cascade_ctx.splits, 
                                      g_cascade_ctx.cascade_count);
}

/* ============================================================================
 * PUBLIC API (Compatibility)
 * ============================================================================ */

int lighting_cascade_splits_init(void) {
    if (g_cascade_ctx.initialized) {
        return 0;
    }
    
    g_cascade_ctx.cascade_count = DEFAULT_CASCADE_COUNT;
    g_cascade_ctx.lambda = 0.5f;  // Balanced between log and uniform
    memset(g_cascade_ctx.splits, 0, sizeof(g_cascade_ctx.splits));
    g_cascade_ctx.initialized = true;
    
    return 0;
}

void lighting_cascade_splits_shutdown(void) {
    if (!g_cascade_ctx.initialized) {
        return;
    }
    
    g_cascade_ctx.initialized = false;
}

int lighting_cascade_splits_create(lighting_cascade_splits_handle_t* out_handle, 
                                   const lighting_cascade_splits_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    out_handle->id = 0;
    return 0;
}

void lighting_cascade_splits_destroy(lighting_cascade_splits_handle_t handle) {
    (void)handle;
}

int lighting_cascade_splits_update(lighting_cascade_splits_handle_t handle, 
                                   const void* data, size_t size) {
    (void)handle; (void)data; (void)size;
    return 0;
}

bool lighting_cascade_splits_is_valid(lighting_cascade_splits_handle_t handle) {
    (void)handle;
    return g_cascade_ctx.initialized;
}

int lighting_cascade_splits_get_info(lighting_cascade_splits_handle_t handle, 
                                     lighting_cascade_splits_info_t* out_info) {
    if (!out_info) return -1;
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = g_cascade_ctx.initialized;
    return 0;
}

void lighting_cascade_splits_mark_dirty(lighting_cascade_splits_handle_t handle) {
    (void)handle;
}

int lighting_cascade_splits_process_pending(void) {
    return 0;
}

uint32_t lighting_cascade_splits_get_count(void) {
    return g_cascade_ctx.cascade_count;
}

size_t lighting_cascade_splits_get_memory_usage(void) {
    return sizeof(cascade_splits_context_t);
}

void lighting_cascade_splits_debug_print(void) {
    // Debug output
}

/* End of cascade_splits.c */
