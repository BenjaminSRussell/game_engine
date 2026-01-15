#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include "lighting/light_types.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_LIGHTS 2048

/**
 * Light Manager System
 * Manages lifecycle and storage of all lights in the scene.
 */
typedef struct LightManager LightManager;

/**
 * Initialization and Destruction
 */
LightManager* LightManager_Create(void);
void LightManager_Destroy(LightManager* manager);

/**
 * Light Creation API
 * Returns a unique light ID, or 0 on failure.
 */
uint32_t LightManager_CreateDirectionalLight(LightManager* manager, Vec3 direction, Vec3 color, float intensity);
uint32_t LightManager_CreatePointLight(LightManager* manager, Vec3 position, float range, Vec3 color, float intensity);
uint32_t LightManager_CreateSpotLight(LightManager* manager, Vec3 position, Vec3 direction, float range, 
                                     float inner_angle, float outer_angle, Vec3 color, float intensity);

/**
 * Light Management
 */
void LightManager_DestroyLight(LightManager* manager, uint32_t light_id);
void LightManager_SetLightEnabled(LightManager* manager, uint32_t light_id, bool enabled);
bool LightManager_IsLightEnabled(const LightManager* manager, uint32_t light_id);

/**
 * Accessors
 * Returns pointer to internal light structure. Do not store this pointer long-term.
 */
Light* LightManager_GetLight(LightManager* manager, uint32_t light_id);

/**
 * Bulk Access
 * Used for culling and rendering systems.
 */
uint32_t LightManager_GetLightCount(const LightManager* manager);
const Light* LightManager_GetLightsArray(const LightManager* manager); // Pointer to array

/**
 * Optional: Update loop (e.g. for animating lights or processing dirty states)
 */
void LightManager_Update(LightManager* manager, float delta_time);

#endif // LIGHT_MANAGER_H
