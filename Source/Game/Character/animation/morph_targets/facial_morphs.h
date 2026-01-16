/*
 * facial_morphs.h
 * Facial expression morphs
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features:
 * - Procedural animation system
 * - Ragdoll physics integration
 * - Animation retargeting
 * - Facial morphs initialization and management
 * - Performance counters and hot-reload
 * - Thread safety and async operations
 * - GPU integration and SIMD optimization
 * - Batch processing and streaming support
 * - LOD support and culling integration
 * - Render graph node integration
 * - Skeletal animation and morph targets
 * - GPU skinning
 */

#ifndef ANIMATION_FACIAL_MORPHS_H
#define ANIMATION_FACIAL_MORPHS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_FACIAL_MORPHS_MAX_MORPH_TARGETS 64
#define ANIMATION_FACIAL_MORPHS_MAX_BONES 256
#define ANIMATION_FACIAL_MORPHS_LOD_LEVELS 4

// Error codes
#define ANIMATION_FACIAL_MORPHS_SUCCESS 0
#define ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM -1
#define ANIMATION_FACIAL_MORPHS_ERROR_NOT_INITIALIZED -2
#define ANIMATION_FACIAL_MORPHS_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_FACIAL_MORPHS_ERROR_CAPACITY_EXCEEDED -4
#define ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE -5
#define ANIMATION_FACIAL_MORPHS_ERROR_SERIALIZATION_FAILED -6
#define ANIMATION_FACIAL_MORPHS_ERROR_GPU_OPERATION_FAILED -7
#define ANIMATION_FACIAL_MORPHS_ERROR_THREAD_ERROR -8
#define ANIMATION_FACIAL_MORPHS_ERROR_FILE_NOT_FOUND -9
#define ANIMATION_FACIAL_MORPHS_ERROR_INVALID_FORMAT -10

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum {
    ANIMATION_FACIAL_MORPHS_TYPE_NONE = 0,
    ANIMATION_FACIAL_MORPHS_TYPE_EXPRESSION,
    ANIMATION_FACIAL_MORPHS_TYPE_PHONEME,
    ANIMATION_FACIAL_MORPHS_TYPE_EMOTION,
    ANIMATION_FACIAL_MORPHS_TYPE_PROCEDURAL,
    ANIMATION_FACIAL_MORPHS_TYPE_COUNT
} animation_facial_morphs_type_t;

typedef enum {
    ANIMATION_FACIAL_MORPHS_LOD_HIGH = 0,
    ANIMATION_FACIAL_MORPHS_LOD_MEDIUM,
    ANIMATION_FACIAL_MORPHS_LOD_LOW,
    ANIMATION_FACIAL_MORPHS_LOD_MINIMAL
} animation_facial_morphs_lod_t;

/* ============================================================================
 * VECTORS AND MATRICES
 * ============================================================================ */

typedef struct {
    float x, y, z;
} animation_facial_morphs_vec3_t;

typedef struct {
    float x, y, z, w;
} animation_facial_morphs_vec4_t;

typedef struct {
    float m[16];  // Column-major 4x4 matrix
} animation_facial_morphs_mat4_t;

/* ============================================================================
 * MORPH TARGET DATA
 * ============================================================================ */

typedef struct {
    uint32_t vertex_index;
    animation_facial_morphs_vec3_t position_delta;
    animation_facial_morphs_vec3_t normal_delta;
    float weight;
} animation_facial_morphs_vertex_delta_t;

typedef struct {
    char name[64];
    facial_expression_type_t type;
    uint32_t vertex_count;
    animation_facial_morphs_vertex_delta_t* vertices;
    float current_weight;
    float target_weight;
    bool enabled;
} animation_facial_morphs_target_t;

/* ============================================================================
 * SKELETAL ANIMATION DATA
 * ============================================================================ */

typedef struct {
    char name[64];
    uint32_t parent_index;
    animation_facial_morphs_mat4_t bind_pose;
    animation_facial_morphs_mat4_t inverse_bind_pose;
    animation_facial_morphs_mat4_t current_transform;
} animation_facial_morphs_bone_t;

typedef struct {
    animation_facial_morphs_vec3_t position;
    animation_facial_morphs_vec4_t rotation;  // Quaternion
    animation_facial_morphs_vec3_t scale;
} animation_facial_morphs_pose_t;

/* ============================================================================
 * PERFORMANCE COUNTERS
 * ============================================================================ */

typedef struct {
    uint64_t total_updates;
    uint64_t total_render_calls;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations;
    uint64_t gpu_operations;
    double total_update_time;
    double total_render_time;
    double peak_memory_usage;
    uint32_t active_morphs;
} animation_facial_morphs_perf_counters_t;

/* ============================================================================
 * BASIC TYPES
 * ============================================================================ */

typedef struct animation_facial_morphs_handle {
    uint32_t id;
} animation_facial_morphs_handle_t;

typedef struct animation_facial_morphs_desc {
    uint32_t vertex_count;
    uint32_t target_count;
    uint32_t bone_count;
    uint32_t flags;
    void* user_data;
} animation_facial_morphs_desc_t;

typedef struct animation_facial_morphs_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t vertex_count;
    uint32_t target_count;
    uint32_t bone_count;
    animation_facial_morphs_perf_counters_t stats;
} animation_facial_morphs_info_t;

/* ============================================================================
 * CORE API
 * ============================================================================ */

/* Initialization */
int animation_facial_morphs_init(void);
void animation_facial_morphs_shutdown(void);

/* Lifecycle */
int animation_facial_morphs_create(animation_facial_morphs_handle_t* out_handle, const animation_facial_morphs_desc_t* desc);
void animation_facial_morphs_destroy(animation_facial_morphs_handle_t handle);

/* Operations */
int animation_facial_morphs_update(animation_facial_morphs_handle_t handle, const void* data, size_t size);
bool animation_facial_morphs_is_valid(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_get_info(animation_facial_morphs_handle_t handle, animation_facial_morphs_info_t* out_info);
void animation_facial_morphs_mark_dirty(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_process_pending(void);

/* ============================================================================
 * SKELETAL ANIMATION
 * ============================================================================ */

int animation_facial_morphs_set_skeleton(animation_facial_morphs_handle_t handle, const animation_facial_morphs_bone_t* bones, uint32_t bone_count);
int animation_facial_morphs_update_bone_transforms(animation_facial_morphs_handle_t handle, const animation_facial_morphs_mat4_t* transforms, uint32_t bone_count);
int animation_facial_morphs_apply_pose(animation_facial_morphs_handle_t handle, const animation_facial_morphs_pose_t* pose);

/* GPU Skinning */
int animation_facial_morphs_upload_to_gpu(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_enable_gpu_skinning(animation_facial_morphs_handle_t handle, bool enable);
bool animation_facial_morphs_is_gpu_skinning_enabled(animation_facial_morphs_handle_t handle);

/* Procedural Animation */
int animation_facial_morphs_set_procedural_params(animation_facial_morphs_handle_t handle, float frequency, float amplitude, float phase, float time_scale);
int animation_facial_morphs_enable_procedural(animation_facial_morphs_handle_t handle, bool enable);
int animation_facial_morphs_update_procedural_animation(animation_facial_morphs_handle_t handle, float time);

/* Ragdoll Physics */
int animation_facial_morphs_enable_ragdoll(animation_facial_morphs_handle_t handle, bool enable);
int animation_facial_morphs_set_ragdoll_params(animation_facial_morphs_handle_t handle, float mass, float friction, float damping);
int animation_facial_morphs_update_ragdoll_physics(animation_facial_morphs_handle_t handle, float delta_time);

/* Animation Retargeting */
int animation_facial_morphs_add_retarget_mapping(animation_facial_morphs_handle_t handle, uint32_t source_bone, uint32_t target_bone, float scale);
int animation_facial_morphs_enable_retargeting(animation_facial_morphs_handle_t handle, bool enable);
int animation_facial_morphs_apply_retargeting(animation_facial_morphs_handle_t handle);

/* Compression */
int animation_facial_morphs_enable_compression(animation_facial_morphs_handle_t handle, bool enable);
int animation_facial_morphs_compress_data(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_decompress_data(animation_facial_morphs_handle_t handle, void** out_data, size_t* out_size);

/* Performance and Debugging */
uint32_t animation_facial_morphs_get_count(void);
size_t animation_facial_morphs_get_memory_usage(void);
int animation_facial_morphs_get_performance_info(animation_facial_morphs_performance_info_t* out_info);
void animation_facial_morphs_debug_print(void);
void animation_facial_morphs_reset_performance_counters(void);

/* Hot-reload */
int animation_facial_morphs_enable_hot_reload(animation_facial_morphs_handle_t handle, bool enable);
int animation_facial_morphs_set_watch_directory(const char* directory);
bool animation_facial_morphs_is_hot_reload_enabled(void);

/* Thread Safety */
int animation_facial_morphs_lock(void);
int animation_facial_morphs_unlock(void);
int animation_facial_morphs_trylock(void);

/* Caching */
int animation_facial_morphs_cache_get(animation_facial_morphs_handle_t handle, void** out_data, size_t* out_size);
int animation_facial_morphs_cache_put(animation_facial_morphs_handle_t handle, const void* data, size_t size);
int animation_facial_morphs_cache_invalidate(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_cache_clear(void);

/* Async Operations */
typedef void (*animation_facial_morphs_async_callback_t)(uint32_t operation_id, int result, void* user_data);

int animation_facial_morphs_async_load(animation_facial_morphs_handle_t handle, const char* filename, animation_facial_morphs_async_callback_t callback, void* user_data);
int animation_facial_morphs_async_save(animation_facial_morphs_handle_t handle, const char* filename, animation_facial_morphs_async_callback_t callback, void* user_data);
int animation_facial_morphs_async_process(animation_facial_morphs_handle_t handle, animation_facial_morphs_async_callback_t callback, void* user_data);
bool animation_facial_morphs_is_operation_complete(uint32_t operation_id);

/* SIMD Optimization */
int animation_facial_morphs_enable_simd(animation_facial_morphs_handle_t handle, bool enable);
bool animation_facial_morphs_is_simd_enabled(animation_facial_morphs_handle_t handle);

/* Batch Processing */
int animation_facial_morphs_batch_add(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_batch_process_all(void);
int animation_facial_morphs_batch_clear(void);

/* Streaming Support */
int animation_facial_morphs_stream_open(animation_facial_morphs_handle_t handle, const char* filename);
int animation_facial_morphs_stream_read(animation_facial_morphs_handle_t handle, void* buffer, size_t buffer_size, size_t* bytes_read);
int animation_facial_morphs_stream_close(animation_facial_morphs_handle_t handle);
bool animation_facial_morphs_is_streaming_active(animation_facial_morphs_handle_t handle);

/* LOD Support */
int animation_facial_morphs_set_lod_distances(animation_facial_morphs_handle_t handle, const float distances[ANIMATION_FACIAL_MORPHS_LOD_LEVELS]);
int animation_facial_morphs_update_lod(animation_facial_morphs_handle_t handle, float distance);
animation_facial_morphs_lod_t animation_facial_morphs_get_current_lod(animation_facial_morphs_handle_t handle);

/* Culling Integration */
int animation_facial_morphs_set_culling_distance(animation_facial_morphs_handle_t handle, float distance);
int animation_facial_morphs_update_culling(animation_facial_morphs_handle_t handle, float distance);
bool animation_facial_morphs_is_culled(animation_facial_morphs_handle_t handle);

/* Render Graph Integration */
int animation_facial_morphs_add_to_render_graph(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_remove_from_render_graph(animation_facial_morphs_handle_t handle);
int animation_facial_morphs_set_render_dependencies(animation_facial_morphs_handle_t handle, const uint32_t* dependencies, uint32_t count);

/* Serialization */
int animation_facial_morphs_serialize(animation_facial_morphs_handle_t handle, const char* filename);
int animation_facial_morphs_deserialize(animation_facial_morphs_handle_t* out_handle, const char* filename);
int animation_facial_morphs_serialize_to_memory(animation_facial_morphs_handle_t handle, void** out_data, size_t* out_size);
int animation_facial_morphs_deserialize_from_memory(animation_facial_morphs_handle_t* out_handle, const void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_FACIAL_MORPHS_H */
