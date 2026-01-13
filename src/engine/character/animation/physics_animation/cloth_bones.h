/*
 * cloth_bones.h
 * Cloth bone simulation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_CLOTH_BONES_H
#define ANIMATION_CLOTH_BONES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_cloth_bones_handle {
    uint32_t id;
} animation_cloth_bones_handle_t;

typedef struct animation_cloth_bones_desc {
    uint32_t flags;
    void* user_data;

    // Physics parameters
    uint32_t bone_count;
    const uint32_t* bone_indices; /* Array of bone indices in the skeleton */
    const Vec3* initial_positions; /* Array of initial positions (bind pose) for rest length calculation */

    Vec3 gravity;
    float stiffness;
    float damping;
    float drag;
    float mass;
    float wind_influence;

    // LOD and optimization
    float lod_distance_threshold;
    bool enable_culling;
} animation_cloth_bones_desc_t;

typedef struct animation_cloth_bones_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t bone_count;
    uint64_t last_update_time_ns;
    bool is_simulating;
} animation_cloth_bones_info_t;

typedef struct animation_cloth_bones_stats {
    uint64_t update_count;
    double total_update_time_ms;
    uint32_t active_simulations;
    size_t memory_usage;
} animation_cloth_bones_stats_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_cloth_bones_init(void);
void animation_cloth_bones_shutdown(void);

/* Lifecycle */
int animation_cloth_bones_create(animation_cloth_bones_handle_t* out_handle, const animation_cloth_bones_desc_t* desc);
void animation_cloth_bones_destroy(animation_cloth_bones_handle_t handle);

/* Operations */
int animation_cloth_bones_update(animation_cloth_bones_handle_t handle, float dt);
int animation_cloth_bones_set_root_transform(animation_cloth_bones_handle_t handle, Vec3 position);
int animation_cloth_bones_reset(animation_cloth_bones_handle_t handle);
bool animation_cloth_bones_is_valid(animation_cloth_bones_handle_t handle);
int animation_cloth_bones_get_info(animation_cloth_bones_handle_t handle, animation_cloth_bones_info_t* out_info);
void animation_cloth_bones_mark_dirty(animation_cloth_bones_handle_t handle);
int animation_cloth_bones_process_pending(void);

/* Physics & Simulation */
int animation_cloth_bones_set_params(animation_cloth_bones_handle_t handle, float stiffness, float damping);
int animation_cloth_bones_apply_force(animation_cloth_bones_handle_t handle, Vec3 force);

/* Serialization */
int animation_cloth_bones_serialize(animation_cloth_bones_handle_t handle, void** out_buffer, size_t* out_size);
int animation_cloth_bones_deserialize(animation_cloth_bones_handle_t handle, const void* buffer, size_t size);

/* Thread Safety */
int animation_cloth_bones_lock(animation_cloth_bones_handle_t handle);
int animation_cloth_bones_unlock(animation_cloth_bones_handle_t handle);

/* Statistics */
uint32_t animation_cloth_bones_get_count(void);
size_t animation_cloth_bones_get_memory_usage(void);
animation_cloth_bones_stats_t animation_cloth_bones_get_global_stats(void);
void animation_cloth_bones_debug_print(void);

/* Advanced Features (Stubs/Future) */
int animation_cloth_bones_set_lod(animation_cloth_bones_handle_t handle, int lod_level);
int animation_cloth_bones_enable_gpu(animation_cloth_bones_handle_t handle, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_CLOTH_BONES_H */
