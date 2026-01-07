#include "shading/brdf/fresnel.h"
#include "shading/brdf/energy_conservation.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <include/math/math.h>

/* ============================================================================
 * ENERGY CONSERVATION MATH FUNCTIONS
 * ============================================================================ */

// Multi-scatter compensation factor (Kulla-Conty)
// e: Directional albedo (integral of BRDF over the hemisphere)
// This is typically precomputed in a LUT, but we can provide a fitted approximation.
float multi_scatter_compensation(float e, float roughness) {
    // Basic approximation to recover energy lost by GGX single scatter
    return 1.0f + (1.0f - e) * roughness;
}

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct shading_energy_conservation_internal {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_energy_conservation_internal_t;

typedef struct shading_energy_conservation_context {
    shading_energy_conservation_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_energy_conservation_context_t;

static shading_energy_conservation_context_t g_energy_conservation_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_energy_conservation_init(void) {
    if (g_energy_conservation_ctx.initialized) {
        return 0;
    }

    g_energy_conservation_ctx.capacity = 256;
    g_energy_conservation_ctx.items = calloc(g_energy_conservation_ctx.capacity, sizeof(shading_energy_conservation_internal_t));
    if (!g_energy_conservation_ctx.items) {
        return -1;
    }

    g_energy_conservation_ctx.count = 0;
    g_energy_conservation_ctx.initialized = true;

    return 0;
}

void shading_energy_conservation_shutdown(void) {
    if (!g_energy_conservation_ctx.initialized) {
        return;
    }

    free(g_energy_conservation_ctx.items);
    g_energy_conservation_ctx.items = NULL;
    g_energy_conservation_ctx.count = 0;
    g_energy_conservation_ctx.capacity = 0;
    g_energy_conservation_ctx.initialized = false;
}

int shading_energy_conservation_create(shading_energy_conservation_handle_t* out_handle, const shading_energy_conservation_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_energy_conservation_ctx.initialized) {
        return -2;
    }

    if (g_energy_conservation_ctx.count >= g_energy_conservation_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_energy_conservation_ctx.count++;
    shading_energy_conservation_internal_t* item = &g_energy_conservation_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;

    out_handle->id = index;
    return 0;
}

void shading_energy_conservation_destroy(shading_energy_conservation_handle_t handle) {
    if (handle.id >= g_energy_conservation_ctx.count) {
        return;
    }
    g_energy_conservation_ctx.items[handle.id].initialized = false;
}

int shading_energy_conservation_update(shading_energy_conservation_handle_t handle, const void* data, size_t size) {
    return 0;
}

bool shading_energy_conservation_is_valid(shading_energy_conservation_handle_t handle) {
    if (handle.id >= g_energy_conservation_ctx.count) {
        return false;
    }
    return g_energy_conservation_ctx.items[handle.id].initialized;
}

int shading_energy_conservation_get_info(shading_energy_conservation_handle_t handle, shading_energy_conservation_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_energy_conservation_ctx.count) {
        return -2;
    }

    const shading_energy_conservation_internal_t* item = &g_energy_conservation_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void shading_energy_conservation_mark_dirty(shading_energy_conservation_handle_t handle) {
}

int shading_energy_conservation_process_pending(void) {
    return 0;
}

uint32_t shading_energy_conservation_get_count(void) {
    return g_energy_conservation_ctx.count;
}

size_t shading_energy_conservation_get_memory_usage(void) {
    return sizeof(g_energy_conservation_ctx) + g_energy_conservation_ctx.capacity * sizeof(shading_energy_conservation_internal_t);
}

void shading_energy_conservation_debug_print(void) {
    printf("Energy Conservation Stats:\n");
    printf("  Count: %u\n", g_energy_conservation_ctx.count);
}
