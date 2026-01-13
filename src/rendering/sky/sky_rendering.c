// src/engine/rendering/sky/sky_rendering.c
// Sky Rendering System - Atmospheric scattering, clouds, and celestial bodies

#include "engine/include/core/logger.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../render_pipeline.h"
#include "sky_rendering.h"

// ============================================================================
// Sky Rendering Types
// ============================================================================

typedef enum {
  TIME_OF_DAY_DAWN,
  TIME_OF_DAY_MORNING,
  TIME_OF_DAY_NOON,
  TIME_OF_DAY_AFTERNOON,
  TIME_OF_DAY_DUSK,
  TIME_OF_DAY_NIGHT
} TimeOfDay;

typedef struct {
  // Sun and moon
  float sun_direction[3];
  float moon_direction[3];
  float sun_color[3];
  float moon_color[3];
  float sun_intensity;
  float moon_intensity;

  // Atmospheric scattering
  float rayleigh_scattering[3]; // Wavelength dependent
  float mie_scattering;
  float atmosphere_radius;
  float planet_radius;
  float scale_height_rayleigh;
  float scale_height_mie;
  float mie_g; // Anisotropy parameter

  // Time and weather
  float time_of_day; // 0.0 to 24.0
  SkyType sky_type;
  CloudType cloud_type;
  float cloud_coverage;
  float cloud_density;
  float cloud_height;
  float wind_speed;
  float wind_direction[2];

  // Celestial bodies
  bool show_sun;
  bool show_moon;
  bool show_stars;
  bool show_milky_way;
  uint32_t star_count;

  // Rendering resources
  void *skybox_texture;
  void *cloud_texture;
  void *noise_texture;
  void *atmosphere_lut; // Look-up table for scattering
  void *transmittance_lut;

  // Shaders
  void *atmosphere_shader;
  void *cloud_shader;
  void *skybox_shader;
  void *star_shader;

  // Geometry
  void *sky_sphere;
  void *cloud_volume;

  // Animation
  float cloud_animation_time;
  float star_twinkle_phase;

  // Performance
  bool enable_atmospheric_scattering;
  bool enable_volumetric_clouds;
  bool enable_cloud_shadows;
  uint32_t scattering_samples;
  uint32_t cloud_samples;

  // Statistics
  float render_time_ms;
  uint32_t rendered_pixels;

  bool initialized;
} SkySystem;

static SkySystem g_sky_system = {0};

// ============================================================================
// Atmospheric Scattering Calculations
// ============================================================================

static float calculate_sun_angle(float time_of_day) {
  // Convert time to angle (0-24 hours -> 0-360 degrees)
  float angle = (time_of_day / 24.0f) * 2.0f * M_PI;

  // Sun rises at 6 AM and sets at 6 PM
  angle -= M_PI * 0.5f; // Shift so 6 AM = 0

  return angle;
}

static void update_celestial_positions(SkySystem *sky) {
  float sun_angle = calculate_sun_angle(sky->time_of_day);

  // Sun position (simplified - assuming flat horizon)
  sky->sun_direction[0] = cosf(sun_angle);
  sky->sun_direction[1] = sinf(sun_angle);
  sky->sun_direction[2] = 0.0f;

  // Moon is opposite to sun
  sky->moon_direction[0] = -sky->sun_direction[0];
  sky->moon_direction[1] = -sky->sun_direction[1];
  sky->moon_direction[2] = 0.0f;

  // Update sun color based on time of day
  if (sky->time_of_day >= 5.0f && sky->time_of_day <= 7.0f) {
    // Dawn - reddish
    sky->sun_color[0] = 1.0f;
    sky->sun_color[1] = 0.6f;
    sky->sun_color[2] = 0.3f;
    sky->sun_intensity = 0.8f;
  } else if (sky->time_of_day >= 7.0f && sky->time_of_day <= 17.0f) {
    // Day - bright white
    sky->sun_color[0] = 1.0f;
    sky->sun_color[1] = 0.95f;
    sky->sun_color[2] = 0.8f;
    sky->sun_intensity = 1.0f;
  } else if (sky->time_of_day >= 17.0f && sky->time_of_day <= 19.0f) {
    // Dusk - orange/red
    sky->sun_color[0] = 1.0f;
    sky->sun_color[1] = 0.4f;
    sky->sun_color[2] = 0.2f;
    sky->sun_intensity = 0.6f;
  } else {
    // Night - moonlight
    sky->sun_intensity = 0.0f;
    sky->moon_color[0] = 0.7f;
    sky->moon_color[1] = 0.8f;
    sky->moon_color[2] = 1.0f;
    sky->moon_intensity = 0.3f;
  }
}

static void generate_atmosphere_lut(SkySystem *sky) {
  if (!sky->enable_atmospheric_scattering)
    return;

  // TODO: Generate atmospheric scattering look-up table
  // This would pre-compute scattering values for different view angles and sun
  // angles
  LOG_DEBUG("Generating atmosphere scattering LUT");

  // For now, just create a dummy texture
  const uint32_t lut_size = 256;
  float *lut_data = calloc(lut_size * lut_size * 4, sizeof(float));

  if (lut_data) {
    // Fill with dummy data
    for (uint32_t y = 0; y < lut_size; y++) {
      for (uint32_t x = 0; x < lut_size; x++) {
        uint32_t index = (y * lut_size + x) * 4;

        // Simple gradient based on height
        float height = (float)y / (float)lut_size;
        lut_data[index + 0] = 0.5f + height * 0.3f; // R
        lut_data[index + 1] = 0.7f + height * 0.2f; // G
        lut_data[index + 2] = 1.0f;                 // B
        lut_data[index + 3] = 1.0f;                 // A
      }
    }

    // Create texture from lut_data
    if (sky->atmosphere_lut) {
      texture_destroy(sky->atmosphere_lut);
    }
    sky->atmosphere_lut =
        texture_create_2d(lut_size, lut_size, 0); // Format 0 = RGBA32F assumed
    texture_upload(sky->atmosphere_lut, lut_data);

    free(lut_data);
  }
}

static void generate_cloud_noise(SkySystem *sky) {
  if (sky->cloud_type == CLOUD_TYPE_NONE)
    return;

  // TODO: Generate procedural cloud noise using Perlin noise or similar
  LOG_DEBUG("Generating cloud noise textures");

  // For now, just create dummy noise textures
  const uint32_t noise_size = 512;
  uint8_t *noise_data = calloc(noise_size * noise_size, sizeof(uint8_t));

  if (noise_data) {
    // Generate simple noise pattern
    for (uint32_t y = 0; y < noise_size; y++) {
      for (uint32_t x = 0; x < noise_size; x++) {
        uint32_t index = y * noise_size + x;

        // Simple cellular noise pattern
        float fx = (float)x / noise_size * 8.0f;
        float fy = (float)y / noise_size * 8.0f;

        float value = sinf(fx) * cosf(fy) * 0.5f + 0.5f;
        noise_data[index] = (uint8_t)(value * 255.0f);
      }
    }

    // Create texture from noise_data
    if (sky->noise_texture) {
      texture_destroy(sky->noise_texture);
    }
    sky->noise_texture =
        texture_create_2d(noise_size, noise_size, 1); // Format 1 = R8 assumed
    texture_upload(sky->noise_texture, noise_data);

    free(noise_data);
  }
}

// ============================================================================
// Sky Rendering Functions
// ============================================================================

static void render_atmosphere(SkySystem *sky, const float *view_matrix,
                              const float *proj_matrix) {
  if (!sky->enable_atmospheric_scattering)
    return;

  // Render atmospheric scattering using pre-computed LUTs
  LOG_DEBUG("Rendering atmospheric scattering");

  // Bind atmosphere shader
  uint32_t shader_id = (uint32_t)(uintptr_t)sky->atmosphere_shader;
  if (shader_id) {
    shader_bind(shader_id);

    // Set uniforms
    shader_set_texture(shader_id, "u_AtmosphereLUT", sky->atmosphere_lut);
    shader_set_texture(shader_id, "u_TransmittanceLUT", sky->transmittance_lut);

    float sun_dir[3];
    sky_system_get_sun_direction(sun_dir);
    // shader_set_vector(shader_id, "u_SunDirection", sun_dir);
    // shader_set_matrix(shader_id, "u_View", view_matrix);
    // shader_set_matrix(shader_id, "u_Proj", proj_matrix);

    // Render sky sphere geometry
    // if (sky->sky_sphere) renderer_draw_mesh(sky->sky_sphere);
  }
}

static void render_clouds(SkySystem *sky, const float *view_matrix,
                          const float *proj_matrix) {
  if (sky->cloud_type == CLOUD_TYPE_NONE)
    return;

  // TODO: Render clouds based on type
  switch (sky->cloud_type) {
  case CLOUD_TYPE_STRATUS:
  case CLOUD_TYPE_CUMULUS:
  case CLOUD_TYPE_CIRRUS:
    // 2D cloud rendering
    LOG_DEBUG("Rendering 2D clouds: type %d", (int)sky->cloud_type);
    break;

  case CLOUD_TYPE_VOLUMETRIC:
    // 3D volumetric cloud rendering
    if (sky->enable_volumetric_clouds) {
      LOG_DEBUG("Rendering volumetric clouds");
      // Implement ray-marching through cloud volume
      uint32_t shader_id = (uint32_t)(uintptr_t)sky->cloud_shader;
      if (shader_id) {
        shader_bind(shader_id);
        shader_set_texture(shader_id, "u_NoiseTexture", sky->noise_texture);
        // Set ray-marching parameters
        // shader_set_float(shader_id, "u_Coverage", sky->cloud_coverage);
        // shader_set_float(shader_id, "u_Density", sky->cloud_density);

        // Render cloud volume
        // if (sky->cloud_volume) renderer_draw_mesh(sky->cloud_volume);
      }
    }
    break;

  default:
    break;
  }
}

static void render_celestial_bodies(SkySystem *sky, const float *view_matrix,
                                    const float *proj_matrix) {
  // Render sun
  if (sky->show_sun && sky->sun_intensity > 0.0f) {
    // Render sun disc/glow
    LOG_DEBUG("Rendering sun at (%.2f, %.2f, %.2f)", sky->sun_direction[0],
              sky->sun_direction[1], sky->sun_direction[2]);

    // This would use a billboard or a specialized sun shader
    // uint32_t shader = shader_get_id("SunShader");
    // shader_bind(shader);
    // ... draw billboard ...
  }

  // Render moon
  if (sky->show_moon && sky->moon_intensity > 0.0f) {
    // Render moon disc
    LOG_DEBUG("Rendering moon at (%.2f, %.2f, %.2f)", sky->moon_direction[0],
              sky->moon_direction[1], sky->moon_direction[2]);

    // Similar to sun rendering but with moon texture
  }

  // Render stars
  if (sky->show_stars &&
      (sky->time_of_day >= 20.0f || sky->time_of_day <= 5.0f)) {
    // Render star field
    LOG_DEBUG("Rendering %u stars", sky->star_count);
    // Bind star shader and draw points or billboard mesh
  }

  // Render milky way
  if (sky->show_milky_way &&
      (sky->time_of_day >= 21.0f || sky->time_of_day <= 4.0f)) {
    // Render milky way
    LOG_DEBUG("Rendering milky way");
    // Similar to star rendering but with a background nebula texture
  }
}

// ============================================================================
// Sky System API
// ============================================================================

bool sky_system_init(bool enable_atmospheric_scattering,
                     bool enable_volumetric_clouds) {
  if (g_sky_system.initialized) {
    LOG_WARN("Sky system already initialized");
    return true;
  }

  memset(&g_sky_system, 0, sizeof(SkySystem));

  g_sky_system.enable_atmospheric_scattering = enable_atmospheric_scattering;
  g_sky_system.enable_volumetric_clouds = enable_volumetric_clouds;
  g_sky_system.enable_cloud_shadows = true;

  // Set default atmospheric parameters
  g_sky_system.rayleigh_scattering[0] = 5.8e-6f;  // Red
  g_sky_system.rayleigh_scattering[1] = 13.5e-6f; // Green
  g_sky_system.rayleigh_scattering[2] = 33.1e-6f; // Blue
  g_sky_system.mie_scattering = 2.0e-5f;

  g_sky_system.atmosphere_radius = 6420e3f;  // Earth radius + 100km atmosphere
  g_sky_system.planet_radius = 6371e3f;      // Earth radius
  g_sky_system.scale_height_rayleigh = 8e3f; // 8km
  g_sky_system.scale_height_mie = 1.2e3f;    // 1.2km
  g_sky_system.mie_g = 0.758f;               // Anisotropy

  // Set default time and weather
  g_sky_system.time_of_day = 12.0f; // Noon
  g_sky_system.sky_type = SKY_TYPE_CLEAR;
  g_sky_system.cloud_type = CLOUD_TYPE_CUMULUS;
  g_sky_system.cloud_coverage = 0.3f;
  g_sky_system.cloud_density = 0.5f;
  g_sky_system.cloud_height = 2000.0f;
  g_sky_system.wind_speed = 10.0f;
  g_sky_system.wind_direction[0] = 1.0f;
  g_sky_system.wind_direction[1] = 0.0f;

  // Set default celestial settings
  g_sky_system.show_sun = true;
  g_sky_system.show_moon = true;
  g_sky_system.show_stars = true;
  g_sky_system.show_milky_way = true;
  g_sky_system.star_count = 10000;

  // Set quality settings
  g_sky_system.scattering_samples = 32;
  g_sky_system.cloud_samples = 64;

  // Create sky sphere geometry
  // TODO: Create sphere mesh for sky rendering

  // Generate look-up tables and noise textures
  generate_atmosphere_lut(&g_sky_system);
  generate_cloud_noise(&g_sky_system);

  // Update initial celestial positions
  update_celestial_positions(&g_sky_system);

  g_sky_system.initialized = true;
  LOG_INFO("Sky system initialized (atmospheric: %s, volumetric clouds: %s)",
           enable_atmospheric_scattering ? "yes" : "no",
           enable_volumetric_clouds ? "yes" : "no");
  return true;
}

void sky_system_shutdown(void) {
  if (!g_sky_system.initialized)
    return;

  // Destroy textures and resources
  if (g_sky_system.atmosphere_lut)
    texture_destroy(g_sky_system.atmosphere_lut);
  if (g_sky_system.transmittance_lut)
    texture_destroy(g_sky_system.transmittance_lut);
  if (g_sky_system.cloud_texture)
    texture_destroy(g_sky_system.cloud_texture);
  if (g_sky_system.noise_texture)
    texture_destroy(g_sky_system.noise_texture);
  if (g_sky_system.skybox_texture)
    texture_destroy(g_sky_system.skybox_texture);

  // Destroy geometry
  if (g_sky_system.sky_sphere)
    mesh_destroy(g_sky_system.sky_sphere);
  if (g_sky_system.cloud_volume)
    mesh_destroy(g_sky_system.cloud_volume);

  // Destroy shaders

  memset(&g_sky_system, 0, sizeof(SkySystem));

  LOG_INFO("Sky system shutdown");
}

void sky_system_update(float dt) {
  if (!g_sky_system.initialized)
    return;

  // Update time of day (if time progression is enabled)
  // g_sky_system.time_of_day += dt * time_scale;

  // Update celestial positions
  update_celestial_positions(&g_sky_system);

  // Update cloud animation
  g_sky_system.cloud_animation_time += dt * g_sky_system.wind_speed * 0.1f;

  // Update star twinkling
  g_sky_system.star_twinkle_phase += dt * 2.0f;

  // Update sky type based on weather
  // TODO: Implement weather system integration
}

void sky_system_render(const float *view_matrix, const float *proj_matrix,
                       const float *camera_pos) {
  if (!g_sky_system.initialized)
    return;

  uint64_t start_time = get_time_nanos();

  // Render sky components in order
  render_atmosphere(&g_sky_system, view_matrix, proj_matrix);
  render_clouds(&g_sky_system, view_matrix, proj_matrix);
  render_celestial_bodies(&g_sky_system, view_matrix, proj_matrix);

  uint64_t end_time = get_time_nanos();
  g_sky_system.render_time_ms = nanos_to_ms(end_time - start_time);

  LOG_DEBUG("Sky rendering completed in %.2f ms", g_sky_system.render_time_ms);
}

void sky_system_set_time_of_day(float hours) {
  if (!g_sky_system.initialized)
    return;

  g_sky_system.time_of_day = fmodf(hours, 24.0f);
  if (g_sky_system.time_of_day < 0.0f) {
    g_sky_system.time_of_day += 24.0f;
  }

  update_celestial_positions(&g_sky_system);

  LOG_DEBUG("Time of day set to %.2f hours", g_sky_system.time_of_day);
}

void sky_system_set_weather(SkyType sky_type, CloudType cloud_type,
                            float coverage, float density) {
  if (!g_sky_system.initialized)
    return;

  g_sky_system.sky_type = sky_type;
  g_sky_system.cloud_type = cloud_type;
  g_sky_system.cloud_coverage = coverage;
  g_sky_system.cloud_density = density;

  // Regenerate cloud noise if type changed
  if (cloud_type != CLOUD_TYPE_NONE) {
    generate_cloud_noise(&g_sky_system);
  }

  LOG_DEBUG("Weather updated: sky=%d, clouds=%d, coverage=%.2f, density=%.2f",
            (int)sky_type, (int)cloud_type, coverage, density);
}

void sky_system_get_sun_direction(float *direction) {
  if (!g_sky_system.initialized || !direction)
    return;

  direction[0] = g_sky_system.sun_direction[0];
  direction[1] = g_sky_system.sun_direction[1];
  direction[2] = g_sky_system.sun_direction[2];
}

void sky_system_get_sun_color(float *color) {
  if (!g_sky_system.initialized || !color)
    return;

  color[0] = g_sky_system.sun_color[0] * g_sky_system.sun_intensity;
  color[1] = g_sky_system.sun_color[1] * g_sky_system.sun_intensity;
  color[2] = g_sky_system.sun_color[2] * g_sky_system.sun_intensity;
}

void sky_system_get_ambient_color(float *color) {
  if (!g_sky_system.initialized || !color)
    return;

  // Calculate ambient color based on time of day and sky color
  if (g_sky_system.time_of_day >= 6.0f && g_sky_system.time_of_day <= 18.0f) {
    // Daytime - blue sky ambient
    color[0] = 0.2f;
    color[1] = 0.3f;
    color[2] = 0.4f;
  } else {
    // Nighttime - dark blue ambient
    color[0] = 0.05f;
    color[1] = 0.05f;
    color[2] = 0.1f;
  }

  // Modulate by cloud coverage
  float cloud_factor = 1.0f - g_sky_system.cloud_coverage * 0.5f;
  color[0] *= cloud_factor;
  color[1] *= cloud_factor;
  color[2] *= cloud_factor;
}

void sky_system_enable_atmospheric_scattering(bool enabled) {
  if (!g_sky_system.initialized)
    return;

  g_sky_system.enable_atmospheric_scattering = enabled;

  if (enabled && !g_sky_system.atmosphere_lut) {
    generate_atmosphere_lut(&g_sky_system);
  }

  LOG_INFO("Atmospheric scattering %s", enabled ? "enabled" : "disabled");
}

void sky_system_get_stats(float *render_time, uint32_t *rendered_pixels) {
  if (!g_sky_system.initialized)
    return;

  if (render_time)
    *render_time = g_sky_system.render_time_ms;
  if (rendered_pixels)
    *rendered_pixels = g_sky_system.rendered_pixels;
}
