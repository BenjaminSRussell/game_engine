/*
 * morph_data.h
 * Morph target vertex data
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_MORPH_DATA_H
#define ANIMATION_MORPH_DATA_H

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

#define ANIMATION_MORPH_DATA_MAX_TARGETS 256
#define ANIMATION_MORPH_DATA_MAX_VERTICES 65536
#define ANIMATION_MORPH_DATA_MAX_BONES 256
#define ANIMATION_MORPH_DATA_MAGIC_NUMBER 0x4D4F5246  // "MORF"
#define ANIMATION_MORPH_DATA_VERSION 1

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum {
    ANIMATION_MORPH_ERROR_NONE = 0,
    ANIMATION_MORPH_ERROR_INVALID_HANDLE = -1,
    ANIMATION_MORPH_ERROR_NOT_INITIALIZED = -2,
    ANIMATION_MORPH_ERROR_OUT_OF_MEMORY = -3,
    ANIMATION_MORPH_ERROR_INVALID_PARAMETER = -4,
    ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED = -5,
    ANIMATION_MORPH_ERROR_GPU_OPERATION_FAILED = -6,
    ANIMATION_MORPH_ERROR_THREAD_ERROR = -7
} animation_morph_error_t;

typedef enum {
    ANIMATION_MORPH_TARGET_POSITION = 0x01,
    ANIMATION_MORPH_TARGET_NORMAL = 0x02,
    ANIMATION_MORPH_TARGET_TANGENT = 0x04,
    ANIMATION_MORPH_TARGET_ALL = 0x07
} animation_morph_target_flags_t;

typedef enum {
    ANIMATION_MORPH_LOD_HIGH = 0,
    ANIMATION_MORPH_LOD_MEDIUM = 1,
    ANIMATION_MORPH_LOD_LOW = 2,
    ANIMATION_MORPH_LOD_COUNT
} animation_morph_lod_level_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_morph_vertex {
    float position[3];
    float normal[3];
    float tangent[3];
    float texcoord[2];
} animation_morph_vertex_t;

typedef struct animation_morph_target {
    char name[64];
    uint32_t vertex_count;
    animation_morph_vertex_t* vertices;
    float* weights;
    uint32_t flags;
    float influence;
} animation_morph_target_t;

typedef struct animation_morph_bone {
    char name[64];
    float transform[16];  // 4x4 matrix
    float inverse_bind[16];
    uint32_t parent_index;
    uint32_t* child_indices;
    uint32_t child_count;
} animation_morph_bone_t;

typedef struct animation_morph_skeleton {
    uint32_t bone_count;
    animation_morph_bone_t* bones;
    float* bone_matrices;
    float* skinning_matrices;
} animation_morph_skeleton_t;

typedef struct animation_morph_stats {
    uint32_t total_vertices;
    uint32_t total_targets;
    uint32_t total_bones;
    uint64_t memory_usage;
    uint64_t gpu_memory_usage;
    uint32_t cache_hits;
    uint32_t cache_misses;
    float average_processing_time;
    uint32_t async_operations_pending;
} animation_morph_stats_t;

typedef struct animation_morph_data_handle {
    uint32_t id;
} animation_morph_data_handle_t;

typedef struct animation_morph_data_desc {
    uint32_t vertex_count;
    uint32_t target_count;
    uint32_t bone_count;
    uint32_t flags;
    void* user_data;
} animation_morph_data_desc_t;

typedef struct animation_morph_data_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t vertex_count;
    uint32_t target_count;
    uint32_t bone_count;
    animation_morph_stats_t stats;
} animation_morph_data_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_morph_data_init(void);
void animation_morph_data_shutdown(void);

/* Lifecycle */
int animation_morph_data_create(animation_morph_data_handle_t* out_handle, const animation_morph_data_desc_t* desc);
void animation_morph_data_destroy(animation_morph_data_handle_t handle);

/* Operations */
int animation_morph_data_update(animation_morph_data_handle_t handle, const void* data, size_t size);
bool animation_morph_data_is_valid(animation_morph_data_handle_t handle);
int animation_morph_data_get_info(animation_morph_data_handle_t handle, animation_morph_data_info_t* out_info);
void animation_morph_data_mark_dirty(animation_morph_data_handle_t handle);
int animation_morph_data_process_pending(void);

/* Morph Target Operations */
int animation_morph_data_add_target(animation_morph_data_handle_t handle, const char* name, 
                                   const animation_morph_vertex_t* vertices, uint32_t vertex_count, 
                                   const float* weights, uint32_t flags);
int animation_morph_data_remove_target(animation_morph_data_handle_t handle, const char* name);
int animation_morph_data_set_target_influence(animation_morph_data_handle_t handle, const char* name, float influence);
float animation_morph_data_get_target_influence(animation_morph_data_handle_t handle, const char* name);

/* Skeletal Animation */
int animation_morph_data_set_skeleton(animation_morph_data_handle_t handle, const animation_morph_skeleton_t* skeleton);
int animation_morph_data_update_bone_transforms(animation_morph_data_handle_t handle, const float* transforms, uint32_t bone_count);
int animation_morph_data_apply_gpu_skinning(animation_morph_data_handle_t handle, const float* bone_matrices, uint32_t bone_count);

/* Animation Compression */
int animation_morph_data_compress_keyframes(animation_morph_data_handle_t handle, float tolerance);
int animation_morph_data_decompress_keyframes(animation_morph_data_handle_t handle);

/* Procedural Animation */
int animation_morph_data_generate_procedural_targets(animation_morph_data_handle_t handle, uint32_t seed, float complexity);
int animation_morph_data_apply_noise(animation_morph_data_handle_t handle, float amplitude, float frequency);

/* Ragdoll Physics */
int animation_morph_data_enable_ragdoll(animation_morph_data_handle_t handle, const float* gravity);
int animation_morph_data_update_ragdoll_physics(animation_morph_data_handle_t handle, float delta_time);

/* Animation Retargeting */
int animation_morph_data_retarget_from_skeleton(animation_morph_data_handle_t handle, const animation_morph_skeleton_t* source_skeleton);
int animation_morph_data_retarget_to_skeleton(animation_morph_data_handle_t handle, const animation_morph_skeleton_t* target_skeleton);

/* Validation and Error Handling */
int animation_morph_data_validate(animation_morph_data_handle_t handle);
const char* animation_morph_data_get_error_string(animation_morph_error_t error);

/* Serialization */
int animation_morph_data_serialize(animation_morph_data_handle_t handle, const char* filename);
int animation_morph_data_deserialize(animation_morph_data_handle_t* out_handle, const char* filename);

/* Performance Counters */
int animation_morph_data_get_stats(animation_morph_data_handle_t handle, animation_morph_stats_t* out_stats);
void animation_morph_data_reset_stats(animation_morph_data_handle_t handle);

/* Hot Reload */
int animation_morph_data_enable_hot_reload(animation_morph_data_handle_t handle, const char* filename);
void animation_morph_data_disable_hot_reload(animation_morph_data_handle_t handle);
bool animation_morph_data_has_pending_reload(animation_morph_data_handle_t handle);

/* Thread Safety */
int animation_morph_data_lock(animation_morph_data_handle_t handle);
int animation_morph_data_unlock(animation_morph_data_handle_t handle);
int animation_morph_data_try_lock(animation_morph_data_handle_t handle);

/* Memory Pooling */
int animation_morph_data_init_memory_pool(size_t pool_size);
void animation_morph_data_cleanup_memory_pool(void);
void* animation_morph_data_pool_alloc(size_t size);
void animation_morph_data_pool_free(void* ptr);

/* Caching Layer */
int animation_morph_data_init_cache(uint32_t max_entries);
void animation_morph_data_cleanup_cache(void);
int animation_morph_data_cache_get(animation_morph_data_handle_t handle, const char* key, void** out_data);
int animation_morph_data_cache_set(animation_morph_data_handle_t handle, const char* key, const void* data, size_t size);

/* Async Operations */
typedef void (*animation_morph_async_callback_t)(animation_morph_data_handle_t handle, int result, void* user_data);
int animation_morph_data_async_update(animation_morph_data_handle_t handle, const void* data, size_t size, 
                                      animation_morph_async_callback_t callback, void* user_data);
int animation_morph_data_process_async_queue(void);

/* GPU Integration */
int animation_morph_data_upload_to_gpu(animation_morph_data_handle_t handle);
int animation_morph_data_download_from_gpu(animation_morph_data_handle_t handle);
uint32_t animation_morph_data_get_gpu_buffer_id(animation_morph_data_handle_t handle);

/* SIMD Optimization */
int animation_morph_data_enable_simd_optimization(animation_morph_data_handle_t handle, bool enable);
bool animation_morph_data_is_simd_optimized(animation_morph_data_handle_t handle);

/* Batch Processing */
int animation_morph_data_process_batch(animation_morph_data_handle_t* handles, uint32_t handle_count);
int animation_morph_data_set_batch_size(uint32_t batch_size);

/* Streaming Support */
int animation_morph_data_stream_init(animation_morph_data_handle_t handle, uint32_t chunk_size);
int animation_morph_data_stream_chunk(animation_morph_data_handle_t handle, uint32_t chunk_index);
void animation_morph_data_stream_cleanup(animation_morph_data_handle_t handle);

/* LOD Support */
int animation_morph_data_set_lod_level(animation_morph_data_handle_t handle, animation_morph_lod_level_t lod_level);
animation_morph_lod_level_t animation_morph_data_get_lod_level(animation_morph_data_handle_t handle);
int animation_morph_data_generate_lod_data(animation_morph_data_handle_t handle, animation_morph_lod_level_t lod_level, float reduction_ratio);

/* Culling Integration */
bool animation_morph_data_is_visible(animation_morph_data_handle_t handle, const float* view_matrix, const float* projection_matrix);
int animation_morph_data_set_culling_bounds(animation_morph_data_handle_t handle, const float* min_bounds, const float* max_bounds);

/* Render Graph Node */
int animation_morph_data_add_render_graph_dependency(animation_morph_data_handle_t handle, const char* node_name);
int animation_morph_data_remove_render_graph_dependency(animation_morph_data_handle_t handle, const char* node_name);
int animation_morph_data_execute_render_graph_node(animation_morph_data_handle_t handle, const char* node_name);

/* Memory Tracking */
size_t animation_morph_data_get_peak_memory_usage(void);
void animation_morph_data_reset_peak_memory_usage(void);

/* Statistics */
uint32_t animation_morph_data_get_count(void);
size_t animation_morph_data_get_memory_usage(void);
void animation_morph_data_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_MORPH_DATA_H */
