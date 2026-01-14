/**
 * @file physics_networking.c
 * @brief Physics networking system implementation
 */

#include "physics_networking.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ========================================
// Creation/Destruction
// ========================================

PhysicsNetworkSystem* physics_network_create(NetworkRole role,
                                             int max_connections, int max_objects) {
    PhysicsNetworkSystem *system = (PhysicsNetworkSystem*)calloc(1, sizeof(PhysicsNetworkSystem));
    if (!system) return NULL;

    system->role = role;
    system->enabled = true;
    system->authoritative_server = (role == NETWORK_ROLE_SERVER);
    system->client_prediction = (role == NETWORK_ROLE_CLIENT);
    system->lag_compensation = true;
    system->delta_compression = true;

    // Connections
    system->connection_capacity = max_connections > 0 ? max_connections : 32;
    system->connections = (NetworkConnection*)calloc(system->connection_capacity, sizeof(NetworkConnection));
    if (!system->connections) {
        free(system);
        return NULL;
    }
    system->next_connection_id = 1;

    // Object states
    system->object_capacity = max_objects > 0 ? max_objects : 1000;
    system->object_states = (ObjectNetworkState*)calloc(system->object_capacity, sizeof(ObjectNetworkState));
    if (!system->object_states) {
        free(system->connections);
        free(system);
        return NULL;
    }

    // Message queues
    system->outgoing_capacity = 1024;
    system->outgoing_queue = (NetworkMessage*)calloc(system->outgoing_capacity, sizeof(NetworkMessage));
    system->incoming_capacity = 1024;
    system->incoming_queue = (NetworkMessage*)calloc(system->incoming_capacity, sizeof(NetworkMessage));

    if (!system->outgoing_queue || !system->incoming_queue) {
        if (system->outgoing_queue) free(system->outgoing_queue);
        if (system->incoming_queue) free(system->incoming_queue);
        free(system->object_states);
        free(system->connections);
        free(system);
        return NULL;
    }

    // Client prediction
    if (system->client_prediction) {
        system->prediction_capacity = 60 * 5; // 5 seconds buffer at 60Hz
        system->prediction_states = (ClientPredictionState*)calloc(system->prediction_capacity, sizeof(ClientPredictionState));
    }

    // Defaults
    system->network_tick_rate = 20.0f; // 20Hz
    system->interpolation_delay = 0.1f; // 100ms
    system->position_tolerance = 0.01f;
    system->rotation_tolerance = 0.01f;
    system->velocity_tolerance = 0.01f;

    return system;
}

void physics_network_destroy(PhysicsNetworkSystem *system) {
    if (!system) return;

    free(system->connections);
    free(system->object_states);
    free(system->outgoing_queue);
    free(system->incoming_queue);
    if (system->prediction_states) {
        free(system->prediction_states);
    }
    free(system);
}

// ========================================
// System Control
// ========================================

void physics_network_set_enabled(PhysicsNetworkSystem *system, bool enabled) {
    if (system) system->enabled = enabled;
}

void physics_network_set_role(PhysicsNetworkSystem *system, NetworkRole role) {
    if (system) system->role = role;
}

void physics_network_update(PhysicsNetworkSystem *system, float delta_time) {
    if (!system || !system->enabled) return;

    system->update_accumulator += delta_time;
    float tick_interval = 1.0f / system->network_tick_rate;

    // Network tick
    while (system->update_accumulator >= tick_interval) {
        // Here we would process queues, sync states, etc.
        // For now, this is a placeholder for the update logic.

        system->update_accumulator -= tick_interval;
        system->last_update_time++; // Increment logical time
    }
}

// ========================================
// Connection Management
// ========================================

uint32_t physics_network_connect(PhysicsNetworkSystem *system, const char *address, uint16_t port) {
    if (!system || system->connection_count >= system->connection_capacity) return 0;

    int index = system->connection_count;
    NetworkConnection *conn = &system->connections[index];

    conn->connection_id = system->next_connection_id++;
    strncpy(conn->address, address, sizeof(conn->address) - 1);
    conn->port = port;
    conn->connected = true; // Assuming immediate connection for this stub

    system->connection_count++;
    return conn->connection_id;
}

bool physics_network_disconnect(PhysicsNetworkSystem *system, uint32_t connection_id) {
    if (!system) return false;

    for (int i = 0; i < system->connection_count; i++) {
        if (system->connections[i].connection_id == connection_id) {
            // Remove connection (swap with last)
            if (i < system->connection_count - 1) {
                system->connections[i] = system->connections[system->connection_count - 1];
            }
            system->connection_count--;
            return true;
        }
    }
    return false;
}

bool physics_network_accept_connection(PhysicsNetworkSystem *system, uint32_t connection_id) {
    // In a real implementation, this would handle incoming connection requests
    return true;
}

NetworkConnection* physics_network_get_connection(PhysicsNetworkSystem *system, uint32_t connection_id) {
    if (!system) return NULL;

    for (int i = 0; i < system->connection_count; i++) {
        if (system->connections[i].connection_id == connection_id) {
            return &system->connections[i];
        }
    }
    return NULL;
}

// ========================================
// Object Synchronization
// ========================================

bool physics_network_register_object(PhysicsNetworkSystem *system, uint64_t object_id,
                                      uint32_t network_id, bool network_owned,
                                      bool client_predicted, bool server_authoritative) {
    if (!system || system->object_count >= system->object_capacity) return false;

    ObjectNetworkState *state = &system->object_states[system->object_count];
    memset(state, 0, sizeof(ObjectNetworkState));

    state->object_id = object_id;
    state->network_id = network_id;
    state->network_owned = network_owned;
    state->client_predicted = client_predicted;
    state->server_authoritative = server_authoritative;
    state->active = true;

    system->object_count++;
    return true;
}

bool physics_network_unregister_object(PhysicsNetworkSystem *system, uint64_t object_id) {
    if (!system) return false;

    for (int i = 0; i < system->object_count; i++) {
        if (system->object_states[i].object_id == object_id) {
            if (i < system->object_count - 1) {
                system->object_states[i] = system->object_states[system->object_count - 1];
            }
            system->object_count--;
            return true;
        }
    }
    return false;
}

bool physics_network_update_object_state(PhysicsNetworkSystem *system, uint64_t object_id,
                                         const float *position, const float *rotation,
                                         const float *velocity, bool active, bool sleeping) {
    if (!system) return false;

    for (int i = 0; i < system->object_count; i++) {
        if (system->object_states[i].object_id == object_id) {
            ObjectNetworkState *state = &system->object_states[i];

            if (position) memcpy(state->position, position, sizeof(float) * 3);
            if (rotation) memcpy(state->rotation, rotation, sizeof(float) * 4);
            if (velocity) memcpy(state->velocity, velocity, sizeof(float) * 3);
            state->active = active;
            state->sleeping = sleeping;
            state->last_update_time = system->last_update_time;

            return true;
        }
    }
    return false;
}

ObjectNetworkState* physics_network_get_object_state(PhysicsNetworkSystem *system, uint64_t object_id) {
    if (!system) return NULL;

    for (int i = 0; i < system->object_count; i++) {
        if (system->object_states[i].object_id == object_id) {
            return &system->object_states[i];
        }
    }
    return NULL;
}

// ========================================
// Client Prediction
// ========================================

void physics_network_set_client_prediction(PhysicsNetworkSystem *system, bool enabled) {
    if (!system) return;

    system->client_prediction = enabled;
    if (enabled && !system->prediction_states) {
        system->prediction_capacity = 60 * 5;
        system->prediction_states = (ClientPredictionState*)calloc(system->prediction_capacity, sizeof(ClientPredictionState));
    }
}

uint64_t physics_network_add_client_input(PhysicsNetworkSystem *system, uint32_t client_id,
                                          const float *input_values, int input_count) {
    if (!system || !system->client_prediction || !system->prediction_states) return 0;

    if (system->prediction_count >= system->prediction_capacity) {
        // Simple ring buffer or shift - for now just reset count or overwrite?
        // Let's shift
        memmove(&system->prediction_states[0], &system->prediction_states[1],
               (system->prediction_capacity - 1) * sizeof(ClientPredictionState));
        system->prediction_count--;
    }

    int index = system->prediction_count;
    ClientPredictionState *state = &system->prediction_states[index];

    state->input_sequence = system->prediction_count + 1; // Simple sequence
    if (input_values && input_count > 0) {
        int count = input_count < 16 ? input_count : 16;
        memcpy(state->input_values, input_values, count * sizeof(float));
    }
    state->processed = false;

    system->prediction_count++;
    return state->input_sequence;
}

bool physics_network_apply_server_correction(PhysicsNetworkSystem *system, uint32_t client_id,
                                             const float *server_position, const float *server_rotation,
                                             const float *server_velocity, uint64_t sequence_number) {
    if (!system || !system->client_prediction) return false;

    // In a real implementation, we would find the prediction state matching the sequence number
    // and check for errors, then trigger a replay/correction if the error is too large.

    return true;
}

// ========================================
// Message Handling
// ========================================

uint32_t physics_network_send_message(PhysicsNetworkSystem *system, NetworkMessageType message_type,
                                       const void *data, size_t data_size,
                                       uint32_t target_id, uint32_t flags) {
    if (!system || system->outgoing_size >= system->outgoing_capacity) return 0;

    NetworkMessage *msg = &system->outgoing_queue[system->outgoing_size];
    msg->message_id = system->outgoing_size + 1;
    msg->type = message_type;
    msg->flags = flags;
    msg->target_id = target_id;

    // Copy data
    if (data && data_size > 0) {
        msg->data = malloc(data_size);
        if (msg->data) {
            memcpy(msg->data, data, data_size);
            msg->data_size = data_size;
        }
    }

    system->outgoing_size++;
    system->messages_sent++;
    system->bytes_sent += data_size;

    return msg->message_id;
}

bool physics_network_receive_message(PhysicsNetworkSystem *system, NetworkMessage *message) {
    if (!system || system->incoming_size == 0 || !message) return false;

    // Pop from incoming queue
    NetworkMessage *src = &system->incoming_queue[0];
    memcpy(message, src, sizeof(NetworkMessage));

    // Shift queue
    memmove(&system->incoming_queue[0], &system->incoming_queue[1],
           (system->incoming_size - 1) * sizeof(NetworkMessage));
    system->incoming_size--;

    return true;
}

uint32_t physics_network_broadcast_message(PhysicsNetworkSystem *system, NetworkMessageType message_type,
                                           const void *data, size_t data_size, uint32_t flags) {
    return physics_network_send_message(system, message_type, data, data_size, 0, flags);
}

// ========================================
// Statistics
// ========================================

void physics_network_get_statistics(PhysicsNetworkSystem *system,
                                     uint64_t *messages_sent, uint64_t *messages_received,
                                     uint64_t *bytes_sent, uint64_t *bytes_received,
                                     float *average_latency, float *packet_loss_rate) {
    if (!system) return;

    if (messages_sent) *messages_sent = system->messages_sent;
    if (messages_received) *messages_received = system->messages_received;
    if (bytes_sent) *bytes_sent = system->bytes_sent;
    if (bytes_received) *bytes_received = system->bytes_received;
    if (average_latency) *average_latency = system->average_latency;
    if (packet_loss_rate) *packet_loss_rate = system->packet_loss_rate;
}

// ========================================
// Configuration
// ========================================

void physics_network_set_tick_rate(PhysicsNetworkSystem *system, float tick_rate) {
    if (system && tick_rate > 0.0f) system->network_tick_rate = tick_rate;
}

void physics_network_set_position_tolerance(PhysicsNetworkSystem *system, float tolerance) {
    if (system) system->position_tolerance = tolerance;
}

void physics_network_set_interpolation_delay(PhysicsNetworkSystem *system, float delay) {
    if (system) system->interpolation_delay = delay;
}
