/*
 * jiggle_bones.h
 * Secondary motion bones
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_JIGGLE_BONES_H
#define ANIMATION_JIGGLE_BONES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_jiggle_bones_handle {
    uint32_t id;
} animation_jiggle_bones_handle_t;

/* Jiggle bone configuration */
typedef struct animation_jiggle_bone_desc {
    float stiffness;           /* Spring stiffness (0.0-1.0) */
    float damping;             /* Damping factor (0.0-1.0) */
    float mass;                /* Bone mass for physics */
    float max_displacement;    /* Maximum displacement from rest */
    float gravity_factor;      /* Gravity influence (0.0-1.0) */
    uint32_t parent_bone_id;   /* Parent bone ID */
    uint32_t flags;            /* Bone-specific flags */
} animation_jiggle_bone_desc_t;

/* Morph target data */
typedef struct animation_morph_target {
    char name[64];             /* Morph target name */
    float weight;             /* Current weight (0.0-1.0) */
    float target_weight;      /* Target weight for blending */
    uint32_t vertex_count;    /* Number of vertices affected */
    uint32_t* vertex_indices; /* Affected vertex indices */
    float* vertex_offsets;    /* Vertex offset data */
} animation_morph_target_t;

/* Animation compression data */
typedef struct animation_compression {
    uint32_t compression_type;    /* 0=none, 1=quantization, 2=keyframe reduction */
    float quantization_precision; /* Precision for quantization */
    float keyframe_tolerance;     /* Tolerance for keyframe reduction */
    uint32_t compressed_size;      /* Size of compressed data */
    void* compressed_data;         /* Compressed animation data */
} animation_compression_t;

/* Procedural animation parameters */
typedef struct animation_procedural {
    float frequency;          /* Animation frequency */
    float amplitude;          /* Animation amplitude */
    float phase;              /* Current phase */
    uint32_t type;            /* 0=sine, 1=perlin, 2=custom */
    uint32_t seed;            /* Random seed */
} animation_procedural_t;

/* Ragdoll physics data */
typedef struct animation_ragdoll {
    bool enabled;             /* Ragdoll enabled */
    float friction;           /* Friction coefficient */
    float restitution;        /* Bounciness */
    float linear_damping;     /* Linear velocity damping */
    float angular_damping;    /* Angular velocity damping */
    uint32_t collision_group; /* Collision group */
} animation_ragdoll_t;

/* Animation retargeting data */
typedef struct animation_retargeting {
    char source_rig[64];      /* Source rig name */
    char target_rig[64];      /* Target rig name */
    float scale_factor;       /* Scale adjustment */
    uint32_t bone_mapping_count; /* Number of bone mappings */
    uint32_t* source_bones;   /* Source bone IDs */
    uint32_t* target_bones;   /* Target bone IDs */
} animation_retargeting_t;

/* Performance counters */
typedef struct animation_performance_counters {
    uint64_t update_calls;        /* Total update calls */
    uint64_t bone_updates;        /* Total bone updates */
    uint64_t morph_updates;       /* Total morph updates */
    uint64_t gpu_skinning_calls;  /* GPU skinning calls */
    uint64_t compression_saves;   /* Bytes saved by compression */
    uint64_t cache_hits;          /* Cache hits */
    uint64_t cache_misses;        /* Cache misses */
    float average_update_time;    /* Average update time (ms) */
    float peak_memory_usage;      /* Peak memory usage (MB) */
} animation_performance_counters_t;

/* LOD configuration */
typedef struct animation_lod_config {
    float distance_thresholds[4];  /* Distance thresholds for 4 LOD levels */
    uint32_t bone_counts[4];      /* Bone counts per LOD level */
    float update_rates[4];        /* Update rates per LOD level */
    bool enable_culling;          /* Enable distance culling */
} animation_lod_config_t;

/* Render graph node data */
typedef struct animation_render_node {
    uint32_t node_id;             /* Render graph node ID */
    uint32_t dependency_count;    /* Number of dependencies */
    uint32_t* dependencies;       /* Dependency node IDs */
    bool compute_shader;          /* Use compute shader */
    uint32_t workgroup_size[3];   /* Compute shader workgroup size */
} animation_render_node_t;

typedef struct animation_jiggle_bones_desc {
    uint32_t bone_count;                      /* Number of jiggle bones */
    animation_jiggle_bone_desc_t* bones;     /* Bone descriptions */
    uint32_t morph_target_count;              /* Number of morph targets */
    animation_morph_target_t* morph_targets;  /* Morph target data */
    animation_compression_t compression;      /* Compression settings */
    animation_procedural_t procedural;        /* Procedural animation */
    animation_ragdoll_t ragdoll;              /* Ragdoll physics */
    animation_retargeting_t retargeting;      /* Retargeting data */
    animation_lod_config_t lod_config;         /* LOD configuration */
    animation_render_node_t render_node;      /* Render graph node */
    uint32_t flags;                           /* Global flags */
    void* user_data;                          /* User data */
} animation_jiggle_bones_desc_t;

typedef struct animation_jiggle_bones_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t bone_count;
    uint32_t morph_target_count;
    animation_performance_counters_t performance;
    float current_lod_level;
    bool gpu_skinning_enabled;
    size_t memory_usage;
} animation_jiggle_bones_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_jiggle_bones_init(void);
void animation_jiggle_bones_shutdown(void);

/* Lifecycle */
int animation_jiggle_bones_create(animation_jiggle_bones_handle_t* out_handle, const animation_jiggle_bones_desc_t* desc);
void animation_jiggle_bones_destroy(animation_jiggle_bones_handle_t handle);

/* Skeletal Animation */
int animation_jiggle_bones_update_skeleton(animation_jiggle_bones_handle_t handle, const float* bone_transforms, uint32_t bone_count);
int animation_jiggle_bones_blend_animations(animation_jiggle_bones_handle_t handle, const uint32_t* animation_ids, const float* weights, uint32_t animation_count);

/* Morph Targets */
int animation_jiggle_bones_set_morph_weight(animation_jiggle_bones_handle_t handle, const char* morph_name, float weight);
float animation_jiggle_bones_get_morph_weight(animation_jiggle_bones_handle_t handle, const char* morph_name);
int animation_jiggle_bones_update_morph_targets(animation_jiggle_bones_handle_t handle, float delta_time);

/* GPU Skinning */
int animation_jiggle_bones_enable_gpu_skinning(animation_jiggle_bones_handle_t handle, bool enable);
int animation_jiggle_bones_upload_skinning_data(animation_jiggle_bones_handle_t handle, void* gpu_buffer);
int animation_jiggle_bones_dispatch_skinning_compute(animation_jiggle_bones_handle_t handle, uint32_t vertex_count);

/* Animation Compression */
int animation_jiggle_bones_compress_animation(animation_jiggle_bones_handle_t handle, uint32_t compression_type);
int animation_jiggle_bones_decompress_animation(animation_jiggle_bones_handle_t handle);
float animation_jiggle_bones_get_compression_ratio(animation_jiggle_bones_handle_t handle);

/* Procedural Animation */
int animation_jiggle_bones_update_procedural(animation_jiggle_bones_handle_t handle, float delta_time);
int animation_jiggle_bones_set_procedural_params(animation_jiggle_bones_handle_t handle, const animation_procedural_t* params);

/* Ragdoll Physics */
int animation_jiggle_bones_enable_ragdoll(animation_jiggle_bones_handle_t handle, bool enable);
int animation_jiggle_bones_update_ragdoll_physics(animation_jiggle_bones_handle_t handle, float delta_time);
int animation_jiggle_bones_apply_ragdoll_forces(animation_jiggle_bones_handle_t handle, const float* forces, const float* positions, uint32_t force_count);

/* Animation Retargeting */
int animation_jiggle_bones_set_retargeting(animation_jiggle_bones_handle_t handle, const animation_retargeting_t* retargeting);
int animation_jiggle_bones_retarget_animation(animation_jiggle_bones_handle_t handle, uint32_t source_animation_id);

/* Validation and Error Handling */
int animation_jiggle_bones_validate(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_get_last_error(animation_jiggle_bones_handle_t handle, char* error_buffer, size_t buffer_size);

/* Serialization */
int animation_jiggle_bones_serialize(animation_jiggle_bones_handle_t handle, void** out_data, size_t* out_size);
int animation_jiggle_bones_deserialize(animation_jiggle_bones_handle_t handle, const void* data, size_t size);

/* Performance and Debugging */
const animation_performance_counters_t* animation_jiggle_bones_get_performance_counters(animation_jiggle_bones_handle_t handle);
void animation_jiggle_bones_reset_performance_counters(animation_jiggle_bones_handle_t handle);

/* Hot Reload */
int animation_jiggle_bones_enable_hot_reload(animation_jiggle_bones_handle_t handle, bool enable);
int animation_jiggle_bones_check_hot_reload(animation_jiggle_bones_handle_t handle);

/* Thread Safety */
int animation_jiggle_bones_lock(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_unlock(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_try_lock(animation_jiggle_bones_handle_t handle);

/* Memory Management */
int animation_jiggle_bones_set_memory_budget(animation_jiggle_bones_handle_t handle, size_t budget_bytes);
size_t animation_jiggle_bones_get_memory_budget(animation_jiggle_bones_handle_t handle);

/* Caching */
int animation_jiggle_bones_invalidate_cache(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_set_cache_size(animation_jiggle_bones_handle_t handle, uint32_t cache_size);

/* Async Operations */
int animation_jiggle_bones_update_async(animation_jiggle_bones_handle_t handle, const void* data, size_t size);
int animation_jiggle_bones_wait_for_async(animation_jiggle_bones_handle_t handle);
bool animation_jiggle_bones_is_async_complete(animation_jiggle_bones_handle_t handle);

/* GPU Integration */
int animation_jiggle_bones_create_gpu_resources(animation_jiggle_bones_handle_t handle);
void animation_jiggle_bones_destroy_gpu_resources(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_sync_gpu_data(animation_jiggle_bones_handle_t handle);

/* SIMD Optimization */
int animation_jiggle_bones_enable_simd(animation_jiggle_bones_handle_t handle, bool enable);
bool animation_jiggle_bones_is_simd_available(void);

/* Batch Processing */
int animation_jiggle_bones_process_batch(animation_jiggle_bones_handle_t* handles, uint32_t handle_count, float delta_time);

/* Streaming Support */
int animation_jiggle_bones_stream_in(animation_jiggle_bones_handle_t handle, const void* stream_data, size_t stream_size);
int animation_jiggle_bones_stream_out(animation_jiggle_bones_handle_t handle, void** out_stream_data, size_t* out_stream_size);

/* LOD Support */
int animation_jiggle_bones_update_lod(animation_jiggle_bones_handle_t handle, const float* camera_position, float lod_distance);
float animation_jiggle_bones_get_current_lod(animation_jiggle_bones_handle_t handle);

/* Culling Integration */
bool animation_jiggle_bones_is_culled(animation_jiggle_bones_handle_t handle, const float* view_matrix, const float* projection_matrix);
int animation_jiggle_bones_set_culling_bounds(animation_jiggle_bones_handle_t handle, const float* min_bounds, const float* max_bounds);

/* Render Graph Integration */
int animation_jiggle_bones_add_to_render_graph(animation_jiggle_bones_handle_t handle, uint32_t render_graph_id);
int animation_jiggle_bones_remove_from_render_graph(animation_jiggle_bones_handle_t handle, uint32_t render_graph_id);

/* Core Operations */
int animation_jiggle_bones_update(animation_jiggle_bones_handle_t handle, const void* data, size_t size);
bool animation_jiggle_bones_is_valid(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_get_info(animation_jiggle_bones_handle_t handle, animation_jiggle_bones_info_t* out_info);
void animation_jiggle_bones_mark_dirty(animation_jiggle_bones_handle_t handle);
int animation_jiggle_bones_process_pending(void);

/* Statistics */
uint32_t animation_jiggle_bones_get_count(void);
size_t animation_jiggle_bones_get_memory_usage(void);
void animation_jiggle_bones_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_JIGGLE_BONES_H */
