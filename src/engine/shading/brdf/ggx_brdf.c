#include "shading/brdf/ggx_brdf.h"
#include <math/vec3.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <include/math/math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/* ============================================================================
 * GGX MATH FUNCTIONS
 * ============================================================================ */

// Trowbridge-Reitz GGX Normal Distribution Function (D)
// ndoth: Dot product of Normal and Halfway vector
// a2: Roughness squared
float ggx_distribution(float ndoth, float a2) {
    float d = (ndoth * ndoth) * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * d * d);
}

// Smith Joint Height-Correlated Masking-Shadowing Function (G)
// ndotv: Dot product of Normal and View vector
// ndotl: Dot product of Normal and Light vector
// a2: Roughness squared
float ggx_visibility(float ndotv, float ndotl, float a2) {
    float gv = ndotl * sqrtf(a2 + (1.0f - a2) * ndotv * ndotv);
    float gl = ndotv * sqrtf(a2 + (1.0f - a2) * ndotl * ndotl);
    return 0.5f / (gv + gl);
}

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct shading_ggx_brdf_internal {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_ggx_brdf_internal_t;

typedef struct shading_ggx_brdf_context {
    shading_ggx_brdf_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_ggx_brdf_context_t;

static shading_ggx_brdf_context_t g_ggx_brdf_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_ggx_brdf_init(void) {
    if (g_ggx_brdf_ctx.initialized) {
        return 0;
    }

    g_ggx_brdf_ctx.capacity = 256;
    g_ggx_brdf_ctx.items = calloc(g_ggx_brdf_ctx.capacity, sizeof(shading_ggx_brdf_internal_t));
    if (!g_ggx_brdf_ctx.items) {
        return -1;
    }

    g_ggx_brdf_ctx.count = 0;
    g_ggx_brdf_ctx.initialized = true;

    return 0;
}

void shading_ggx_brdf_shutdown(void) {
    if (!g_ggx_brdf_ctx.initialized) {
        return;
    }

    free(g_ggx_brdf_ctx.items);
    g_ggx_brdf_ctx.items = NULL;
    g_ggx_brdf_ctx.count = 0;
    g_ggx_brdf_ctx.capacity = 0;
    g_ggx_brdf_ctx.initialized = false;
}

int shading_ggx_brdf_create(shading_ggx_brdf_handle_t* out_handle, const shading_ggx_brdf_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ggx_brdf_ctx.initialized) {
        return -2;
    }

    if (g_ggx_brdf_ctx.count >= g_ggx_brdf_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_ggx_brdf_ctx.count++;
    shading_ggx_brdf_internal_t* item = &g_ggx_brdf_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;

    out_handle->id = index;
    return 0;
}

void shading_ggx_brdf_destroy(shading_ggx_brdf_handle_t handle) {
    if (handle.id >= g_ggx_brdf_ctx.count) {
        return;
    }
    g_ggx_brdf_ctx.items[handle.id].initialized = false;
}

int shading_ggx_brdf_update(shading_ggx_brdf_handle_t handle, const void* data, size_t size) {
    return 0;
}

bool shading_ggx_brdf_is_valid(shading_ggx_brdf_handle_t handle) {
    if (handle.id >= g_ggx_brdf_ctx.count) {
        return false;
    }
    return g_ggx_brdf_ctx.items[handle.id].initialized;
}

int shading_ggx_brdf_get_info(shading_ggx_brdf_handle_t handle, shading_ggx_brdf_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    if (handle.id >= g_ggx_brdf_ctx.count) {
        return -2;
    }
    const shading_ggx_brdf_internal_t* item = &g_ggx_brdf_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void shading_ggx_brdf_mark_dirty(shading_ggx_brdf_handle_t handle) {
}

int shading_ggx_brdf_process_pending(void) {
    return 0;
}

uint32_t shading_ggx_brdf_get_count(void) {
    return g_ggx_brdf_ctx.count;
}

size_t shading_ggx_brdf_get_memory_usage(void) {
    return sizeof(g_ggx_brdf_ctx) + g_ggx_brdf_ctx.capacity * sizeof(shading_ggx_brdf_internal_t);
}

void shading_ggx_brdf_debug_print(void) {
    printf("GGX BRDF Stats:\n");
    printf("  Count: %u\n", g_ggx_brdf_ctx.count);
}
