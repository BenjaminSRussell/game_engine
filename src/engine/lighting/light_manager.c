#include "light_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <stdio.h>

#define DEFAULT_SHADOW_MAP_RES 1024
#define DEFAULT_SHADOW_BIAS 0.005f
#define DEFAULT_SHADOW_NORMAL_BIAS 0.02f

struct LightManager {
    Light lights[MAX_LIGHTS];
    uint32_t active_count;
    uint32_t next_id;
};

LightManager* LightManager_Create(void) {
    LightManager* manager = (LightManager*)malloc(sizeof(LightManager));
    if (manager) {
        memset(manager, 0, sizeof(LightManager));
        manager->next_id = 1; // 0 is invalid/null
    }
    return manager;
}

void LightManager_Destroy(LightManager* manager) {
    if (manager) {
        free(manager);
    }
}

static Light* FindFreeSlot(LightManager* manager) {
    // Simple linear scan for now. 
    // Optimization: could maintain a free list or track max index.
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (manager->lights[i].base.id == 0) {
            return &manager->lights[i];
        }
    }
    return NULL;
}

static void InitBaseLight(Light* light, uint32_t id, LightType type, Vec3 color, float intensity) {
    memset(light, 0, sizeof(Light));
    light->base.id = id;
    light->base.type = type;
    light->base.enabled = true;
    light->base.color = color;
    light->base.intensity = intensity;
    light->base.cast_shadows = false;
    light->base.shadow_map_resolution = DEFAULT_SHADOW_MAP_RES;
    light->base.shadow_bias = DEFAULT_SHADOW_BIAS;
    light->base.shadow_normal_bias = DEFAULT_SHADOW_NORMAL_BIAS;
}

uint32_t LightManager_CreateDirectionalLight(LightManager* manager, Vec3 direction, Vec3 color, float intensity) {
    if (!manager) return 0;
    
    Light* slot = FindFreeSlot(manager);
    if (!slot) return 0; // Out of slots
    
    uint32_t id = manager->next_id++;
    InitBaseLight(slot, id, LIGHT_TYPE_DIRECTIONAL, color, intensity);
    
    // Normalize direction
    float len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (len > 0.0001f) {
        slot->directional.direction = vec3(direction.x / len, direction.y / len, direction.z / len);
    } else {
        slot->directional.direction = vec3(0, -1, 0);
    }
    
    manager->active_count++;
    return id;
}

uint32_t LightManager_CreatePointLight(LightManager* manager, Vec3 position, float range, Vec3 color, float intensity) {
    if (!manager) return 0;
    
    Light* slot = FindFreeSlot(manager);
    if (!slot) return 0;
    
    uint32_t id = manager->next_id++;
    InitBaseLight(slot, id, LIGHT_TYPE_POINT, color, intensity);
    
    slot->point.position = position;
    slot->point.range = range;
    
    // Default attenuation matching standard falloff
    slot->point.constant_attenuation = 1.0f;
    slot->point.linear_attenuation = 4.5f / range; 
    slot->point.quadratic_attenuation = 75.0f / (range * range);
    
    manager->active_count++;
    return id;
}

uint32_t LightManager_CreateSpotLight(LightManager* manager, Vec3 position, Vec3 direction, float range, 
                                     float inner_angle, float outer_angle, Vec3 color, float intensity) {
    if (!manager) return 0;
    
    Light* slot = FindFreeSlot(manager);
    if (!slot) return 0;
    
    uint32_t id = manager->next_id++;
    InitBaseLight(slot, id, LIGHT_TYPE_SPOT, color, intensity);
    
    slot->spot.position = position;
    slot->spot.range = range;
    slot->spot.inner_cone_angle = inner_angle;
    slot->spot.outer_cone_angle = outer_angle;
    
    float len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (len > 0.0001f) {
        slot->spot.direction = vec3(direction.x / len, direction.y / len, direction.z / len);
    } else {
        slot->spot.direction = vec3(0, -1, 0);
    }

    // Default attenuation
    slot->spot.constant_attenuation = 1.0f;
    slot->spot.linear_attenuation = 4.5f / range;
    slot->spot.quadratic_attenuation = 75.0f / (range * range);
    
    manager->active_count++;
    return id;
}

void LightManager_DestroyLight(LightManager* manager, uint32_t light_id) {
    if (!manager || light_id == 0) return;
    
    // Linear search for ID (simpler than map for <2048 items usually, but map would be better if frequent)
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (manager->lights[i].base.id == light_id) {
            memset(&manager->lights[i], 0, sizeof(Light));
            manager->active_count--;
            return;
        }
    }
}

void LightManager_SetLightEnabled(LightManager* manager, uint32_t light_id, bool enabled) {
    if (!manager) return;
    Light* l = LightManager_GetLight(manager, light_id);
    if (l) {
        l->base.enabled = enabled;
    }
}

bool LightManager_IsLightEnabled(const LightManager* manager, uint32_t light_id) {
    if (!manager) return false;
    // const cast hack if we reuse GetLight, or duplicate logic. 
    // Since GetLight returns non-const, we can't call it easily on const manager without cast.
    // Iterating manually:
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (manager->lights[i].base.id == light_id) {
            return manager->lights[i].base.enabled;
        }
    }
    return false;
}

Light* LightManager_GetLight(LightManager* manager, uint32_t light_id) {
    if (!manager || light_id == 0) return NULL;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (manager->lights[i].base.id == light_id) {
            return &manager->lights[i];
        }
    }
    return NULL;
}

uint32_t LightManager_GetLightCount(const LightManager* manager) {
    return manager ? manager->active_count : 0;
}

const Light* LightManager_GetLightsArray(const LightManager* manager) {
    return manager ? manager->lights : NULL;
}

void LightManager_Update(LightManager* manager, float delta_time) {
    // Placeholder for animation updates
    (void)manager;
    (void)delta_time;
}
