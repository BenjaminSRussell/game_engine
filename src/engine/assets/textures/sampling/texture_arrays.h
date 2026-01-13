/*
 * texture_arrays.h
 * Texture array management
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_TEXTURE_ARRAYS_H
#define TEXTURE_TEXTURE_ARRAYS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_TEXTURE_ARRAYS_MAX_LAYERS 2048
#define TEXTURE_TEXTURE_ARRAYS_MAX_MIP_LEVELS 16
#define TEXTURE_TEXTURE_ARRAYS_MAX_ANISOTROPY 16
#define TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE 64
#define TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE 128
#define TEXTURE_TEXTURE_ARRAYS_SIMD_WIDTH 16
#define TEXTURE_TEXTURE_ARRAYS_BATCH_SIZE 32
#define TEXTURE_TEXTURE_ARRAYS_VIRTUAL_TILE_SIZE 128
#define TEXTURE_TEXTURE_ARRAYS_FEEDBACK_BUFFER_SIZE 1024
#define TEXTURE_TEXTURE_ARRAYS_MEMORY_POOL_SIZE (64 * 1024 * 1024) // 64MB
#define TEXTURE_TEXTURE_ARRAYS_WORKER_THREADS 4

/* ============================================================================
 * ERROR CODES
 * ============================================================================ */

typedef enum {
    TEXTURE_ARRAYS_SUCCESS = 0,
    TEXTURE_ARRAYS_ERROR_INVALID_PARAM = -1,
    TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_ARRAYS_ERROR_INVALID_HANDLE = -3,
    TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY = -4,
    TEXTURE_ARRAYS_ERROR_ALREADY_INITIALIZED = -5,
    TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED = -6,
    TEXTURE_ARRAYS_ERROR_FILE_WATCH_FAILED = -7,
    TEXTURE_ARRAYS_ERROR_COMPRESSION_FAILED = -8,
    TEXTURE_ARRAYS_ERROR_FORMAT_NOT_SUPPORTED = -9,
    TEXTURE_ARRAYS_ERROR_GPU_OPERATION_FAILED = -10
} texture_arrays_error_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Texture format types */
typedef enum {
    TEXTURE_FORMAT_R8_UNORM = 0,
    TEXTURE_FORMAT_RG8_UNORM,
    TEXTURE_FORMAT_RGB8_UNORM,
    TEXTURE_FORMAT_RGBA8_UNORM,
    TEXTURE_FORMAT_R16_FLOAT,
    TEXTURE_FORMAT_RG16_FLOAT,
    TEXTURE_FORMAT_RGB16_FLOAT,
    TEXTURE_FORMAT_RGBA16_FLOAT,
    TEXTURE_FORMAT_R32_FLOAT,
    TEXTURE_FORMAT_RG32_FLOAT,
    TEXTURE_FORMAT_RGB32_FLOAT,
    TEXTURE_FORMAT_RGBA32_FLOAT,
    TEXTURE_FORMAT_BC1_UNORM,
    TEXTURE_FORMAT_BC3_UNORM,
    TEXTURE_FORMAT_BC5_UNORM,
    TEXTURE_FORMAT_BC7_UNORM,
    TEXTURE_FORMAT_ASTC_4x4_UNORM,
    TEXTURE_FORMAT_ASTC_6x6_UNORM,
    TEXTURE_FORMAT_ASTC_8x8_UNORM,
    TEXTURE_FORMAT_COUNT
} texture_format_t;

/* Texture filtering modes */
typedef enum {
    TEXTURE_FILTER_NEAREST = 0,
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_TRILINEAR,
    TEXTURE_FILTER_ANISOTROPIC
} texture_filter_t;

/* Texture wrap modes */
typedef enum {
    TEXTURE_WRAP_REPEAT = 0,
    TEXTURE_WRAP_CLAMP_TO_EDGE,
    TEXTURE_WRAP_CLAMP_TO_BORDER,
    TEXTURE_WRAP_MIRRORED_REPEAT
} texture_wrap_t;

/* Anisotropic filtering levels */
typedef enum {
    ANISOTROPIC_1X = 1,
    ANISOTROPIC_2X = 2,
    ANISOTROPIC_4X = 4,
    ANISOTROPIC_8X = 8,
    ANISOTROPIC_16X = 16
} anisotropic_level_t;

/* LOD information */
typedef struct {
    uint32_t level;
    float distance_threshold;
    uint32_t width;
    uint32_t height;
    size_t memory_size;
    bool active;
} texture_lod_info_t;

/* Virtual texture page */
typedef struct {
    uint32_t x, y;
    uint32_t mip_level;
    uint32_t layer;
    bool resident;
    uint64_t last_access;
    void* data;
    size_t data_size;
} texture_virtual_page_t;

/* Performance counters */
typedef struct {
    uint64_t samples_processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t async_operations;
    uint64_t compression_operations;
    uint64_t virtual_texture_operations;
    uint64_t batch_operations;
    uint64_t processing_time_ms;
    size_t memory_peak_usage;
} texture_arrays_performance_counters_t;

/* Cache entry */
typedef struct {
    uint32_t texture_id;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool dirty;
} texture_cache_entry_t;

/* Async operation */
typedef struct {
    uint32_t id;
    enum {
        ASYNC_OP_COMPRESS,
        ASYNC_OP_GENERATE_MIPMAPS,
        ASYNC_OP_UPLOAD,
        ASYNC_OP_DOWNLOAD,
        ASYNC_OP_CONVERT_FORMAT
    } type;
    void* data;
    size_t data_size;
    bool completed;
    int error_code;
    pthread_mutex_t completion_mutex;
    pthread_cond_t completion_cond;
} texture_async_operation_t;

/* Render graph node */
typedef struct {
    uint32_t node_id;
    uint32_t texture_id;
    enum {
        NODE_TYPE_SAMPLE,
        NODE_TYPE_FILTER,
        NODE_TYPE_COMPRESS,
        NODE_TYPE_GENERATE_MIPMAPS
    } type;
    uint32_t dependencies[8];
    uint32_t dependency_count;
    bool processed;
    uint64_t timestamp;
} texture_render_graph_node_t;

/* Batch processing operation */
typedef struct {
    uint32_t* texture_ids;
    uint32_t texture_count;
    enum {
        BATCH_OP_FILTER,
        BATCH_OP_COMPRESS,
        BATCH_OP_GENERATE_MIPMAPS,
        BATCH_OP_CONVERT_FORMAT
    } operation_type;
    void* operation_params;
    bool completed;
} texture_batch_operation_t;

/* GPU context */
typedef struct {
    bool initialized;
    void* device;
    void* command_queue;
    void* shader_program;
    uint32_t* texture_handles;
    uint32_t max_textures;
    pthread_mutex_t gpu_mutex;
} texture_gpu_context_t;

typedef struct texture_texture_arrays_handle {
    uint32_t id;
} texture_texture_arrays_handle_t;

typedef struct texture_texture_arrays_desc {
    uint32_t flags;
    void* user_data;
    texture_format_t format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t array_size;
    uint32_t mip_levels;
    texture_filter_t filter_mode;
    anisotropic_level_t anisotropy_level;
    bool anisotropic_enabled;
    bool virtual_texturing_enabled;
    bool compression_enabled;
} texture_texture_arrays_desc_t;

typedef struct texture_texture_arrays_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    texture_format_t format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t array_size;
    uint32_t mip_levels;
    texture_filter_t filter_mode;
    anisotropic_level_t anisotropy_level;
    bool anisotropic_enabled;
    bool virtual_texturing_enabled;
    bool compression_enabled;
    size_t memory_usage;
    bool gpu_resident;
} texture_texture_arrays_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_texture_arrays_init(void);
void texture_texture_arrays_shutdown(void);

/* Lifecycle */
int texture_texture_arrays_create(texture_texture_arrays_handle_t* out_handle, const texture_texture_arrays_desc_t* desc);
void texture_texture_arrays_destroy(texture_texture_arrays_handle_t handle);

/* Operations */
int texture_texture_arrays_update(texture_texture_arrays_handle_t handle, const void* data, size_t size);
bool texture_texture_arrays_is_valid(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_get_info(texture_texture_arrays_handle_t handle, texture_texture_arrays_info_t* out_info);
void texture_texture_arrays_mark_dirty(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_process_pending(void);

/* Format Conversion */
int texture_texture_arrays_convert_format(texture_texture_arrays_handle_t handle, texture_format_t target_format);
int texture_texture_arrays_get_format_size(texture_format_t format, size_t* out_size);

/* Anisotropic Filtering */
int texture_texture_arrays_set_anisotropic_filtering(texture_texture_arrays_handle_t handle, anisotropic_level_t level, bool enabled);
int texture_texture_arrays_get_anisotropic_filtering(texture_texture_arrays_handle_t handle, anisotropic_level_t* out_level, bool* out_enabled);

/* Texture Arrays Initialization */
int texture_texture_arrays_init_array(texture_texture_arrays_handle_t handle, uint32_t layers);
int texture_texture_arrays_resize_array(texture_texture_arrays_handle_t handle, uint32_t new_layers);

/* Texture Arrays Cleanup/Shutdown */
int texture_texture_arrays_cleanup_array(texture_texture_arrays_handle_t handle);
void texture_texture_arrays_cleanup_all_arrays(void);

/* Texture Arrays Validation */
int texture_texture_arrays_validate_array(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_validate_all_arrays(void);

/* Texture Arrays Error Handling */
const char* texture_texture_arrays_get_error_string(texture_arrays_error_t error_code);
int texture_texture_arrays_get_last_error(texture_texture_arrays_handle_t handle, texture_arrays_error_t* out_error);

/* Texture Arrays Serialization */
int texture_texture_arrays_serialize(texture_texture_arrays_handle_t handle, void* buffer, size_t buffer_size, size_t* out_written);
int texture_texture_arrays_deserialize(texture_texture_arrays_handle_t* out_handle, const void* buffer, size_t buffer_size);
int texture_texture_arrays_serialize_state(void* buffer, size_t buffer_size, size_t* out_written);
int texture_texture_arrays_deserialize_state(const void* buffer, size_t buffer_size);

/* Texture Arrays Performance Counters */
int texture_texture_arrays_get_performance_counters(texture_arrays_performance_counters_t* out_counters);
int texture_texture_arrays_reset_performance_counters(void);
void texture_texture_arrays_update_performance_counters(uint64_t samples, uint64_t time_ms);

/* Texture Arrays Hot-Reload */
int texture_texture_arrays_enable_hot_reload(texture_texture_arrays_handle_t handle, const char* file_path);
int texture_texture_arrays_disable_hot_reload(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_check_hot_reload(texture_texture_arrays_handle_t handle);

/* Texture Arrays Thread Safety */
int texture_texture_arrays_lock(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_unlock(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_try_lock(texture_texture_arrays_handle_t handle);

/* Texture Arrays Memory Pooling */
int texture_texture_arrays_init_memory_pool(size_t pool_size);
void* texture_texture_arrays_pool_alloc(size_t size);
void texture_texture_arrays_pool_free(void* ptr);
size_t texture_texture_arrays_get_pool_usage(void);

/* Texture Arrays Caching Layer */
int texture_texture_arrays_cache_add(texture_texture_arrays_handle_t handle, const void* data, size_t size);
int texture_texture_arrays_cache_get(texture_texture_arrays_handle_t handle, void** out_data, size_t* out_size);
int texture_texture_arrays_cache_remove(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_cache_clear(void);
uint32_t texture_texture_arrays_cache_get_hit_rate(void);

/* Texture Arrays Async Operations */
int texture_texture_arrays_async_upload(texture_texture_arrays_handle_t handle, const void* data, size_t size);
int texture_texture_arrays_async_download(texture_texture_arrays_handle_t handle, void* buffer, size_t buffer_size);
int texture_texture_arrays_async_compress(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_async_generate_mipmaps(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_async_wait(texture_texture_arrays_handle_t handle, uint32_t timeout_ms);

/* Texture Arrays GPU Integration */
int texture_texture_arrays_gpu_upload(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_gpu_download(texture_texture_arrays_handle_t handle, void* buffer, size_t buffer_size);
int texture_texture_arrays_gpu_bind(texture_texture_arrays_handle_t handle, uint32_t unit);
int texture_texture_arrays_gpu_unbind(uint32_t unit);
bool texture_texture_arrays_is_gpu_resident(texture_texture_arrays_handle_t handle);

/* Texture Arrays SIMD Optimization */
int texture_texture_arrays_enable_simd(bool enabled);
bool texture_texture_arrays_is_simd_enabled(void);
int texture_texture_arrays_simd_filter(texture_texture_arrays_handle_t handle, const void* src, void* dst, size_t pixel_count);

/* Texture Arrays Batch Processing */
int texture_texture_arrays_batch_begin(void);
int texture_texture_arrays_batch_add(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_batch_filter(anisotropic_level_t anisotropy_level);
int texture_texture_arrays_batch_compress(uint32_t compression_level);
int texture_texture_arrays_batch_generate_mipmaps(void);
int texture_texture_arrays_batch_end(void);

/* Texture Arrays LOD Support */
int texture_texture_arrays_set_lod_range(texture_texture_arrays_handle_t handle, float min_lod, float max_lod, float lod_bias);
int texture_texture_arrays_get_lod_range(texture_texture_arrays_handle_t handle, float* out_min_lod, float* out_max_lod, float* out_lod_bias);
int texture_texture_arrays_select_lod_level(texture_texture_arrays_handle_t handle, uint32_t level);
uint32_t texture_texture_arrays_get_active_lod_level(texture_texture_arrays_handle_t handle);

/* Texture Arrays Culling Integration */
int texture_texture_arrays_enable_culling(texture_texture_arrays_handle_t handle, bool enabled);
int texture_texture_arrays_set_culling_threshold(texture_texture_arrays_handle_t handle, float threshold);
int texture_texture_arrays_is_culled(texture_texture_arrays_handle_t handle, bool* out_culled);

/* Texture Arrays Render Graph Node */
int texture_texture_arrays_add_render_node(uint32_t texture_id, uint32_t node_type, const uint32_t* dependencies, uint32_t dependency_count);
int texture_texture_arrays_remove_render_node(uint32_t node_id);
int texture_texture_arrays_process_render_graph(void);
bool texture_texture_arrays_is_render_node_processed(uint32_t node_id);

/* Virtual Texturing */
int texture_texture_arrays_enable_virtual_texturing(texture_texture_arrays_handle_t handle, bool enabled);
int texture_texture_arrays_load_virtual_tile(texture_texture_arrays_handle_t handle, uint32_t x, uint32_t y, uint32_t mip_level, uint32_t layer);
int texture_texture_arrays_unload_virtual_tile(texture_texture_arrays_handle_t handle, uint32_t x, uint32_t y, uint32_t mip_level, uint32_t layer);
int texture_texture_arrays_update_virtual_tile_residency(texture_texture_arrays_handle_t handle);

/* BC/ASTC Compression */
int texture_texture_arrays_compress_bc(texture_texture_arrays_handle_t handle, uint32_t compression_level);
int texture_texture_arrays_compress_astc(texture_texture_arrays_handle_t handle, uint32_t block_size);
int texture_texture_arrays_decompress(texture_texture_arrays_handle_t handle);
float texture_texture_arrays_get_compression_ratio(texture_texture_arrays_handle_t handle);

/* Mipmap Generation */
int texture_texture_arrays_generate_mipmaps(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_set_mipmap_filter(texture_texture_arrays_handle_t handle, texture_filter_t filter);
uint32_t texture_texture_arrays_get_mipmap_level_count(texture_texture_arrays_handle_t handle);

/* Bindless Textures */
int texture_texture_arrays_enable_bindless(texture_texture_arrays_handle_t handle);
uint64_t texture_texture_arrays_get_bindless_handle(texture_texture_arrays_handle_t handle);
int texture_texture_arrays_bindless_bind(uint64_t handle, uint32_t unit);
void texture_texture_arrays_bindless_unbind(uint32_t unit);

/* Feedback Analysis */
int texture_texture_arrays_enable_feedback_analysis(texture_texture_arrays_handle_t handle, bool enabled);
int texture_texture_arrays_get_feedback_data(texture_texture_arrays_handle_t handle, float* out_min_lod, float* out_max_lod, float* out_avg_lod);
int texture_texture_arrays_reset_feedback_analysis(texture_texture_arrays_handle_t handle);

/* Statistics */
uint32_t texture_texture_arrays_get_count(void);
size_t texture_texture_arrays_get_memory_usage(void);
void texture_texture_arrays_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_TEXTURE_ARRAYS_H */
