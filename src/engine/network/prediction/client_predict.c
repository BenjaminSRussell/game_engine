/**
 * =================================================================================================
 *                      CLIENT-SIDE PREDICTION & RECONCILIATION
 * =================================================================================================
 * 
 * PURPOSE: Enable responsive local gameplay with server authority
 * FEATURES: Input buffering, rollback-and-replay, smoothing
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_PREDICTED_INPUTS 64
#define MAX_ENTITY_SNAPSHOTS 32

// =================================================================================================
// INPUT REPRESENTATION
// =================================================================================================

typedef struct {
    uint32_t sequence;       // Monotonic sequence number
    uint32_t timestamp_ms;   // Client timestamp
    
    // Player input state
    float move_x, move_z;    // Movement direction (-1 to 1)
    float look_yaw, look_pitch;
    uint32_t buttons;        // Bitflags for jump, crouch, etc.
} PlayerInput;

// =================================================================================================
// PREDICTED STATE
// =================================================================================================

typedef struct {
    float position[3];
    float velocity[3];
    float rotation[4];  // Quaternion
    
    uint32_t last_acked_sequence;
    uint32_t

 predicted_sequence;
} PredictedState;

// =================================================================================================
// CLIENT PREDICTION MANAGER
// =================================================================================================

typedef struct {
    // Input history
    PlayerInput input_buffer[MAX_PREDICTED_INPUTS];
    uint32_t input_head;
    uint32_t input_tail;
    uint32_t next_sequence;
    
    // Predicted state
    PredictedState predicted;
    
    // Server-confirmed state (for reconciliation)
    PredictedState server_state;
    uint32_t last_server_ack;
    
    // Smoothing for non-player entities
    float smooth_factor;
} PredictionManager;

// =================================================================================================
// INITIALIZATION
// =================================================================================================

PredictionManager* prediction_create() {
    PredictionManager* pred = (PredictionManager*)calloc(1, sizeof(PredictionManager));
    pred->next_sequence = 1;
    pred->smooth_factor = 0.2f;
    return pred;
}

void prediction_destroy(PredictionManager* pred) {
    free(pred);
}

// =================================================================================================
// INPUT CAPTURE & BUFFERING
// =================================================================================================

void prediction_add_input(PredictionManager* pred, float move_x, float move_z, 
                          float yaw, float pitch, uint32_t buttons) {
    uint32_t idx = pred->input_head % MAX_PREDICTED_INPUTS;
    PlayerInput* input = &pred->input_buffer[idx];
    
    input->sequence = pred->next_sequence++;
    input->timestamp_ms = 0; // Would use actual game time
    input->move_x = move_x;
    input->move_z = move_z;
    input->look_yaw = yaw;
    input->look_pitch = pitch;
    input->buttons = buttons;
    
    pred->input_head++;
}

// =================================================================================================
// PREDICTION SIMULATION
// =================================================================================================

static void simulate_movement(PredictedState* state, const PlayerInput* input, float dt) {
    // Simple movement physics
    float speed = 5.0f;
    
    state->velocity[0] = input->move_x * speed;
    state->velocity[2] = input->move_z * speed;
    
    // Apply gravity
    state->velocity[1] -= 9.81f * dt;
    
    // Integrate position
    state->position[0] += state->velocity[0] * dt;
    state->position[1] += state->velocity[1] * dt;
    state->position[2] += state->velocity[2] * dt;
    
    // Ground collision
    if (state->position[1] <  0.0f) {
        state->position[1] = 0.0f;
        state->velocity[1] = 0.0f;
        
        // Jump if button pressed
        if (input->buttons & 0x01) {
            state->velocity[1] = 5.0f;
        }
    }
}

void prediction_predict(PredictionManager* pred, float dt) {
    // Simulate all unacknowledged inputs
    uint32_t start_seq = pred->predicted.last_acked_sequence + 1;
    
    for (uint32_t i = pred->input_tail; i < pred->input_head; i++) {
        PlayerInput* input = &pred->input_buffer[i % MAX_PREDICTED_INPUTS];
        
        if (input->sequence >= start_seq) {
            simulate_movement(&pred->predicted, input, dt);
        }
    }
    
    pred->predicted.predicted_sequence = pred->next_sequence - 1;
}

// =================================================================================================
// SERVER RECONCILIATION
// =================================================================================================

void prediction_reconcile(PredictionManager* pred, float server_pos[3], float server_vel[3], 
                          uint32_t acked_sequence, float dt) {
    // Server confirmed state up to this sequence
    pred->server_state.position[0] = server_pos[0];
    pred->server_state.position[1] = server_pos[1];
    pred->server_state.position[2] = server_pos[2];
    
    pred->server_state.velocity[0] = server_vel[0];
    pred->server_state.velocity[1] = server_vel[1];
    pred->server_state.velocity[2] = server_vel[2];
    
    pred->last_server_ack = acked_sequence;
    pred->predicted.last_acked_sequence = acked_sequence;
    
    // Check for prediction error
    float dx = pred->predicted.position[0] - server_pos[0];
    float dy = pred->predicted.position[1] - server_pos[1];
    float dz = pred->predicted.position[2] - server_pos[2];
    float error = sqrtf(dx*dx + dy*dy + dz*dz);
    
    if (error > 0.1f) {
        // Significant error: rollback and replay
        pred->predicted = pred->server_state;
        
        // Re-simulate unacknowledged inputs
        for (uint32_t i = pred->input_tail; i < pred->input_head; i++) {
            PlayerInput* input = &pred->input_buffer[i % MAX_PREDICTED_INPUTS];
            
            if (input->sequence > acked_sequence) {
                simulate_movement(&pred->predicted, input, dt);
            }
        }
    }
    
    // Clean up acknowledged inputs
    while (pred->input_tail < pred->input_head) {
        uint32_t idx = pred->input_tail % MAX_PREDICTED_INPUTS;
        if (pred->input_buffer[idx].sequence > acked_sequence) break;
        pred->input_tail++;
    }
}

// =================================================================================================
// ENTITY INTERPOLATION (for non-player entities)
// =================================================================================================

void prediction_interpolate_entity(float current[3], float target[3], float smooth) {
    current[0] += (target[0] - current[0]) * smooth;
    current[1] += (target[1] - current[1]) * smooth;
    current[2] += (target[2] - current[2]) * smooth;
}
