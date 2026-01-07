/*
 * cloth_brdf.c
 * Cloth shading BRDF implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "shading/cloth/cloth_brdf.h"
#include "include/math/vec3.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_CLOTH_BRDF_MAX_COUNT 64
#define SHADING_CLOTH_BRDF_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_params {
    vec3_t albedo;
    vec3_t sheen_color;
    float roughness;
    float subsurface_color; // Approximation for fabric transmission
} cloth_params_t;

typedef struct shading_cloth_brdf_internal {
    uint32_t id;
    uint32_t flags;
    cloth_params_t params;
    bool initialized;
    bool dirty;
} shading_cloth_brdf_internal_t;

typedef struct shading_cloth_brdf_context {
    shading_cloth_brdf_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_cloth_brdf_context_t;

static shading_cloth_brdf_context_t g_cloth_brdf_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Charlie distribution for cloth/sheen
// D(m) = (2 + 1/a) * sin(theta)^ (1/a) / 2pi
static float distribution_charlie(float roughness, float NdotH) {
    float inv_r = 1.0f / (roughness * roughness + EPSILON); // Convert perceptual roughness
    float cos2h = NdotH * NdotH;
    float sin2h = 1.0f - cos2h;
    if (sin2h < 0.0f) sin2h = 0.0f;
    
    return (2.0f + inv_r) * powf(sin2h, inv_r * 0.5f) / (2.0f * PI);
}

// Ashikhmin visibility term (often used with Charlie)
static float visibility_ashikhmin(float NdotL, float NdotV) {
    return 1.0f / (4.0f * (NdotL + NdotV - NdotL * NdotV));
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_cloth_brdf_init(void) {
    if (g_cloth_brdf_ctx.initialized) return 0;

    g_cloth_brdf_ctx.capacity = SHADING_CLOTH_BRDF_DEFAULT_CAPACITY;
    g_cloth_brdf_ctx.items = calloc(g_cloth_brdf_ctx.capacity, sizeof(shading_cloth_brdf_internal_t));
    
    if (!g_cloth_brdf_ctx.items) return -1;
    
    g_cloth_brdf_ctx.count = 0;
    g_cloth_brdf_ctx.initialized = true;
    
    return 0;
}

void shading_cloth_brdf_shutdown(void) {
    if (!g_cloth_brdf_ctx.initialized) return;
    
    free(g_cloth_brdf_ctx.items);
    g_cloth_brdf_ctx.items = NULL;
    g_cloth_brdf_ctx.count = 0;
    g_cloth_brdf_ctx.capacity = 0;
    g_cloth_brdf_ctx.initialized = false;
}

int shading_cloth_brdf_create(shading_cloth_brdf_handle_t* out_handle, const shading_cloth_brdf_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_cloth_brdf_ctx.initialized) return -2;
    
    if (g_cloth_brdf_ctx.count >= g_cloth_brdf_ctx.capacity) {
        uint32_t new_capacity = g_cloth_brdf_ctx.capacity * 2;
        if (new_capacity > SHADING_CLOTH_BRDF_MAX_COUNT) new_capacity = SHADING_CLOTH_BRDF_MAX_COUNT;
        
        if (new_capacity == g_cloth_brdf_ctx.capacity) return -3;
        
        void* new_items = realloc(g_cloth_brdf_ctx.items, new_capacity * sizeof(shading_cloth_brdf_internal_t));
        if (!new_items) return -4;
        
        g_cloth_brdf_ctx.items = new_items;
        g_cloth_brdf_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_cloth_brdf_ctx.count++;
    shading_cloth_brdf_internal_t* item = &g_cloth_brdf_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default velvet values
    item->params.albedo = vec3_set(0.5f, 0.0f, 0.0f); // Red velvet
    item->params.sheen_color = vec3_set(1.0f, 0.8f, 0.8f);
    item->params.roughness = 0.8f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_cloth_brdf_destroy(shading_cloth_brdf_handle_t handle) {
    if (handle.id >= g_cloth_brdf_ctx.count) return;
    g_cloth_brdf_ctx.items[handle.id].initialized = false;
}

int shading_cloth_brdf_update(shading_cloth_brdf_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_cloth_brdf_ctx.count) return -1;
    
    shading_cloth_brdf_internal_t* item = &g_cloth_brdf_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(cloth_params_t)) {
        memcpy(&item->params, data, sizeof(cloth_params_t));
        item->dirty = true;
    }
    
    return 0;
}

bool shading_cloth_brdf_is_valid(shading_cloth_brdf_handle_t handle) {
    if (handle.id >= g_cloth_brdf_ctx.count) return false;
    return g_cloth_brdf_ctx.items[handle.id].initialized;
}

// Evaluate Cloth BRDF (Charlie Sheen)
void evaluate_cloth_brdf(shading_cloth_brdf_handle_t handle,
                        vec3_t N, vec3_t V, vec3_t L,
                        vec3_t* out_color) {
    
    if (!g_cloth_brdf_ctx.initialized || handle.id >= g_cloth_brdf_ctx.count) return;
    shading_cloth_brdf_internal_t* item = &g_cloth_brdf_ctx.items[handle.id];
    
    vec3_t H = vec3_normalize(vec3_add(V, L));
    
    float NdotL = MAX(vec3_dot(N, L), 0.0f);
    float NdotV = MAX(vec3_dot(N, V), 0.0f);
    float NdotH = MAX(vec3_dot(N, H), 0.0f);
    
    // Diffuse term (Lambert for simplicity, could be Disney diffuse)
    // float diffuse = NdotL / PI;
    // vec3_t diffuse_term = vec3_mul_scalar(item->params.albedo, diffuse);
    // Wait, I don't have vec3_mul implementation handy, doing component-wise
    
    float diffuse_factor = NdotL / PI;
    vec3_t diffuse_term;
    diffuse_term.x = item->params.albedo.x * diffuse_factor;
    diffuse_term.y = item->params.albedo.y * diffuse_factor;
    diffuse_term.z = item->params.albedo.z * diffuse_factor;
    
    // Specular / Sheen term (Charlie)
    float D = distribution_charlie(item->params.roughness, NdotH);
    float Vis = visibility_ashikhmin(NdotL, NdotV);
    
    // F is usually just sheen color for cloth, no Fresnel (or Schlick with sheen color)
    vec3_t F = item->params.sheen_color; 
    
    // Specular = (D * Vis * F) * NdotL? Vis includes NdotL cancellation usually depending on formula,
    // Standard cooked: (D * F * G) / (4 * NdotL * NdotV)
    // Ashikhmin Vis approximates G / (4 * NdotL * NdotV)
    
    float spec_factor = D * Vis * NdotL; // Applying NdotL at end for lighting integral
    
    vec3_t spec_term;
    spec_term.x = F.x * spec_factor;
    spec_term.y = F.y * spec_factor;
    spec_term.z = F.z * spec_factor;
    
    // Combine
    if (out_color) {
        out_color->x = diffuse_term.x + spec_term.x;
        out_color->y = diffuse_term.y + spec_term.y;
        out_color->z = diffuse_term.z + spec_term.z;
    }
}

uint32_t shading_cloth_brdf_get_count(void) {
    return g_cloth_brdf_ctx.count;
}

size_t shading_cloth_brdf_get_memory_usage(void) {
    size_t total = sizeof(g_cloth_brdf_ctx);
    total += g_cloth_brdf_ctx.capacity * sizeof(shading_cloth_brdf_internal_t);
    return total;
}

void shading_cloth_brdf_debug_print(void) {
    // Debug print
}
