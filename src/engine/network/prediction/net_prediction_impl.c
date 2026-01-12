// net_prediction_impl.c - Client-side prediction implementation
// TODO: PREDICTION - Implement entity state prediction beyond position/velocity
// TODO: PREDICTION - Add rollback and replay system for misprediction correction
// TODO: PREDICTION - Implement adaptive prediction based on network quality
// TODO: PREDICTION - Add prediction for multiple entities (not just player)
// TODO: PREDICTION - Implement prediction smoothing and visual correction
#include "include/network/network_prediction.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

ClientPrediction *prediction_create(void) {
    ClientPrediction *pred = malloc(sizeof(ClientPrediction));
    if (!pred) {
        log_error("Failed to allocate client prediction");
        return NULL;
    }
    
    memset(pred, 0, sizeof(ClientPrediction));
    pred->head = 0;
    pred->count = 0;
    pred->last_acknowledged_frame = 0;
    pred->mispredicted = false;
    
    log_info("Client prediction system created");
    return pred;
}

void prediction_destroy(ClientPrediction *pred) {
    if (pred) {
        // Free any allocated input states
        for (u32 i = 0; i < MAX_PREDICTION_HISTORY; i++) {
            if (pred->history[i].input_state) {
                free(pred->history[i].input_state);
            }
        }
        free(pred);
        log_info("Client prediction system destroyed");
    }
}

void prediction_add_frame(ClientPrediction *pred, u32 frame, Vec3 pos, Vec3 vel) {
    if (!pred) {
        return;
    }
    
    u32 index = pred->head % MAX_PREDICTION_HISTORY;
    
    // Free old input state if it exists
    if (pred->history[index].input_state) {
        free(pred->history[index].input_state);
        pred->history[index].input_state = NULL;
    }
    
    // Add new frame
    pred->history[index].frame_number = frame;
    pred->history[index].position = pos;
    pred->history[index].velocity = vel;
    pred->history[index].timestamp = 0.0f; // Will be set by caller
    pred->history[index].input_state = NULL; // Will be set by caller
    
    pred->head++;
    if (pred->count < MAX_PREDICTION_HISTORY) {
        pred->count++;
    }
    
    log_debug("Added prediction frame %u at position (%.2f, %.2f, %.2f)", 
              frame, pos.x, pos.y, pos.z);
}

void prediction_add_frame_with_input(ClientPrediction *pred, u32 frame, Vec3 pos, 
                                   Vec3 vel, void *input_state, u32 input_size) {
    if (!pred) {
        return;
    }
    
    u32 index = pred->head % MAX_PREDICTION_HISTORY;
    
    // Free old input state if it exists
    if (pred->history[index].input_state) {
        free(pred->history[index].input_state);
    }
    
    // Add new frame with input
    pred->history[index].frame_number = frame;
    pred->history[index].position = pos;
    pred->history[index].velocity = vel;
    pred->history[index].timestamp = 0.0f;
    
    // Copy input state
    if (input_state && input_size > 0) {
        pred->history[index].input_state = malloc(input_size);
        if (pred->history[index].input_state) {
            memcpy(pred->history[index].input_state, input_state, input_size);
        }
    } else {
        pred->history[index].input_state = NULL;
    }
    
    pred->head++;
    if (pred->count < MAX_PREDICTION_HISTORY) {
        pred->count++;
    }
    
    log_debug("Added prediction frame %u with input (%u bytes)", frame, input_size);
}

void prediction_reconcile(ClientPrediction *pred, u32 server_frame, Vec3 server_pos) {
    if (!pred) {
        return;
    }
    
    // Find the predicted frame for this server frame
    PredictionFrame *predicted_frame = NULL;
    for (u32 i = 0; i < pred->count; i++) {
        u32 index = (pred->head - pred->count + i) % MAX_PREDICTION_HISTORY;
        if (pred->history[index].frame_number == server_frame) {
            predicted_frame = &pred->history[index];
            break;
        }
    }
    
    if (!predicted_frame) {
        log_warn("Server frame %u not found in prediction history", server_frame);
        return;
    }
    
    // Check if prediction was correct
    f32 pos_diff = vec3_length(vec3_sub(server_pos, predicted_frame->position));
    
    if (pos_diff > 0.1f) { // Threshold for considering it a misprediction
        pred->mispredicted = true;
        log_info("Misprediction detected! Server pos: (%.2f, %.2f, %.2f), "
                 "Predicted pos: (%.2f, %.2f, %.2f), Diff: %.3f",
                 server_pos.x, server_pos.y, server_pos.z,
                 predicted_frame->position.x, predicted_frame->position.y, 
                 predicted_frame->position.z, pos_diff);
        
        // Correct the position
        predicted_frame->position = server_pos;
    } else {
        pred->mispredicted = false;
        log_debug("Prediction correct for frame %u", server_frame);
    }
    
    pred->last_acknowledged_frame = server_frame;
}

bool prediction_get_frame(ClientPrediction *pred, u32 frame, PredictionFrame *out_frame) {
    if (!pred || !out_frame) {
        return false;
    }
    
    for (u32 i = 0; i < pred->count; i++) {
        u32 index = (pred->head - pred->count + i) % MAX_PREDICTION_HISTORY;
        if (pred->history[index].frame_number == frame) {
            *out_frame = pred->history[index];
            return true;
        }
    }
    
    return false;
}

Vec3 prediction_get_current_position(ClientPrediction *pred) {
    if (!pred || pred->count == 0) {
        return (Vec3){0, 0, 0};
    }
    
    u32 index = (pred->head - 1) % MAX_PREDICTION_HISTORY;
    return pred->history[index].position;
}

Vec3 prediction_get_current_velocity(ClientPrediction *pred) {
    if (!pred || pred->count == 0) {
        return (Vec3){0, 0, 0};
    }
    
    u32 index = (pred->head - 1) % MAX_PREDICTION_HISTORY;
    return pred->history[index].velocity;
}

bool prediction_was_mispredicted(ClientPrediction *pred) {
    return pred ? pred->mispredicted : false;
}

u32 prediction_get_last_acknowledged_frame(ClientPrediction *pred) {
    return pred ? pred->last_acknowledged_frame : 0;
}

void prediction_clear_old_frames(ClientPrediction *pred, u32 before_frame) {
    if (!pred) {
        return;
    }
    
    u32 removed = 0;
    for (u32 i = 0; i < pred->count; i++) {
        u32 index = (pred->head - pred->count + i) % MAX_PREDICTION_HISTORY;
        if (pred->history[index].frame_number < before_frame) {
            if (pred->history[index].input_state) {
                free(pred->history[index].input_state);
                pred->history[index].input_state = NULL;
            }
            removed++;
        } else {
            break; // Frames are in order, so we can stop here
        }
    }
    
    if (removed > 0) {
        pred->count -= removed;
        log_debug("Cleared %u old prediction frames", removed);
    }
}

// Get frames that need to be re-simulated after reconciliation
u32 prediction_get_frames_to_resimulate(ClientPrediction *pred, PredictionFrame *frames, u32 max_frames) {
    if (!pred || !frames || max_frames == 0) {
        return 0;
    }
    
    u32 count = 0;
    for (u32 i = 0; i < pred->count && count < max_frames; i++) {
        u32 index = (pred->head - pred->count + i) % MAX_PREDICTION_HISTORY;
        if (pred->history[index].frame_number > pred->last_acknowledged_frame) {
            frames[count] = pred->history[index];
            count++;
        }
    }
    
    return count;
}

// TODO: PREDICTION - Add prediction accuracy metrics and statistics
// TODO: PREDICTION - Implement prediction debugging and visualization tools
// TODO: PREDICTION - Add prediction for complex interactions (physics, collisions)
// TODO: PREDICTION - Implement prediction memory optimization (circular buffers)
