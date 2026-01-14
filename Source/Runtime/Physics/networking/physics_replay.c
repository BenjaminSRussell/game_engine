/**
 * @file physics_replay.c
 * @brief Physics replay system implementation
 */

#include "physics_replay.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct PhysicsReplaySystem {
    PhysicsWorld *world;
    bool is_recording;
    bool is_replaying;

    // Recorded data
    void *initial_state_buffer;
    size_t initial_state_size;

    ReplayEvent *events;
    int event_count;
    int event_capacity;

    uint64_t current_frame;
    int current_event_index;
} PhysicsReplaySystem;

// ========================================
// Implementation
// ========================================

PhysicsReplaySystem* physics_replay_create(PhysicsWorld *world) {
    if (!world) return NULL;

    PhysicsReplaySystem *system = (PhysicsReplaySystem*)calloc(1, sizeof(PhysicsReplaySystem));
    if (!system) return NULL;

    system->world = world;
    system->event_capacity = 1024;
    system->events = (ReplayEvent*)calloc(system->event_capacity, sizeof(ReplayEvent));

    return system;
}

void physics_replay_destroy(PhysicsReplaySystem *system) {
    if (!system) return;

    if (system->initial_state_buffer) free(system->initial_state_buffer);
    if (system->events) free(system->events);
    free(system);
}

bool physics_replay_start_recording(PhysicsReplaySystem *system) {
    if (!system || !system->world) return false;

    // Capture initial state using engine serialization
    system->initial_state_size = physics_world_serialize(system->world, NULL, 0);
    if (system->initial_state_size == 0) return false;

    if (system->initial_state_buffer) free(system->initial_state_buffer);
    system->initial_state_buffer = malloc(system->initial_state_size);
    if (!system->initial_state_buffer) return false;

    if (physics_world_serialize(system->world, system->initial_state_buffer, system->initial_state_size) == 0) {
        return false;
    }

    // Reset events
    system->event_count = 0;
    system->current_frame = 0;

    system->is_recording = true;
    system->is_replaying = false;

    return true;
}

static void add_event(PhysicsReplaySystem *system, ReplayEventType type, uint64_t object_id,
                      const float *vector, const float *point) {
    if (!system || !system->is_recording) return;

    if (system->event_count >= system->event_capacity) {
        system->event_capacity *= 2;
        system->events = (ReplayEvent*)realloc(system->events, system->event_capacity * sizeof(ReplayEvent));
    }

    ReplayEvent *event = &system->events[system->event_count++];
    event->frame = system->current_frame;
    event->type = type;
    event->object_id = object_id;

    if (vector) memcpy(event->vector_data, vector, sizeof(float) * 3);
    else memset(event->vector_data, 0, sizeof(float) * 3);

    if (point) memcpy(event->point_data, point, sizeof(float) * 3);
    else memset(event->point_data, 0, sizeof(float) * 3);
}

void physics_replay_apply_force(PhysicsReplaySystem *system, uint64_t object_id,
                                const float *force, const float *point) {
    if (!system) return;

    // Record
    if (system->is_recording) {
        add_event(system, REPLAY_EVENT_APPLY_FORCE, object_id, force, point);
    }

    // Apply
    physics_world_apply_force(system->world, object_id, force, point);
}

void physics_replay_apply_impulse(PhysicsReplaySystem *system, uint64_t object_id,
                                  const float *impulse, const float *point) {
    if (!system) return;

    // Record
    if (system->is_recording) {
        add_event(system, REPLAY_EVENT_APPLY_IMPULSE, object_id, impulse, point);
    }

    // Apply
    physics_world_apply_impulse(system->world, object_id, impulse, point);
}

void physics_replay_apply_torque(PhysicsReplaySystem *system, uint64_t object_id, const float *torque) {
    if (!system) return;

    // Record
    if (system->is_recording) {
        add_event(system, REPLAY_EVENT_APPLY_TORQUE, object_id, torque, NULL);
    }

    // Apply
    physics_world_apply_torque(system->world, object_id, torque);
}

void physics_replay_stop_recording(PhysicsReplaySystem *system) {
    if (!system) return;
    system->is_recording = false;
}

bool physics_replay_start_replay(PhysicsReplaySystem *system) {
    if (!system || !system->world || !system->initial_state_buffer) return false;

    // Restore state using engine serialization
    if (physics_world_deserialize(system->world, system->initial_state_buffer, system->initial_state_size) == 0) {
        return false;
    }

    system->current_frame = 0;
    system->current_event_index = 0;

    system->is_replaying = true;
    system->is_recording = false;

    return true;
}

void physics_replay_update(PhysicsReplaySystem *system) {
    if (!system || !system->world) return;

    if (system->is_replaying) {
        // Process events for current frame
        while (system->current_event_index < system->event_count) {
            ReplayEvent *event = &system->events[system->current_event_index];
            if (event->frame > system->current_frame) break;

            // Apply event
            switch (event->type) {
                case REPLAY_EVENT_APPLY_FORCE:
                    physics_world_apply_force(system->world, event->object_id,
                                              event->vector_data, event->point_data);
                    break;
                case REPLAY_EVENT_APPLY_IMPULSE:
                    physics_world_apply_impulse(system->world, event->object_id,
                                                event->vector_data, event->point_data);
                    break;
                case REPLAY_EVENT_APPLY_TORQUE:
                    physics_world_apply_torque(system->world, event->object_id, event->vector_data);
                    break;
            }

            system->current_event_index++;
        }
    }

    // Step world
    // We use a fixed delta time for replay consistency
    float dt = physics_world_get_time_step(system->world);
    if (dt <= 0.0f) dt = 0.016f; // Default

    physics_world_update(system->world, dt);

    system->current_frame++;
}

void physics_replay_stop_replay(PhysicsReplaySystem *system) {
    if (!system) return;
    system->is_replaying = false;
}

bool physics_replay_is_recording(PhysicsReplaySystem *system) {
    return system ? system->is_recording : false;
}

bool physics_replay_is_replaying(PhysicsReplaySystem *system) {
    return system ? system->is_replaying : false;
}

uint64_t physics_replay_get_frame(PhysicsReplaySystem *system) {
    return system ? system->current_frame : 0;
}

int physics_replay_get_event_count(PhysicsReplaySystem *system) {
    return system ? system->event_count : 0;
}
