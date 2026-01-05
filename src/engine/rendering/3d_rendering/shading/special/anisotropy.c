/*
 * anisotropy.c
 * Anisotropic highlights implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "anisotropy.h"
#include "../../math/vec3.h"
#include "../../../include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_ANISOTROPY_MAX_COUNT 64
#define SHADING_ANISOTROPY_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct anisotropy_params {
    float roughness;
    float anisotropy; // 0.0 to 1.0 (or -1 to 1)
    float rotation;   // rotation of anisotropy axis
    vec3_t direction; // optional direction override
} anisotropy_params_t;

typedef struct shading_anisotropy_internal {
    uint32_t id;
    uint32_t flags;
    anisotropy_params_t params;
    bool initialized;
    bool dirty;
} shading_anisotropy_internal_t;

typedef struct shading_anisotropy_context {
    shading_anisotropy_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_anisotropy_context_t;

static shading_anisotropy_context_t g_anisotropy_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Anisotropic GGX distribution
// T, B, N frame must be provided
static float distribution_ggx_anisotropic(float NdotH, float TdotH, float BdotH, float ax, float ay) {
    float inv_ax = 1.0f / (ax * ax);
    float inv_ay = 1.0f / (ay * ay);
    
    float exponent = (TdotH * TdotH * inv_ax) + (BdotH * BdotH * inv_ay) + (NdotH * NdotH);
    
    // Note: this assumes ax, ay are squared roughnesses effectively, standard formula varies slightly
    // Standard Disney:
    // f = 1 / (pi * ax * ay) * 1 / ((H.X/ax)^2 + (H.Y/ay)^2 + H.Z^2)^2
    
    float denom = TdotH * TdotH / (ax * ax) + BdotH * BdotH / (ay * ay) + NdotH * NdotH;
    return 1.0f / (PI * ax * ay * denom * denom);
}

static float geometry_smith_anisotropic(float NdotV, float TdotV, float BdotV, float ax, float ay) {
    // Lambda_V
    float lambda_v = NdotV * NdotV; // roughly
    // Accurate Smith for Anisotropic:
    // lambda = (-1 + sqrt(1 + alpha_x^2 * tan^2_phi_x + alpha_y^2 * tan^2_phi_y)) / 2
    
    // Simplified:
    // just use isotropic geometric shadowing with averaged roughness for performance,
    // or full calculation. Let's use averaged for this implementation snippet.
    
    float rough_avg = sqrtf(ax * ax + ay * ay); // roughly
    float k = (rough_avg + 1.0f) * (rough_avg + 1.0f) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_anisotropy_init(void) {
    if (g_anisotropy_ctx.initialized) return 0;

    g_anisotropy_ctx.capacity = SHADING_ANISOTROPY_DEFAULT_CAPACITY;
    g_anisotropy_ctx.items = calloc(g_anisotropy_ctx.capacity, sizeof(shading_anisotropy_internal_t));
    
    if (!g_anisotropy_ctx.items) return -1;
    
    g_anisotropy_ctx.count = 0;
    g_anisotropy_ctx.initialized = true;
    
    return 0;
}

void shading_anisotropy_shutdown(void) {
    if (!g_anisotropy_ctx.initialized) return;
    
    free(g_anisotropy_ctx.items);
    g_anisotropy_ctx.items = NULL;
    g_anisotropy_ctx.count = 0;
    g_anisotropy_ctx.capacity = 0;
    g_anisotropy_ctx.initialized = false;
}

int shading_anisotropy_create(shading_anisotropy_handle_t* out_handle, const shading_anisotropy_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_anisotropy_ctx.initialized) return -2;
    
    if (g_anisotropy_ctx.count >= g_anisotropy_ctx.capacity) {
        uint32_t new_capacity = g_anisotropy_ctx.capacity * 2;
        if (new_capacity > SHADING_ANISOTROPY_MAX_COUNT) new_capacity = SHADING_ANISOTROPY_MAX_COUNT;
        
        if (new_capacity == g_anisotropy_ctx.capacity) return -3;
        
        void* new_items = realloc(g_anisotropy_ctx.items, new_capacity * sizeof(shading_anisotropy_internal_t));
        if (!new_items) return -4;
        
        g_anisotropy_ctx.items = new_items;
        g_anisotropy_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_anisotropy_ctx.count++;
    shading_anisotropy_internal_t* item = &g_anisotropy_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default anisotropy
    item->params.roughness = 0.5f;
    item->params.anisotropy = 0.0f; // isotropic
    item->params.rotation = 0.0f;
    item->params.direction = vec3_set(1.0f, 0.0f, 0.0f);
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_anisotropy_destroy(shading_anisotropy_handle_t handle) {
    if (handle.id >= g_anisotropy_ctx.count) return;
    g_anisotropy_ctx.items[handle.id].initialized = false;
}

int shading_anisotropy_update(shading_anisotropy_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_anisotropy_ctx.count) return -1;
    
    shading_anisotropy_internal_t* item = &g_anisotropy_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(anisotropy_params_t)) {
        memcpy(&item->params, data, sizeof(anisotropy_params_t));
        item->dirty = true;
    }
    
    return 0;
}

bool shading_anisotropy_is_valid(shading_anisotropy_handle_t handle) {
    if (handle.id >= g_anisotropy_ctx.count) return false;
    return g_anisotropy_ctx.items[handle.id].initialized;
}

// Helper to rotate tangent
static vec3_t rotate_vector(vec3_t v, vec3_t axis, float angle) {
    // simplified rotation for 2D tangent plane usually
    // T' = cos(a)*T + sin(a)*B
    return v; // Placeholder 3D rotation requires quaternion or matrix utils not fully imported
}

void evaluate_anisotropy(shading_anisotropy_handle_t handle,
                        vec3_t N, vec3_t T, vec3_t B,
                        vec3_t V, vec3_t L,
                        float* out_D) {
    
    if (!g_anisotropy_ctx.initialized || handle.id >= g_anisotropy_ctx.count) return;
    shading_anisotropy_internal_t* item = &g_anisotropy_ctx.items[handle.id];
    
    // Calculate aspect ratio
    float aspect = sqrtf(1.0f - item->params.anisotropy * 0.9f);
    float ax = item->params.roughness / aspect;
    float ay = item->params.roughness * aspect;
    
    vec3_t H = vec3_normalize(vec3_add(V, L));
    
    float NdotH = MAX(vec3_dot(N, H), 0.0f);
    float TdotH = vec3_dot(T, H);
    float BdotH = vec3_dot(B, H);
    
    if (out_D) {
        *out_D = distribution_ggx_anisotropic(NdotH, TdotH, BdotH, ax, ay);
    }
}

uint32_t shading_anisotropy_get_count(void) {
    return g_anisotropy_ctx.count;
}

size_t shading_anisotropy_get_memory_usage(void) {
    size_t total = sizeof(g_anisotropy_ctx);
    total += g_anisotropy_ctx.capacity * sizeof(shading_anisotropy_internal_t);
    return total;
}

void shading_anisotropy_debug_print(void) {
    // Debug print
}
