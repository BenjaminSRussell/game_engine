/*
 * skin_shading.c
 * Skin shading model implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "shading/subsurface/skin_shading.h"
#include "shading/subsurface/sss_profile.h"
#include "include/math/vec3.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_SKIN_SHADING_MAX_COUNT 64
#define SHADING_SKIN_SHADING_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Skin parameters
typedef struct skin_params {
    vec3_t diffuse_albedo;
    vec3_t specular_strength;
    float roughness_1; // Primary lobe (oily layer)
    float roughness_2; // Secondary lobe (skin surface)
    float lobe_mix;    // Ratio between lobes
    float sss_strength;
} skin_params_t;

typedef struct shading_skin_shading_internal {
    uint32_t id;
    uint32_t flags;
    skin_params_t params;
    shading_sss_profile_handle_t sss_profile;
    bool initialized;
    bool dirty;
} shading_skin_shading_internal_t;

typedef struct shading_skin_shading_context {
    shading_skin_shading_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_skin_shading_context_t;

static shading_skin_shading_context_t g_skin_shading_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_skin_shading_init(void) {
    if (g_skin_shading_ctx.initialized) return 0;

    g_skin_shading_ctx.capacity = SHADING_SKIN_SHADING_DEFAULT_CAPACITY;
    g_skin_shading_ctx.items = calloc(g_skin_shading_ctx.capacity, sizeof(shading_skin_shading_internal_t));
    
    if (!g_skin_shading_ctx.items) return -1;
    
    g_skin_shading_ctx.count = 0;
    g_skin_shading_ctx.initialized = true;
    
    return 0;
}

void shading_skin_shading_shutdown(void) {
    if (!g_skin_shading_ctx.initialized) return;
    
    free(g_skin_shading_ctx.items);
    g_skin_shading_ctx.items = NULL;
    g_skin_shading_ctx.count = 0;
    g_skin_shading_ctx.capacity = 0;
    g_skin_shading_ctx.initialized = false;
}

int shading_skin_shading_create(shading_skin_shading_handle_t* out_handle, const shading_skin_shading_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_skin_shading_ctx.initialized) return -2;
    
    if (g_skin_shading_ctx.count >= g_skin_shading_ctx.capacity) {
        uint32_t new_capacity = g_skin_shading_ctx.capacity * 2;
        if (new_capacity > SHADING_SKIN_SHADING_MAX_COUNT) new_capacity = SHADING_SKIN_SHADING_MAX_COUNT;
        
        if (new_capacity == g_skin_shading_ctx.capacity) return -3;
        
        void* new_items = realloc(g_skin_shading_ctx.items, new_capacity * sizeof(shading_skin_shading_internal_t));
        if (!new_items) return -4;
        
        g_skin_shading_ctx.items = new_items;
        g_skin_shading_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_skin_shading_ctx.count++;
    shading_skin_shading_internal_t* item = &g_skin_shading_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default skin values
    item->params.diffuse_albedo = vec3_set(0.8f, 0.6f, 0.5f);
    item->params.specular_strength = vec3_set(0.1f, 0.1f, 0.1f);
    item->params.roughness_1 = 0.3f;  // Wet/oily
    item->params.roughness_2 = 0.6f;  // Dry/rough
    item->params.lobe_mix = 0.5f;
    item->params.sss_strength = 1.0f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_skin_shading_destroy(shading_skin_shading_handle_t handle) {
    if (handle.id >= g_skin_shading_ctx.count) return;
    g_skin_shading_ctx.items[handle.id].initialized = false;
}

int shading_skin_shading_update(shading_skin_shading_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_skin_shading_ctx.count) return -1;
    
    shading_skin_shading_internal_t* item = &g_skin_shading_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(skin_params_t)) {
        memcpy(&item->params, data, sizeof(skin_params_t));
    }
    
    item->dirty = true;
    return 0;
}

bool shading_skin_shading_is_valid(shading_skin_shading_handle_t handle) {
    if (handle.id >= g_skin_shading_ctx.count) return false;
    return g_skin_shading_ctx.items[handle.id].initialized;
}

// Helper: GGX Distribution
static float distribution_ggx(vec3_t N, vec3_t H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = MAX(vec3_dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
}

// Logic simulation for skin surface evaluation
// This would be part of the fragment shader generation
void evaluate_skin_surface(shading_skin_shading_handle_t handle,
                          vec3_t N, vec3_t V, vec3_t L,
                          vec3_t* out_diffuse, vec3_t* out_specular) {
    
    if (!g_skin_shading_ctx.initialized || handle.id >= g_skin_shading_ctx.count) return;
    
    shading_skin_shading_internal_t* item = &g_skin_shading_ctx.items[handle.id];
    vec3_t H = vec3_normalize(vec3_add(V, L));
    
    // Dual lobe specular
    float D1 = distribution_ggx(N, H, item->params.roughness_1);
    float D2 = distribution_ggx(N, H, item->params.roughness_2);
    float D = D1 * item->params.lobe_mix + D2 * (1.0f - item->params.lobe_mix);
    
    // Simple Fresnel (Schlick)
    float HdotV = MAX(vec3_dot(H, V), 0.0f);
    vec3_t F0 = vec3_set(0.04f, 0.04f, 0.04f); // Skin IOR ~1.45
    vec3_t F_vec = vec3_sub(vec3_set(1.0f, 1.0f, 1.0f), F0);
    // float pow5 = powf(1.0f - HdotV, 5.0f); // Optimization: can use spherical gaussian approx
    float pow5 = (1.0f - HdotV);
    pow5 *= pow5 * pow5 * pow5 * pow5;
    
    // Manual lerp for vec3
    // F = F0 + (1-F0) * pow5
    vec3_t F;
    F.x = F0.x + F_vec.x * pow5;
    F.y = F0.y + F_vec.y * pow5;
    F.z = F0.z + F_vec.z * pow5;
    
    // Simplified Vis approximation
    float Vis = 0.25f; // Constant for now
    
    // Specular Term
    // spec = D * F * Vis
    vec3_t spec;
    spec.x = D * F.x * Vis * item->params.specular_strength.x;
    spec.y = D * F.y * Vis * item->params.specular_strength.y;
    spec.z = D * F.z * Vis * item->params.specular_strength.z;
    
    if (out_specular) *out_specular = spec;
    
    // Diffuse is primarily handled by SSS, but we return base albedo here
    if (out_diffuse) *out_diffuse = item->params.diffuse_albedo;
}

uint32_t shading_skin_shading_get_count(void) {
    return g_skin_shading_ctx.count;
}

size_t shading_skin_shading_get_memory_usage(void) {
    size_t total = sizeof(g_skin_shading_ctx);
    total += g_skin_shading_ctx.capacity * sizeof(shading_skin_shading_internal_t);
    return total;
}

void shading_skin_shading_debug_print(void) {
    // Debug print
}
