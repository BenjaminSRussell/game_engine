/*
 * rust_growth.c
 * Procedural rust patterns implementation
 *
 * Part of the Rust Corrosion subsystem
 * Advanced 3D Rendering Engine
 */

#include "rust_growth.h"
#include "../../../include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RUST_GROWTH_MAX_COUNT 512
#define RUST_GROWTH_DEFAULT_CAPACITY 32

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rust_growth_internal {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rust_growth_internal_t;

typedef struct rust_growth_context {
    rust_growth_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rust_growth_context_t;

static rust_growth_context_t g_rust_growth_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float procedural_noise(vec3_t p) {
    // Simple 3D fractals for rust variation
    return (sinf(p.x * 2.0f) * cosf(p.y * 2.0f) * sinf(p.z * 2.1f)) * 0.5f + 0.5f;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rust_growth_init(void) {
    if (g_rust_growth_ctx.initialized) return 0;
    g_rust_growth_ctx.capacity = RUST_GROWTH_DEFAULT_CAPACITY;
    g_rust_growth_ctx.items = calloc(g_rust_growth_ctx.capacity, sizeof(rust_growth_internal_t));
    if (!g_rust_growth_ctx.items) return -1;
    g_rust_growth_ctx.count = 0;
    g_rust_growth_ctx.initialized = true;
    return 0;
}

void rust_growth_shutdown(void) {
    if (!g_rust_growth_ctx.initialized) return;
    free(g_rust_growth_ctx.items);
    g_rust_growth_ctx.items = NULL;
    g_rust_growth_ctx.count = 0;
    g_rust_growth_ctx.capacity = 0;
    g_rust_growth_ctx.initialized = false;
}

int rust_growth_create(rust_growth_handle_t* out_handle, const rust_growth_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_rust_growth_ctx.initialized) return -2;
    if (g_rust_growth_ctx.count >= g_rust_growth_ctx.capacity) {
        uint32_t new_capacity = g_rust_growth_ctx.capacity * 2;
        if (new_capacity > RUST_GROWTH_MAX_COUNT) new_capacity = RUST_GROWTH_MAX_COUNT;
        void* new_items = realloc(g_rust_growth_ctx.items, new_capacity * sizeof(rust_growth_internal_t));
        if (!new_items) return -4;
        g_rust_growth_ctx.items = new_items;
        g_rust_growth_ctx.capacity = new_capacity;
    }
    uint32_t index = g_rust_growth_ctx.count++;
    rust_growth_internal_t* item = &g_rust_growth_ctx.items[index];
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    out_handle->id = index;
    return 0;
}

void rust_growth_destroy(rust_growth_handle_t handle) {
    if (handle.id >= g_rust_growth_ctx.count) return;
    g_rust_growth_ctx.items[handle.id].initialized = false;
}

float rust_growth_calculate_mask(rust_growth_handle_t handle, 
                               const rust_pattern_params_t* params,
                               vec3_t world_pos,
                               float curvature,
                               float salt_exposure) {
    if (handle.id >= g_rust_growth_ctx.count) return 0.0f;
    if (!g_rust_growth_ctx.items[handle.id].initialized) return 0.0f;

    // Rust thrives on edges (high curvature) and scratches
    float edge_factor = saturate(curvature * params->spread_rate);
    
    // Sample multi-scale noise for rust patterns
    vec3_t noise_p = vec3_multiply_scalar(world_pos, params->noise_scale);
    float n1 = procedural_noise(noise_p);
    float n2 = procedural_noise(vec3_multiply_scalar(noise_p, 2.3f)) * 0.5f;
    float combined_noise = saturate(n1 + n2);
    
    // Combine factors
    float rust_mask = combined_noise * (edge_factor + salt_exposure * 0.4f);
    
    // Pitting intensity (makes rust appear in patches or deep holes)
    rust_mask = powf(rust_mask, 1.0f / (params->pitting_intensity + 0.1f));
    
    return saturate(rust_mask);
}

uint32_t rust_growth_get_count(void) {
    return g_rust_growth_ctx.count;
}

size_t rust_growth_get_memory_usage(void) {
    return sizeof(g_rust_growth_ctx) + g_rust_growth_ctx.capacity * sizeof(rust_growth_internal_t);
}
