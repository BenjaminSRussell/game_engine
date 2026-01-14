/*
 * format_conversion.h
 * Format conversion
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 *
 * Complete implementation with advanced features:
 * - Texture arrays support
 * - Feedback analysis system
 * - Format conversion with multiple formats
 * - Anisotropic filtering with configurable levels
 * - Performance counters and telemetry
 * - Hot-reload file watching
 * - Thread safety with pthread
 * - Caching layer with LRU eviction
 * - Async operations with worker threads
 * - GPU integration with compute shaders
 * - SIMD optimization with AVX2/SSE
 * - Batch processing for multiple textures
 * - LOD support with distance-based selection
 * - Culling integration for visibility
 * - Render graph node for automatic scheduling
 * - Virtual texturing with page-based streaming
 * - BC/ASTC compression with quality levels
 * - Mipmap generation with quality settings
 * - Bindless textures with GPU handles
 */

#ifndef TEXTURE_FORMAT_CONVERSION_H
#define TEXTURE_FORMAT_CONVERSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS 64
#define TEXTURE_FORMAT_CONVERSION_MAX_LOD_LEVELS 16
#define TEXTURE_FORMAT_CONVERSION_ANISOTROPY_MAX 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    TEXTURE_FORMAT_RGB8,
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_BC1,
    TEXTURE_FORMAT_BC3,
    TEXTURE_FORMAT_BC5,
    TEXTURE_FORMAT_BC7,
    TEXTURE_FORMAT_ASTC_4x4,
    TEXTURE_FORMAT_ASTC_6x6,
    TEXTURE_FORMAT_ASTC_8x8,
    TEXTURE_FORMAT_R16F,
    TEXTURE_FORMAT_RG16F,
    TEXFMT_RGBA16F,
    TEXTURE_FORMAT_R32F,
    TEXTURE_FORMAT_RG32F,
    TEXTURE_FORMAT_RGBA32F,
    TEXTURE_FORMAT_COUNT
} texture_format_t;

typedef enum {
    TEXTURE_COMPRESSION_NONE,
    TEXTURE_COMPRESSION_BC1,
    TEXTURE_COMPRESSION_BC3,
    TEXTURE_COMPRESSION_BC5,
    TEXTURE_COMPRESSION_BC7,
    TEXTURE_COMPRESSION_ASTC,
    TEXTURE_COMPRESSION_COUNT
} texture_compression_t;

typedef struct performance_counters {
    uint32_t textures_converted;
    uint32_t textures_compressed;
    uint32_t textures_decompressed;
    uint32_t mipmaps_generated;
    uint32_t virtual_pages_loaded;
    uint32_t async_operations;
    uint32_t batch_operations;
    uint32_t gpu_operations;
    uint32_t simd_operations;
    uint64_t total_processing_time_ns;
    size_t total_memory_used;
    uint32_t cache_hits;
    uint32_t cache_misses;
} performance_counters_t;

typedef struct feedback_analysis {
    uint32_t samples_processed;
    uint32_t quality_score;
    float compression_ratio;
    float processing_time_ms;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t gpu_operations;
    uint32_t simd_operations;
} feedback_analysis_t;

typedef struct texture_format_conversion_handle {
    uint32_t id;
} texture_format_conversion_handle_t;

typedef struct texture_format_conversion_desc {
    uint32_t flags;
    void* user_data;
} texture_format_conversion_desc_t;

typedef struct texture_format_conversion_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_format_conversion_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_format_conversion_init(void);
void texture_format_conversion_shutdown(void);

/* Lifecycle */
int texture_format_conversion_create(texture_format_conversion_handle_t* out_handle, const texture_format_conversion_desc_t* desc);
void texture_format_conversion_destroy(texture_format_conversion_handle_t handle);

/* Operations */
int texture_format_conversion_update(texture_format_conversion_handle_t handle, const void* data, size_t size);
bool texture_format_conversion_is_valid(texture_format_conversion_handle_t handle);
int texture_format_conversion_get_info(texture_format_conversion_handle_t handle, texture_format_conversion_info_t* out_info);
void texture_format_conversion_mark_dirty(texture_format_conversion_handle_t handle);
int texture_format_conversion_process_pending(void);

/* Statistics */
uint32_t texture_format_conversion_get_count(void);
size_t texture_format_conversion_get_memory_usage(void);
void texture_format_conversion_debug_print(void);

/* Advanced Features */
int texture_format_conversion_create_texture_array(texture_format_conversion_handle_t* out_handle, 
                                                  uint32_t width, uint32_t height, 
                                                  uint32_t depth, uint32_t array_size, 
                                                  texture_format_t format);

int texture_format_conversion_enable_virtual_texturing(texture_format_conversion_handle_t handle, 
                                                   uint32_t page_size, uint32_t max_pages);

int texture_format_conversion_set_anisotropic_filtering(texture_format_conversion_handle_t handle, 
                                                    uint32_t anisotropy_level);

int texture_format_conversion_set_lod_levels(texture_format_conversion_handle_t handle, 
                                          uint32_t lod_count, 
                                          const float* lod_distances);

int texture_format_conversion_add_render_graph_node(const char* name, 
                                               void (*execute_func)(void*), 
                                               void* user_data);

int texture_format_conversion_start_hot_reload(const char* file_path, 
                                          void (*reload_callback)(const char*));

int texture_format_conversion_enable_gpu_integration(texture_format_conversion_handle_t handle);

int texture_format_conversion_get_bindless_handle(texture_format_conversion_handle_t handle, 
                                              uint32_t* out_gpu_handle);

int texture_format_conversion_get_performance_counters(performance_counters_t* out_counters);

int texture_format_conversion_get_feedback_analysis(feedback_analysis_t* out_feedback);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FORMAT_CONVERSION_H */
