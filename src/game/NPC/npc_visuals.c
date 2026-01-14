/*
 * NPC Visuals System
 * Visual representation and rendering for NPCs
 */

#include "npc/npc_visuals.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

// Simple Vec3 structure for NPC visuals
typedef struct {
    float x, y, z;
} SimpleVec3;

// NPC visual data
typedef struct {
    int npc_id;
    char model_name[64];
    SimpleVec3 position;
    SimpleVec3 rotation;
    SimpleVec3 scale;
    int visible;
    float animation_time;
    int current_animation;
    float health_bar_alpha;
} NPCVisual;

#define MAX_NPC_VISUALS 64
static NPCVisual g_npc_visuals[MAX_NPC_VISUALS];
static int g_visual_count = 0;

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
void npc_visuals_init(void) {
    memset(g_npc_visuals, 0, sizeof(g_npc_visuals));
    g_visual_count = 0;
    
    LOG_INFO("NPC visuals system initialized");
}

// Shutdown NPC visuals system
void npc_visuals_shutdown(void) {
    memset(g_npc_visuals, 0, sizeof(g_npc_visuals));
    g_visual_count = 0;
    
    LOG_INFO("NPC visuals system shutdown");
}

// Create NPC visual
void* npc_visuals_create(int npc_id, const char* model_name, Vec3 position) {
    if (g_visual_count >= MAX_NPC_VISUALS) {
        LOG_ERROR("NPC visuals: Maximum visuals reached");
        return NULL;
    }
    
    NPCVisual* visual = &g_npc_visuals[g_visual_count];
    visual->npc_id = npc_id;
    strncpy(visual->model_name, model_name ? model_name : "default", 
            sizeof(visual->model_name) - 1);
    visual->position = position;
    visual->rotation = (Vec3){0, 0, 0};
    visual->scale = (Vec3){1, 1, 1};
    visual->visible = 1;
    visual->animation_time = 0.0f;
    visual->current_animation = NPC_ANIM_IDLE;
    visual->health_bar_alpha = 0.0f;
    
    g_visual_count++;
    LOG_INFO("NPC visual created: ID %d, Model %s", npc_id, model_name);
    
    return visual;
}

// Remove NPC visual
void npc_visuals_remove(int npc_id) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            
            // Move last visual to this slot
            if (i < g_visual_count - 1) {
                g_npc_visuals[i] = g_npc_visuals[g_visual_count - 1];
            }
            
            g_visual_count--;
            LOG_INFO("NPC visual removed: ID %d", npc_id);
            return;
        }
    }
    
    LOG_WARNING("NPC visual not found for removal: ID %d", npc_id);
}

// Update NPC visual position
void npc_visuals_update_position(int npc_id, Vec3 position) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            g_npc_visuals[i].position = position;
            return;
        }
    }
    
    LOG_WARNING("NPC visual not found for position update: ID %d", npc_id);
}

// Update NPC visual animation
void npc_visuals_update_animation(int npc_id, NPCAnimation animation) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            g_npc_visuals[i].current_animation = animation;
            g_npc_visuals[i].animation_time = 0.0f;
            LOG_DEBUG("NPC %d animation changed to %d", npc_id, animation);
            return;
        }
    }
    
    LOG_WARNING("NPC visual not found for animation update: ID %d", npc_id);
}

// Update NPC visuals
void npc_visuals_update(float dt) {
    for (int i = 0; i < g_visual_count; i++) {
        NPCVisual* visual = &g_npc_visuals[i];
        
        // Update animation time
        visual->animation_time += dt;
        
        // Simple animation state machine
        switch (visual->current_animation) {
            case NPC_ANIM_IDLE:
                // Idle breathing animation
                visual->rotation.y = sinf(visual->animation_time * 2.0f) * 0.05f;
                break;
                
            case NPC_ANIM_WALK:
            case NPC_ANIM_RUN:
                // Walking/running animation
                visual->rotation.y = sinf(visual->animation_time * 10.0f) * 0.1f;
                visual->scale.y = 1.0f + sinf(visual->animation_time * 10.0f) * 0.05f;
                break;
                
            case NPC_ANIM_ATTACK:
                // Attack animation
                if (visual->animation_time < 0.5f) {
                    visual->rotation.x = sinf(visual->animation_time * 20.0f) * 0.2f;
                } else {
                    visual->rotation.x = 0;
                }
                break;
                
            case NPC_ANIM_HURT:
                // Hurt animation
                visual->rotation.z = sinf(visual->animation_time * 15.0f) * 0.1f;
                visual->health_bar_alpha = 1.0f;
                break;
                
            case NPC_ANIM_DIE:
                // Death animation
                visual->scale.y = fmaxf(0.1f, 1.0f - visual->animation_time * 0.5f);
                visual->rotation.z = visual->animation_time * 2.0f;
                break;
                
            case NPC_ANIM_WORK:
                // Work animation
                visual->rotation.y = cosf(visual->animation_time * 8.0f) * 0.3f;
                visual->rotation.x = sinf(visual->animation_time * 12.0f) * 0.1f;
                break;
                
            case NPC_ANIM_SLEEP:
                // Sleep animation
                visual->rotation.y = sinf(visual->animation_time * 1.0f) * 0.02f;
                visual->scale.y = 1.0f + sinf(visual->animation_time * 2.0f) * 0.05f;
                break;
        }
        
        // Fade out health bar
        if (visual->health_bar_alpha > 0.0f) {
            visual->health_bar_alpha -= dt * 0.5f;
            if (visual->health_bar_alpha < 0.0f) {
                visual->health_bar_alpha = 0.0f;
            }
        }
    }
}

// Get NPC visual
void* npc_visuals_get(int npc_id) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            return &g_npc_visuals[i];
        }
    }
    
    return NULL;
}

// Get all NPC visuals
void* npc_visuals_get_all(int* count) {
    if (count) {
        *count = g_visual_count;
    }
    return g_npc_visuals;
}

// Set NPC visibility
void npc_visuals_set_visibility(int npc_id, int visible) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            g_npc_visuals[i].visible = visible;
            LOG_DEBUG("NPC %d visibility set to %d", npc_id, visible);
            return;
        }
    }
    
    LOG_WARNING("NPC visual not found for visibility: ID %d", npc_id);
}

// Get NPC visibility
int npc_visuals_get_visibility(int npc_id) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            return g_npc_visuals[i].visible;
        }
    }
    
    return 0;
}

// Show health bar
void npc_visuals_show_health_bar(int npc_id, float duration) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            g_npc_visuals[i].health_bar_alpha = 1.0f;
            LOG_DEBUG("Health bar shown for NPC %d", npc_id);
            return;
        }
    }
    
    LOG_WARNING("NPC visual not found for health bar: ID %d", npc_id);
}

// Hide health bar
void npc_visuals_hide_health_bar(int npc_id) {
    for (int i = 0; i < g_visual_count; i++) {
        if (g_npc_visuals[i].npc_id == npc_id) {
            g_npc_visuals[i].health_bar_alpha = 0.0f;
            LOG_DEBUG("Health bar hidden for NPC %d", npc_id);
            return;
        }
    }
    
    LOG_WARNING("NPC visual not found for health bar: ID %d", npc_id);
}

// Get visual count
int npc_visuals_get_count(void) {
    return g_visual_count;
}
