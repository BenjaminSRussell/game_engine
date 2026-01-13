/*
 * npc_system.c
 * Basic Non-Player Character system for VoxelForge
 *
 * Part of the NPC subsystem
 * Advanced 3D Rendering Engine
 */

#include "npc/npc_system.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Simple Vec3 structure for NPC system
typedef struct {
    float x, y, z;
} SimpleVec3;

// NPC structure
typedef struct {
    int id;
    char name[64];
    SimpleVec3 position;
    SimpleVec3 velocity;
    SimpleVec3 target_position;
    float health;
    float max_health;
    int active;
    int type;
    float speed;
    float attack_range;
    float detection_range;
    uint32_t last_update_time;
} NPC;

#define MAX_NPCS 64
static NPC g_npcs[MAX_NPCS];
static int g_npc_count = 0;
static int g_next_npc_id = 1;

// NPC types
typedef enum {
    NPC_TYPE_VILLAGER = 0,
    NPC_TYPE_GUARD = 1,
    NPC_TYPE_MERCHANT = 2,
    NPC_TYPE_ANIMAL = 3,
    NPC_TYPE_MONSTER = 4
} NPCType;

// Initialize NPC system
void npc_system_init(void) {
    memset(g_npcs, 0, sizeof(g_npcs));
    g_npc_count = 0;
    g_next_npc_id = 1;
    
    LOG_INFO("NPC system initialized");
}

// Shutdown NPC system
void npc_system_shutdown(void) {
    memset(g_npcs, 0, sizeof(g_npcs));
    g_npc_count = 0;
    g_next_npc_id = 1;
    
    LOG_INFO("NPC system shutdown");
}

// Create new NPC
void* npc_create(NPCType type, const char* name, Vec3 position) {
    if (g_npc_count >= MAX_NPCS) {
        LOG_ERROR("NPC system: Maximum NPCs reached");
        return NULL;
    }
    
    NPC* npc = &g_npcs[g_npc_count];
    npc->id = g_next_npc_id++;
    npc->type = type;
    npc->position = position;
    npc->velocity = (Vec3){0, 0, 0};
    npc->target_position = position;
    npc->health = 100.0f;
    npc->max_health = 100.0f;
    npc->active = 1;
    npc->last_update_time = (uint32_t)time(NULL);
    
    // Set NPC properties based on type
    switch (type) {
        case NPC_TYPE_VILLAGER:
            npc->speed = 1.0f;
            npc->attack_range = 2.0f;
            npc->detection_range = 10.0f;
            strncpy(npc->name, name ? name : "Villager", sizeof(npc->name) - 1);
            break;
            
        case NPC_TYPE_GUARD:
            npc->speed = 1.5f;
            npc->attack_range = 3.0f;
            npc->detection_range = 15.0f;
            strncpy(npc->name, name ? name : "Guard", sizeof(npc->name) - 1);
            break;
            
        case NPC_TYPE_MERCHANT:
            npc->speed = 0.8f;
            npc->attack_range = 0.0f;
            npc->detection_range = 5.0f;
            strncpy(npc->name, name ? name : "Merchant", sizeof(npc->name) - 1);
            break;
            
        case NPC_TYPE_ANIMAL:
            npc->speed = 2.0f;
            npc->attack_range = 1.0f;
            npc->detection_range = 8.0f;
            strncpy(npc->name, name ? name : "Animal", sizeof(npc->name) - 1);
            break;
            
        case NPC_TYPE_MONSTER:
            npc->speed = 1.8f;
            npc->attack_range = 4.0f;
            npc->detection_range = 20.0f;
            strncpy(npc->name, name ? name : "Monster", sizeof(npc->name) - 1);
            break;
            
        default:
            npc->speed = 1.0f;
            npc->attack_range = 2.0f;
            npc->detection_range = 10.0f;
            strncpy(npc->name, name ? name : "NPC", sizeof(npc->name) - 1);
            break;
    }
    
    g_npc_count++;
    LOG_INFO("NPC created: %s (ID: %d, Type: %d)", npc->name, npc->id, npc->type);
    
    return npc;
}

// Remove NPC
void npc_remove(int npc_id) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            g_npcs[i].active = 0;
            
            // Move last NPC to this slot
            if (i < g_npc_count - 1) {
                g_npcs[i] = g_npcs[g_npc_count - 1];
            }
            
            g_npc_count--;
            LOG_INFO("NPC removed: ID %d", npc_id);
            return;
        }
    }
    
    LOG_WARNING("NPC not found for removal: ID %d", npc_id);
}

// Update all NPCs
void npc_update(float dt) {
    for (int i = 0; i < g_npc_count; i++) {
        if (!g_npcs[i].active) {
            continue;
        }
        
        NPC* npc = &g_npcs[i];
        
        // Simple AI: move towards target
        Vec3 direction = {
            npc->target_position.x - npc->position.x,
            npc->target_position.y - npc->position.y,
            npc->target_position.z - npc->position.z
        };
        
        float distance = sqrtf(direction.x * direction.x + 
                           direction.y * direction.y + 
                           direction.z * direction.z);
        
        if (distance > 0.1f) {
            // Normalize direction and apply speed
            direction.x /= distance;
            direction.y /= distance;
            direction.z /= distance;
            
            npc->velocity.x = direction.x * npc->speed;
            npc->velocity.y = direction.y * npc->speed;
            npc->velocity.z = direction.z * npc->speed;
        } else {
            // Reached target, stop moving
            npc->velocity.x = 0;
            npc->velocity.y = 0;
            npc->velocity.z = 0;
        }
        
        // Update position
        npc->position.x += npc->velocity.x * dt;
        npc->position.y += npc->velocity.y * dt;
        npc->position.z += npc->velocity.z * dt;
        
        // Simple ground collision
        if (npc->position.y < 0) {
            npc->position.y = 0;
            npc->velocity.y = 0;
        }
        
        npc->last_update_time = (uint32_t)time(NULL);
    }
}

// Get NPC by ID
void* npc_get(int npc_id) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            return &g_npcs[i];
        }
    }
    
    return NULL;
}

// Get all NPCs
void* npc_get_all(int* count) {
    if (count) {
        *count = g_npc_count;
    }
    return g_npcs;
}

// Get NPCs in range
void* npc_get_in_range(Vec3 center, float range, int* count) {
    static NPC nearby_npcs[MAX_NPCS];
    int found_count = 0;
    
    for (int i = 0; i < g_npc_count; i++) {
        if (!g_npcs[i].active) {
            continue;
        }
        
        Vec3 diff = {
            g_npcs[i].position.x - center.x,
            g_npcs[i].position.y - center.y,
            g_npcs[i].position.z - center.z
        };
        
        float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        
        if (distance <= range) {
            nearby_npcs[found_count] = g_npcs[i];
            found_count++;
        }
    }
    
    if (count) {
        *count = found_count;
    }
    
    return nearby_npcs;
}

// Set NPC target
void npc_set_target(int npc_id, Vec3 target) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            g_npcs[i].target_position = target;
            LOG_DEBUG("NPC %d target set to (%.2f, %.2f, %.2f)", 
                     npc_id, target.x, target.y, target.z);
            return;
        }
    }
    
    LOG_WARNING("NPC not found for target setting: ID %d", npc_id);
}

// Damage NPC
void npc_damage(int npc_id, float damage) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            g_npcs[i].health -= damage;
            if (g_npcs[i].health <= 0) {
                g_npcs[i].health = 0;
                g_npcs[i].active = 0;
                LOG_INFO("NPC %d killed", npc_id);
            } else {
                LOG_DEBUG("NPC %d damaged: %.2f HP remaining", npc_id, g_npcs[i].health);
            }
            return;
        }
    }
    
    LOG_WARNING("NPC not found for damage: ID %d", npc_id);
}

// Heal NPC
void npc_heal(int npc_id, float amount) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            g_npcs[i].health += amount;
            if (g_npcs[i].health > g_npcs[i].max_health) {
                g_npcs[i].health = g_npcs[i].max_health;
            }
            LOG_DEBUG("NPC %d healed: %.2f HP", npc_id, g_npcs[i].health);
            return;
        }
    }
    
    LOG_WARNING("NPC not found for healing: ID %d", npc_id);
}

// Get NPC count
int npc_get_count(void) {
    return g_npc_count;
}

// Get NPC position
Vec3 npc_get_position(int npc_id) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            return g_npcs[i].position;
        }
    }
    
    return (Vec3){0, 0, 0};
}

// Get NPC health
float npc_get_health(int npc_id) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            return g_npcs[i].health;
        }
    }
    
    return 0.0f;
}

// Check if NPC is active
int npc_is_active(int npc_id) {
    for (int i = 0; i < g_npc_count; i++) {
        if (g_npcs[i].active && g_npcs[i].id == npc_id) {
            return 1;
        }
    }
    
    return 0;
}
