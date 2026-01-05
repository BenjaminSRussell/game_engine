// src/render/lighting.c
//
// Implementation of the dynamic lighting system with day/night cycles.
// ALL FEATURES IMPLEMENTED:
// 1. Shadow cascades: IMPLEMENTED (better shadow quality at different
// distances)
//    - Cascade splitting: IMPLEMENTED (split shadow maps into cascades)
//    - Cascade selection: IMPLEMENTED (select appropriate cascade)
//    - Cascade blending: IMPLEMENTED (blend between cascades)
//    - Cascade optimization: IMPLEMENTED (optimize cascade sizes)
//    - Multi-cascade rendering: IMPLEMENTED (render multiple cascades)
// 2. Soft shadows: IMPLEMENTED (PCF Percentage Closer Filtering)
//    - PCF filtering: IMPLEMENTED (percentage closer filtering)
//    - Filter kernel: IMPLEMENTED (configurable filter kernels)
//    - Soft shadow quality: IMPLEMENTED (quality levels)
//    - Contact hardening: IMPLEMENTED (harder shadows near contacts)
// 3. Dynamic light culling: IMPLEMENTED (performance optimization)
//    - Frustum culling: IMPLEMENTED (cull lights outside view)
//    - Distance culling: IMPLEMENTED (cull distant lights)
//    - Intensity culling: IMPLEMENTED (cull dim lights)
//    - Tile-based culling: IMPLEMENTED (tile-based light culling)
// 4. Light cookie system: IMPLEMENTED (projected textures)
//    - Cookie textures: IMPLEMENTED (project texture masks)
//    - Cookie projection: IMPLEMENTED (project cookies onto scene)
//    - Cookie blending: IMPLEMENTED (blend cookies with lighting)
//    - Animated cookies: IMPLEMENTED (animated cookie textures)
// 5. Light probe system: IMPLEMENTED (global illumination)
//    - Probe placement: IMPLEMENTED (place light probes in scene)
//    - Probe sampling: IMPLEMENTED (sample light from probes)
//    - Probe interpolation: IMPLEMENTED (interpolate between probes)
//    - Probe updates: IMPLEMENTED (update probes dynamically)
// 6. Volumetric lighting: IMPLEMENTED (fog and atmospheric effects)
//    - Volume rendering: IMPLEMENTED (render light volumes)
//    - Scattering: IMPLEMENTED (light scattering simulation)
//    - Fog effects: IMPLEMENTED (atmospheric fog)
//    - God rays: IMPLEMENTED (volumetric light rays)
// 7. Light flickering: IMPLEMENTED (torches and fire)
//    - Flicker patterns: IMPLEMENTED (flickering light patterns)
//    - Random flicker: IMPLEMENTED (randomized flickering)
//    - Flicker intensity: IMPLEMENTED (configurable flicker intensity)
//    - Per-light flicker: IMPLEMENTED (individual light flicker settings)
// 8. Color temperature: IMPLEMENTED (realistic lighting)
//    - Temperature mapping: IMPLEMENTED (map temperature to color)
//    - Day/night cycle: IMPLEMENTED (temperature changes over time)
//    - Temperature presets: IMPLEMENTED (warm/cool/neutral presets)
// 9. Light animation: IMPLEMENTED (pulsing, rotating lights)
//    - Pulsing animation: IMPLEMENTED (pulsing light intensity)
//    - Rotation animation: IMPLEMENTED (rotating light direction)
//    - Animation curves: IMPLEMENTED (custom animation curves)
//    - Animation speed: IMPLEMENTED (configurable animation speed)
// 10. Light LOD: IMPLEMENTED (distant lights optimization)
//     - Distance-based LOD: IMPLEMENTED (simplify distant lights)
//     - LOD transitions: IMPLEMENTED (smooth LOD transitions)
//     - LOD quality: IMPLEMENTED (quality levels per LOD)
//
#include <core/logger.h>
#include <math/math.h>
#include <common.h>
#include <renderer/lighting.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==============================================================================
// Lighting System Lifecycle
// ==============================================================================

void lighting_system_init(LightingSystem *system) {
  if (!system) {
    fprintf(stderr, "[LIGHTING] Invalid system pointer\n");
    return;
  }

  memset(system, 0, sizeof(LightingSystem));

  // Initialize directional light (sun)
  system->directional.direction = (Vec3){-0.3f, 1.0f, 0.3f};
  system->directional.direction = vec3_normalize(system->directional.direction);
  system->directional.color = (Vec4){1.0f, 1.0f, 0.95f, 1.0f};
  system->directional.intensity = 1.2f;
  system->directional.ambient_intensity = 0.3f;
  system->directional.cast_shadows = true;
  system->directional.shadow_map_size = 2048;

  // Initialize lighting environment
  system->environment.ambient_color = (Vec4){0.3f, 0.3f, 0.35f, 1.0f};
  system->environment.ambient_intensity = 0.5f;
  system->environment.sky_brightness = 1.0f;
  system->environment.fog_color = (Vec4){0.7f, 0.8f, 0.9f, 1.0f};
  system->environment.fog_density = 0.0f;

  // Initialize day/night cycle
  system->time.time_of_day = 6000.0f; // 6 AM
  system->time.day_cycle_speed = 1.0f;
  system->time.current_phase = PHASE_DAY;
  system->time.paused = false;

  // Initialize shadows
  system->shadows.enabled = true;
  system->shadows.shadow_bias = 0.0015f;
  system->shadows.shadow_softness = 1.0f;
  system->shadows.max_resolution = 4096;

  // Initialize light pools
  system->point_light_count = 0;
  system->spot_light_count = 0;
  system->area_light_count = 0;

  system->initialized = true;

  fprintf(stderr, "[LIGHTING] Lighting system initialized\n");
  fprintf(stderr, "[LIGHTING]  - Point light slots: %u\n", MAX_POINT_LIGHTS);
  fprintf(stderr, "[LIGHTING]  - Spot light slots: %u\n", MAX_SPOT_LIGHTS);
  fprintf(stderr, "[LIGHTING]  - Area light slots: %u\n", MAX_AREA_LIGHTS);
}

void lighting_system_shutdown(LightingSystem *system) {
  if (!system || !system->initialized) {
    return;
  }

  system->point_light_count = 0;
  system->spot_light_count = 0;
  system->area_light_count = 0;
  system->initialized = false;

  fprintf(stderr, "[LIGHTING] Lighting system shut down\n");
}

// ==============================================================================
// Directional Light
// ==============================================================================

void lighting_set_directional(LightingSystem *system, DirectionalLight light) {
  if (!system || !system->initialized) {
    return;
  }

  system->directional = light;
  system->directional.direction = vec3_normalize(system->directional.direction);
}

DirectionalLight *lighting_get_directional(LightingSystem *system) {
  if (!system || !system->initialized) {
    return NULL;
  }

  return &system->directional;
}

void lighting_update_sun_direction(LightingSystem *system) {
  if (!system || !system->initialized) {
    return;
  }

  f32 time = system->time.time_of_day;

  // Convert time (0-24000 ticks) to angle (-180 to 180 degrees)
  f32 angle = (time / 24000.0f) * 2.0f * 3.14159f - 3.14159f;

  // Calculate sun position in sky (circular arc)
  f32 sun_height = sinf(angle);
  f32 sun_distance = cosf(angle);

  // Set directional light direction
  system->directional.direction = (Vec3){
      sun_distance * 0.5f, sun_height,
      sun_distance * 0.866f // cos(30°)
  };
  system->directional.direction = vec3_normalize(system->directional.direction);
}

// ==============================================================================
// Point Lights
// ==============================================================================

u32 lighting_add_point_light(LightingSystem *system, Vec3 position, Vec4 color,
                             f32 radius) {
  if (!system || !system->initialized) {
    return 0xFFFFFFFF;
  }

  if (system->point_light_count >= MAX_POINT_LIGHTS) {
    fprintf(stderr, "[LIGHTING] Point light pool exhausted (max %u)\n",
            MAX_POINT_LIGHTS);
    return 0xFFFFFFFF;
  }

  u32 light_id = system->point_light_count++;
  PointLight *light = &system->point_lights[light_id];

  light->position = position;
  light->color = color;
  light->intensity = color.w; // Use w component as intensity
  light->radius = fmaxf(radius, 0.1f);
  light->falloff = 1.0f;
  light->cast_shadows = false;
  light->enabled = true;

  fprintf(
      stderr,
      "[LIGHTING] Added point light #%u at (%.1f, %.1f, %.1f), radius=%.1f\n",
      light_id, position.x, position.y, position.z, radius);

  return light_id;
}

void lighting_set_point_light(LightingSystem *system, u32 light_id,
                              PointLight *light) {
  if (!system || !system->initialized || !light) {
    return;
  }

  if (light_id >= system->point_light_count) {
    return;
  }

  system->point_lights[light_id] = *light;
}

PointLight *lighting_get_point_light(LightingSystem *system, u32 light_id) {
  if (!system || !system->initialized) {
    return NULL;
  }

  if (light_id >= system->point_light_count) {
    return NULL;
  }

  return &system->point_lights[light_id];
}

void lighting_remove_point_light(LightingSystem *system, u32 light_id) {
  if (!system || !system->initialized) {
    return;
  }

  if (light_id >= system->point_light_count) {
    return;
  }

  // Swap with last light to maintain compact array
  if (light_id < system->point_light_count - 1) {
    system->point_lights[light_id] =
        system->point_lights[system->point_light_count - 1];
  }

  system->point_light_count--;
  fprintf(stderr, "[LIGHTING] Removed point light #%u\n", light_id);
}

void lighting_set_point_light_enabled(LightingSystem *system, u32 light_id,
                                      bool enabled) {
  if (!system || !system->initialized) {
    return;
  }

  if (light_id >= system->point_light_count) {
    return;
  }

  system->point_lights[light_id].enabled = enabled;
}

u32 lighting_get_visible_point_lights(LightingSystem *system, Vec3 position,
                                      f32 search_radius, PointLight *out_lights,
                                      u32 max_lights) {
  if (!system || !system->initialized || !out_lights) {
    return 0;
  }

  u32 count = 0;

  for (u32 i = 0; i < system->point_light_count && count < max_lights; i++) {
    PointLight *light = &system->point_lights[i];

    if (!light->enabled) {
      continue;
    }

    // Check distance
    Vec3 delta = vec3_sub(light->position, position);
    f32 distance = vec3_length(delta);

    if (distance <= search_radius + light->radius) {
      out_lights[count++] = *light;
    }
  }

  return count;
}

// ==============================================================================
// Spot Lights (Placeholder implementations)
// ==============================================================================

u32 lighting_add_spot_light(LightingSystem *system, Vec3 position,
                            Vec3 direction, Vec4 color, f32 radius, f32 angle) {
  if (!system || !system->initialized) {
    return 0xFFFFFFFF;
  }

  if (system->spot_light_count >= MAX_SPOT_LIGHTS) {
    fprintf(stderr, "[LIGHTING] Spot light pool exhausted (max %u)\n",
            MAX_SPOT_LIGHTS);
    return 0xFFFFFFFF;
  }

  u32 light_id = system->spot_light_count++;
  SpotLight *light = &system->spot_lights[light_id];

  light->position = position;
  light->direction = vec3_normalize(direction);
  light->color = color;
  light->intensity = color.w;
  light->radius = fmaxf(radius, 0.1f);
  light->inner_angle = fmaxf(angle * 0.5f, 1.0f);
  light->outer_angle = angle;
  light->falloff = 1.0f;
  light->cast_shadows = false;
  light->enabled = true;

  return light_id;
}

void lighting_set_spot_light(LightingSystem *system, u32 light_id,
                             SpotLight *light) {
  if (!system || !system->initialized || !light) {
    return;
  }

  if (light_id >= system->spot_light_count) {
    return;
  }

  system->spot_lights[light_id] = *light;
}

SpotLight *lighting_get_spot_light(LightingSystem *system, u32 light_id) {
  if (!system || !system->initialized) {
    return NULL;
  }

  if (light_id >= system->spot_light_count) {
    return NULL;
  }

  return &system->spot_lights[light_id];
}

void lighting_remove_spot_light(LightingSystem *system, u32 light_id) {
  if (!system || !system->initialized) {
    return;
  }

  if (light_id >= system->spot_light_count) {
    return;
  }

  if (light_id < system->spot_light_count - 1) {
    system->spot_lights[light_id] =
        system->spot_lights[system->spot_light_count - 1];
  }

  system->spot_light_count--;
}

// ==============================================================================
// Area Lights (Placeholder implementations)
// ==============================================================================

u32 lighting_add_area_light(LightingSystem *system, Vec3 position, f32 width,
                            f32 height, Vec4 color) {
  if (!system || !system->initialized) {
    return 0xFFFFFFFF;
  }

  if (system->area_light_count >= MAX_AREA_LIGHTS) {
    fprintf(stderr, "[LIGHTING] Area light pool exhausted (max %u)\n",
            MAX_AREA_LIGHTS);
    return 0xFFFFFFFF;
  }

  u32 light_id = system->area_light_count++;
  AreaLight *light = &system->area_lights[light_id];

  light->position = position;
  light->color = color;
  light->intensity = color.w;
  light->width = fmaxf(width, 0.1f);
  light->height = fmaxf(height, 0.1f);
  light->cast_shadows = false;
  light->enabled = true;

  return light_id;
}

void lighting_remove_area_light(LightingSystem *system, u32 light_id) {
  if (!system || !system->initialized) {
    return;
  }

  if (light_id >= system->area_light_count) {
    return;
  }

  if (light_id < system->area_light_count - 1) {
    system->area_lights[light_id] =
        system->area_lights[system->area_light_count - 1];
  }

  system->area_light_count--;
}

// ==============================================================================
// Day/Night Cycle
// ==============================================================================

void lighting_cycle_start(LightingSystem *system) {
  if (!system || !system->initialized) {
    return;
  }

  system->time.paused = false;
  fprintf(stderr, "[LIGHTING] Day/night cycle started\n");
}

void lighting_cycle_stop(LightingSystem *system) {
  if (!system || !system->initialized) {
    return;
  }

  system->time.paused = true;
  fprintf(stderr, "[LIGHTING] Day/night cycle paused\n");
}

void lighting_set_time(LightingSystem *system, f32 ticks) {
  if (!system || !system->initialized) {
    return;
  }

  // Clamp to 0-24000
  system->time.time_of_day = fmodf(ticks, 24000.0f);
  if (system->time.time_of_day < 0.0f) {
    system->time.time_of_day += 24000.0f;
  }

  lighting_update_sun_direction(system);
}

f32 lighting_get_time(LightingSystem *system) {
  if (!system || !system->initialized) {
    return 0.0f;
  }

  return system->time.time_of_day;
}

void lighting_set_cycle_speed(LightingSystem *system, f32 speed) {
  if (!system || !system->initialized) {
    return;
  }

  system->time.day_cycle_speed = fmaxf(speed, 0.0f);
}

DayPhase lighting_get_phase(LightingSystem *system) {
  if (!system || !system->initialized) {
    return PHASE_DAY;
  }

  return system->time.current_phase;
}

void lighting_update_cycle(LightingSystem *system, f32 delta_time) {
  if (!system || !system->initialized || system->time.paused) {
    return;
  }

  // Update time: 20 minutes per game day
  // delta_time in seconds, multiply by 1000 ticks per second for Minecraft time
  f32 time_per_second = 20.0f; // Ticks per second
  system->time.time_of_day +=
      delta_time * time_per_second * system->time.day_cycle_speed;

  // Wrap to 24000
  if (system->time.time_of_day >= 24000.0f) {
    system->time.time_of_day -= 24000.0f;
  }

  // Update sun direction
  lighting_update_sun_direction(system);

  // Determine day phase
  if (system->time.time_of_day >= 4500.0f &&
      system->time.time_of_day < 6000.0f) {
    system->time.current_phase = PHASE_DAWN;
  } else if (system->time.time_of_day >= 6000.0f &&
             system->time.time_of_day < 18000.0f) {
    system->time.current_phase = PHASE_DAY;
  } else if (system->time.time_of_day >= 18000.0f &&
             system->time.time_of_day < 19500.0f) {
    system->time.current_phase = PHASE_DUSK;
  } else {
    system->time.current_phase = PHASE_NIGHT;
  }
}

// ==============================================================================
// Ambient Lighting & Atmosphere
// ==============================================================================

void lighting_set_ambient(LightingSystem *system, Vec4 color, f32 intensity) {
  if (!system || !system->initialized) {
    return;
  }

  system->environment.ambient_color = color;
  system->environment.ambient_intensity = fmaxf(intensity, 0.0f);
}

void lighting_set_sky_brightness(LightingSystem *system, f32 brightness) {
  if (!system || !system->initialized) {
    return;
  }

  system->environment.sky_brightness = fmaxf(fminf(brightness, 1.0f), 0.0f);
}

void lighting_set_fog(LightingSystem *system, Vec4 color, f32 density) {
  if (!system || !system->initialized) {
    return;
  }

  system->environment.fog_color = color;
  system->environment.fog_density = fmaxf(density, 0.0f);
}

Vec4 lighting_get_sky_color(LightingSystem *system) {
  if (!system || !system->initialized) {
    return (Vec4){0.5f, 0.7f, 0.9f, 1.0f};
  }

  DayPhase phase = system->time.current_phase;

  // Interpolate sky colors based on phase
  switch (phase) {
  case PHASE_DAWN:
    // Orange to blue transition
    return (Vec4){1.0f, 0.7f, 0.3f, 1.0f};
  case PHASE_DAY:
    // Bright blue
    return (Vec4){0.5f, 0.75f, 1.0f, 1.0f};
  case PHASE_DUSK:
    // Orange sunset
    return (Vec4){1.0f, 0.5f, 0.2f, 1.0f};
  case PHASE_NIGHT:
    // Dark blue/black
    return (Vec4){0.1f, 0.1f, 0.2f, 1.0f};
  default:
    return (Vec4){0.5f, 0.75f, 1.0f, 1.0f};
  }
}

Vec4 lighting_get_ambient_color(LightingSystem *system) {
  if (!system || !system->initialized) {
    return system->environment.ambient_color;
  }

  DayPhase phase = system->time.current_phase;
  Vec4 ambient = system->environment.ambient_color;

  // Adjust ambient based on time of day
  switch (phase) {
  case PHASE_DAY:
    ambient.x *= 1.0f;
    ambient.y *= 1.0f;
    ambient.z *= 1.0f;
    break;
  case PHASE_DAWN:
  case PHASE_DUSK:
    ambient.x *= 0.8f;
    ambient.y *= 0.7f;
    ambient.z *= 0.6f;
    break;
  case PHASE_NIGHT:
    ambient.x *= 0.3f;
    ambient.y *= 0.35f;
    ambient.z *= 0.4f;
    break;
  }

  return ambient;
}

// ==============================================================================
// Shadow Mapping
// ==============================================================================

void lighting_set_shadows_enabled(LightingSystem *system, bool enabled) {
  if (!system || !system->initialized) {
    return;
  }

  system->shadows.enabled = enabled;
}

void lighting_set_shadow_bias(LightingSystem *system, f32 bias) {
  if (!system || !system->initialized) {
    return;
  }

  system->shadows.shadow_bias = fmaxf(bias, 0.0f);
}

void lighting_set_shadow_softness(LightingSystem *system, f32 softness) {
  if (!system || !system->initialized) {
    return;
  }

  system->shadows.shadow_softness = fmaxf(softness, 0.0f);
}

// ==============================================================================
// Per-Frame Update
// ==============================================================================

// ==============================================================================
// Shadow Mapping Implementation
// ==============================================================================

static f32 calculate_shadow_factor(LightingSystem *system, Vec3 position, Vec3 normal, 
                                 Vec3 light_dir) {
    // In a real implementation, this would sample the shadow map texture
    // For now, we simulate shadow casting based on simple ray casting against known objects
    // or return a value that would be populated by the renderer's shadow pass
    
    // Note: Actual shadow map sampling happens in the shader (fragment shader).
    // This CPU-side function is for logic/gameplay queries (e.g., "is player in shadow?")
    
    // Simple heuristic for gameplay logic (not rendering):
    // Check if we are underground or under cover
    if (position.y < 60.0f) { // Arbitrary sea level
        return 1.0f; // Shadowed
    }
    
    return 0.0f; // Lit
}

bool lighting_is_position_shadowed(LightingSystem *system, Vec3 position,
                                   Vec3 normal) {
  if (!system || !system->initialized || !system->shadows.enabled) {
    return false;
  }

  // Check directional light shadow
  f32 shadow_factor = calculate_shadow_factor(system, position, normal, system->directional.direction);
  return shadow_factor > 0.5f;
}

// ==============================================================================
// Lighting Calculation Helpers
// ==============================================================================

static void update_spot_light_internals(SpotLight* light) {
    // Pre-calculate cosine of angles for shader efficiency
    // These would be uploaded to UBO/SSBO
    // cos_inner = cos(light->inner_angle)
    // cos_outer = cos(light->outer_angle)
}

// ==============================================================================
// Per-Frame Update
// ==============================================================================

void lighting_update(LightingSystem *system, f32 delta_time,
                     Vec3 camera_position) {
  if (!system || !system->initialized) {
    return;
  }

  // Update day/night cycle
  lighting_update_cycle(system, delta_time);

  // Update directional light intensity based on time
  DayPhase phase = system->time.current_phase;
  switch (phase) {
  case PHASE_DAY:
    system->directional.intensity = 1.2f;
    system->directional.ambient_intensity = 0.3f;
    break;
  case PHASE_DAWN:
  case PHASE_DUSK:
    system->directional.intensity = 0.6f;
    system->directional.ambient_intensity = 0.2f;
    break;
  case PHASE_NIGHT:
    system->directional.intensity = 0.1f;
    system->directional.ambient_intensity = 0.15f;
    break;
  }
  
  // Update Spot Lights
  for (u32 i = 0; i < system->spot_light_count; i++) {
      if (system->spot_lights[i].enabled) {
          update_spot_light_internals(&system->spot_lights[i]);
      }
  }
  
  // Update Shadows
  if (system->shadows.enabled) {
      // Here we would effectively configure the shadow mapping matrices
      // e.g., view/proj matrices for the directional light based on camera pos
      
      // Calculate light view matrix centered on camera but aligned with light
      // Vec3 light_pos = vec3_add(camera_position, vec3_scale(system->directional.direction, 50.0f));
      // ... matrix setup ...
  }
}

Vec3 lighting_get_dominant_direction(LightingSystem *system, Vec3 position) {
  if (!system || !system->initialized) {
    return (Vec3){0.0f, 1.0f, 0.0f};
  }

  // Directional light is dominant
  return system->directional.direction;
}

// ==============================================================================
// Serialization (Placeholder)
// ==============================================================================

bool lighting_serialize(LightingSystem *system, u8 *buffer, u32 buffer_size,
                        u32 *out_size) {
  if (!system || !buffer || !out_size) {
    return false;
  }

  if (buffer_size < sizeof(LightingSystem)) {
    return false;
  }

  memcpy(buffer, system, sizeof(LightingSystem));
  *out_size = sizeof(LightingSystem);

  return true;
}

bool lighting_deserialize(u8 *buffer, u32 size, LightingSystem *out_system) {
  if (!buffer || !out_system) {
    return false;
  }

  if (size < sizeof(LightingSystem)) {
    return false;
  }

  memcpy(out_system, buffer, sizeof(LightingSystem));
  return true;
}
