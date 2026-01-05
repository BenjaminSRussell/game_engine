/*
 * volumetric_fog.c
 * Volumetric fog rendering
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "volumetric_fog.h"
#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include "../../math/mat4.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_VOLUMETRIC_FOG_MAX_COUNT 16
#define PI 3.14159265359f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_volumetric_fog_data {
    // Fog parameters
    float density_global;
    float height_falloff;
    float anisotropy; // g factor for Henyey-Greenstein
    vec3_t scattering_color;
    vec3_t absorption_color;
    
    // Bounds
    float start_distance;
    float end_distance;
    
    // Render settings
    int step_count;
    bool enable_shadows;
} lighting_volumetric_fog_data_t;

typedef struct lighting_volumetric_fog_internal {
    uint32_t id;
    uint32_t flags;
    lighting_volumetric_fog_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} lighting_volumetric_fog_internal_t;

typedef struct lighting_volumetric_fog_context {
    lighting_volumetric_fog_internal_t items[LIGHTING_VOLUMETRIC_FOG_MAX_COUNT];
    uint32_t count;
    bool initialized;
} lighting_volumetric_fog_context_t;

static lighting_volumetric_fog_context_t g_volumetric_fog_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Henyey-Greenstein Phase Function
// g: anisotropy (-1 = back scattering, 0 = isotropic, 1 = forward scattering)
// cos_theta: dot(view_dir, light_dir)
static float phase_function_hg(float g, float cos_theta) {
    float g2 = g * g;
    float num = 1.0f - g2;
    float denom = 1.0f + g2 - 2.0f * g * cos_theta;
    return (1.0f / (4.0f * PI)) * (num / powf(denom, 1.5f));
}

// Schlick phase function approximation (cheaper)
static float phase_function_schlick(float k, float cos_theta) {
    float k2 = k * k;
    float num = 1.0f - k2;
    float denom = 1.0f + k * cos_theta;
    return (1.0f / (4.0f * PI)) * (num / (denom * denom));
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_volumetric_fog_init(void) {
    if (g_volumetric_fog_ctx.initialized) return 0;
    
    memset(&g_volumetric_fog_ctx, 0, sizeof(g_volumetric_fog_ctx));
    g_volumetric_fog_ctx.initialized = true;
    
    return 0;
}

void lighting_volumetric_fog_shutdown(void) {
    if (!g_volumetric_fog_ctx.initialized) return;

    for (uint32_t i = 0; i < LIGHTING_VOLUMETRIC_FOG_MAX_COUNT; i++) {
        if (g_volumetric_fog_ctx.items[i].initialized) {
            lighting_volumetric_fog_handle_t h = {i};
            lighting_volumetric_fog_destroy(h);
        }
    }
    
    g_volumetric_fog_ctx.initialized = false;
    g_volumetric_fog_ctx.count = 0;
}

int lighting_volumetric_fog_create(lighting_volumetric_fog_handle_t* out_handle, const lighting_volumetric_fog_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_volumetric_fog_ctx.initialized) return -2;
    
    int free_index = -1;
    for (int i = 0; i < LIGHTING_VOLUMETRIC_FOG_MAX_COUNT; i++) {
        if (!g_volumetric_fog_ctx.items[i].initialized) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) return -3;
    
    lighting_volumetric_fog_internal_t* item = &g_volumetric_fog_ctx.items[free_index];
    item->id = free_index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(lighting_volumetric_fog_data_t));
    if (!item->data) return -4;
    
    // Defaults
    item->data->density_global = 0.01f;
    item->data->height_falloff = 0.1f;
    item->data->anisotropy = 0.5f; // Forward scattering
    item->data->scattering_color = vec3_set(1.0f, 1.0f, 1.0f);
    item->data->absorption_color = vec3_set(0.0f, 0.0f, 0.0f);
    item->data->step_count = 64;
    item->data->start_distance = 0.1f;
    item->data->end_distance = 100.0f;
    
    item->initialized = true;
    item->dirty = true;
    g_volumetric_fog_ctx.count++;
    
    out_handle->id = free_index;
    return 0;
}

void lighting_volumetric_fog_destroy(lighting_volumetric_fog_handle_t handle) {
    if (handle.id >= LIGHTING_VOLUMETRIC_FOG_MAX_COUNT) return;
    lighting_volumetric_fog_internal_t* item = &g_volumetric_fog_ctx.items[handle.id];
    
    if (item->initialized) {
        if (item->data) free(item->data);
        item->initialized = false;
        if (g_volumetric_fog_ctx.count > 0) g_volumetric_fog_ctx.count--;
    }
}

int lighting_volumetric_fog_update(lighting_volumetric_fog_handle_t handle, const void* data, size_t size) {
    if (handle.id >= LIGHTING_VOLUMETRIC_FOG_MAX_COUNT) return -1;
    lighting_volumetric_fog_internal_t* item = &g_volumetric_fog_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    // In a real implementation we would copy the user data here
    item->dirty = true;
    return 0;
}

bool lighting_volumetric_fog_is_valid(lighting_volumetric_fog_handle_t handle) {
    if (handle.id >= LIGHTING_VOLUMETRIC_FOG_MAX_COUNT) return false;
    return g_volumetric_fog_ctx.items[handle.id].initialized;
}

int lighting_volumetric_fog_get_info(lighting_volumetric_fog_handle_t handle, lighting_volumetric_fog_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= LIGHTING_VOLUMETRIC_FOG_MAX_COUNT) return -2;
    
    lighting_volumetric_fog_internal_t* item = &g_volumetric_fog_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void lighting_volumetric_fog_mark_dirty(lighting_volumetric_fog_handle_t handle) {
    if (handle.id < LIGHTING_VOLUMETRIC_FOG_MAX_COUNT) {
        g_volumetric_fog_ctx.items[handle.id].dirty = true;
    }
}

int lighting_volumetric_fog_process_pending(void) {
    // Process dirty items
    return 0;
}

// Core computational function for volumetric scattering
// This would typcially be running in a compute shader, but we provide the C implementation reference
void lighting_volumetric_fog_compute_scattering(
    const lighting_volumetric_fog_data_t* fog_data,
    vec3_t view_pos,
    vec3_t view_dir,
    vec3_t light_dir,
    vec3_t light_color,
    float* out_scattering,
    float* out_transmittance
) {
    // Ray march setup
    vec3_t current_pos = view_pos;
    float step_size = (fog_data->end_distance - fog_data->start_distance) / (float)fog_data->step_count;
    vec3_t step_vec = vec3_set(view_dir.x * step_size, view_dir.y * step_size, view_dir.z * step_size);
    
    float optical_depth = 0.0f;
    float scattering_accum = 0.0f;
    
    // Calculate phase function once
    float cos_theta = vec3_dot(view_dir, light_dir);
    float phase = phase_function_hg(fog_data->anisotropy, cos_theta);
    
    for (int i = 0; i < fog_data->step_count; i++) {
        // Sample density at current_pos
        // Simple height-based fog
        float height = current_pos.y;
        float density = fog_data->density_global * expf(-height * fog_data->height_falloff);
        
        if (density > 0.0f) {
            float step_optical_depth = density * step_size;
            optical_depth += step_optical_depth;
            
            // Transmittance = exp(-optical_depth)
            float transmittance = expf(-optical_depth);
            
            // In-scattering
            // Lin(x) = L * phase * density * transmittance
            scattering_accum += density * phase * transmittance * step_size;
        }
        
        current_pos = vec3_add(current_pos, step_vec);
    }
    
    if (out_scattering) *out_scattering = scattering_accum;
    if (out_transmittance) *out_transmittance = expf(-optical_depth);
}

uint32_t lighting_volumetric_fog_get_count(void) {
    return g_volumetric_fog_ctx.count;
}

size_t lighting_volumetric_fog_get_memory_usage(void) {
    size_t total = sizeof(lighting_volumetric_fog_context_t);
    for (uint32_t i = 0; i < LIGHTING_VOLUMETRIC_FOG_MAX_COUNT; i++) {
        if (g_volumetric_fog_ctx.items[i].initialized && g_volumetric_fog_ctx.items[i].data) {
            total += sizeof(lighting_volumetric_fog_data_t);
        }
    }
    return total;
}

void lighting_volumetric_fog_debug_print(void) {
    // Debug implementation
}
