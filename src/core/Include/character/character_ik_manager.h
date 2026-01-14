#ifndef CHARACTER_IK_MANAGER_H
#define CHARACTER_IK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "math/vec3.h"
#include "math/quat.h"

// Maximum number of bones in an IK chain
#define MAX_IK_CHAIN_LENGTH 8

// IK solver types
typedef enum ik_solver_type {
    IK_SOLVER_FOOT = 0,
    IK_SOLVER_HAND,
    IK_SOLVER_SPINE,
    IK_SOLVER_HEAD,
    IK_SOLVER_LOOK_AT,
    IK_SOLVER_FABRIK,
    IK_SOLVER_CCD,
    IK_SOLVER_TWO_BONE,
    IK_SOLVER_AIM
} ik_solver_type_t;

// IK priority levels
typedef enum ik_priority {
    IK_PRIORITY_LOW = 0,
    IK_PRIORITY_MEDIUM,
    IK_PRIORITY_HIGH,
    IK_PRIORITY_CRITICAL
} ik_priority_t;

// Forward declarations for opaque types
typedef struct ik_target ik_target_t;
typedef struct ik_chain ik_chain_t;
typedef struct ik_solver ik_solver_t;

// Forward declarations for specific solver types
typedef struct foot_ik_solver_t foot_ik_solver_t;
typedef struct hand_ik_solver_t hand_ik_solver_t;
typedef struct spine_ik_solver_t spine_ik_solver_t;
typedef struct look_at_solver_t look_at_solver_t;
typedef struct fabrik_solver_t fabrik_solver_t;
typedef struct ccd_solver_t ccd_solver_t;
typedef struct two_bone_ik_t two_bone_ik_t;
typedef struct aim_ik_solver_t aim_ik_solver_t;

// Initialize the character IK manager
bool character_ik_init(void);

// Shutdown the character IK manager
void character_ik_shutdown(void);

// Create an IK target
// entity_id: Entity the target belongs to
// type: Type of IK solver to use
// priority: Priority of this target
// Returns: Target ID or 0 on failure
uint32_t character_ik_create_target(uint32_t entity_id, ik_solver_type_t type, ik_priority_t priority);

// Set target world position
// target_id: Target to modify
// position: World position
bool character_ik_set_target_position(uint32_t target_id, vec3_t position);

// Set target world rotation
// target_id: Target to modify
// rotation: World rotation
bool character_ik_set_target_rotation(uint32_t target_id, quat_t rotation);

// Set target influence weight
// target_id: Target to modify
// weight: Weight (0.0 to 1.0)
bool character_ik_set_target_weight(uint32_t target_id, float weight);

// Lock/unlock target position
// target_id: Target to modify
// lock: Whether to lock position
bool character_ik_lock_target_position(uint32_t target_id, bool lock);

// Lock/unlock target rotation
// target_id: Target to modify
// lock: Whether to lock rotation
bool character_ik_lock_target_rotation(uint32_t target_id, bool lock);

// Create an IK chain
// entity_id: Entity the chain belongs to
// bone_ids: Array of bone IDs in the chain
// bone_count: Number of bones in the chain
// solver_type: Type of solver to use
// Returns: Chain ID or 0 on failure
uint32_t character_ik_create_chain(uint32_t entity_id, const int* bone_ids, uint32_t bone_count, ik_solver_type_t solver_type);

// Set solver for a chain
// chain_id: Chain to modify
// solver_id: Solver to use
bool character_ik_set_chain_solver(uint32_t chain_id, uint32_t solver_id);

// Set blend weight for a chain
// chain_id: Chain to modify
// weight: Blend weight (0.0 to 1.0)
bool character_ik_set_chain_blend_weight(uint32_t chain_id, float weight);

// Create an IK solver
// type: Type of solver to create
// Returns: Solver ID or 0 on failure
uint32_t character_ik_create_solver(ik_solver_type_t type);

// Update all IK systems
// delta_time: Time since last update
void character_ik_update(float delta_time);

// Enable/disable a target
// target_id: Target to modify
// enable: Whether to enable
bool character_ik_enable_target(uint32_t target_id, bool enable);

// Enable/disable a chain
// chain_id: Chain to modify
// enable: Whether to enable
bool character_ik_enable_chain(uint32_t chain_id, bool enable);

// Enable/disable a solver
// solver_id: Solver to modify
// enable: Whether to enable
bool character_ik_enable_solver(uint32_t solver_id, bool enable);

// Set IK update frequency
// frequency: Updates per second
void character_ik_set_update_frequency(float frequency);

// Get IK system statistics
// active_targets: Output number of active targets
// active_chains: Output number of active chains
// active_solvers: Output number of active solvers
// solve_time_ms: Output average solve time
void character_ik_get_statistics(uint32_t* active_targets, uint32_t* active_chains, 
                                uint32_t* active_solvers, float* solve_time_ms);

// Utility function to get current time (placeholder)
#include "core/utils.h"

#endif // CHARACTER_IK_MANAGER_H
