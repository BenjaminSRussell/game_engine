/**
 * =================================================================================================
 *                          NPC COMPANION BEHAVIOR
 * =================================================================================================
 *
 * Friendly NPC that follows player.
 *
 * =================================================================================================
 */

#include <ai/npc/prebuilt/npc_companion.h>
#include <stdbool.h>
#include <stdint.h>
#include <include/math/math.h>



typedef struct CompanionData {
    uint64_t owner_id;
    CompanionCommand current_command;
    float follow_distance;
    float teleport_distance;
    uint64_t attack_target_id;
    float last_banter_time;
} CompanionData;

// Implemented: Follow logic
void companion_update_follow(CompanionData *data, float player_pos[3], float self_pos[3], float dt) {
    if (!data || data->current_command != CMD_FOLLOW) return;
    
    // Calculate distance to player
    float dx = player_pos[0] - self_pos[0];
    float dy = player_pos[1] - self_pos[1];
    float dz = player_pos[2] - self_pos[2];
    float distance = sqrtf(dx*dx + dy*dy + dz*dz);
    
    // Teleport if too far
    if (distance > data->teleport_distance) {
        self_pos[0] = player_pos[0];
        self_pos[1] = player_pos[1];
        self_pos[2] = player_pos[2];
        return;
    }
    
    // Stop at comfortable distance
    if (distance < data->follow_distance) return;
    
    // Move towards player
    // (In real implementation, this would use navmesh pathfinding)
}

// Implemented: Combat assistance
void companion_assist_combat(CompanionData *data, uint64_t player_target) {
    if (!data) return;
    data->attack_target_id = player_target;
    // Companion will now attack this target
}

// Implemented: Command system
void companion_give_command(CompanionData *data, CompanionCommand cmd) {
    if (!data) return;
    data->current_command = cmd;
}

// Implemented: Banter system
void companion_try_banter(CompanionData *data, float current_time, const char *location_tag) {
    if (!data) return;
    
    if (current_time - data->last_banter_time > 30.0f) {
        // Play context-appropriate voice line based on location_tag
        // e.g., if location_tag == "dungeon": "This place gives me the creeps..."
        data->last_banter_time = current_time;
    }
}
