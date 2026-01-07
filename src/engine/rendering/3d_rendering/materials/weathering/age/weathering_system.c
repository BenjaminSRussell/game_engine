/*
 * weathering_system.c
 * Overall weathering controller implementation
 *
 * Part of the Age Weathering subsystem
 * Advanced 3D Rendering Engine
 */

#include "weathering_system.h"
#include "../rust_corrosion/rust_growth.h"
#include "../dirt_grunge/dirt_accumulation.h"
#include "../wear_tear/wear_edges.h"
#include "../wetness_system/wetness_mask.h"
#include "../../../include/math/math.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WEATHERING_SYSTEM_MAX_COUNT 1024
#define WEATHERING_SYSTEM_DEFAULT_CAPACITY 64

#define RUST_COLOR (vec3_t){0.44f, 0.22f, 0.13f}
#define DIRT_COLOR (vec3_t){0.25f, 0.20f, 0.15f}

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct weathering_system_internal {
    uint32_t id;
    uint32_t flags;
    weathering_params_t params;
    bool initialized;
    bool dirty;
} weathering_system_internal_t;

typedef struct weathering_system_context {
    weathering_system_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} weathering_system_context_t;

static weathering_system_context_t g_weathering_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Mock noise and map sampling functions (to be replaced with actual implementations)
static float sample_3d_noise(uint32_t texture, vec3_t pos) {
    // In a real implementation, this would sample a 3D texture or compute procedural noise
    // For now, return a value based on position to simulate variation
    return (sinf(pos.x) + cosf(pos.y) + sinf(pos.z)) * 0.5f + 0.5f;
}

static float sample_cavity(uint32_t map, vec2_t uv) {
    // Mock cavity map sampling
    return 0.5f; 
}

static float sample_curvature(uint32_t map, vec2_t uv) {
    // Mock curvature map sampling
    return 0.5f;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int weathering_system_init(void) {
    if (g_weathering_ctx.initialized) return 0;

    g_weathering_ctx.capacity = WEATHERING_SYSTEM_DEFAULT_CAPACITY;
    g_weathering_ctx.items = calloc(g_weathering_ctx.capacity, sizeof(weathering_system_internal_t));
    
    if (!g_weathering_ctx.items) return -1;
    
    g_weathering_ctx.count = 0;
    g_weathering_ctx.initialized = true;
    
    return 0;
}

void weathering_system_shutdown(void) {
    if (!g_weathering_ctx.initialized) return;
    
    free(g_weathering_ctx.items);
    g_weathering_ctx.items = NULL;
    g_weathering_ctx.count = 0;
    g_weathering_ctx.capacity = 0;
    g_weathering_ctx.initialized = false;
}

int weathering_system_create(weathering_system_handle_t* out_handle, const weathering_system_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_weathering_ctx.initialized) return -2;
    
    if (g_weathering_ctx.count >= g_weathering_ctx.capacity) {
        uint32_t new_capacity = g_weathering_ctx.capacity * 2;
        if (new_capacity > WEATHERING_SYSTEM_MAX_COUNT) new_capacity = WEATHERING_SYSTEM_MAX_COUNT;
        
        if (new_capacity == g_weathering_ctx.capacity) return -3;
        
        void* new_items = realloc(g_weathering_ctx.items, new_capacity * sizeof(weathering_system_internal_t));
        if (!new_items) return -4;
        
        g_weathering_ctx.items = new_items;
        g_weathering_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_weathering_ctx.count++;
    weathering_system_internal_t* item = &g_weathering_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    memset(&item->params, 0, sizeof(weathering_params_t));
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void weathering_system_destroy(weathering_system_handle_t handle) {
    if (handle.id >= g_weathering_ctx.count) return;
    g_weathering_ctx.items[handle.id].initialized = false;
}

int weathering_system_update_params(weathering_system_handle_t handle, const weathering_params_t* params) {
    if (handle.id >= g_weathering_ctx.count) return -1;
    
    weathering_system_internal_t* item = &g_weathering_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    memcpy(&item->params, params, sizeof(weathering_params_t));
    item->dirty = true;
    return 0;
}

bool weathering_system_is_valid(weathering_system_handle_t handle) {
    if (handle.id >= g_weathering_ctx.count) return false;
    return g_weathering_ctx.items[handle.id].initialized;
}

void weathering_system_apply(weathering_system_handle_t handle, weathering_material_state_t* mat, const weathering_params_t* params) {
    if (!mat || !params) return;

    // 1. Sample maps and noise
    float noise = sample_3d_noise(params->noise_texture, vec3_multiply_scalar(params->world_position, 0.1f));
    float cavity = sample_cavity(params->cavity_map, mat->uv);
    float curvature = sample_curvature(params->curvature_map, mat->uv);

    // 2. Rust in cavities and edges (oxidation decreases metallic, increases roughness)
    // Rust grows where there is high exposure and noise, favoring cavities/edges
    float rust_mask = saturate(params->rust_amount * (noise + cavity * 0.5f + curvature * 0.3f));
    mat->albedo = vec3_lerp(mat->albedo, RUST_COLOR, rust_mask);
    mat->roughness = lerp(mat->roughness, 0.85f, rust_mask);
    mat->metallic = lerp(mat->metallic, 0.0f, rust_mask);

    // 3. Dirt accumulation in cavities
    float dirt_mask = saturate(params->dirt_amount * cavity * noise);
    mat->albedo = vec3_lerp(mat->albedo, DIRT_COLOR, dirt_mask);
    mat->roughness = lerp(mat->roughness, 0.9f, dirt_mask);
    mat->metallic = lerp(mat->metallic, 0.0f, dirt_mask);

    // 4. Edge wear (curvature based rubbing/polishing)
    float wear_mask = saturate(params->wear_amount * curvature * noise);
    // Worn edges often reveal base material or just look polished
    mat->albedo = vec3_add_scalar(mat->albedo, wear_mask * 0.1f); 
    mat->roughness = lerp(mat->roughness, 0.2f, wear_mask);

    // 5. Wetness (darkens albedo, smooths surface)
    if (params->wetness > 0.01f) {
        float up_factor = saturate(params->surface_normal.y); // More wet on flat surfaces
        float wet_mask = params->wetness * (0.8f + 0.2f * up_factor);
        
        // Darken albedo due to absorption
        mat->albedo = vec3_multiply_scalar(mat->albedo, lerp(1.0f, 0.6f, wet_mask));
        // Smooth surface
        mat->roughness = lerp(mat->roughness, 0.1f, wet_mask);
    }
}

uint32_t weathering_system_get_count(void) {
    return g_weathering_ctx.count;
}

size_t weathering_system_get_memory_usage(void) {
    size_t total = sizeof(g_weathering_ctx);
    total += g_weathering_ctx.capacity * sizeof(weathering_system_internal_t);
    return total;
}
