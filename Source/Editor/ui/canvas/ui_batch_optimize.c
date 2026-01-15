/*
 * ui_batch_optimize.c
 * Optimization utilities for UI batching
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "editor/ui/canvas/ui_batch_optimize.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_BATCH_CACHE_MAX_ENTRIES 512
#define UI_BATCH_MEMORY_POOL_INITIAL_SIZE (32 * 1024 * 1024) /* 32MB */

/* SIMD capabilities flags */
#define SIMD_CAP_SSE2   0x01
#define SIMD_CAP_AVX    0x02
#define SIMD_CAP_AVX2   0x04
#define SIMD_CAP_NEON   0x08

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_batch_memory_block {
    void* memory;
    bool in_use;
    size_t size;
} ui_batch_memory_block_t;

typedef struct ui_batch_memory_pool {
    ui_batch_memory_block_t* blocks;
    uint32_t block_count;
    size_t block_size;

    size_t total_allocated;
    size_t total_available;
    bool initialized;
} ui_batch_memory_pool_t;

typedef struct ui_batch_cache {
    ui_batch_cache_entry_t entries[UI_BATCH_CACHE_MAX_ENTRIES];
    uint32_t entry_count;
    uint64_t hit_count;
    uint64_t miss_count;
} ui_batch_cache_t;

typedef struct ui_batch_optimize_context {
    ui_batch_memory_pool_t memory_pool;
    ui_batch_cache_t cache;

    ui_batch_optimize_options_t options;
    uint32_t simd_capabilities;

    bool initialized;
} ui_batch_optimize_context_t;

static ui_batch_optimize_context_t g_optimize_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS - MEMORY MANAGEMENT
 * ============================================================================ */

static void* ui_batch_opt_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void ui_batch_opt_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/* Detect SIMD capabilities */
static uint32_t ui_batch_detect_simd(void) {
    uint32_t caps = 0;

    /* Check for SSE2 */
#ifdef __SSE2__
    caps |= SIMD_CAP_SSE2;
#endif

    /* Check for AVX */
#ifdef __AVX__
    caps |= SIMD_CAP_AVX;
#endif

    /* Check for AVX2 */
#ifdef __AVX2__
    caps |= SIMD_CAP_AVX2;
#endif

    /* Check for NEON (ARM) */
#ifdef __ARM_NEON__
    caps |= SIMD_CAP_NEON;
#endif

    return caps;
}

/* Hash function for cache */
static uint64_t ui_batch_hash_data(const void* data, size_t size) {
    uint64_t hash = 5381;
    const uint8_t* bytes = (const uint8_t*)data;

    for (size_t i = 0; i < size && i < 256; i++) {
        hash = ((hash << 5) + hash) + bytes[i];
    }

    return hash;
}

/* ============================================================================
 * PUBLIC API - INITIALIZATION
 * ============================================================================ */

int ui_batch_optimize_init(const ui_batch_optimize_options_t* options) {
    if (g_optimize_ctx.initialized) {
        return 0;
    }

    memset(&g_optimize_ctx, 0, sizeof(g_optimize_ctx));

    if (options) {
        g_optimize_ctx.options = *options;
    } else {
        g_optimize_ctx.options.enable_pooling = true;
        g_optimize_ctx.options.enable_simd = true;
        g_optimize_ctx.options.enable_caching = true;
        g_optimize_ctx.options.enable_compression = false;
        g_optimize_ctx.options.simd_path = 0; /* Auto */
    }

    g_optimize_ctx.simd_capabilities = ui_batch_detect_simd();

    if (g_optimize_ctx.options.enable_pooling) {
        g_optimize_ctx.memory_pool.block_size = 65536; /* 64KB blocks */
        g_optimize_ctx.memory_pool.block_count = 512;
        g_optimize_ctx.memory_pool.blocks = ui_batch_opt_malloc(g_optimize_ctx.memory_pool.block_count *
                                                                  sizeof(ui_batch_memory_block_t));
        if (!g_optimize_ctx.memory_pool.blocks) {
            return -1;
        }

        g_optimize_ctx.memory_pool.total_available = UI_BATCH_MEMORY_POOL_INITIAL_SIZE;
        g_optimize_ctx.memory_pool.total_allocated = 0;
        g_optimize_ctx.memory_pool.initialized = true;
    }

    if (g_optimize_ctx.options.enable_caching) {
        g_optimize_ctx.cache.entry_count = 0;
        g_optimize_ctx.cache.hit_count = 0;
        g_optimize_ctx.cache.miss_count = 0;
    }

    g_optimize_ctx.initialized = true;
    return 0;
}

void ui_batch_optimize_shutdown(void) {
    if (!g_optimize_ctx.initialized) {
        return;
    }

    if (g_optimize_ctx.memory_pool.initialized) {
        for (uint32_t i = 0; i < g_optimize_ctx.memory_pool.block_count; i++) {
            ui_batch_opt_free(g_optimize_ctx.memory_pool.blocks[i].memory);
        }
        ui_batch_opt_free(g_optimize_ctx.memory_pool.blocks);
    }

    for (uint32_t i = 0; i < g_optimize_ctx.cache.entry_count; i++) {
        ui_batch_opt_free(g_optimize_ctx.cache.entries[i].data);
    }

    memset(&g_optimize_ctx, 0, sizeof(g_optimize_ctx));
}

/* ============================================================================
 * PUBLIC API - MEMORY POOLING
 * ============================================================================ */

int ui_batch_memory_pool_create(size_t block_size, uint32_t block_count) {
    if (!g_optimize_ctx.memory_pool.initialized) {
        return -1;
    }

    if (block_count > g_optimize_ctx.memory_pool.block_count) {
        return -2;
    }

    for (uint32_t i = 0; i < block_count; i++) {
        g_optimize_ctx.memory_pool.blocks[i].memory = ui_batch_opt_malloc(block_size);
        if (!g_optimize_ctx.memory_pool.blocks[i].memory) {
            return -3;
        }
        g_optimize_ctx.memory_pool.blocks[i].size = block_size;
        g_optimize_ctx.memory_pool.blocks[i].in_use = false;
    }

    return 0;
}

void* ui_batch_memory_pool_alloc(size_t size) {
    if (!g_optimize_ctx.memory_pool.initialized || !g_optimize_ctx.options.enable_pooling) {
        return malloc(size);
    }

    for (uint32_t i = 0; i < g_optimize_ctx.memory_pool.block_count; i++) {
        ui_batch_memory_block_t* block = &g_optimize_ctx.memory_pool.blocks[i];
        if (!block->in_use && block->size >= size) {
            block->in_use = true;
            g_optimize_ctx.memory_pool.total_allocated += size;
            return block->memory;
        }
    }

    /* Fallback to malloc if no pool block available */
    return malloc(size);
}

void ui_batch_memory_pool_free(void* ptr) {
    if (!ptr) {
        return;
    }

    if (!g_optimize_ctx.memory_pool.initialized || !g_optimize_ctx.options.enable_pooling) {
        free(ptr);
        return;
    }

    for (uint32_t i = 0; i < g_optimize_ctx.memory_pool.block_count; i++) {
        ui_batch_memory_block_t* block = &g_optimize_ctx.memory_pool.blocks[i];
        if (block->memory == ptr) {
            block->in_use = false;
            g_optimize_ctx.memory_pool.total_allocated -= block->size;
            return;
        }
    }

    /* If not found in pool, it was malloc'd */
    free(ptr);
}

int ui_batch_memory_pool_get_stats(size_t* out_used, size_t* out_available) {
    if (!out_used || !out_available) {
        return -1;
    }

    if (!g_optimize_ctx.memory_pool.initialized) {
        return -2;
    }

    *out_used = g_optimize_ctx.memory_pool.total_allocated;
    *out_available = g_optimize_ctx.memory_pool.total_available;

    return 0;
}

/* ============================================================================
 * PUBLIC API - SIMD OPERATIONS
 * ============================================================================ */

uint32_t ui_batch_simd_get_capabilities(void) {
    return g_optimize_ctx.simd_capabilities;
}

int ui_batch_simd_transform_vertices(ui_rendering_vertex_t* vertices,
                                      uint32_t count,
                                      const float* transform_matrix) {
    if (!vertices || count == 0 || !transform_matrix) {
        return -1;
    }

    /* Simple transform implementation - production would use SIMD */
    for (uint32_t i = 0; i < count; i++) {
        float x = vertices[i].x;
        float y = vertices[i].y;
        float z = vertices[i].z;

        /* Apply 4x4 matrix multiplication */
        vertices[i].x = x * transform_matrix[0] + y * transform_matrix[4] + z * transform_matrix[8] + transform_matrix[12];
        vertices[i].y = x * transform_matrix[1] + y * transform_matrix[5] + z * transform_matrix[9] + transform_matrix[13];
        vertices[i].z = x * transform_matrix[2] + y * transform_matrix[6] + z * transform_matrix[10] + transform_matrix[14];
    }

    return 0;
}

int ui_batch_simd_blend_colors(uint32_t* colors,
                                uint32_t count,
                                uint32_t blend_color,
                                float blend_factor) {
    if (!colors || count == 0 || blend_factor < 0 || blend_factor > 1.0f) {
        return -1;
    }

    uint8_t br = (blend_color >> 16) & 0xFF;
    uint8_t bg = (blend_color >> 8) & 0xFF;
    uint8_t bb = blend_color & 0xFF;
    uint8_t ba = (blend_color >> 24) & 0xFF;

    for (uint32_t i = 0; i < count; i++) {
        uint8_t r = (colors[i] >> 16) & 0xFF;
        uint8_t g = (colors[i] >> 8) & 0xFF;
        uint8_t b = colors[i] & 0xFF;
        uint8_t a = (colors[i] >> 24) & 0xFF;

        r = (uint8_t)(r * (1.0f - blend_factor) + br * blend_factor);
        g = (uint8_t)(g * (1.0f - blend_factor) + bg * blend_factor);
        b = (uint8_t)(b * (1.0f - blend_factor) + bb * blend_factor);
        a = (uint8_t)(a * (1.0f - blend_factor) + ba * blend_factor);

        colors[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    return 0;
}

/* ============================================================================
 * PUBLIC API - BATCH CACHING
 * ============================================================================ */

int ui_batch_cache_enable(ui_rendering_ui_batch_handle_t batch_handle) {
    if (!g_optimize_ctx.options.enable_caching) {
        return -1;
    }
    return 0;
}

int ui_batch_cache_disable(ui_rendering_ui_batch_handle_t batch_handle) {
    return ui_batch_cache_clear(batch_handle);
}

int ui_batch_cache_get(ui_rendering_ui_batch_handle_t batch_handle,
                        void** out_data,
                        size_t* out_size) {
    if (!out_data || !out_size) {
        return -1;
    }

    /* Simplified cache lookup */
    for (uint32_t i = 0; i < g_optimize_ctx.cache.entry_count; i++) {
        if (g_optimize_ctx.cache.entries[i].data != NULL) {
            *out_data = g_optimize_ctx.cache.entries[i].data;
            *out_size = g_optimize_ctx.cache.entries[i].size;
            g_optimize_ctx.cache.hit_count++;
            return 0;
        }
    }

    g_optimize_ctx.cache.miss_count++;
    return -2;
}

int ui_batch_cache_update(ui_rendering_ui_batch_handle_t batch_handle,
                           const void* data,
                           size_t size) {
    if (!data || size == 0 || !g_optimize_ctx.options.enable_caching) {
        return -1;
    }

    if (g_optimize_ctx.cache.entry_count >= UI_BATCH_CACHE_MAX_ENTRIES) {
        return -2;
    }

    ui_batch_cache_entry_t* entry = &g_optimize_ctx.cache.entries[g_optimize_ctx.cache.entry_count++];
    entry->data = ui_batch_opt_malloc(size);
    if (!entry->data) {
        g_optimize_ctx.cache.entry_count--;
        return -3;
    }

    memcpy(entry->data, data, size);
    entry->size = size;
    entry->hash = ui_batch_hash_data(data, size);
    entry->timestamp = 0;

    return 0;
}

int ui_batch_cache_clear(ui_rendering_ui_batch_handle_t batch_handle) {
    for (uint32_t i = 0; i < g_optimize_ctx.cache.entry_count; i++) {
        ui_batch_opt_free(g_optimize_ctx.cache.entries[i].data);
    }
    g_optimize_ctx.cache.entry_count = 0;
    return 0;
}

/* ============================================================================
 * PUBLIC API - COMPRESSION
 * ============================================================================ */

int ui_batch_compress_geometry(const ui_rendering_vertex_t* vertices,
                                uint32_t vertex_count,
                                const uint32_t* indices,
                                uint32_t index_count,
                                void** out_compressed,
                                size_t* out_size) {
    if (!vertices || vertex_count == 0 || !indices || index_count == 0 ||
        !out_compressed || !out_size) {
        return -1;
    }

    /* Simple compression: just pack data without actual compression for now */
    size_t total_size = sizeof(uint32_t) * 2 +
                        vertex_count * sizeof(ui_rendering_vertex_t) +
                        index_count * sizeof(uint32_t);

    void* compressed = ui_batch_opt_malloc(total_size);
    if (!compressed) {
        return -2;
    }

    uint8_t* dst = (uint8_t*)compressed;
    uint32_t* header = (uint32_t*)dst;
    header[0] = vertex_count;
    header[1] = index_count;
    dst += sizeof(uint32_t) * 2;

    memcpy(dst, vertices, vertex_count * sizeof(ui_rendering_vertex_t));
    dst += vertex_count * sizeof(ui_rendering_vertex_t);

    memcpy(dst, indices, index_count * sizeof(uint32_t));

    *out_compressed = compressed;
    *out_size = total_size;

    return 0;
}

int ui_batch_decompress_geometry(const void* compressed,
                                  size_t compressed_size,
                                  ui_rendering_vertex_t** out_vertices,
                                  uint32_t* out_vertex_count,
                                  uint32_t** out_indices,
                                  uint32_t* out_index_count) {
    if (!compressed || compressed_size < sizeof(uint32_t) * 2 ||
        !out_vertices || !out_vertex_count || !out_indices || !out_index_count) {
        return -1;
    }

    const uint8_t* src = (const uint8_t*)compressed;
    const uint32_t* header = (const uint32_t*)src;
    uint32_t vertex_count = header[0];
    uint32_t index_count = header[1];
    src += sizeof(uint32_t) * 2;

    size_t expected_size = sizeof(uint32_t) * 2 +
                          vertex_count * sizeof(ui_rendering_vertex_t) +
                          index_count * sizeof(uint32_t);

    if (compressed_size < expected_size) {
        return -2;
    }

    ui_rendering_vertex_t* vertices = ui_batch_opt_malloc(vertex_count * sizeof(ui_rendering_vertex_t));
    if (!vertices) {
        return -3;
    }

    uint32_t* indices = ui_batch_opt_malloc(index_count * sizeof(uint32_t));
    if (!indices) {
        ui_batch_opt_free(vertices);
        return -4;
    }

    memcpy(vertices, src, vertex_count * sizeof(ui_rendering_vertex_t));
    src += vertex_count * sizeof(ui_rendering_vertex_t);

    memcpy(indices, src, index_count * sizeof(uint32_t));

    *out_vertices = vertices;
    *out_vertex_count = vertex_count;
    *out_indices = indices;
    *out_index_count = index_count;

    return 0;
}

/* ============================================================================
 * PUBLIC API - STATISTICS & PROFILING
 * ============================================================================ */

size_t ui_batch_optimize_get_memory_usage(void) {
    return g_optimize_ctx.memory_pool.total_allocated;
}

float ui_batch_cache_get_hit_rate(void) {
    uint64_t total = g_optimize_ctx.cache.hit_count + g_optimize_ctx.cache.miss_count;
    if (total == 0) {
        return 0.0f;
    }
    return (float)g_optimize_ctx.cache.hit_count / (float)total;
}

void ui_batch_optimize_reset_stats(void) {
    g_optimize_ctx.cache.hit_count = 0;
    g_optimize_ctx.cache.miss_count = 0;
}

/* End of ui_batch_optimize.c */
