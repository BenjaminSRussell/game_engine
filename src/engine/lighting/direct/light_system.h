#ifndef LIGHT_SYSTEM_H
#define LIGHT_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Dynamic Lighting System
 * Supports point, spot, directional, and area lights with optimized culling
 */

// Light types
typedef enum {
  LIGHT_TYPE_POINT,
  LIGHT_TYPE_SPOT,
  LIGHT_TYPE_DIRECTIONAL,
  LIGHT_TYPE_AREA
} LightType;

// Light structure
typedef struct {
  uint32_t id;
  LightType type;
  bool enabled;
  bool cast_shadows;

  // Transform
  float position[3];
  float direction[3];

  // Color and intensity
  float color[3]; // RGB
  float intensity;
  float temperature; // Kelvin (optional color temperature)

  // Attenuation (for point/spot)
  float range;
  float constant_attenuation;
  float linear_attenuation;
  float quadratic_attenuation;

  // Spot light specific
  float inner_cone_angle;  // Radians
  float outer_cone_angle;  // Radians
  uint32_t cookie_texture; // Gobo/pattern texture

  // Area light specific
  float width; // For rectangular area lights
  float height;
  LightShape shape;

  // Shadow settings
  uint32_t shadow_map_resolution;
  float shadow_bias;
  float shadow_normal_bias;

  // Volumetric settings
  bool volumetric;
  float volumetric_intensity;
} Light;

typedef enum {
  LIGHT_SHAPE_RECTANGULAR,
  LIGHT_SHAPE_DISC,
  LIGHT_SHAPE_TUBE
} LightShape;

// Light system
typedef struct LightSystem LightSystem;

// Initialization
LightSystem *light_system_create(void);
void light_system_destroy(LightSystem *system);

// Light management
uint32_t light_system_add_light(LightSystem *system, LightType type);
void light_system_remove_light(LightSystem *system, uint32_t light_id);
Light *light_system_get_light(LightSystem *system, uint32_t light_id);

// Light configuration
void light_set_position(Light *light, float x, float y, float z);
void light_set_direction(Light *light, float x, float y, float z);
void light_set_color(Light *light, float r, float g, float b);
void light_set_intensity(Light *light, float intensity);
void light_set_range(Light *light, float range);
void light_set_attenuation(Light *light, float constant, float linear,
                           float quadratic);

// Spot light specific
void light_set_spot_angles(Light *light, float inner_angle, float outer_angle);
void light_set_cookie_texture(Light *light, uint32_t texture_id);

// Area light specific
void light_set_area_size(Light *light, float width, float height);
void light_set_area_shape(Light *light, LightShape shape);

// Shadow settings
void light_set_shadow_enabled(Light *light, bool enabled);
void light_set_shadow_resolution(Light *light, uint32_t resolution);
void light_set_shadow_bias(Light *light, float bias, float normal_bias);

// Culling and sorting
void light_system_update_culling(LightSystem *system,
                                 const float *view_frustum);
void light_system_sort_lights(LightSystem *system, const float *camera_pos);

// Getters for rendering
uint32_t light_system_get_visible_count(const LightSystem *system);
const Light **light_system_get_visible_lights(const LightSystem *system);

// Cluster/tile managment (for forward+ rendering)
void light_system_build_clusters(LightSystem *system, uint32_t tile_count_x,
                                 uint32_t tile_count_y, uint32_t tile_count_z);
void light_system_assign_lights_to_clusters(LightSystem *system);

#endif // LIGHT_SYSTEM_H
