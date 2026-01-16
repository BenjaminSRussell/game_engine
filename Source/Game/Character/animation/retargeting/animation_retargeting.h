/*
 * animation_retargeting.h
 * Enterprise-grade animation retargeting system
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features:
 * - Professional bone mapping algorithms
 * - Hierarchical pose transfer
 * - Scale compensation and adjustment
 * - Real-time retargeting with LOD support
 * - Multi-threaded processing
 * - Comprehensive error handling
 * - Performance optimization with caching
 * - Support for different skeleton topologies
 * - Advanced bone matching algorithms
 * - Motion preservation techniques
 */

#ifndef ANIMATION_RETARGETING_H
#define ANIMATION_RETARGETING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_RETARGETING_MAX_BONES 256
#define ANIMATION_RETARGETING_MAX_MAPPINGS 512
#define ANIMATION_RETARGETING_MAX_LOD_LEVELS 4
#define ANIMATION_RETARGETING_CACHE_SIZE 1024
#define ANIMATION_RETARGETING_MAX_SKELETONS 64
#define ANIMATION_RETARGETING_MAX_CLIPS 2048

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_retargeting_handle {
    uint32_t id;
} animation_retargeting_handle_t;

typedef struct animation_vec3 {
    float x, y, z;
} animation_vec3_t;

typedef struct animation_quat {
    float x, y, z, w;
} animation_quat_t;

typedef struct animation_transform {
    animation_vec3_t position;
    animation_quat_t rotation;
    animation_vec3_t scale;
} animation_transform_t;

typedef struct animation_bone {
    uint32_t id;
    char name[64];
    uint32_t parent_id;
    uint32_t child_count;
    uint32_t child_ids[32];
    animation_transform_t bind_pose;
    animation_transform_t inverse_bind_pose;
    float length;
    bool is_end_effector;
} animation_bone_t;

typedef struct animation_skeleton {
    char name[128];
    uint32_t bone_count;
    animation_bone_t bones[ANIMATION_RETARGETING_MAX_BONES];
    animation_transform_t root_transform;
    float units_per_meter;
    bool is_initialized;
} animation_skeleton_t;

typedef enum animation_retargeting_mapping_type {
    ANIMATION_RETARGETING_MAP_DIRECT = 0,
    ANIMATION_RETARGETING_MAP_SYMMETRIC = 1,
    ANIMATION_RETARGETING_MAP_HIERARCHICAL = 2,
    ANIMATION_RETARGETING_MAP_PROPORTIONAL = 3,
    ANIMATION_RETARGETING_MAP_CUSTOM = 4
} animation_retargeting_mapping_type_t;

typedef struct animation_retargeting_bone_mapping {
    uint32_t source_bone_id;
    uint32_t target_bone_id;
    animation_retargeting_mapping_type_t mapping_type;
    float weight;
    animation_transform_t offset_transform;
    bool preserve_length;
    bool preserve_proportions;
    float influence_radius;
    bool is_valid;
} animation_retargeting_bone_mapping_t;

typedef struct animation_retargeting_chain {
    uint32_t source_chain_start;
    uint32_t source_chain_end;
    uint32_t target_chain_start;
    uint32_t target_chain_end;
    uint32_t chain_length;
    bool preserve_chain_length;
    float chain_weight;
} animation_retargeting_chain_t;

typedef enum animation_retargeting_quality {
    ANIMATION_RETARGETING_QUALITY_LOW = 0,
    ANIMATION_RETARGETING_QUALITY_MEDIUM = 1,
    ANIMATION_RETARGETING_QUALITY_HIGH = 2,
    ANIMATION_RETARGETING_QUALITY_ULTRA = 3
} animation_retargeting_quality_t;

typedef struct animation_retargeting_settings {
    animation_retargeting_quality_t quality;
    float position_weight;
    float rotation_weight;
    float scale_weight;
    bool enable_position_retargeting;
    bool enable_rotation_retargeting;
    bool enable_scale_retargeting;
    bool preserve_root_motion;
    bool enable_ik_correction;
    bool enable_foot_sliding_correction;
    bool enable_joint_limits;
    float smoothness_factor;
    uint32_t lod_level;
    uint32_t flags;
} animation_retargeting_settings_t;

typedef struct animation_retargeting_cache_entry {
    uint32_t source_animation_id;
    uint32_t target_skeleton_id;
    void* retargeted_data;
    size_t data_size;
    uint64_t last_access_time;
    uint32_t access_count;
    bool is_valid;
} animation_retargeting_cache_entry_t;

typedef struct animation_retargeting_performance_stats {
    uint64_t total_retargets;
    uint64_t cache_hits;
    uint64_t cache_misses;
    float average_retarget_time_ms;
    float peak_retarget_time_ms;
    uint64_t total_bones_processed;
    uint64_t failed_mappings;
    uint64_t successful_mappings;
    float memory_usage_mb;
    uint64_t frame_count;
} animation_retargeting_performance_stats_t;

typedef struct animation_retargeting_desc {
    char name[128];
    animation_retargeting_settings_t settings;
    uint32_t max_cache_entries;
    bool enable_multithreading;
    bool enable_async_processing;
    size_t memory_budget;
    uint32_t flags;
    void* user_data;
} animation_retargeting_desc_t;

typedef struct animation_retargeting_info {
    uint32_t id;
    char name[128];
    uint32_t flags;
    bool initialized;
    uint32_t mapping_count;
    uint32_t chain_count;
    uint32_t cache_entry_count;
    animation_retargeting_performance_stats_t performance;
    float current_quality;
    size_t memory_usage;
} animation_retargeting_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization and Lifecycle */
int animation_retargeting_init(void);
void animation_retargeting_shutdown(void);

int animation_retargeting_create(animation_retargeting_handle_t* out_handle, const animation_retargeting_desc_t* desc);
void animation_retargeting_destroy(animation_retargeting_handle_t handle);

/* Skeleton Management */
int animation_retargeting_register_skeleton(animation_retargeting_handle_t handle, const char* name, const animation_skeleton_t* skeleton, uint32_t* out_skeleton_id);
int animation_retargeting_unregister_skeleton(animation_retargeting_handle_t handle, uint32_t skeleton_id);
int animation_retargeting_get_skeleton_info(animation_retargeting_handle_t handle, uint32_t skeleton_id, animation_skeleton_t* out_skeleton);

/* Bone Mapping */
int animation_retargeting_create_bone_mapping(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, animation_retargeting_bone_mapping_t* mapping);
int animation_retargeting_auto_map_bones(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, float similarity_threshold);
int animation_retargeting_create_symmetric_mapping(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const char* left_prefix, const char* right_prefix);
int animation_retargeting_create_chain_mapping(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, uint32_t source_start, uint32_t source_end, uint32_t target_start, uint32_t target_end);

/* Retargeting Operations */
int animation_retargeting_retarget_pose(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const animation_transform_t* source_pose, animation_transform_t* out_target_pose, uint32_t pose_size);
int animation_retargeting_retarget_animation(animation_retargeting_handle_t handle, uint32_t source_animation_id, uint32_t target_skeleton_id, uint32_t* out_animation_id);
int animation_retargeting_retarget_animation_async(animation_retargeting_handle_t handle, uint32_t source_animation_id, uint32_t target_skeleton_id, uint32_t* out_operation_id);
int animation_retargeting_retarget_frame(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const animation_transform_t* source_frame, animation_transform_t* out_target_frame, uint32_t bone_count);

/* Quality and LOD */
int animation_retargeting_set_quality(animation_retargeting_handle_t handle, animation_retargeting_quality_t quality);
animation_retargeting_quality_t animation_retargeting_get_quality(animation_retargeting_handle_t handle);
int animation_retargeting_update_lod(animation_retargeting_handle_t handle, float distance, const float* camera_position);

/* Caching */
int animation_retargeting_enable_caching(animation_retargeting_handle_t handle, bool enable);
int animation_retargeting_clear_cache(animation_retargeting_handle_t handle);
int animation_retargeting_set_cache_size(animation_retargeting_handle_t handle, uint32_t max_entries);
size_t animation_retargeting_get_cache_usage(animation_retargeting_handle_t handle);

/* Validation and Error Handling */
int animation_retargeting_validate_mapping(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id);
int animation_retargeting_get_last_error(animation_retargeting_handle_t handle, char* error_buffer, size_t buffer_size);

/* Performance and Debugging */
const animation_retargeting_performance_stats_t* animation_retargeting_get_performance_stats(animation_retargeting_handle_t handle);
void animation_retargeting_reset_performance_stats(animation_retargeting_handle_t handle);
int animation_retargeting_enable_debug_drawing(animation_retargeting_handle_t handle, bool enable);
int animation_retargeting_draw_debug(animation_retargeting_handle_t handle, void* debug_renderer);

/* Serialization */
int animation_retargeting_serialize_mappings(animation_retargeting_handle_t handle, const char* filename);
int animation_retargeting_deserialize_mappings(animation_retargeting_handle_t handle, const char* filename);
int animation_retargeting_export_mappings(animation_retargeting_handle_t handle, const char* filename, const char* format);

/* Memory Management */
int animation_retargeting_set_memory_budget(animation_retargeting_handle_t handle, size_t budget_bytes);
size_t animation_retargeting_get_memory_budget(animation_retargeting_handle_t handle);
size_t animation_retargeting_get_memory_usage(animation_retargeting_handle_t handle);

/* Statistics */
uint32_t animation_retargeting_get_skeleton_count(animation_retargeting_handle_t handle);
uint32_t animation_retargeting_get_mapping_count(animation_retargeting_handle_t handle);
animation_retargeting_info_t animation_retargeting_get_info(animation_retargeting_handle_t handle);

/* Thread Safety */
int animation_retargeting_lock(animation_retargeting_handle_t handle);
int animation_retargeting_unlock(animation_retargeting_handle_t handle);
int animation_retargeting_try_lock(animation_retargeting_handle_t handle);

/* Advanced Features */
int animation_retargeting_create_ik_chain(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, uint32_t chain_start, uint32_t chain_end, uint32_t* out_chain_id);
int animation_retargeting_solve_ik(animation_retargeting_handle_t handle, uint32_t chain_id, const animation_vec3_t* target_position, animation_transform_t* out_pose);
int animation_retargeting_apply_joint_limits(animation_retargeting_handle_t handle, uint32_t target_skeleton_id, animation_transform_t* pose, uint32_t bone_count);
int animation_retargeting_correct_foot_sliding(animation_retargeting_handle_t handle, uint32_t target_skeleton_id, animation_transform_t* pose, uint32_t bone_count, const animation_vec3_t* ground_plane);

/* Utility Functions */
float animation_retargeting_calculate_bone_similarity(const char* bone_name1, const char* bone_name2);
int animation_retargeting_find_bone_by_name(const animation_skeleton_t* skeleton, const char* bone_name);
int animation_retargeting_get_bone_hierarchy_level(const animation_skeleton_t* skeleton, uint32_t bone_id);
bool animation_retargeting_are_bones_symmetric(const char* bone_name1, const char* bone_name2, const char* left_prefix, const char* right_prefix);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_RETARGETING_H */