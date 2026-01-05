/*
 * age_simulation.c
 * Material aging simulation implementation
 *
 * Part of the Age Weathering subsystem
 * Advanced 3D Rendering Engine
 */

#include "age_simulation.h"
#include "../../../include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define AGE_SIMULATION_MAX_COUNT 1024
#define AGE_SIMULATION_DEFAULT_CAPACITY 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct age_simulation_internal {
    uint32_t id;
    uint32_t flags;
    age_params_t params;
    bool initialized;
} age_simulation_internal_t;

typedef struct age_simulation_context {
    age_simulation_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} age_simulation_context_t;

static age_simulation_context_t g_age_sim_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int age_simulation_init(void) {
    if (g_age_sim_ctx.initialized) return 0;

    g_age_sim_ctx.capacity = AGE_SIMULATION_DEFAULT_CAPACITY;
    g_age_sim_ctx.items = calloc(g_age_sim_ctx.capacity, sizeof(age_simulation_internal_t));
    
    if (!g_age_sim_ctx.items) return -1;
    
    g_age_sim_ctx.count = 0;
    g_age_sim_ctx.initialized = true;
    
    return 0;
}

void age_simulation_shutdown(void) {
    if (!g_age_sim_ctx.initialized) return;
    
    free(g_age_sim_ctx.items);
    g_age_sim_ctx.items = NULL;
    g_age_sim_ctx.count = 0;
    g_age_sim_ctx.capacity = 0;
    g_age_sim_ctx.initialized = false;
}

int age_simulation_create(age_simulation_handle_t* out_handle, const age_simulation_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_age_sim_ctx.initialized) return -2;
    
    if (g_age_sim_ctx.count >= g_age_sim_ctx.capacity) {
        uint32_t new_capacity = g_age_sim_ctx.capacity * 2;
        if (new_capacity > AGE_SIMULATION_MAX_COUNT) new_capacity = AGE_SIMULATION_MAX_COUNT;
        
        if (new_capacity == g_age_sim_ctx.capacity) return -3;
        
        void* new_items = realloc(g_age_sim_ctx.items, new_capacity * sizeof(age_simulation_internal_t));
        if (!new_items) return -4;
        
        g_age_sim_ctx.items = new_items;
        g_age_sim_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_age_sim_ctx.count++;
    age_simulation_internal_t* item = &g_age_sim_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    memset(&item->params, 0, sizeof(age_params_t));
    
    item->initialized = true;
    
    out_handle->id = index;
    return 0;
}

void age_simulation_destroy(age_simulation_handle_t handle) {
    if (handle.id >= g_age_sim_ctx.count) return;
    g_age_sim_ctx.items[handle.id].initialized = false;
}

int age_simulation_update(age_simulation_handle_t handle, const age_params_t* params) {
    if (handle.id >= g_age_sim_ctx.count) return -1;
    
    age_simulation_internal_t* item = &g_age_sim_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    memcpy(&item->params, params, sizeof(age_params_t));
    return 0;
}

bool age_simulation_is_valid(age_simulation_handle_t handle) {
    if (handle.id >= g_age_sim_ctx.count) return false;
    return g_age_sim_ctx.items[handle.id].initialized;
}

void age_simulation_calculate_factors(age_simulation_handle_t handle, 
                                    float* out_rust, 
                                    float* out_dirt, 
                                    float* out_wear) {
    if (handle.id >= g_age_sim_ctx.count) return;
    
    age_simulation_internal_t* item = &g_age_sim_ctx.items[handle.id];
    if (!item->initialized) return;

    float age = item->params.time_years;
    
    // Rust: grows over time with moisture
    // Logarithmic growth pattern (fast at first, then stabilizes)
    if (out_rust) {
        *out_rust = saturate(0.2f * log2f(age + 1.0f) * item->params.moisture_exposure);
    }
    
    // Dirt: accumulates over time
    // Linear growth with environmental factors
    if (out_dirt) {
        *out_dirt = saturate(0.1f * age * (1.0f + item->params.uv_exposure * 0.5f));
    }
    
    // Wear: increases with physical stress and time
    if (out_wear) {
        *out_wear = saturate(0.3f * powf(age, 0.5f) * item->params.physical_stress);
    }
}

uint32_t age_simulation_get_count(void) {
    return g_age_sim_ctx.count;
}

size_t age_simulation_get_memory_usage(void) {
    size_t total = sizeof(g_age_sim_ctx);
    total += g_age_sim_ctx.capacity * sizeof(age_simulation_internal_t);
    return total;
}
