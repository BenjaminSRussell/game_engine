/*
 * ragdoll_setup.h
 * Ragdoll skeleton setup
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_RAGDOLL_SETUP_H
#define ANIMATION_RAGDOLL_SETUP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_ragdoll_setup_handle {
    uint32_t id;
} animation_ragdoll_setup_handle_t;

typedef enum RagdollBoneShapeType {
    RAGDOLL_SHAPE_SPHERE,
    RAGDOLL_SHAPE_BOX,
    RAGDOLL_SHAPE_CAPSULE
} RagdollBoneShapeType;

typedef struct RagdollBoneSetup {
    char name[64];
    uint32_t bone_index;
    RagdollBoneShapeType shape_type;
    struct {
        float radius;
        float height;
        float half_extents[3];
    } dimensions;
    float offset[3];
    float rotation[4]; // Quaternion
    float mass;
    float friction;
    float restitution;

    // Joint config (constraint to parent)
    int32_t parent_bone_index; // -1 if root
    // Angular limits in radians
    float limits_min[3];
    float limits_max[3];
} RagdollBoneSetup;

typedef struct animation_ragdoll_setup_desc {
    uint32_t skeleton_id;
    uint32_t flags;
    uint32_t bone_count;
    RagdollBoneSetup* bones;
    void* user_data;
} animation_ragdoll_setup_desc_t;

typedef struct animation_ragdoll_setup_info {
    uint32_t id;
    uint32_t skeleton_id;
    uint32_t flags;
    uint32_t bone_count;
    bool initialized;
} animation_ragdoll_setup_info_t;

// Forward declaration for physics world
struct PhysicsWorld;
struct RigidBody;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_ragdoll_setup_init(void);
void animation_ragdoll_setup_shutdown(void);

/* Lifecycle */
int animation_ragdoll_setup_create(animation_ragdoll_setup_handle_t* out_handle, const animation_ragdoll_setup_desc_t* desc);
void animation_ragdoll_setup_destroy(animation_ragdoll_setup_handle_t handle);

/* Operations */
int animation_ragdoll_setup_update(animation_ragdoll_setup_handle_t handle, const void* data, size_t size);
bool animation_ragdoll_setup_is_valid(animation_ragdoll_setup_handle_t handle);
int animation_ragdoll_setup_get_info(animation_ragdoll_setup_handle_t handle, animation_ragdoll_setup_info_t* out_info);
void animation_ragdoll_setup_mark_dirty(animation_ragdoll_setup_handle_t handle);
int animation_ragdoll_setup_process_pending(void);

/* Physics Instantiation */
// Instantiates the ragdoll in the given physics world
// Returns an array of rigid body pointers via out_bodies (must be pre-allocated with bone_count)
// Returns 0 on success
int animation_ragdoll_setup_instantiate(animation_ragdoll_setup_handle_t handle,
                                        struct PhysicsWorld* world,
                                        const float* root_transform_mat4,
                                        struct RigidBody** out_bodies);

/* Statistics */
uint32_t animation_ragdoll_setup_get_count(void);
size_t animation_ragdoll_setup_get_memory_usage(void);
void animation_ragdoll_setup_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_RAGDOLL_SETUP_H */
