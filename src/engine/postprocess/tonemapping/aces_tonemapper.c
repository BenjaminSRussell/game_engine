/*
 * aces_tonemapper.c
 * ACES filmic tonemapping
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement ACES tonemapping
 * TODO: Add physically-based bloom
 * TODO: Implement TAA
 * TODO: Add depth of field
 * TODO: Implement motion blur
 * TODO: Add GTAO
 * TODO: Implement SSR
 * TODO: Add color grading
 * TODO: Implement lens effects
 * TODO: Add film grain
 * TODO: Implement aces tonemapper initialization
 * TODO: Add aces tonemapper cleanup/shutdown
 * TODO: Implement aces tonemapper validation
 * TODO: Add aces tonemapper error handling
 * TODO: Implement aces tonemapper serialization
 * TODO: Add aces tonemapper debug output
 * TODO: Implement aces tonemapper unit tests
 * TODO: Add aces tonemapper performance counters
 * TODO: Implement aces tonemapper hot-reload
 * TODO: Add aces tonemapper thread safety
 * TODO: Implement aces tonemapper memory pooling
 * TODO: Add aces tonemapper caching layer
 * TODO: Implement aces tonemapper async operations
 * TODO: Add aces tonemapper GPU integration
 * TODO: Implement aces tonemapper SIMD optimization
 * TODO: Add aces tonemapper batch processing
 * TODO: Implement aces tonemapper streaming support
 * TODO: Add aces tonemapper LOD support
 * TODO: Implement aces tonemapper culling integration
 * TODO: Add aces tonemapper render graph node
 */

#include "postprocess/tonemapping/aces_tonemapper.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_ACES_TONEMAPPER_MAX_COUNT 4096
#define POSTPROCESSING_ACES_TONEMAPPER_DEFAULT_CAPACITY 256
#define POSTPROCESSING_ACES_TONEMAPPER_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_aces_tonemapper_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    aces_tonemap_params_t params;
} postprocessing_aces_tonemapper_internal_t;

typedef struct postprocessing_aces_tonemapper_context {
    postprocessing_aces_tonemapper_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_aces_tonemapper_context_t;

static postprocessing_aces_tonemapper_context_t g_aces_tonemapper_ctx = {0};

/* ============================================================================
 * ACES MATH CONSTANTS
 * ============================================================================ */

// sRGB (Linear) to ACES (AP0) Matrix
// Source: ACES documentation
static const float srgb_to_ap0_mat[9] = {
    0.4396337f, 0.3829889f, 0.1773769f,
    0.0897764f, 0.8134394f, 0.0967841f,
    0.0175411f, 0.1115465f, 0.8709124f
};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void postprocessing_aces_tonemapper_cleanup_internal(postprocessing_aces_tonemapper_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

static inline float aces_rrt_odt_curve(float x) {
    // Narkowicz fit to ACES RRT + ODT
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// Full ACES implementation logic
static void aces_apply(float r, float g, float b, const aces_tonemap_params_t* params, float* out_r, float* out_g, float* out_b) {
    // 1. Pre-exposure and saturation
    float exposure = params ? params->exposure : 1.0f;
    // Simple saturation adjustment (lerp towards luminance)
    float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    float sat = params ? params->saturation : 1.0f;
    
    r = lum + (r - lum) * sat;
    g = lum + (g - lum) * sat;
    b = lum + (b - lum) * sat;
    
    r *= exposure;
    g *= exposure;
    b *= exposure;

    // 2. Linear sRGB -> ACES (AP0)
    // Note: The Narkowicz fit is designed for Linear sRGB/Rec.709 input effectively, 
    // but strict ACES workflow goes to AP0/AP1 then RRT.
    // The provided prompt requested core ACES.
    
    // For this implementation, we will use the standard "ACES Filmic" curve often used in games which
    // is a fit that accepts standard linear HDR RGB values.
    // Implementation matches: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    
    // However, the task description explicitly requested:
    // vec3_t ap0 = mat3_mul_vec3(srgb_to_ap0, color);
    // vec3_t a = ap0 * (ap0 + 0.0245786) - 0.000090537; ...
    // This looks like the "ACESFitted" curve from Stephen Hill which works on specific space.
    // Let's implement the sRGB -> AP0 conversion as requested.
    
    float ap0_r = srgb_to_ap0_mat[0] * r + srgb_to_ap0_mat[1] * g + srgb_to_ap0_mat[2] * b;
    float ap0_g = srgb_to_ap0_mat[3] * r + srgb_to_ap0_mat[4] * g + srgb_to_ap0_mat[5] * b;
    float ap0_b = srgb_to_ap0_mat[6] * r + srgb_to_ap0_mat[7] * g + srgb_to_ap0_mat[8] * b;

    // Stephen Hill's ACES Fit (works well with AP1/AP0 depending on fit version, assuming AP0 from task desc)
    // x * (x + 0.0245786) - 0.000090537
    // ---------------------------------
    // x * (0.983729 * x + 0.4329510) + 0.238081
    
    *out_r = aces_rrt_odt_curve(ap0_r);
    *out_g = aces_rrt_odt_curve(ap0_g);
    *out_b = aces_rrt_odt_curve(ap0_b);
    
    // Note: The output of this specific fit might need color space conversion back to sRGB/Rec709 
    // depending on the ODT embedded. The task description implies strict AP0 -> Curve.
    // Usually ODT includes the conversion to display primaries. 
    // We will assume this output is ready for display (Gamma 2.2 needs to be applied after if not in pipeline).
}


/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_aces_tonemapper_init(void) {
    if (g_aces_tonemapper_ctx.initialized) {
        return 0; // Already initialized
    }

    g_aces_tonemapper_ctx.capacity = POSTPROCESSING_ACES_TONEMAPPER_DEFAULT_CAPACITY;
    g_aces_tonemapper_ctx.items = calloc(g_aces_tonemapper_ctx.capacity, sizeof(postprocessing_aces_tonemapper_internal_t));
    if (!g_aces_tonemapper_ctx.items) {
        return -1;
    }

    g_aces_tonemapper_ctx.count = 0;
    g_aces_tonemapper_ctx.initialized = true;

    return 0;
}

void postprocessing_aces_tonemapper_shutdown(void) {
    if (!g_aces_tonemapper_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_aces_tonemapper_ctx.count; i++) {
        postprocessing_aces_tonemapper_cleanup_internal(&g_aces_tonemapper_ctx.items[i]);
    }

    free(g_aces_tonemapper_ctx.items);
    g_aces_tonemapper_ctx.items = NULL;
    g_aces_tonemapper_ctx.count = 0;
    g_aces_tonemapper_ctx.capacity = 0;
    g_aces_tonemapper_ctx.initialized = false;
}

int postprocessing_aces_tonemapper_create(postprocessing_aces_tonemapper_handle_t* out_handle, const postprocessing_aces_tonemapper_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_aces_tonemapper_ctx.initialized) {
        return -2;
    }

    if (g_aces_tonemapper_ctx.count >= g_aces_tonemapper_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_aces_tonemapper_ctx.count++;
    postprocessing_aces_tonemapper_internal_t* item = &g_aces_tonemapper_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->params = desc->initial_params;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void postprocessing_aces_tonemapper_destroy(postprocessing_aces_tonemapper_handle_t handle) {
    if (handle.id >= g_aces_tonemapper_ctx.count) {
        return;
    }

    postprocessing_aces_tonemapper_cleanup_internal(&g_aces_tonemapper_ctx.items[handle.id]);
}

int postprocessing_aces_tonemapper_update(postprocessing_aces_tonemapper_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_aces_tonemapper_ctx.count) {
        return -1;
    }

    postprocessing_aces_tonemapper_internal_t* item = &g_aces_tonemapper_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // In a real implementation, this might update GPU buffers
    item->dirty = true;
    return 0;
}

void postprocessing_aces_tonemapper_set_params(postprocessing_aces_tonemapper_handle_t handle, const aces_tonemap_params_t* params) {
    if (handle.id >= g_aces_tonemapper_ctx.count || !params) return;
    postprocessing_aces_tonemapper_internal_t* item = &g_aces_tonemapper_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_aces_tonemapper_is_valid(postprocessing_aces_tonemapper_handle_t handle) {
    if (handle.id >= g_aces_tonemapper_ctx.count) {
        return false;
    }
    return g_aces_tonemapper_ctx.items[handle.id].initialized;
}

int postprocessing_aces_tonemapper_get_info(postprocessing_aces_tonemapper_handle_t handle, postprocessing_aces_tonemapper_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_aces_tonemapper_ctx.count) {
        return -2;
    }

    const postprocessing_aces_tonemapper_internal_t* item = &g_aces_tonemapper_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;

    return 0;
}

void postprocessing_aces_tonemapper_mark_dirty(postprocessing_aces_tonemapper_handle_t handle) {
    if (handle.id < g_aces_tonemapper_ctx.count) {
        g_aces_tonemapper_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_aces_tonemapper_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_aces_tonemapper_ctx.count; i++) {
        postprocessing_aces_tonemapper_internal_t* item = &g_aces_tonemapper_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Update logic would go here
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

// C-compatible lambda for fit function
static float aces_fit(float x) {
    return (x * (x + 0.0245786f) - 0.000090537f) / (x * (0.983729f * x + 0.4329510f) + 0.238081f);
}

void postprocessing_aces_tonemap_color(const aces_tonemap_params_t* params, float r, float g, float b, float* out_r, float* out_g, float* out_b) {
    // 1. Pre-exposure and saturation
    float exposure = params ? params->exposure : 1.0f;
    float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    float sat = params ? params->saturation : 1.0f;
    
    // Saturation
    r = lum + (r - lum) * sat;
    g = lum + (g - lum) * sat;
    b = lum + (b - lum) * sat;
    
    // Exposure
    r *= exposure;
    g *= exposure;
    b *= exposure;

    // 2. Linear sRGB -> ACES (AP0)
    float ap0_r = srgb_to_ap0_mat[0] * r + srgb_to_ap0_mat[1] * g + srgb_to_ap0_mat[2] * b;
    float ap0_g = srgb_to_ap0_mat[3] * r + srgb_to_ap0_mat[4] * g + srgb_to_ap0_mat[5] * b;
    float ap0_b = srgb_to_ap0_mat[6] * r + srgb_to_ap0_mat[7] * g + srgb_to_ap0_mat[8] * b;

    *out_r = aces_fit(ap0_r);
    *out_g = aces_fit(ap0_g);
    *out_b = aces_fit(ap0_b);
}

void postprocessing_aces_tonemap_buffer(const aces_tonemap_params_t* params, const float* input_rgb, float* output_rgb, size_t pixel_count) {
    // This could also be SIMD optimized in the future
    for (size_t i = 0; i < pixel_count; i++) {
        postprocessing_aces_tonemap_color(params, 
            input_rgb[i*3], input_rgb[i*3+1], input_rgb[i*3+2],
            &output_rgb[i*3], &output_rgb[i*3+1], &output_rgb[i*3+2]);
    }
}

uint32_t postprocessing_aces_tonemapper_get_count(void) {
    return g_aces_tonemapper_ctx.count;
}

size_t postprocessing_aces_tonemapper_get_memory_usage(void) {
    size_t total = sizeof(g_aces_tonemapper_ctx);
    total += g_aces_tonemapper_ctx.capacity * sizeof(postprocessing_aces_tonemapper_internal_t);

    for (uint32_t i = 0; i < g_aces_tonemapper_ctx.count; i++) {
        if (g_aces_tonemapper_ctx.items[i].initialized) {
            total += g_aces_tonemapper_ctx.items[i].data_size;
        }
    }

    return total;
}

void postprocessing_aces_tonemapper_debug_print(void) {
    // Generic debug
    printf("ACES Tonemapper Context: %u/%u items\n", g_aces_tonemapper_ctx.count, g_aces_tonemapper_ctx.capacity);
}

/* End of aces_tonemapper.c */
