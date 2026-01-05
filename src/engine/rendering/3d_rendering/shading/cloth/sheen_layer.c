/*
 * sheen_layer.c
 * Sheen layer implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "sheen_layer.h"
#include "../../math/vec3.h"
#include "../../../include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_SHEEN_LAYER_MAX_COUNT 64
#define SHADING_SHEEN_LAYER_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct sheen_params {
    vec3_t sheen_color;
    float roughness;
    float intensity;
} sheen_params_t;

typedef struct shading_sheen_layer_internal {
    uint32_t id;
    uint32_t flags;
    sheen_params_t params;
    bool initialized;
    bool dirty;
} shading_sheen_layer_internal_t;

typedef struct shading_sheen_layer_context {
    shading_sheen_layer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_sheen_layer_context_t;

static shading_sheen_layer_context_t g_sheen_layer_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Reusing Charlie distribution for sheen
static float distribution_charlie(float roughness, float NdotH) {
    float inv_r = 1.0f / (roughness * roughness + EPSILON); 
    float cos2h = NdotH * NdotH;
    float sin2h = 1.0f - cos2h;
    if (sin2h < 0.0f) sin2h = 0.0f;
    return (2.0f + inv_r) * powf(sin2h, inv_r * 0.5f) / (2.0f * PI);
}

static float visibility_ashikhmin(float NdotL, float NdotV) {
    return 1.0f / (4.0f * (NdotL + NdotV - NdotL * NdotV));
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_sheen_layer_init(void) {
    if (g_sheen_layer_ctx.initialized) return 0;

    g_sheen_layer_ctx.capacity = SHADING_SHEEN_LAYER_DEFAULT_CAPACITY;
    g_sheen_layer_ctx.items = calloc(g_sheen_layer_ctx.capacity, sizeof(shading_sheen_layer_internal_t));
    
    if (!g_sheen_layer_ctx.items) return -1;
    
    g_sheen_layer_ctx.count = 0;
    g_sheen_layer_ctx.initialized = true;
    
    return 0;
}

void shading_sheen_layer_shutdown(void) {
    if (!g_sheen_layer_ctx.initialized) return;
    
    free(g_sheen_layer_ctx.items);
    g_sheen_layer_ctx.items = NULL;
    g_sheen_layer_ctx.count = 0;
    g_sheen_layer_ctx.capacity = 0;
    g_sheen_layer_ctx.initialized = false;
}

int shading_sheen_layer_create(shading_sheen_layer_handle_t* out_handle, const shading_sheen_layer_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_sheen_layer_ctx.initialized) return -2;
    
    if (g_sheen_layer_ctx.count >= g_sheen_layer_ctx.capacity) {
        uint32_t new_capacity = g_sheen_layer_ctx.capacity * 2;
        if (new_capacity > SHADING_SHEEN_LAYER_MAX_COUNT) new_capacity = SHADING_SHEEN_LAYER_MAX_COUNT;
        
        if (new_capacity == g_sheen_layer_ctx.capacity) return -3;
        
        void* new_items = realloc(g_sheen_layer_ctx.items, new_capacity * sizeof(shading_sheen_layer_internal_t));
        if (!new_items) return -4;
        
        g_sheen_layer_ctx.items = new_items;
        g_sheen_layer_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_sheen_layer_ctx.count++;
    shading_sheen_layer_internal_t* item = &g_sheen_layer_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default sheen
    item->params.sheen_color = vec3_set(1.0f, 1.0f, 1.0f);
    item->params.roughness = 0.5f;
    item->params.intensity = 1.0f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_sheen_layer_destroy(shading_sheen_layer_handle_t handle) {
    if (handle.id >= g_sheen_layer_ctx.count) return;
    g_sheen_layer_ctx.items[handle.id].initialized = false;
}

int shading_sheen_layer_update(shading_sheen_layer_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_sheen_layer_ctx.count) return -1;
    
    shading_sheen_layer_internal_t* item = &g_sheen_layer_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(sheen_params_t)) {
        memcpy(&item->params, data, sizeof(sheen_params_t));
        item->dirty = true;
    }
    
    return 0;
}

bool shading_sheen_layer_is_valid(shading_sheen_layer_handle_t handle) {
    if (handle.id >= g_sheen_layer_ctx.count) return false;
    return g_sheen_layer_ctx.items[handle.id].initialized;
}

// Evaluate independent sheen layer
void evaluate_sheen_layer(shading_sheen_layer_handle_t handle,
                         vec3_t N, vec3_t V, vec3_t L,
                         vec3_t* out_sheen) {
                             
    if (!g_sheen_layer_ctx.initialized || handle.id >= g_sheen_layer_ctx.count) return;
    shading_sheen_layer_internal_t* item = &g_sheen_layer_ctx.items[handle.id];
    
    vec3_t H = vec3_normalize(vec3_add(V, L));
    float NdotL = MAX(vec3_dot(N, L), 0.0f);
    float NdotV = MAX(vec3_dot(N, V), 0.0f);
    float NdotH = MAX(vec3_dot(N, H), 0.0f);
    
    float D = distribution_charlie(item->params.roughness, NdotH);
    float Vis = visibility_ashikhmin(NdotL, NdotV);
    
    float sheen_factor = D * Vis * NdotL * item->params.intensity;
    
    if (out_sheen) {
        out_sheen->x = item->params.sheen_color.x * sheen_factor;
        out_sheen->y = item->params.sheen_color.y * sheen_factor;
        out_sheen->z = item->params.sheen_color.z * sheen_factor;
    }
}

uint32_t shading_sheen_layer_get_count(void) {
    return g_sheen_layer_ctx.count;
}

size_t shading_sheen_layer_get_memory_usage(void) {
    size_t total = sizeof(g_sheen_layer_ctx);
    total += g_sheen_layer_ctx.capacity * sizeof(shading_sheen_layer_internal_t);
    return total;
}

void shading_sheen_layer_debug_print(void) {
    // Debug print
}
