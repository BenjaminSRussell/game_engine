/*
 * NPC Visuals System Header
 */

#ifndef NPC_VISUALS_H
#define NPC_VISUALS_H

#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// Animation types
typedef enum {
    NPC_ANIM_IDLE = 0,
    NPC_ANIM_WALK = 1,
    NPC_ANIM_RUN = 2,
    NPC_ANIM_ATTACK = 3,
    NPC_ANIM_HURT = 4,
    NPC_ANIM_DIE = 5,
    NPC_ANIM_WORK = 6,
    NPC_ANIM_SLEEP = 7
} NPCAnimation;

// Initialize NPC visuals system
void npc_visuals_init(void);

// Shutdown NPC visuals system
void npc_visuals_shutdown(void);

// Create NPC visual
void* npc_visuals_create(int npc_id, const char* model_name, Vec3 position);

// Remove NPC visual
void npc_visuals_remove(int npc_id);

// Update NPC visual position
void npc_visuals_update_position(int npc_id, Vec3 position);

// Update NPC visual animation
void npc_visuals_update_animation(int npc_id, NPCAnimation animation);

// Update NPC visuals
void npc_visuals_update(float dt);

// Get NPC visual
void* npc_visuals_get(int npc_id);

// Get all NPC visuals
void* npc_visuals_get_all(int* count);

// Set NPC visibility
void npc_visuals_set_visibility(int npc_id, int visible);

// Get NPC visibility
int npc_visuals_get_visibility(int npc_id);

// Show health bar
void npc_visuals_show_health_bar(int npc_id, float duration);

// Hide health bar
void npc_visuals_hide_health_bar(int npc_id);

// Get visual count
int npc_visuals_get_count(void);

#ifdef __cplusplus
}
#endif

#endif // NPC_VISUALS_H
