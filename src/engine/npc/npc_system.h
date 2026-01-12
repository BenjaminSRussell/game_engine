/*
 * NPC System Header
 */

#ifndef NPC_SYSTEM_H
#define NPC_SYSTEM_H

#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// NPC types
typedef enum {
    NPC_TYPE_VILLAGER = 0,
    NPC_TYPE_GUARD = 1,
    NPC_TYPE_MERCHANT = 2,
    NPC_TYPE_ANIMAL = 3,
    NPC_TYPE_MONSTER = 4
} NPCType;

// Initialize NPC system
void npc_system_init(void);

// Shutdown NPC system
void npc_system_shutdown(void);

// Create new NPC
void* npc_create(NPCType type, const char* name, Vec3 position);

// Remove NPC
void npc_remove(int npc_id);

// Update all NPCs
void npc_update(float dt);

// Get NPC by ID
void* npc_get(int npc_id);

// Get all NPCs
void* npc_get_all(int* count);

// Get NPCs in range
void* npc_get_in_range(Vec3 center, float range, int* count);

// Set NPC target
void npc_set_target(int npc_id, Vec3 target);

// Damage NPC
void npc_damage(int npc_id, float damage);

// Heal NPC
void npc_heal(int npc_id, float amount);

// Get NPC count
int npc_get_count(void);

// Get NPC position
Vec3 npc_get_position(int npc_id);

// Get NPC health
float npc_get_health(int npc_id);

// Check if NPC is active
int npc_is_active(int npc_id);

#ifdef __cplusplus
}
#endif

#endif // NPC_SYSTEM_H
