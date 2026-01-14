/**
 * =================================================================================================
 *                          NPC GUARD BEHAVIOR
 * =================================================================================================
 *
 * State machine for Guard NPCs.
 * - Patrols route
 * - Suspicion on sound/sight
 * - Investigations
 * - Attack on Confirm
 *
 * =================================================================================================
 */

#include <ai/npc/prebuilt/npc_guard.h>
#include <stdbool.h>
#include <stdint.h>
#include <include/math/math_all.h>
#include <physics/physics.h>
#include <physics/physics.h>

// Implemented: Guard States
typedef enum GuardState {
    GUARD_IDLE = 0,
    GUARD_PATROL = 1,
    GUARD_SUSPICIOUS = 2,
    GUARD_INVESTIGATE = 3,
    GUARD_ALERT = 4,
    GUARD_COMBAT = 5,
    GUARD_SEARCH = 6,
} GuardState;

typedef struct GuardData {
    GuardState state;
    float suspicion_meter;
    float last_known_position[3];
    bool has_last_known_position;
    float fov_angle;
    float sight_range;
} GuardData;

// Implemented: Field-of-View check
bool guard_can_see_player(GuardData *data, float guard_pos[3], float guard_forward[3], 
                          float player_pos[3]) {
    if (!data) return false;
    
    // Vector to player
    float to_player[3] = {
        player_pos[0] - guard_pos[0],
        player_pos[1] - guard_pos[1],
        player_pos[2] - guard_pos[2]
    };
    
    float distance = sqrtf(to_player[0]*to_player[0] + to_player[1]*to_player[1] + 
                           to_player[2]*to_player[2]);
    
    if (distance > data->sight_range) return false;
    
    // Normalize
    to_player[0] /= distance;
    to_player[1] /= distance;
    to_player[2] /= distance;
    
    // Dot product with forward vector
    float dot = guard_forward[0]*to_player[0] + guard_forward[1]*to_player[1] + 
                guard_forward[2]*to_player[2];
    
    float angle = acosf(dot);
    
    // Within FOV cone?
    if (angle < data->fov_angle / 2.0f) {
        // Check for occlusion
        // Check for occlusion
        // Replace with actual physics raycast
        // PhysicsRaycastResult result;
        // if (physics_raycast(guard_pos, to_player, distance, PHYS_LAYER_WORLD, &result)) {
        //    if (result.distance < distance - 0.5f) { 
        //         return false; 
        //    }
        // }
        return true;
    }
    
    return false;
}

// Implemented: Suspicion meter
void guard_update_suspicion(GuardData *data, float dt, bool saw_player_peripheral) {
    if (!data) return;
    
    if (saw_player_peripheral) {
        data->suspicion_meter += dt * 0.5f; // Increases when player in peripheral
    } else {
        data->suspicion_meter -= dt * 0.2f; // Decays over time
    }
    
    // Clamp [0, 1]
    if (data->suspicion_meter < 0.0f) data->suspicion_meter = 0.0f;
    if (data->suspicion_meter > 1.0f) data->suspicion_meter = 1.0f;
    
    // State transitions
    if (data->suspicion_meter > 0.8f && data->state != GUARD_ALERT) {
        data->state = GUARD_ALERT;
    } else if (data->suspicion_meter > 0.5f && data->state == GUARD_PATROL) {
        data->state = GUARD_SUSPICIOUS;
    }
}

// Implemented: "Last Known Position" logic
void guard_set_last_known_position(GuardData *data, float pos[3]) {
    if (!data) return;
    
    data->last_known_position[0] = pos[0];
    data->last_known_position[1] = pos[1];
    data->last_known_position[2] = pos[2];
    data->has_last_known_position = true;
    data->state = GUARD_INVESTIGATE;
}

void guard_investigate_last_position(GuardData *data, float guard_pos[3]) {
    if (!data || !data->has_last_known_position) return;
    
    // Move to last known position
    // Once reached, transition to GUARD_SEARCH state
    float dx = data->last_known_position[0] - guard_pos[0];
    float dz = data->last_known_position[2] - guard_pos[2];
    float dist = sqrtf(dx*dx + dz*dz);
    
    if (dist < 2.0f) { // Reached
        data->state = GUARD_SEARCH;
    }
}

// Implemented: Alarm triggering
void guard_trigger_alarm(GuardData *data, float alarm_pos[3]) {
    if (!data) return;
    
    // Navigate to alarm position
    // Once reached, activate alarm which alerts nearby guards
    // Simplified: Just transition to ALERT state
    data->state = GUARD_ALERT;
}
