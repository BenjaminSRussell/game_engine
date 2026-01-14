/*
 * state_sync.h
 *
 * Entity state synchronization for multiplayer gameplay
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * Provides functionality for:
 * - Entity state snapshot creation and transmission
 * - Delta compression for bandwidth optimization
 * - Interpolation of remote entity states
 * - Prediction for smooth movement
 */

#ifndef STATE_SYNC_H
#define STATE_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

typedef uint32_t EntityID;
typedef uint64_t StateTimestamp;

/* State change types */
typedef enum {
    STATE_CHANGE_POSITION = 1,
    STATE_CHANGE_ROTATION = 2,
    STATE_CHANGE_SCALE = 4,
    STATE_CHANGE_VELOCITY = 8,
    STATE_CHANGE_ANIMATION = 16,
    STATE_CHANGE_PROPERTY = 32,
} StateChangeType;

/* Vector3 for positions */
typedef struct {
    float x, y, z;
} Vec3;

/* Quaternion for rotations */
typedef struct {
    float x, y, z, w;
} Quat;

/* Entity state snapshot */
typedef struct {
    EntityID entity_id;
    StateTimestamp timestamp;
    uint32_t change_flags;

    /* Transform data */
    Vec3 position;
    Quat rotation;
    Vec3 scale;

    /* Physics data */
    Vec3 velocity;
    Vec3 angular_velocity;

    /* Animation data */
    uint32_t animation_id;
    float animation_time;

    /* Custom properties */
    uint32_t property_count;
    uint8_t* property_data;
    size_t property_data_size;
} EntityState;

/* State snapshot batch */
typedef struct {
    EntityState* states;
    uint32_t state_count;
    StateTimestamp snapshot_time;
} StateSnapshot;

/* Interpolated state for rendering */
typedef struct {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    Vec3 velocity;
    float interpolation_factor;
} InterpolatedState;

/* State synchronizer context */
typedef struct StateSynchronizer StateSynchronizer;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/* Create and destroy synchronizer */
StateSynchronizer* state_sync_create(uint32_t max_entities);
void state_sync_destroy(StateSynchronizer* sync);

/* Register entity for synchronization */
int state_sync_register_entity(
    StateSynchronizer* sync,
    EntityID entity_id
);

int state_sync_unregister_entity(
    StateSynchronizer* sync,
    EntityID entity_id
);

/* Update entity state */
int state_sync_update_position(
    StateSynchronizer* sync,
    EntityID entity_id,
    const Vec3* position
);

int state_sync_update_rotation(
    StateSynchronizer* sync,
    EntityID entity_id,
    const Quat* rotation
);

int state_sync_update_velocity(
    StateSynchronizer* sync,
    EntityID entity_id,
    const Vec3* velocity
);

int state_sync_update_animation(
    StateSynchronizer* sync,
    EntityID entity_id,
    uint32_t animation_id,
    float animation_time
);

int state_sync_update_property(
    StateSynchronizer* sync,
    EntityID entity_id,
    uint32_t property_id,
    const void* data,
    size_t data_size
);

/* Create snapshots for transmission */
int state_sync_create_snapshot(
    StateSynchronizer* sync,
    StateSnapshot** snapshot,
    StateChangeType change_mask
);

void state_sync_free_snapshot(StateSnapshot* snapshot);

/* Apply remote state updates */
int state_sync_apply_state(
    StateSynchronizer* sync,
    const EntityState* state
);

int state_sync_apply_snapshot(
    StateSynchronizer* sync,
    const StateSnapshot* snapshot
);

/* Interpolation support */
int state_sync_get_interpolated_state(
    StateSynchronizer* sync,
    EntityID entity_id,
    float interpolation_time,
    InterpolatedState* state
);

/* Delta compression */
typedef struct {
    uint8_t* data;
    size_t size;
    uint32_t entity_count;
} DeltaCompressed;

int state_sync_delta_compress_snapshot(
    StateSynchronizer* sync,
    const StateSnapshot* snapshot,
    DeltaCompressed** compressed
);

void state_sync_free_delta_compressed(DeltaCompressed* compressed);

int state_sync_delta_decompress(
    StateSynchronizer* sync,
    const DeltaCompressed* compressed,
    StateSnapshot** snapshot
);

/* Prediction */
int state_sync_predict_state(
    StateSynchronizer* sync,
    EntityID entity_id,
    float prediction_time,
    Vec3* predicted_position
);

/* Statistics */
typedef struct {
    uint32_t synchronized_entities;
    uint32_t snapshots_created;
    uint32_t snapshots_applied;
    uint64_t bytes_transmitted;
    uint64_t bytes_saved_by_delta;
    float avg_delta_compression_ratio;
} SyncStatistics;

int state_sync_get_statistics(
    StateSynchronizer* sync,
    SyncStatistics* stats
);

/* Configuration */
int state_sync_set_update_rate(StateSynchronizer* sync, uint32_t updates_per_second);
int state_sync_set_interpolation_enabled(StateSynchronizer* sync, bool enabled);
int state_sync_set_prediction_enabled(StateSynchronizer* sync, bool enabled);

#ifdef __cplusplus
}
#endif

#endif // STATE_SYNC_H
