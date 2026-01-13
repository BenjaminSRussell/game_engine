/**
 * @file physics_replay.h
 * @brief Physics replay system for deterministic verification
 */

#ifndef PHYSICS_REPLAY_H
#define PHYSICS_REPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../world/physics_world_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Types
// ========================================

typedef enum ReplayEventType {
    REPLAY_EVENT_APPLY_FORCE,
    REPLAY_EVENT_APPLY_IMPULSE,
    REPLAY_EVENT_APPLY_TORQUE
} ReplayEventType;

typedef struct ReplayEvent {
    uint64_t frame;
    ReplayEventType type;
    uint64_t object_id;
    float vector_data[3]; // Force, impulse, or torque vector
    float point_data[3];  // Application point (if applicable)
} ReplayEvent;

typedef struct PhysicsReplaySystem PhysicsReplaySystem;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create physics replay system
 * @param world Physics world to attach to
 * @return Replay system or NULL on failure
 */
PhysicsReplaySystem* physics_replay_create(PhysicsWorld *world);

/**
 * Destroy physics replay system
 * @param system Replay system
 */
void physics_replay_destroy(PhysicsReplaySystem *system);

// ========================================
// Recording Control
// ========================================

/**
 * Start recording
 * @param system Replay system
 * @return True if successful
 */
bool physics_replay_start_recording(PhysicsReplaySystem *system);

/**
 * Record apply force event
 * Wrapper for physics_world_apply_force that records the event
 */
void physics_replay_apply_force(PhysicsReplaySystem *system, uint64_t object_id,
                                const float *force, const float *point);

/**
 * Record apply impulse event
 * Wrapper for physics_world_apply_impulse that records the event
 */
void physics_replay_apply_impulse(PhysicsReplaySystem *system, uint64_t object_id,
                                  const float *impulse, const float *point);

/**
 * Record apply torque event
 * Wrapper for physics_world_apply_torque that records the event
 */
void physics_replay_apply_torque(PhysicsReplaySystem *system, uint64_t object_id, const float *torque);

/**
 * Stop recording
 * @param system Replay system
 */
void physics_replay_stop_recording(PhysicsReplaySystem *system);

// ========================================
// Replay Control
// ========================================

/**
 * Start replay
 * Resets the physics world to the recorded initial state
 * @param system Replay system
 * @return True if successful
 */
bool physics_replay_start_replay(PhysicsReplaySystem *system);

/**
 * Update replay
 * Should be called every frame instead of manually stepping the physics world
 * @param system Replay system
 */
void physics_replay_update(PhysicsReplaySystem *system);

/**
 * Stop replay
 * @param system Replay system
 */
void physics_replay_stop_replay(PhysicsReplaySystem *system);

// ========================================
// Status
// ========================================

bool physics_replay_is_recording(PhysicsReplaySystem *system);
bool physics_replay_is_replaying(PhysicsReplaySystem *system);
uint64_t physics_replay_get_frame(PhysicsReplaySystem *system);
int physics_replay_get_event_count(PhysicsReplaySystem *system);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_REPLAY_H */
