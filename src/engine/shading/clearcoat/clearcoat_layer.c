/*
 * clearcoat_layer.c
 * Clearcoat layer implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "shading/clearcoat/clearcoat_layer.h"
#include "include/math/vec3.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_CLEARCOAT_LAYER_MAX_COUNT 64
#define SHADING_CLEARCOAT_LAYER_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct clearcoat_params {
    float strength;
    float roughness;
    float ior;
    vec3_t normal; // Usually from a separate normal map
} clearcoat_params_t;

typedef struct shading_clearcoat_layer_internal {
    uint32_t id;
    uint32_t flags;
    clearcoat_params_t params;
    bool initialized;
    bool dirty;
} shading_clearcoat_layer_internal_t;

typedef struct shading_clearcoat_layer_context {
    shading_clearcoat_layer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_clearcoat_layer_context_t;

static shading_clearcoat_layer_context_t g_clearcoat_layer_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float distribution_ggx(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    
    return nom / denom;
}

static float geometry_schlick_ggx(float NdotV, float roughness) {
    float k = (roughness * roughness) / 2.0f; // IBL k used often for clearcoat
    return NdotV / (NdotV * (1.0f - k) + k);
}

static float geometry_smith(float NdotV, float NdotL, float roughness) {
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);
    return ggx1 * ggx2;
}

static float fresnel_schlick(float cos_theta, float f0) {
    // float pow5 = powf(1.0f - cos_theta, 5.0f);
    float base = 1.0f - cos_theta;
    float pow5 = base * base * base * base * base;
    return f0 + (1.0f - f0) * pow5;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_clearcoat_layer_init(void) {
    if (g_clearcoat_layer_ctx.initialized) return 0;

    g_clearcoat_layer_ctx.capacity = SHADING_CLEARCOAT_LAYER_DEFAULT_CAPACITY;
    g_clearcoat_layer_ctx.items = calloc(g_clearcoat_layer_ctx.capacity, sizeof(shading_clearcoat_layer_internal_t));
    
    if (!g_clearcoat_layer_ctx.items) return -1;
    
    g_clearcoat_layer_ctx.count = 0;
    g_clearcoat_layer_ctx.initialized = true;
    
    return 0;
}

void shading_clearcoat_layer_shutdown(void) {
    if (!g_clearcoat_layer_ctx.initialized) return;
    
    free(g_clearcoat_layer_ctx.items);
    g_clearcoat_layer_ctx.items = NULL;
    g_clearcoat_layer_ctx.count = 0;
    g_clearcoat_layer_ctx.capacity = 0;
    g_clearcoat_layer_ctx.initialized = false;
}

int shading_clearcoat_layer_create(shading_clearcoat_layer_handle_t* out_handle, const shading_clearcoat_layer_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_clearcoat_layer_ctx.initialized) return -2;
    
    if (g_clearcoat_layer_ctx.count >= g_clearcoat_layer_ctx.capacity) {
        uint32_t new_capacity = g_clearcoat_layer_ctx.capacity * 2;
        if (new_capacity > SHADING_CLEARCOAT_LAYER_MAX_COUNT) new_capacity = SHADING_CLEARCOAT_LAYER_MAX_COUNT;
        
        if (new_capacity == g_clearcoat_layer_ctx.capacity) return -3;
        
        void* new_items = realloc(g_clearcoat_layer_ctx.items, new_capacity * sizeof(shading_clearcoat_layer_internal_t));
        if (!new_items) return -4;
        
        g_clearcoat_layer_ctx.items = new_items;
        g_clearcoat_layer_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_clearcoat_layer_ctx.count++;
    shading_clearcoat_layer_internal_t* item = &g_clearcoat_layer_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default clearcoat
    item->params.strength = 1.0f;
    item->params.roughness = 0.05f; // Very smooth
    item->params.ior = 1.5f;        // Polyurethane
    item->params.normal = vec3_set(0.0f, 0.0f, 1.0f); // Default flat normal
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_clearcoat_layer_destroy(shading_clearcoat_layer_handle_t handle) {
    if (handle.id >= g_clearcoat_layer_ctx.count) return;
    g_clearcoat_layer_ctx.items[handle.id].initialized = false;
}

int shading_clearcoat_layer_update(shading_clearcoat_layer_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_clearcoat_layer_ctx.count) return -1;
    
    shading_clearcoat_layer_internal_t* item = &g_clearcoat_layer_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(clearcoat_params_t)) {
        memcpy(&item->params, data, sizeof(clearcoat_params_t));
        item->dirty = true;
    }
    
    return 0;
}

bool shading_clearcoat_layer_is_valid(shading_clearcoat_layer_handle_t handle) {
    if (handle.id >= g_clearcoat_layer_ctx.count) return false;
    return g_clearcoat_layer_ctx.items[handle.id].initialized;
}

// Evaluate Clearcoat
// Returns clearcoat specular contribution and modification factors for base layer
void evaluate_clearcoat_layer(shading_clearcoat_layer_handle_t handle,
                             vec3_t N_clearcoat, // Potentially different normal
                             vec3_t V, vec3_t L,
                             vec3_t* out_specular,
                             float* out_fresnel) {
                                  
    if (!g_clearcoat_layer_ctx.initialized || handle.id >= g_clearcoat_layer_ctx.count) return;
    shading_clearcoat_layer_internal_t* item = &g_clearcoat_layer_ctx.items[handle.id];

    if (item->params.strength <= EPSILON) {
        if (out_specular) *out_specular = vec3_set(0,0,0);
        if (out_fresnel) *out_fresnel = 0.0f;
        return;
    }
    
    vec3_t H = vec3_normalize(vec3_add(V, L));
    
    float NdotL = MAX(vec3_dot(N_clearcoat, L), 0.0f);
    float NdotV = MAX(vec3_dot(N_clearcoat, V), 0.0f);
    float NdotH = MAX(vec3_dot(N_clearcoat, H), 0.0f);
    float VdotH = MAX(vec3_dot(V, H), 0.0f); // Same as LdotH
    
    // Clearcoat Specular (GGX)
    float D = distribution_ggx(NdotH, item->params.roughness);
    float G = geometry_smith(NdotV, NdotL, 0.25f); // Fixed roughness for visibility often used for clearcoat
    
    // Fresnel at IOR 1.5 -> F0 = 0.04
    float f0 = 0.04f;
    float F = fresnel_schlick(VdotH, f0);
    
    // Specular term
    // Standard Cook-Torrance denominator 4*NdotL*NdotV is sometimes cancelled or simplified 
    // for clearcoat depending on performance. Using standard here.
    float spec = (D * F * G) / (4.0f * NdotL * NdotV + 0.001f);
    
    float intensity = spec * item->params.strength * NdotL;
    
    if (out_specular) {
        // Clearcoat is dielectric, so white * intensity
        out_specular->x = intensity;
        out_specular->y = intensity;
        out_specular->z = intensity;
    }
    
    // Pass fresnel out for energy conservation (attenuating base layer)
    // Base layer color should be multiplied by (1 - F) ideally
    if (out_fresnel) {
        *out_fresnel = F * item->params.strength;
    }
}


uint32_t shading_clearcoat_layer_get_count(void) {
    return g_clearcoat_layer_ctx.count;
}

size_t shading_clearcoat_layer_get_memory_usage(void) {
    size_t total = sizeof(g_clearcoat_layer_ctx);
    total += g_clearcoat_layer_ctx.capacity * sizeof(shading_clearcoat_layer_internal_t);
    return total;
}

void shading_clearcoat_layer_debug_print(void) {
    // Debug print
}
