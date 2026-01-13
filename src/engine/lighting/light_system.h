#ifndef LIGHT_SYSTEM_H
#define LIGHT_SYSTEM_H

#include "light_types.h"
#include <stdbool.h>

// Initialize the lighting system
bool light_system_init(void);

// Shutdown the lighting system
void light_system_shutdown(void);

// Update the lighting system (e.g. animate lights, cull lights)
void light_system_update(float delta_time);

// Create a new light and return its ID (0 on failure)
uint32_t light_system_create_light(LightType type);

// Destroy a light by ID
void light_system_destroy_light(uint32_t id);

// Get a pointer to the light data for modification
Light* light_system_get_light(uint32_t id);

// Get all active lights in a flat array (useful for rendering)
// out_count will be populated with the number of lights
// Returns a pointer to the internal array of lights
const Light* light_system_get_all_lights(uint32_t* out_count);

// Prune invalid or disabled lights (optional optimization)
void light_system_prune(void);

#endif // LIGHT_SYSTEM_H
