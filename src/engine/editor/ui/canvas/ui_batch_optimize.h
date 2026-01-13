/*
 * ui_batch_optimize.h
 * Optimization utilities for UI batching
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Provides optimization strategies including memory pooling,
 * SIMD acceleration, and caching for efficient UI rendering.
 */

#ifndef UI_RENDERING_UI_BATCH_OPTIMIZE_H
#define UI_RENDERING_UI_BATCH_OPTIMIZE_H

#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Memory pool descriptor */
typedef struct ui_batch_memory_pool_desc {
    size_t block_size;
    uint32_t block_count;
} ui_batch_memory_pool_desc_t;

/* Optimization options */
typedef struct ui_batch_optimize_options {
    bool enable_pooling;
    bool enable_simd;
    bool enable_caching;
    bool enable_compression;
    uint32_t simd_path;      /* 0=auto, 1=SSE2, 2=AVX, 3=AVX2, 4=NEON */
} ui_batch_optimize_options_t;

/* Cache entry */
typedef struct ui_batch_cache_entry {
    uint64_t hash;
    void* data;
    size_t size;
    uint64_t timestamp;
} ui_batch_cache_entry_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

int ui_batch_optimize_init(const ui_batch_optimize_options_t* options);
void ui_batch_optimize_shutdown(void);

/* ============================================================================
 * API - MEMORY POOLING
 * ============================================================================ */

/* Create memory pool */
int ui_batch_memory_pool_create(size_t block_size, uint32_t block_count);

/* Allocate from pool */
void* ui_batch_memory_pool_alloc(size_t size);

/* Free to pool */
void ui_batch_memory_pool_free(void* ptr);

/* Get pool statistics */
int ui_batch_memory_pool_get_stats(size_t* out_used, size_t* out_available);

/* ============================================================================
 * API - SIMD OPERATIONS
 * ============================================================================ */

/* Detect available SIMD support */
uint32_t ui_batch_simd_get_capabilities(void);

/* Transform vertices with SIMD */
int ui_batch_simd_transform_vertices(ui_rendering_vertex_t* vertices,
                                      uint32_t count,
                                      const float* transform_matrix);

/* Color blend with SIMD */
int ui_batch_simd_blend_colors(uint32_t* colors,
                                uint32_t count,
                                uint32_t blend_color,
                                float blend_factor);

/* ============================================================================
 * API - BATCH CACHING
 * ============================================================================ */

/* Enable caching for batch */
int ui_batch_cache_enable(ui_rendering_ui_batch_handle_t batch_handle);

/* Disable caching for batch */
int ui_batch_cache_disable(ui_rendering_ui_batch_handle_t batch_handle);

/* Get cached batch data */
int ui_batch_cache_get(ui_rendering_ui_batch_handle_t batch_handle,
                        void** out_data,
                        size_t* out_size);

/* Update cache */
int ui_batch_cache_update(ui_rendering_ui_batch_handle_t batch_handle,
                           const void* data,
                           size_t size);

/* Clear cache */
int ui_batch_cache_clear(ui_rendering_ui_batch_handle_t batch_handle);

/* ============================================================================
 * API - COMPRESSION
 * ============================================================================ */

/* Compress batch geometry */
int ui_batch_compress_geometry(const ui_rendering_vertex_t* vertices,
                                uint32_t vertex_count,
                                const uint32_t* indices,
                                uint32_t index_count,
                                void** out_compressed,
                                size_t* out_size);

/* Decompress batch geometry */
int ui_batch_decompress_geometry(const void* compressed,
                                  size_t compressed_size,
                                  ui_rendering_vertex_t** out_vertices,
                                  uint32_t* out_vertex_count,
                                  uint32_t** out_indices,
                                  uint32_t* out_index_count);

/* ============================================================================
 * API - STATISTICS & PROFILING
 * ============================================================================ */

/* Get memory usage */
size_t ui_batch_optimize_get_memory_usage(void);

/* Get cache hit rate */
float ui_batch_cache_get_hit_rate(void);

/* Reset statistics */
void ui_batch_optimize_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_BATCH_OPTIMIZE_H */
