/*
 * state_sync.c
 *
 * Entity state synchronization implementation
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 */

#include "state_sync.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define DEFAULT_UPDATE_RATE 20  /* 20 updates per second */
#define MAX_PROPERTIES_PER_ENTITY 32

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct {
    EntityID entity_id;
    bool registered;

    /* Current state */
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    Vec3 velocity;
    Vec3 angular_velocity;

    /* Animation */
    uint32_t animation_id;
    float animation_time;

    /* Last synchronized state */
    EntityState last_state;
    StateTimestamp last_update_time;

    /* Properties */
    struct {
        uint8_t data[256];
        size_t size;
    } properties[MAX_PROPERTIES_PER_ENTITY];
} SynchronizedEntity;

typedef struct StateSynchronizer {
    SynchronizedEntity* entities;
    uint32_t max_entities;
    uint32_t entity_count;

    /* Configuration */
    uint32_t update_rate;
    bool interpolation_enabled;
    bool prediction_enabled;

    /* Statistics */
    SyncStatistics stats;

    pthread_mutex_t lock;
} StateSynchronizer;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static StateTimestamp get_current_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (StateTimestamp)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static SynchronizedEntity* find_entity(
    StateSynchronizer* sync,
    EntityID entity_id) {

    if (!sync) {
        return NULL;
    }

    for (uint32_t i = 0; i < sync->entity_count; i++) {
        if (sync->entities[i].entity_id == entity_id) {
            return &sync->entities[i];
        }
    }

    return NULL;
}

static Vec3 vec3_lerp(const Vec3* a, const Vec3* b, float t) {
    Vec3 result = {
        a->x + (b->x - a->x) * t,
        a->y + (b->y - a->y) * t,
        a->z + (b->z - a->z) * t
    };
    return result;
}

static Quat quat_slerp(const Quat* a, const Quat* b, float t) {
    /* Simplified slerp - linear interpolation for now */
    Quat result = {
        a->x + (b->x - a->x) * t,
        a->y + (b->y - a->y) * t,
        a->z + (b->z - a->z) * t,
        a->w + (b->w - a->w) * t
    };

    /* Normalize */
    float mag = sqrtf(result.x * result.x + result.y * result.y +
                     result.z * result.z + result.w * result.w);
    if (mag > 0) {
        result.x /= mag;
        result.y /= mag;
        result.z /= mag;
        result.w /= mag;
    }

    return result;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

StateSynchronizer* state_sync_create(uint32_t max_entities) {
    if (max_entities == 0) {
        max_entities = 1024;
    }

    StateSynchronizer* sync = (StateSynchronizer*)malloc(sizeof(StateSynchronizer));
    if (!sync) {
        return NULL;
    }

    sync->entities = (SynchronizedEntity*)calloc(max_entities, sizeof(SynchronizedEntity));
    if (!sync->entities) {
        free(sync);
        return NULL;
    }

    sync->max_entities = max_entities;
    sync->entity_count = 0;
    sync->update_rate = DEFAULT_UPDATE_RATE;
    sync->interpolation_enabled = true;
    sync->prediction_enabled = false;
    memset(&sync->stats, 0, sizeof(sync->stats));
    pthread_mutex_init(&sync->lock, NULL);

    return sync;
}

void state_sync_destroy(StateSynchronizer* sync) {
    if (!sync) {
        return;
    }

    pthread_mutex_destroy(&sync->lock);
    free(sync->entities);
    free(sync);
}

int state_sync_register_entity(
    StateSynchronizer* sync,
    EntityID entity_id) {

    if (!sync || entity_id == 0) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    /* Check if already registered */
    if (find_entity(sync, entity_id)) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    if (sync->entity_count >= sync->max_entities) {
        pthread_mutex_unlock(&sync->lock);
        return -3;  /* No space */
    }

    SynchronizedEntity* entity = &sync->entities[sync->entity_count];
    entity->entity_id = entity_id;
    entity->registered = true;
    entity->scale.x = entity->scale.y = entity->scale.z = 1.0f;
    entity->rotation.w = 1.0f;  /* Identity quaternion */
    entity->last_update_time = get_current_timestamp();

    sync->entity_count++;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_unregister_entity(
    StateSynchronizer* sync,
    EntityID entity_id) {

    if (!sync) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    entity->registered = false;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_update_position(
    StateSynchronizer* sync,
    EntityID entity_id,
    const Vec3* position) {

    if (!sync || !position) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    entity->position = *position;
    entity->last_update_time = get_current_timestamp();

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_update_rotation(
    StateSynchronizer* sync,
    EntityID entity_id,
    const Quat* rotation) {

    if (!sync || !rotation) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    entity->rotation = *rotation;
    entity->last_update_time = get_current_timestamp();

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_update_velocity(
    StateSynchronizer* sync,
    EntityID entity_id,
    const Vec3* velocity) {

    if (!sync || !velocity) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    entity->velocity = *velocity;
    entity->last_update_time = get_current_timestamp();

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_update_animation(
    StateSynchronizer* sync,
    EntityID entity_id,
    uint32_t animation_id,
    float animation_time) {

    if (!sync) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    entity->animation_id = animation_id;
    entity->animation_time = animation_time;
    entity->last_update_time = get_current_timestamp();

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_update_property(
    StateSynchronizer* sync,
    EntityID entity_id,
    uint32_t property_id,
    const void* data,
    size_t data_size) {

    if (!sync || property_id >= MAX_PROPERTIES_PER_ENTITY || !data) {
        return -1;
    }

    if (data_size > sizeof(sync->entities[0].properties[0].data)) {
        return -2;  /* Data too large */
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -3;
    }

    memcpy(entity->properties[property_id].data, data, data_size);
    entity->properties[property_id].size = data_size;
    entity->last_update_time = get_current_timestamp();

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_create_snapshot(
    StateSynchronizer* sync,
    StateSnapshot** snapshot,
    StateChangeType change_mask) {

    if (!sync || !snapshot) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    *snapshot = (StateSnapshot*)malloc(sizeof(StateSnapshot));
    if (!*snapshot) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    (*snapshot)->state_count = sync->entity_count;
    (*snapshot)->snapshot_time = get_current_timestamp();
    (*snapshot)->states = (EntityState*)calloc(sync->entity_count, sizeof(EntityState));

    if (!(*snapshot)->states) {
        free(*snapshot);
        *snapshot = NULL;
        pthread_mutex_unlock(&sync->lock);
        return -3;
    }

    /* Copy entity states */
    for (uint32_t i = 0; i < sync->entity_count; i++) {
        SynchronizedEntity* entity = &sync->entities[i];
        EntityState* state = &(*snapshot)->states[i];

        state->entity_id = entity->entity_id;
        state->timestamp = entity->last_update_time;
        state->change_flags = change_mask;
        state->position = entity->position;
        state->rotation = entity->rotation;
        state->scale = entity->scale;
        state->velocity = entity->velocity;
        state->animation_id = entity->animation_id;
        state->animation_time = entity->animation_time;
    }

    sync->stats.snapshots_created++;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

void state_sync_free_snapshot(StateSnapshot* snapshot) {
    if (!snapshot) {
        return;
    }

    if (snapshot->states) {
        free(snapshot->states);
    }

    free(snapshot);
}

int state_sync_apply_state(
    StateSynchronizer* sync,
    const EntityState* state) {

    if (!sync || !state) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, state->entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    if (state->change_flags & STATE_CHANGE_POSITION) {
        entity->position = state->position;
    }
    if (state->change_flags & STATE_CHANGE_ROTATION) {
        entity->rotation = state->rotation;
    }
    if (state->change_flags & STATE_CHANGE_VELOCITY) {
        entity->velocity = state->velocity;
    }
    if (state->change_flags & STATE_CHANGE_ANIMATION) {
        entity->animation_id = state->animation_id;
        entity->animation_time = state->animation_time;
    }

    entity->last_state = *state;
    sync->stats.snapshots_applied++;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_apply_snapshot(
    StateSynchronizer* sync,
    const StateSnapshot* snapshot) {

    if (!sync || !snapshot) {
        return -1;
    }

    for (uint32_t i = 0; i < snapshot->state_count; i++) {
        state_sync_apply_state(sync, &snapshot->states[i]);
    }

    return 0;
}

int state_sync_get_interpolated_state(
    StateSynchronizer* sync,
    EntityID entity_id,
    float interpolation_time,
    InterpolatedState* state) {

    if (!sync || !state || interpolation_time < 0.0f || interpolation_time > 1.0f) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    /* Interpolate between last state and current state */
    if (sync->interpolation_enabled && entity->last_state.entity_id != 0) {
        state->position = vec3_lerp(&entity->last_state.position, &entity->position, interpolation_time);
        state->rotation = quat_slerp(&entity->last_state.rotation, &entity->rotation, interpolation_time);
        state->scale = vec3_lerp(&entity->last_state.scale, &entity->scale, interpolation_time);
    } else {
        state->position = entity->position;
        state->rotation = entity->rotation;
        state->scale = entity->scale;
    }

    state->velocity = entity->velocity;
    state->interpolation_factor = interpolation_time;

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_delta_compress_snapshot(
    StateSynchronizer* sync,
    const StateSnapshot* snapshot,
    DeltaCompressed** compressed) {

    if (!sync || !snapshot || !compressed) {
        return -1;
    }

    /* Simplified delta compression - just store the snapshot data */
    *compressed = (DeltaCompressed*)malloc(sizeof(DeltaCompressed));
    if (!*compressed) {
        return -2;
    }

    size_t data_size = snapshot->state_count * sizeof(EntityState);
    (*compressed)->data = (uint8_t*)malloc(data_size);
    if (!(*compressed)->data) {
        free(*compressed);
        *compressed = NULL;
        return -3;
    }

    memcpy((*compressed)->data, snapshot->states, data_size);
    (*compressed)->size = data_size;
    (*compressed)->entity_count = snapshot->state_count;

    pthread_mutex_lock(&sync->lock);
    sync->stats.bytes_transmitted += data_size;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

void state_sync_free_delta_compressed(DeltaCompressed* compressed) {
    if (!compressed) {
        return;
    }

    if (compressed->data) {
        free(compressed->data);
    }

    free(compressed);
}

int state_sync_delta_decompress(
    StateSynchronizer* sync,
    const DeltaCompressed* compressed,
    StateSnapshot** snapshot) {

    if (!sync || !compressed || !snapshot) {
        return -1;
    }

    *snapshot = (StateSnapshot*)malloc(sizeof(StateSnapshot));
    if (!*snapshot) {
        return -2;
    }

    (*snapshot)->state_count = compressed->entity_count;
    (*snapshot)->snapshot_time = get_current_timestamp();
    (*snapshot)->states = (EntityState*)malloc(compressed->size);

    if (!(*snapshot)->states) {
        free(*snapshot);
        *snapshot = NULL;
        return -3;
    }

    memcpy((*snapshot)->states, compressed->data, compressed->size);

    return 0;
}

int state_sync_predict_state(
    StateSynchronizer* sync,
    EntityID entity_id,
    float prediction_time,
    Vec3* predicted_position) {

    if (!sync || !predicted_position || prediction_time < 0.0f) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    SynchronizedEntity* entity = find_entity(sync, entity_id);
    if (!entity) {
        pthread_mutex_unlock(&sync->lock);
        return -2;
    }

    if (sync->prediction_enabled) {
        predicted_position->x = entity->position.x + entity->velocity.x * prediction_time;
        predicted_position->y = entity->position.y + entity->velocity.y * prediction_time;
        predicted_position->z = entity->position.z + entity->velocity.z * prediction_time;
    } else {
        *predicted_position = entity->position;
    }

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_get_statistics(
    StateSynchronizer* sync,
    SyncStatistics* stats) {

    if (!sync || !stats) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);

    memcpy(stats, &sync->stats, sizeof(SyncStatistics));
    stats->synchronized_entities = sync->entity_count;

    /* Calculate compression ratio */
    if (sync->stats.bytes_transmitted > 0) {
        stats->avg_delta_compression_ratio =
            (float)sync->stats.bytes_saved_by_delta / sync->stats.bytes_transmitted;
    }

    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_set_update_rate(StateSynchronizer* sync, uint32_t updates_per_second) {
    if (!sync || updates_per_second == 0) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);
    sync->update_rate = updates_per_second;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_set_interpolation_enabled(StateSynchronizer* sync, bool enabled) {
    if (!sync) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);
    sync->interpolation_enabled = enabled;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

int state_sync_set_prediction_enabled(StateSynchronizer* sync, bool enabled) {
    if (!sync) {
        return -1;
    }

    pthread_mutex_lock(&sync->lock);
    sync->prediction_enabled = enabled;
    pthread_mutex_unlock(&sync->lock);

    return 0;
}

/* End of state_sync.c */
