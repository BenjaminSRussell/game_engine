#ifndef LIGHT_SYSTEM_H
#define LIGHT_SYSTEM_H

#include "light_types.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_LIGHTS 1024

/**
 * Light System
 * Canonical implementation for managing lights in the scene.
 */

// Initialize the light system
void LightSystem_Init(void);

// Shutdown the light system and free resources
void LightSystem_Shutdown(void);

// Update the light system (e.g. animate lights, cull, etc.)
void LightSystem_Update(float delta_time);

// Create a new directional light
// Returns the light ID or 0 on failure
uint32_t LightSystem_AddDirectionalLight(Vec3 direction, Vec3 color, float intensity, bool cast_shadows);

// Create a new point light
// Returns the light ID or 0 on failure
uint32_t LightSystem_AddPointLight(Vec3 position, float range, Vec3 color, float intensity, bool cast_shadows);

// Create a new spot light
// Returns the light ID or 0 on failure
uint32_t LightSystem_AddSpotLight(Vec3 position, Vec3 direction, float range,
                                  float inner_angle, float outer_angle,
                                  Vec3 color, float intensity, bool cast_shadows);

// Remove a light by ID
void LightSystem_RemoveLight(uint32_t light_id);

// Get a pointer to a light by ID (do not store this pointer long-term)
Light* LightSystem_GetLight(uint32_t light_id);

// Set light enabled state
void LightSystem_SetLightEnabled(uint32_t light_id, bool enabled);

// Get the total number of active lights
uint32_t LightSystem_GetActiveLightCount(void);

// Get an array of pointers to active lights
// This allows for efficient iteration without checking for null/empty slots
const Light** LightSystem_GetActiveLights(uint32_t* out_count);

#endif // LIGHT_SYSTEM_H
