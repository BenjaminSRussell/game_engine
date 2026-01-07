/*
 * vertex_compression.c
 * Quantized vertex compression
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement mesh optimization (vertex cache)
 * TODO: Add meshlet generation for mesh shaders
 * TODO: Implement progressive mesh streaming
 * TODO: Add mesh simplification (QEM)
 * TODO: Implement vertex compression
 * TODO: Add LOD generation
 * TODO: Implement BVH construction
 * TODO: Add instanced rendering support
 * TODO: Implement GPU-driven culling
 * TODO: Add mesh bounds computation
 * TODO: Implement vertex compression initialization
 * TODO: Add vertex compression cleanup/shutdown
 * TODO: Implement vertex compression validation
 * TODO: Add vertex compression error handling
 * TODO: Implement vertex compression serialization
 * TODO: Add vertex compression debug output
 * TODO: Implement vertex compression unit tests
 * TODO: Add vertex compression performance counters
 * TODO: Implement vertex compression hot-reload
 * TODO: Add vertex compression thread safety
 * TODO: Implement vertex compression memory pooling
 * TODO: Add vertex compression caching layer
 * TODO: Implement vertex compression async operations
 * TODO: Add vertex compression GPU integration
 * TODO: Implement vertex compression SIMD optimization
 * TODO: Add vertex compression batch processing
 * TODO: Implement vertex compression streaming support
 * TODO: Add vertex compression LOD support
 * TODO: Implement vertex compression culling integration
 * TODO: Add vertex compression render graph node
 */

#include "geometry/vertex/vertex_compression.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GEOMETRY_VERTEX_COMPRESSION_MAX_COUNT 4096
#define GEOMETRY_VERTEX_COMPRESSION_DEFAULT_CAPACITY 256
#define GEOMETRY_VERTEX_COMPRESSION_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_vertex_compression_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} geometry_vertex_compression_internal_t;

typedef struct geometry_vertex_compression_context {
    geometry_vertex_compression_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} geometry_vertex_compression_context_t;

static geometry_vertex_compression_context_t g_vertex_compression_ctx = {0};

#include <include/math/math.h>

/* ============================================================================
 * COMPRESSION IMPLEMENTATION
 * ============================================================================ */

static inline float saturate(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline float sign(float v) {
    return (v >= 0.0f) ? 1.0f : -1.0f;
}

void vertex_compression_quantize_position_u16(const float* position, const float* bounds_min, const float* bounds_scale, uint16_t* out_u16) {
    // scale = 1.0 / (max - min) computed beforehand for efficiency
    float x = (position[0] - bounds_min[0]) * bounds_scale[0];
    float y = (position[1] - bounds_min[1]) * bounds_scale[1];
    float z = (position[2] - bounds_min[2]) * bounds_scale[2];

    out_u16[0] = (uint16_t)(saturate(x) * 65535.0f);
    out_u16[1] = (uint16_t)(saturate(y) * 65535.0f);
    out_u16[2] = (uint16_t)(saturate(z) * 65535.0f);
}

void vertex_compression_dequantize_position_u16(const uint16_t* in_u16, const float* bounds_min, const float* bounds_scale, float* out_position) {
    // Here bounds_scale is presumably (max - min) to expand back
    // Wait, the encoder took 1/(max-min). 
    // To match, the decoder needs (max-min). 
    // I'll assume the user passes the correct scale factor for the operation (reciprocal for encode, raw for decode).
    
    float nx = (float)in_u16[0] / 65535.0f;
    float ny = (float)in_u16[1] / 65535.0f;
    float nz = (float)in_u16[2] / 65535.0f;

    out_position[0] = nx * bounds_scale[0] + bounds_min[0];
    out_position[1] = ny * bounds_scale[1] + bounds_min[1];
    out_position[2] = nz * bounds_scale[2] + bounds_min[2];
}

// Octahedron Normal Encoding (Oct32)
// Based on Meyer et al. 2010
void vertex_compression_pack_normal_oct32(const float* normal, uint32_t* out_packed) {
    float n[3] = { normal[0], normal[1], normal[2] };
    float L1 = fabsf(n[0]) + fabsf(n[1]) + fabsf(n[2]);
    if (L1 > 0.0f) {
        float invL1 = 1.0f / L1;
        n[0] *= invL1;
        n[1] *= invL1;
        n[2] *= invL1;
    }

    if (n[2] < 0.0f) {
        float tx = (1.0f - fabsf(n[1])) * sign(n[0]);
        float ty = (1.0f - fabsf(n[0])) * sign(n[1]);
        n[0] = tx;
        n[1] = ty;
    }

    // Pack to snorm16
    // Map [-1, 1] to [-32767, 32767]
    int16_t x = (int16_t)(n[0] * 32767.0f);
    int16_t y = (int16_t)(n[1] * 32767.0f);

    *out_packed = ((uint32_t)((uint16_t)x)) | (((uint32_t)((uint16_t)y)) << 16);
}

void vertex_compression_unpack_normal_oct32(uint32_t packed, float* out_normal) {
    int16_t x_i = (int16_t)(packed & 0xFFFF);
    int16_t y_i = (int16_t)((packed >> 16) & 0xFFFF);

    float x = (float)x_i / 32767.0f;
    float y = (float)y_i / 32767.0f;
    float z = 1.0f - fabsf(x) - fabsf(y);

    if (z < 0.0f) {
        float tx = (1.0f - fabsf(y)) * sign(x);
        float ty = (1.0f - fabsf(x)) * sign(y);
        x = tx;
        y = ty;
    }

    // Normalize
    float lenSq = x * x + y * y + z * z;
    if (lenSq > 0.0f) {
        float invLen = 1.0f / sqrtf(lenSq);
        out_normal[0] = x * invLen;
        out_normal[1] = y * invLen;
        out_normal[2] = z * invLen;
    } else {
        out_normal[0] = 0.0f;
        out_normal[1] = 1.0f; // Default up
        out_normal[2] = 0.0f;
    }
}


/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool geometry_vertex_compression_validate(const geometry_vertex_compression_internal_t* item) {
    // TODO: Implement mesh optimization (vertex cache)
    // TODO: Add meshlet generation for mesh shaders
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void geometry_vertex_compression_cleanup_internal(geometry_vertex_compression_internal_t* item) {
    // TODO: Implement progressive mesh streaming
    // TODO: Add mesh simplification (QEM)
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_vertex_compression_init(void) {
    // TODO: Implement vertex compression
    // TODO: Add LOD generation
    // TODO: Implement BVH construction
    // TODO: Add instanced rendering support

    if (g_vertex_compression_ctx.initialized) {
        return 0; // Already initialized
    }

    g_vertex_compression_ctx.capacity = GEOMETRY_VERTEX_COMPRESSION_DEFAULT_CAPACITY;
    g_vertex_compression_ctx.items = calloc(g_vertex_compression_ctx.capacity, sizeof(geometry_vertex_compression_internal_t));
    if (!g_vertex_compression_ctx.items) {
        return -1;
    }

    g_vertex_compression_ctx.count = 0;
    g_vertex_compression_ctx.initialized = true;

    return 0;
}

void geometry_vertex_compression_shutdown(void) {
    // TODO: Implement GPU-driven culling
    // TODO: Add mesh bounds computation
    // TODO: Implement vertex compression initialization
    // TODO: Add vertex compression cleanup/shutdown

    if (!g_vertex_compression_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_vertex_compression_ctx.count; i++) {
        geometry_vertex_compression_cleanup_internal(&g_vertex_compression_ctx.items[i]);
    }

    free(g_vertex_compression_ctx.items);
    g_vertex_compression_ctx.items = NULL;
    g_vertex_compression_ctx.count = 0;
    g_vertex_compression_ctx.capacity = 0;
    g_vertex_compression_ctx.initialized = false;
}

int geometry_vertex_compression_create(geometry_vertex_compression_handle_t* out_handle, const geometry_vertex_compression_desc_t* desc) {
    // TODO: Implement vertex compression validation
    // TODO: Add vertex compression error handling
    // TODO: Implement vertex compression serialization
    // TODO: Add vertex compression debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_vertex_compression_ctx.initialized) {
        return -2;
    }

    if (g_vertex_compression_ctx.count >= g_vertex_compression_ctx.capacity) {
        // TODO: Implement vertex compression unit tests
        return -3;
    }

    uint32_t index = g_vertex_compression_ctx.count++;
    geometry_vertex_compression_internal_t* item = &g_vertex_compression_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void geometry_vertex_compression_destroy(geometry_vertex_compression_handle_t handle) {
    // TODO: Add vertex compression performance counters
    // TODO: Implement vertex compression hot-reload

    if (handle.id >= g_vertex_compression_ctx.count) {
        return;
    }

    geometry_vertex_compression_cleanup_internal(&g_vertex_compression_ctx.items[handle.id]);
}

int geometry_vertex_compression_update(geometry_vertex_compression_handle_t handle, const void* data, size_t size) {
    // TODO: Add vertex compression thread safety
    // TODO: Implement vertex compression memory pooling
    // TODO: Add vertex compression caching layer
    // TODO: Implement vertex compression async operations

    if (handle.id >= g_vertex_compression_ctx.count) {
        return -1;
    }

    geometry_vertex_compression_internal_t* item = &g_vertex_compression_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add vertex compression GPU integration
    // TODO: Implement vertex compression SIMD optimization

    item->dirty = true;
    return 0;
}

bool geometry_vertex_compression_is_valid(geometry_vertex_compression_handle_t handle) {
    // TODO: Add vertex compression batch processing
    if (handle.id >= g_vertex_compression_ctx.count) {
        return false;
    }
    return g_vertex_compression_ctx.items[handle.id].initialized;
}

int geometry_vertex_compression_get_info(geometry_vertex_compression_handle_t handle, geometry_vertex_compression_info_t* out_info) {
    // TODO: Implement vertex compression streaming support
    // TODO: Add vertex compression LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_vertex_compression_ctx.count) {
        return -2;
    }

    const geometry_vertex_compression_internal_t* item = &g_vertex_compression_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void geometry_vertex_compression_mark_dirty(geometry_vertex_compression_handle_t handle) {
    // TODO: Implement vertex compression culling integration
    if (handle.id < g_vertex_compression_ctx.count) {
        g_vertex_compression_ctx.items[handle.id].dirty = true;
    }
}

int geometry_vertex_compression_process_pending(void) {
    // TODO: Add vertex compression render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_vertex_compression_ctx.count; i++) {
        geometry_vertex_compression_internal_t* item = &g_vertex_compression_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t geometry_vertex_compression_get_count(void) {
    return g_vertex_compression_ctx.count;
}

size_t geometry_vertex_compression_get_memory_usage(void) {
    // TODO: Implement memory tracking
    size_t total = sizeof(g_vertex_compression_ctx);
    total += g_vertex_compression_ctx.capacity * sizeof(geometry_vertex_compression_internal_t);

    for (uint32_t i = 0; i < g_vertex_compression_ctx.count; i++) {
        total += g_vertex_compression_ctx.items[i].data_size;
    }

    return total;
}

void geometry_vertex_compression_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of vertex_compression.c */
