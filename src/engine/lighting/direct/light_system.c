#include "lighting/direct/light_system.h"
#include <include/math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Dynamic Lighting System Implementation
 */

#define MAX_LIGHTS 1024
#define MAX_VISIBLE_LIGHTS 256

struct LightSystem {
  Light lights[MAX_LIGHTS];
  uint32_t light_count;
  uint32_t next_id;

  // Visible lights after culling
  Light *visible_lights[MAX_VISIBLE_LIGHTS];
  uint32_t visible_count;

  // Cluster data for forward+ rendering
  uint32_t cluster_count;
  // Additional cluster data would go here
};

// Create light system
LightSystem *light_system_create(void) {
  LightSystem *system = calloc(1, sizeof(LightSystem));
  if (!system)
    return NULL;

  system->light_count = 0;
  system->next_id = 1;
  system->visible_count = 0;

  printf("[LightSystem] Created lighting system\n");
  return system;
}

// Destroy light system
void light_system_destroy(LightSystem *system) {
  if (!system)
    return;
  free(system);
  printf("[LightSystem] Destroyed lighting system\n");
}

// Add light
uint32_t light_system_add_light(LightSystem *system, LightType type) {
  if (!system || system->light_count >= MAX_LIGHTS) {
    printf("[LightSystem] ERROR: Cannot add more lights\n");
    return 0;
  }

  Light *light = &system->lights[system->light_count];
  memset(light, 0, sizeof(Light));

  light->id = system->next_id++;
  light->type = type;
  light->enabled = true;
  light->cast_shadows = false;

  // Default values
  light->color[0] = 1.0f;
  light->color[1] = 1.0f;
  light->color[2] = 1.0f;
  light->intensity = 1.0f;
  light->temperature = 6500.0f; // Daylight

  light->range = 10.0f;
  light->constant_attenuation = 1.0f;
  light->linear_attenuation = 0.09f;
  light->quadratic_attenuation = 0.032f;

  if (type == LIGHT_TYPE_SPOT) {
    light->inner_cone_angle = 0.523599f; // 30 degrees
    light->outer_cone_angle = 0.785398f; // 45 degrees
  } else if (type == LIGHT_TYPE_AREA) {
    light->width = 1.0f;
    light->height = 1.0f;
    light->shape = LIGHT_SHAPE_RECTANGULAR;
  }

  light->shadow_map_resolution = 1024;
  light->shadow_bias = 0.005f;
  light->shadow_normal_bias = 0.01f;

  system->light_count++;

  printf("[LightSystem] Added %s light (ID: %u)\n",
         type == LIGHT_TYPE_POINT         ? "Point"
         : type == LIGHT_TYPE_SPOT        ? "Spot"
         : type == LIGHT_TYPE_DIRECTIONAL ? "Directional"
                                          : "Area",
         light->id);

  return light->id;
}

// Remove light
void light_system_remove_light(LightSystem *system, uint32_t light_id) {
  if (!system)
    return;

  for (uint32_t i = 0; i < system->light_count; i++) {
    if (system->lights[i].id == light_id) {
      // Shift remaining lights
      memmove(&system->lights[i], &system->lights[i + 1],
              (system->light_count - i - 1) * sizeof(Light));
      system->light_count--;
      printf("[LightSystem] Removed light ID: %u\n", light_id);
      return;
    }
  }
}

// Get light
Light *light_system_get_light(LightSystem *system, uint32_t light_id) {
  if (!system)
    return NULL;

  for (uint32_t i = 0; i < system->light_count; i++) {
    if (system->lights[i].id == light_id) {
      return &system->lights[i];
    }
  }

  return NULL;
}

// Configuration functions
void light_set_position(Light *light, float x, float y, float z) {
  if (!light)
    return;
  light->position[0] = x;
  light->position[1] = y;
  light->position[2] = z;
}

void light_set_direction(Light *light, float x, float y, float z) {
  if (!light)
    return;
  // Normalize
  float len = sqrtf(x * x + y * y + z * z);
  if (len > 1e-6f) {
    light->direction[0] = x / len;
    light->direction[1] = y / len;
    light->direction[2] = z / len;
  }
}

void light_set_color(Light *light, float r, float g, float b) {
  if (!light)
    return;
  light->color[0] = r;
  light->color[1] = g;
  light->color[2] = b;
}

void light_set_intensity(Light *light, float intensity) {
  if (!light)
    return;
  light->intensity = intensity;
}

void light_set_range(Light *light, float range) {
  if (!light)
    return;
  light->range = range;
}

void light_set_attenuation(Light *light, float constant, float linear,
                           float quadratic) {
  if (!light)
    return;
  light->constant_attenuation = constant;
  light->linear_attenuation = linear;
  light->quadratic_attenuation = quadratic;
}

// Spot light
void light_set_spot_angles(Light *light, float inner_angle, float outer_angle) {
  if (!light || light->type != LIGHT_TYPE_SPOT)
    return;
  light->inner_cone_angle = inner_angle;
  light->outer_cone_angle = outer_angle;
}

void light_set_cookie_texture(Light *light, uint32_t texture_id) {
  if (!light || light->type != LIGHT_TYPE_SPOT)
    return;
  light->cookie_texture = texture_id;
}

// Area light
void light_set_area_size(Light *light, float width, float height) {
  if (!light || light->type != LIGHT_TYPE_AREA)
    return;
  light->width = width;
  light->height = height;
}

void light_set_area_shape(Light *light, LightShape shape) {
  if (!light || light->type != LIGHT_TYPE_AREA)
    return;
  light->shape = shape;
}

// Shadows
void light_set_shadow_enabled(Light *light, bool enabled) {
  if (!light)
    return;
  light->cast_shadows = enabled;
}

void light_set_shadow_resolution(Light *light, uint32_t resolution) {
  if (!light)
    return;
  light->shadow_map_resolution = resolution;
}

void light_set_shadow_bias(Light *light, float bias, float normal_bias) {
  if (!light)
    return;
  light->shadow_bias = bias;
  light->shadow_normal_bias = normal_bias;
}

// Culling (simplified - would integrate with actual frustum culling)
void light_system_update_culling(LightSystem *system,
                                 const float *view_frustum) {
  if (!system)
    return;

  system->visible_count = 0;

  for (uint32_t i = 0;
       i < system->light_count && system->visible_count < MAX_VISIBLE_LIGHTS;
       i++) {
    Light *light = &system->lights[i];

    if (!light->enabled)
      continue;

    // Directional lights are always visible
    if (light->type == LIGHT_TYPE_DIRECTIONAL) {
      system->visible_lights[system->visible_count++] = light;
      continue;
    }

    // TODO: Actual frustum culling for point/spot/area lights
    // For now, include all enabled lights
    system->visible_lights[system->visible_count++] = light;
  }
}

// Sorting (by distance to camera)
void light_system_sort_lights(LightSystem *system, const float *camera_pos) {
  if (!system)
    return;

  // TODO: Implement sorting by distance/importance
  // This would sort visible_lights array
}

// Getters
uint32_t light_system_get_visible_count(const LightSystem *system) {
  return system ? system->visible_count : 0;
}

const Light **light_system_get_visible_lights(const LightSystem *system) {
  return system ? (const Light **)system->visible_lights : NULL;
}

// Clustering (stub - full implementation would be more complex)
void light_system_build_clusters(LightSystem *system, uint32_t tile_count_x,
                                 uint32_t tile_count_y, uint32_t tile_count_z) {
  if (!system)
    return;

  system->cluster_count = tile_count_x * tile_count_y * tile_count_z;
  printf("[LightSystem] Built %u clusters (%ux%ux%u)\n", system->cluster_count,
         tile_count_x, tile_count_y, tile_count_z);
}

void light_system_assign_lights_to_clusters(LightSystem *system) {
  if (!system)
    return;
  // TODO: Implement cluster light assignment
}
