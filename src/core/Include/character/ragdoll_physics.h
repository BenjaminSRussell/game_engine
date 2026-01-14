#ifndef RAGDOLL_PHYSICS_H
#define RAGDOLL_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>
#include "math/vec3.h"
#include "math/quat.h"

// Maximum number of bones in a ragdoll
#define MAX_RAGDOLL_BONES 64

// Forward declarations for opaque types
typedef struct ragdoll_instance ragdoll_instance_t;
typedef struct ragdoll_bone ragdoll_bone_t;
typedef struct ragdoll_constraint ragdoll_constraint_t;

// Initialize the ragdoll physics system
bool ragdoll_init(void);

// Shutdown the ragdoll physics system
void ragdoll_shutdown(void);

// Create a ragdoll instance
// entity_id: Entity the ragdoll belongs to
// skeleton_id: Skeleton to base ragdoll on
// Returns: Ragdoll ID or 0 on failure
uint32_t ragdoll_create(uint32_t entity_id, uint32_t skeleton_id);

// Activate ragdoll physics (blend from animation to physics)
// ragdoll_id: Ragdoll to activate
// blend_time: Time to blend from animation to physics
bool ragdoll_activate(uint32_t ragdoll_id, float blend_time);

// Deactivate ragdoll physics (blend from physics back to animation)
// ragdoll_id: Ragdoll to deactivate
// blend_time: Time to blend from physics to animation
bool ragdoll_deactivate(uint32_t ragdoll_id, float blend_time);

// Update all ragdoll instances
// delta_time: Time since last update
void ragdoll_update(float delta_time);

// Apply force to a specific bone
// ragdoll_id: Ragdoll to modify
// bone_index: Index of bone to apply force to
// force: Force vector in world space
void ragdoll_apply_force(uint32_t ragdoll_id, uint32_t bone_index, const vec3_t force);

// Apply impulse to a specific bone
// ragdoll_id: Ragdoll to modify
// bone_index: Index of bone to apply impulse to
// impulse: Impulse vector in world space
void ragdoll_apply_impulse(uint32_t ragdoll_id, uint32_t bone_index, const vec3_t impulse);

// Apply torque to a specific bone
// ragdoll_id: Ragdoll to modify
// bone_index: Index of bone to apply torque to
// torque: Torque vector in world space
void ragdoll_apply_torque(uint32_t ragdoll_id, uint32_t bone_index, const vec3_t torque);

// Set mass for a specific bone
// ragdoll_id: Ragdoll to modify
// bone_index: Index of bone to modify
// mass: New mass value
bool ragdoll_set_bone_mass(uint32_t ragdoll_id, uint32_t bone_index, float mass);

// Get current transform for a specific bone
// ragdoll_id: Ragdoll to query
// bone_index: Index of bone to query
// position: Output world position
// rotation: Output world rotation
void ragdoll_get_bone_transform(uint32_t ragdoll_id, uint32_t bone_index, vec3_t* position, quat_t* rotation);

// Enable/disable auto-sleep for ragdoll
// ragdoll_id: Ragdoll to modify
// auto_sleep: Whether to enable auto-sleep
void ragdoll_set_auto_sleep(uint32_t ragdoll_id, bool auto_sleep);

// Set global gravity for ragdoll physics
// gravity: Gravity value (negative is down)
void ragdoll_set_gravity(float gravity);

// Get ragdoll system statistics
// active_ragdolls: Output number of active ragdolls
// sleeping_ragdolls: Output number of sleeping ragdolls
// average_bones: Output average number of bones per ragdoll
void ragdoll_get_statistics(uint32_t* active_ragdolls, uint32_t* sleeping_ragdolls, float* average_bones);

// Utility function to get current time (placeholder)
#include "core/utils.h"

#endif // RAGDOLL_PHYSICS_H
