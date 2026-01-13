/*
 * anisotropic_filter.h
 * Anisotropic filtering
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 *
 * Features:
 * - Anisotropic filtering with configurable levels (1x to 16x)
 * - Virtual texturing with page-based streaming
 * - BC/ASTC compression support
 * - Mipmap generation with quality levels
 * - Bindless texture management
 * - Texture arrays and 2D/3D support
 * - Feedback analysis for quality metrics
 * - Format conversion utilities
 * - Performance counters and profiling
 * - Hot-reload for development
 * - Thread-safe operations
 * - Memory pooling and caching
 * - Async operations with worker threads
 * - GPU integration with compute shaders
 * - SIMD optimization for CPU processing
 * - Batch processing for multiple textures
 * - LOD support with level selection
 * - Culling integration for visibility
 * - Render graph node integration
 */

#ifndef TEXTURE_ANISOTROPIC_FILTER_H
#define TEXTURE_ANISOTROPIC_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Error codes
typedef enum {
    TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE = 0,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM = -1,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE = -4,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_COMPRESSION_FAILED = -5,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_GPU_INTEGRATION_FAILED = -6,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_SERIALIZATION_FAILED = -7,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_THREAD_ERROR = -8,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_ASYNC_OPERATION_FAILED = -9,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_VIRTUAL_TEXTURE_FAILED = -10
} texture_anisotropic_filter_error_t;

// Anisotropic filtering levels
typedef enum {
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_1X = 1,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_2X = 2,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_4X = 4,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_8X = 8,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_16X = 16
} texture_anisotropic_filter_level_t;

// Compression formats
typedef enum {
    TEXTURE_ANISOTROPIC_COMPRESSION_NONE = 0,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC1,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC2,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC3,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC4,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC5,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC6H,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC7,
    TEXTURE_ANISOTROPIC_COMPRESSION_ASTC_4x4,
    TEXTURE_ANISOTROPIC_COMPRESSION_ASTC_6x6,
    TEXTURE_ANISOTROPIC_COMPRESSION_ASTC_8x8
} texture_anisotropic_compression_t;

// Performance counters
typedef struct {
    uint64_t total_samples_processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t async_operations;
    uint64_t compression_operations;
    uint64_t virtual_texture_operations;
    uint64_t batch_operations;
    double total_processing_time_ms;
    double average_processing_time_ms;
    size_t peak_memory_usage;
    size_t current_memory_usage;
} texture_anisotropic_performance_counters_t;

typedef struct texture_anisotropic_filter_handle {
    uint32_t id;
} texture_anisotropic_filter_handle_t;

typedef struct texture_anisotropic_filter_desc {
    uint32_t flags;
    void* user_data;
    texture_anisotropic_filter_level_t anisotropy_level;
    texture_anisotropic_compression_t compression_format;
    uint32_t width, height, depth;
    uint32_t mip_levels;
    bool use_virtual_texturing;
    const char* file_path; // For hot-reload
} texture_anisotropic_filter_desc_t;

typedef struct texture_anisotropic_filter_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    texture_anisotropic_filter_level_t anisotropy_level;
    texture_anisotropic_compression_t compression_format;
    uint32_t width, height, depth;
    uint32_t mip_levels;
    bool use_virtual_texturing;
    bool dirty;
    uint64_t frame_updated;
} texture_anisotropic_filter_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_anisotropic_filter_init(void);
void texture_anisotropic_filter_shutdown(void);

/* Lifecycle */
int texture_anisotropic_filter_create(texture_anisotropic_filter_handle_t* out_handle, const texture_anisotropic_filter_desc_t* desc);
void texture_anisotropic_filter_destroy(texture_anisotropic_filter_handle_t handle);

/* Operations */
int texture_anisotropic_filter_update(texture_anisotropic_filter_handle_t handle, const void* data, size_t size);
bool texture_anisotropic_filter_is_valid(texture_anisotropic_filter_handle_t handle);
int texture_anisotropic_filter_get_info(texture_anisotropic_filter_handle_t handle, texture_anisotropic_filter_info_t* out_info);
void texture_anisotropic_filter_mark_dirty(texture_anisotropic_filter_handle_t handle);
int texture_anisotropic_filter_process_pending(void);

/* Advanced Features */
int texture_anisotropic_filter_set_anisotropy_level(texture_anisotropic_filter_handle_t handle, texture_anisotropic_filter_level_t level);
int texture_anisotropic_filter_set_compression_format(texture_anisotropic_filter_handle_t handle, texture_anisotropic_compression_t format);
int texture_anisotropic_filter_enable_virtual_texturing(texture_anisotropic_filter_handle_t handle, bool enable);
int texture_anisotropic_filter_generate_mipmaps(texture_anisotropic_filter_handle_t handle);
int texture_anisotropic_filter_load_virtual_page(texture_anisotropic_filter_handle_t handle, uint32_t page_x, uint32_t page_y, uint32_t level);

/* Hot-reload */
int texture_anisotropic_filter_set_hot_reload_callback(texture_anisotropic_filter_handle_t handle, void (*callback)(texture_anisotropic_filter_handle_t));
int texture_anisotropic_filter_start_file_watching(const char* file_path);
void texture_anisotropic_filter_stop_file_watching(void);

/* Async Operations */
int texture_anisotropic_filter_async_compress(texture_anisotropic_filter_handle_t handle, texture_anisotropic_compression_t format);
int texture_anisotropic_filter_async_generate_mipmaps(texture_anisotropic_filter_handle_t handle);
bool texture_anisotropic_filter_is_async_operation_complete(texture_anisotropic_filter_handle_t handle, uint32_t operation_id);

/* GPU Integration */
int texture_anisotropic_filter_enable_gpu_processing(texture_anisotropic_filter_handle_t handle, bool enable);
bool texture_anisotropic_filter_is_gpu_available(void);

/* Caching */
int texture_anisotropic_filter_cache_add(texture_anisotropic_filter_handle_t handle, uint32_t level, const void* data, size_t size);
bool texture_anisotropic_filter_cache_lookup(texture_anisotropic_filter_handle_t handle, uint32_t level, void** data, size_t* size);
void texture_anisotropic_filter_cache_clear(void);

/* LOD Support */
int texture_anisotropic_filter_set_lod_level(texture_anisotropic_filter_handle_t handle, uint32_t level);
uint32_t texture_anisotropic_filter_get_current_lod(texture_anisotropic_filter_handle_t handle);
int texture_anisotropic_filter_update_lod_distances(texture_anisotropic_filter_handle_t handle, float distance);

/* Culling Integration */
int texture_anisotropic_filter_enable_culling(texture_anisotropic_filter_handle_t handle, bool enable);
bool texture_anisotropic_filter_is_visible(texture_anisotropic_filter_handle_t handle, float distance);
void texture_anisotropic_filter_update_culling_threshold(float threshold);

/* Render Graph Integration */
int texture_anisotropic_filter_add_render_graph_node(texture_anisotropic_filter_handle_t handle, uint32_t node_id, uint32_t input_texture_id, uint32_t output_texture_id);
void texture_anisotropic_filter_remove_render_graph_node(uint32_t node_id);
bool texture_anisotropic_filter_is_render_graph_node_enabled(uint32_t node_id);

/* Format Conversion */
int texture_anisotropic_filter_convert_format(texture_anisotropic_filter_handle_t handle, texture_anisotropic_compression_t target_format);
bool texture_anisotropic_filter_is_format_supported(texture_anisotropic_compression_t format);

/* Statistics */
uint32_t texture_anisotropic_filter_get_count(void);
size_t texture_anisotropic_filter_get_memory_usage(void);
void texture_anisotropic_filter_get_performance_counters(texture_anisotropic_performance_counters_t* out_counters);
void texture_anisotropic_filter_reset_performance_counters(void);
void texture_anisotropic_filter_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_ANISOTROPIC_FILTER_H */
