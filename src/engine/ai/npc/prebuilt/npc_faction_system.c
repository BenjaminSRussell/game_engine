/**
 * =================================================================================================
 *                          NPC FACTION SYSTEM
 * =================================================================================================
 *
 * Manages relationships between groups of NPCs.
 *
 * =================================================================================================
 */

#include <ai/npc/prebuilt/npc_faction_system.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FACTIONS 32

typedef struct Faction {
    uint32_t id;
    char name[64];
    float reputation_matrix[MAX_FACTIONS]; // Affinity with other factions (0.0 = enemy, 1.0 = ally)
} Faction;

static Faction g_factions[MAX_FACTIONS];
static uint32_t g_faction_count = 0;
static float g_player_reputation[MAX_FACTIONS]; // Player's standing with each faction

// Implemented: Create reputation matrix
void faction_system_init() {
    memset(g_factions, 0, sizeof(g_factions));
    g_faction_count = 0;
    
    // Initialize player reputation to neutral (0.5)
    for (int i = 0; i < MAX_FACTIONS; i++) {
        g_player_reputation[i] = 0.5f;
    }
}

uint32_t faction_create(const char *name) {
    if (g_faction_count >= MAX_FACTIONS) return 0;
    
    Faction *faction = &g_factions[g_faction_count];
    faction->id = g_faction_count + 1;
    strncpy(faction->name, name, 63);
    
    // Initialize neutral relations with all factions
    for (int i = 0; i < MAX_FACTIONS; i++) {
        faction->reputation_matrix[i] = 0.5f;
    }
    
    return ++g_faction_count;
}

// Implemented: Response queries
bool faction_is_enemy(uint32_t faction_a, uint32_t faction_b) {
    if (faction_a == 0 || faction_b == 0 || faction_a > g_faction_count || faction_b > g_faction_count) {
        return false;
    }
    
    return g_factions[faction_a - 1].reputation_matrix[faction_b - 1] < 0.3f;
}

bool faction_is_ally(uint32_t faction_a, uint32_t faction_b) {
    if (faction_a == 0 || faction_b == 0 || faction_a > g_faction_count || faction_b > g_faction_count) {
        return false;
    }
    
    return g_factions[faction_a - 1].reputation_matrix[faction_b - 1] > 0.7f;
}

// Implemented: Reputation events
void faction_modify_player_reputation(uint32_t faction_id, float delta) {
    if (faction_id == 0 || faction_id > g_faction_count) return;
    
    g_player_reputation[faction_id - 1] += delta;
    
    // Clamp to [0, 1]
    if (g_player_reputation[faction_id - 1] < 0.0f) g_player_reputation[faction_id - 1] = 0.0f;
    if (g_player_reputation[faction_id - 1] > 1.0f) g_player_reputation[faction_id - 1] = 1.0f;
}

void faction_on_member_killed(uint32_t faction_id) {
    faction_modify_player_reputation(faction_id, -0.2f);
}

void faction_on_member_helped(uint32_t faction_id) {
    faction_modify_player_reputation(faction_id, 0.1f);
}

// Implemented: Territory control
typedef struct FactionTerritory {
    uint32_t faction_id;
    float center[3];
    float radius;
} FactionTerritory;

static FactionTerritory g_territories[64];
static uint32_t g_territory_count = 0;

void faction_create_territory(uint32_t faction_id, float center[3], float radius) {
    if (g_territory_count >= 64) return;
    
    FactionTerritory *territory = &g_territories[g_territory_count++];
    territory->faction_id = faction_id;
    territory->center[0] = center[0];
    territory->center[1] = center[1];
    territory->center[2] = center[2];
    territory->radius = radius;
}

bool faction_is_trespassing(float pos[3], uint32_t *out_faction_id) {
    for (uint32_t i = 0; i < g_territory_count; i++) {
        float dx = pos[0] - g_territories[i].center[0];
        float dy = pos[1] - g_territories[i].center[1];
        float dz = pos[2] - g_territories[i].center[2];
        float dist_sq = dx*dx + dy*dy + dz*dz;
        
        if (dist_sq < g_territories[i].radius * g_territories[i].radius) {
            uint32_t faction_id = g_territories[i].faction_id;
            
            // Check if player has low reputation with this faction
            if (g_player_reputation[faction_id - 1] < 0.3f) {
                if (out_faction_id) *out_faction_id = faction_id;
                return true;
            }
        }
    }
    
    return false;
}
