/*
 * gi_probe_grid.c
 * Irradiance probe grid
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement clustered light culling
 * TODO: Add ray-traced shadows
 * TODO: Implement cascaded shadow maps
 * TODO: Add area light support
 * TODO: Implement global illumination
 * TODO: Add volumetric lighting
 * TODO: Implement light probes
 * TODO: Add IES profile support
 * TODO: Implement lightmap baking
 * TODO: Add real-time GI
 * TODO: Implement gi probe grid initialization
 * TODO: Add gi probe grid cleanup/shutdown
 * TODO: Implement gi probe grid validation
 * TODO: Add gi probe grid error handling
 * TODO: Implement gi probe grid serialization
 * TODO: Add gi probe grid debug output
 * TODO: Implement gi probe grid unit tests
 * TODO: Add gi probe grid performance counters
 * TODO: Implement gi probe grid hot-reload
 * TODO: Add gi probe grid thread safety
 * TODO: Implement gi probe grid memory pooling
 * TODO: Add gi probe grid caching layer
 * TODO: Implement gi probe grid async operations
 * TODO: Add gi probe grid GPU integration
 * TODO: Implement gi probe grid SIMD optimization
 * TODO: Add gi probe grid batch processing
 * TODO: Implement gi probe grid streaming support
 * TODO: Add gi probe grid LOD support
 * TODO: Implement gi probe grid culling integration
 * TODO: Add gi probe grid render graph node
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "gi_probe_grid.h"
#include "../../../../include/common.h"
#include "../../../../include/core/types.h"
#include "../../../../include/math/vec3.h"
#include "../../../../include/math/math.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_GI_PROBE_GRID_MAX_COUNT 4096
#define LIGHTING_GI_PROBE_GRID_DEFAULT_CAPACITY 256
#define LIGHTING_GI_PROBE_GRID_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ivec3 {
    i32 x, y, z;
} ivec3_t;

typedef struct irradiance_probe {
    Vec3 position;
    Vec3 sh_coefficients[9];  // L0, L1, L2 SH bands
} irradiance_probe_t;

typedef struct probe_grid_data {
    irradiance_probe_t* probes;
    ivec3_t resolution;
    Vec3 min_bounds;
    Vec3 max_bounds;
    Vec3 cell_size;
} probe_grid_data_t;

typedef struct lighting_gi_probe_grid_internal {
    uint32_t id;
    uint32_t flags;
    probe_grid_data_t grid;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} lighting_gi_probe_grid_internal_t;

typedef struct lighting_gi_probe_grid_context {
    lighting_gi_probe_grid_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} lighting_gi_probe_grid_context_t;

static lighting_gi_probe_grid_context_t g_gi_probe_grid_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static INLINE f32 evaluate_sh(const Vec3* coefficients, Vec3 normal) {
    // SH L0, L1, L2 evaluation
    // SH constants
    const f32 SH_C0 = 0.28209479177387814f; // 1 / (2 * sqrt(PI))
    const f32 SH_C1 = 0.4886025119029199f;  // sqrt(3) / (2 * sqrt(PI))
    const f32 SH_C2 = 1.0925484305920792f;  // sqrt(15) / (2 * sqrt(PI))
    const f32 SH_C3 = 0.31539156525252005f; // sqrt(5) / (4 * sqrt(PI))
    const f32 SH_C4 = 0.5462742152960396f;  // sqrt(15) / (4 * sqrt(PI))

    f32 x = normal.x;
    f32 y = normal.y;
    f32 z = normal.z;

    // L0
    Vec3 irradiance = vec3_mul(coefficients[0], SH_C0);

    // L1
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[1], SH_C1 * y));
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[2], SH_C1 * z));
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[3], SH_C1 * x));

    // L2
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[4], SH_C2 * x * y));
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[5], SH_C2 * y * z));
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[6], SH_C3 * (3.0f * z * z - 1.0f)));
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[7], SH_C2 * x * z));
    irradiance = vec3_add(irradiance, vec3_mul(coefficients[8], SH_C4 * (x * x - y * y)));

    // Return average irradiance or could return Vec3 (modify signature if needed)
    // For now, let's keep it consistent with the "evaluate_sh" name
    return (irradiance.x + irradiance.y + irradiance.z) / 3.0f; 
}

// Fixed SH evaluation returning Vec3
static INLINE Vec3 evaluate_sh_vec3(const Vec3* coefficients, Vec3 normal) {
    const f32 SH_C0 = 0.28209479177387814f;
    const f32 SH_C1 = 0.4886025119029199f;
    const f32 SH_C2 = 1.0925484305920792f;
    const f32 SH_C3 = 0.31539156525252005f;
    const f32 SH_C4 = 0.5462742152960396f;

    f32 x = normal.x;
    f32 y = normal.y;
    f32 z = normal.z;

    Vec3 res = vec3_mul(coefficients[0], SH_C0);
    res = vec3_add(res, vec3_mul(coefficients[1], SH_C1 * y));
    res = vec3_add(res, vec3_mul(coefficients[2], SH_C1 * z));
    res = vec3_add(res, vec3_mul(coefficients[3], SH_C1 * x));
    res = vec3_add(res, vec3_mul(coefficients[4], SH_C2 * x * y));
    res = vec3_add(res, vec3_mul(coefficients[5], SH_C2 * y * z));
    res = vec3_add(res, vec3_mul(coefficients[6], SH_C3 * (3.0f * z * z - 1.0f)));
    res = vec3_add(res, vec3_mul(coefficients[7], SH_C2 * x * z));
    res = vec3_add(res, vec3_mul(coefficients[8], SH_C4 * (x * x - y * y)));

    return res;
}

static INLINE uint32_t get_probe_index(const probe_grid_data_t* grid, ivec3_t pos) {
    if (pos.x < 0 || pos.x >= grid->resolution.x ||
        pos.y < 0 || pos.y >= grid->resolution.y ||
        pos.z < 0 || pos.z >= grid->resolution.z) {
        return 0; // Out of bounds fallback
    }
    return pos.z * (grid->resolution.x * grid->resolution.y) + pos.y * grid->resolution.x + pos.x;
}

static bool lighting_gi_probe_grid_validate(const lighting_gi_probe_grid_internal_t* item) {
    // TODO: Implement clustered light culling
    // TODO: Add ray-traced shadows
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void lighting_gi_probe_grid_cleanup_internal(lighting_gi_probe_grid_internal_t* item) {
    if (!item) return;
    if (item->grid.probes) {
        free(item->grid.probes);
        item->grid.probes = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_gi_probe_grid_init(void) {
    // TODO: Implement global illumination
    // TODO: Add volumetric lighting
    // TODO: Implement light probes
    // TODO: Add IES profile support

    if (g_gi_probe_grid_ctx.initialized) {
        return 0; // Already initialized
    }

    g_gi_probe_grid_ctx.capacity = LIGHTING_GI_PROBE_GRID_DEFAULT_CAPACITY;
    g_gi_probe_grid_ctx.items = calloc(g_gi_probe_grid_ctx.capacity, sizeof(lighting_gi_probe_grid_internal_t));
    if (!g_gi_probe_grid_ctx.items) {
        return -1;
    }

    g_gi_probe_grid_ctx.count = 0;
    g_gi_probe_grid_ctx.initialized = true;

    return 0;
}

void lighting_gi_probe_grid_shutdown(void) {
    // TODO: Implement lightmap baking
    // TODO: Add real-time GI
    // TODO: Implement gi probe grid initialization
    // TODO: Add gi probe grid cleanup/shutdown

    if (!g_gi_probe_grid_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gi_probe_grid_ctx.count; i++) {
        lighting_gi_probe_grid_cleanup_internal(&g_gi_probe_grid_ctx.items[i]);
    }

    free(g_gi_probe_grid_ctx.items);
    g_gi_probe_grid_ctx.items = NULL;
    g_gi_probe_grid_ctx.count = 0;
    g_gi_probe_grid_ctx.capacity = 0;
    g_gi_probe_grid_ctx.initialized = false;
}

int lighting_gi_probe_grid_create(lighting_gi_probe_grid_handle_t* out_handle, const lighting_gi_probe_grid_desc_t* desc) {
    // TODO: Implement gi probe grid validation
    // TODO: Add gi probe grid error handling
    // TODO: Implement gi probe grid serialization
    // TODO: Add gi probe grid debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gi_probe_grid_ctx.initialized) {
        return -2;
    }

    if (g_gi_probe_grid_ctx.count >= g_gi_probe_grid_ctx.capacity) {
        // TODO: Implement gi probe grid unit tests
        return -3;
    }

    uint32_t index = g_gi_probe_grid_ctx.count++;
    lighting_gi_probe_grid_internal_t* item = &g_gi_probe_grid_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->grid, 0, sizeof(probe_grid_data_t));
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void lighting_gi_probe_grid_destroy(lighting_gi_probe_grid_handle_t handle) {
    // TODO: Add gi probe grid performance counters
    // TODO: Implement gi probe grid hot-reload

    if (handle.id >= g_gi_probe_grid_ctx.count) {
        return;
    }

    lighting_gi_probe_grid_cleanup_internal(&g_gi_probe_grid_ctx.items[handle.id]);
}

int lighting_gi_probe_grid_update(lighting_gi_probe_grid_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_gi_probe_grid_ctx.count) {
        return -1;
    }

    lighting_gi_probe_grid_internal_t* item = &g_gi_probe_grid_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (data && size == sizeof(probe_grid_data_t)) {
        memcpy(&item->grid, data, size);
    }

    item->dirty = true;
    return 0;
}

bool lighting_gi_probe_grid_is_valid(lighting_gi_probe_grid_handle_t handle) {
    // TODO: Add gi probe grid batch processing
    if (handle.id >= g_gi_probe_grid_ctx.count) {
        return false;
    }
    return g_gi_probe_grid_ctx.items[handle.id].initialized;
}

Vec3 lighting_gi_probe_grid_sample(lighting_gi_probe_grid_handle_t handle, Vec3 position, Vec3 normal) {
    if (handle.id >= g_gi_probe_grid_ctx.count) return vec3_zero();
    lighting_gi_probe_grid_internal_t* item = &g_gi_probe_grid_ctx.items[handle.id];
    if (!item->initialized) return vec3_zero();

    probe_grid_data_t* grid = &item->grid;
    if (!grid->probes) return vec3_zero();

    // Find 8 surrounding probes
    Vec3 rel_pos = vec3_sub(position, grid->min_bounds);
    
    // Convert to grid space
    Vec3 grid_pos;
    grid_pos.x = rel_pos.x / (grid->cell_size.x > 0 ? grid->cell_size.x : 1.0f);
    grid_pos.y = rel_pos.y / (grid->cell_size.y > 0 ? grid->cell_size.y : 1.0f);
    grid_pos.z = rel_pos.z / (grid->cell_size.z > 0 ? grid->cell_size.z : 1.0f);

    ivec3_t base;
    base.x = (i32)floorf(grid_pos.x);
    base.y = (i32)floorf(grid_pos.y);
    base.z = (i32)floorf(grid_pos.z);

    f32 tx = grid_pos.x - (f32)base.x;
    f32 ty = grid_pos.y - (f32)base.y;
    f32 tz = grid_pos.z - (f32)base.z;

    Vec3 irradiance = vec3_zero();

    for (int i = 0; i < 8; i++) {
        ivec3_t offset = { (i & 1), (i >> 1) & 1, (i >> 2) & 1 };
        ivec3_t p_pos = { base.x + offset.x, base.y + offset.y, base.z + offset.z };
        
        // Clamp to grid bounds
        if (p_pos.x < 0) p_pos.x = 0; if (p_pos.x >= grid->resolution.x) p_pos.x = grid->resolution.x - 1;
        if (p_pos.y < 0) p_pos.y = 0; if (p_pos.y >= grid->resolution.y) p_pos.y = grid->resolution.y - 1;
        if (p_pos.z < 0) p_pos.z = 0; if (p_pos.z >= grid->resolution.z) p_pos.z = grid->resolution.z - 1;

        uint32_t index = get_probe_index(grid, p_pos);
        irradiance_probe_t* probe = &grid->probes[index];

        f32 weight = (offset.x ? tx : (1.0f - tx)) *
                     (offset.y ? ty : (1.0f - ty)) *
                     (offset.z ? tz : (1.0f - tz));

        irradiance = vec3_add(irradiance, vec3_mul(evaluate_sh_vec3(probe->sh_coefficients, normal), weight));
    }

    return irradiance;
}

void lighting_gi_probe_grid_mark_dirty(lighting_gi_probe_grid_handle_t handle) {
    // TODO: Implement gi probe grid culling integration
    if (handle.id < g_gi_probe_grid_ctx.count) {
        g_gi_probe_grid_ctx.items[handle.id].dirty = true;
    }
}

int lighting_gi_probe_grid_process_pending(void) {
    // TODO: Add gi probe grid render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_gi_probe_grid_ctx.count; i++) {
        lighting_gi_probe_grid_internal_t* item = &g_gi_probe_grid_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t lighting_gi_probe_grid_get_count(void) {
    return g_gi_probe_grid_ctx.count;
}

size_t lighting_gi_probe_grid_get_memory_usage(void) {
    // TODO: Implement memory tracking
    size_t total = sizeof(g_gi_probe_grid_ctx);
    total += g_gi_probe_grid_ctx.capacity * sizeof(lighting_gi_probe_grid_internal_t);

    for (uint32_t i = 0; i < g_gi_probe_grid_ctx.count; i++) {
        if (g_gi_probe_grid_ctx.items[i].grid.probes) {
            uint32_t res = g_gi_probe_grid_ctx.items[i].grid.resolution.x * 
                           g_gi_probe_grid_ctx.items[i].grid.resolution.y * 
                           g_gi_probe_grid_ctx.items[i].grid.resolution.z;
            total += res * sizeof(irradiance_probe_t);
        }
    }

    return total;
}

void lighting_gi_probe_grid_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of gi_probe_grid.c */
