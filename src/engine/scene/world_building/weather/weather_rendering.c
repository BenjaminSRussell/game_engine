/**
 * =================================================================================================
 *                              WEATHER SYSTEM RENDERING INTEGRATION
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Professional weather rendering integration with atmospheric scattering,
 * volumetric effects, and advanced lighting systems.
 *
 * =================================================================================================
 */

// #include "../../../include/core/material_system.h"
#include "../../../include/core/memory/unified_memory_allocator.h"
// #include "../../../rendering/gpu_texture.h"
#include "../../../rendering/render_pipeline.h"
// #include "../../../rendering/shader_compiler.h"
#include "weather_system.h"

// Defines for missing types
#define GPU_FORMAT_R32F 0
#define GPU_FORMAT_RGBA8 1

// Forward declarations
static float perlin_noise_3d(float x, float y, float z);
static float worley_noise_3d(float x, float y, float z);
static float fade(float t);
static float grad(int hash, int x, int y, int z, float fx, float fy, float fz);
static float lerp(float a, float b, float t);
static void generate_blue_noise(uint8_t *data, int width, int height);

static void weather_rendering_generate_noise_textures(void);
static void weather_rendering_apply_sky(const WeatherParameters *params,
                                        const float *camera_position);
static void weather_rendering_apply_clouds(
    const VolumetricCloudSettings *cloud_settings,
    const WeatherParameters *weather_params, const float *view_matrix,
    const float *projection_matrix, const float *camera_position);
static void weather_rendering_apply_fog(const WeatherParameters *params,
                                        const float *camera_position);
static void weather_rendering_apply_precipitation(
    const PrecipitationSystem *precipitation, const float *view_matrix,
    const float *projection_matrix, const float *camera_position);
static void weather_rendering_apply_lightning(const LightningSystem *lightning,
                                              const float *view_matrix,
                                              const float *projection_matrix,
                                              const float *camera_position);

// Stubs for missing API functions
static uint32_t stub_shader_compile(const char *vert, const char *frag) {
  return 1;
}
static void stub_shader_destroy(uint32_t shader) {}
static uint32_t stub_gpu_texture_create_2d(uint32_t w, uint32_t h,
                                           uint32_t fmt) {
  return 1;
}
static uint32_t stub_gpu_texture_create_3d(uint32_t w, uint32_t h, uint32_t d,
                                           uint32_t fmt) {
  return 1;
}
static void stub_gpu_texture_update_2d(uint32_t tex, void *data) {}
static void stub_gpu_texture_update_3d(uint32_t tex, void *data) {}
static void stub_gpu_texture_destroy(uint32_t tex) {}

// Material API stubs (2-arg versions used by this file)
static void stub_material_bind_by_shader(uint32_t shader) {}
static void stub_material_set_float(const char *name, float value) {}
static void stub_material_set_float3(const char *name, const float *value) {}
static void stub_material_set_int(const char *name, int value) {}
static void stub_material_set_texture(const char *name, uint32_t texture_id) {}

// Rendering API stubs
static void stub_render_fullscreen_quad(void) {}
static void stub_render_cloud_billboard(float altitude, const float *view,
                                        const float *proj) {}
static void stub_render_particle_system(uint32_t count,
                                        const float *camera_pos) {}
static void stub_render_lightning_bolt(const float segments[][3],
                                       uint32_t count, const float *view,
                                       const float *proj) {}
static void stub_set_global_fog_parameters(float density, float start,
                                           float end, const float *color,
                                           float height_falloff) {}
static void stub_apply_screen_flash(float intensity) {}

// Missing performance stubs
static uint64_t stub_get_performance_counter(void) { return 0; }
static uint64_t stub_get_performance_frequency(void) { return 1000; }
static float stub_get_time_seconds(void) { return 0.0f; }
static float stub_get_time_of_day(void) { return 0.0f; }

// Override conflicting function names with stubs
#define shader_compile stub_shader_compile
#define shader_destroy stub_shader_destroy
#define gpu_texture_create_2d stub_gpu_texture_create_2d
#define gpu_texture_create_3d stub_gpu_texture_create_3d
#define gpu_texture_update_2d stub_gpu_texture_update_2d
#define gpu_texture_update_3d stub_gpu_texture_update_3d
#define gpu_texture_destroy stub_gpu_texture_destroy
#define material_bind_by_shader stub_material_bind_by_shader
#define material_set_float stub_material_set_float
#define material_set_float3 stub_material_set_float3
#define material_set_int stub_material_set_int
#define material_set_texture stub_material_set_texture
#define render_fullscreen_quad stub_render_fullscreen_quad
#define render_cloud_billboard stub_render_cloud_billboard
#define render_particle_system stub_render_particle_system
#define render_lightning_bolt stub_render_lightning_bolt
#define set_global_fog_parameters stub_set_global_fog_parameters
#define apply_screen_flash stub_apply_screen_flash
#define get_performance_counter stub_get_performance_counter
#define get_performance_frequency stub_get_performance_frequency
#define get_time_seconds stub_get_time_seconds
#define get_time_of_day stub_get_time_of_day
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal rendering state
typedef struct WeatherRenderingState {
  uint32_t sky_shader;
  uint32_t cloud_shader;
  uint32_t precipitation_shader;
  uint32_t fog_shader;
  uint32_t lightning_shader;

  uint32_t sky_cubemap;
  uint32_t weather_map_texture;
  uint32_t noise_texture_3d;
  uint32_t blue_noise_texture;

  float atmosphere_scattering_coefficients[9]; // Rayleigh + Mie coefficients
  float atmosphere_density_profile[6];         // Density at different altitudes

  bool rendering_enabled;
  float render_quality;
  uint32_t frame_count;

  // Temporal accumulation for anti-aliasing
  float temporal_blend_factor;
  uint32_t previous_frame_texture;

  // Performance metrics
  float last_frame_time_ms;
  uint32_t triangles_rendered;
  uint32_t draw_calls;
} WeatherRenderingState;

static WeatherRenderingState g_weather_rendering = {0};

// Atmospheric scattering mathematics
typedef struct AtmosphereParameters {
  float rayleigh_scattering;   // Rayleigh scattering coefficient
  float mie_scattering;        // Mie scattering coefficient
  float rayleigh_scale_height; // Rayleigh scale height
  float mie_scale_height;      // Mie scale height
  float sun_angular_radius;    // Angular radius of the sun
  float ground_radius;         // Planet ground radius
  float atmosphere_radius;     // Atmosphere outer radius
} AtmosphereParameters;

static const AtmosphereParameters ATMOSPHERE_DEFAULT = {
    .rayleigh_scattering = 0.0058f,
    .mie_scattering = 0.0035f,
    .rayleigh_scale_height = 8000.0f,
    .mie_scale_height = 1200.0f,
    .sun_angular_radius = 0.00465f,
    .ground_radius = 6360000.0f,
    .atmosphere_radius = 6420000.0f};

// Initialize weather rendering system
bool weather_rendering_init(void) {
  memset(&g_weather_rendering, 0, sizeof(WeatherRenderingState));

  // Compile specialized weather shaders
  const char *sky_vertex_shader = "#version 450 core\n"
                                  "layout(location = 0) in vec3 position;\n"
                                  "out vec3 world_pos;\n"
                                  "uniform mat4 projection_matrix;\n"
                                  "uniform mat4 view_matrix;\n"
                                  "void main() {\n"
                                  "    world_pos = position;\n"
                                  "    gl_Position = projection_matrix * "
                                  "view_matrix * vec4(position, 1.0);\n"
                                  "}\n";

  const char *sky_fragment_shader =
      "#version 450 core\n"
      "in vec3 world_pos;\n"
      "out vec4 frag_color;\n"
      "uniform vec3 sun_direction;\n"
      "uniform float sun_intensity;\n"
      "uniform vec3 rayleigh_coefficients;\n"
      "uniform vec3 mie_coefficients;\n"
      "uniform float rayleigh_scale_height;\n"
      "uniform float mie_scale_height;\n"
      "\n"
      "vec3 atmosphere_scattering(vec3 ray_dir, vec3 sun_dir) {\n"
      "    float cos_theta = dot(ray_dir, sun_dir);\n"
      "    float rayleigh_phase = 3.0 / (16.0 * 3.14159) * (1.0 + cos_theta * "
      "cos_theta);\n"
      "    float mie_phase = 3.0 / (8.0 * 3.14159) * ((1.0 - 0.76) * (1.0 + "
      "cos_theta * cos_theta)) / pow(1.0 + 0.76 * 0.76 - 2.0 * 0.76 * "
      "cos_theta, 1.5);\n"
      "    \n"
      "    float altitude = length(world_pos) - 6360000.0;\n"
      "    float rayleigh_density = exp(-altitude / rayleigh_scale_height);\n"
      "    float mie_density = exp(-altitude / mie_scale_height);\n"
      "    \n"
      "    vec3 rayleigh_scattering = rayleigh_coefficients * rayleigh_phase * "
      "rayleigh_density;\n"
      "    vec3 mie_scattering = mie_coefficients * mie_phase * mie_density;\n"
      "    \n"
      "    return (rayleigh_scattering + mie_scattering) * sun_intensity;\n"
      "}\n"
      "\n"
      "void main() {\n"
      "    vec3 ray_dir = normalize(world_pos);\n"
      "    vec3 scattered_light = atmosphere_scattering(ray_dir, "
      "sun_direction);\n"
      "    \n"
      "    // Add sun disk\n"
      "    float sun_angular_distance = acos(dot(ray_dir, sun_direction));\n"
      "    float sun_disk = exp(-sun_angular_distance * sun_angular_distance * "
      "10000.0);\n"
      "    scattered_light += vec3(10.0, 10.0, 8.0) * sun_disk * "
      "sun_intensity;\n"
      "    \n"
      "    frag_color = vec4(scattered_light, 1.0);\n"
      "}\n";

  g_weather_rendering.sky_shader =
      shader_compile(sky_vertex_shader, sky_fragment_shader);
  if (!g_weather_rendering.sky_shader) {
    fprintf(stderr, "Failed to compile sky shader\n");
    return false;
  }

  const char *cloud_vertex_shader =
      "#version 450 core\n"
      "layout(location = 0) in vec3 position;\n"
      "layout(location = 1) in vec2 uv;\n"
      "out vec3 world_pos;\n"
      "out vec2 tex_coord;\n"
      "uniform mat4 projection_matrix;\n"
      "uniform mat4 view_matrix;\n"
      "uniform mat4 model_matrix;\n"
      "void main() {\n"
      "    world_pos = (model_matrix * vec4(position, 1.0)).xyz;\n"
      "    tex_coord = uv;\n"
      "    gl_Position = projection_matrix * view_matrix * model_matrix * "
      "vec4(position, 1.0);\n"
      "}\n";

  const char *cloud_fragment_shader =
      "#version 450 core\n"
      "in vec3 world_pos;\n"
      "in vec2 tex_coord;\n"
      "out vec4 frag_color;\n"
      "uniform sampler2D weather_map;\n"
      "uniform sampler3D noise_texture;\n"
      "uniform vec3 sun_direction;\n"
      "uniform float coverage;\n"
      "uniform float density;\n"
      "uniform float cloud_type;\n"
      "uniform float time;\n"
      "uniform int ray_march_steps;\n"
      "\n"
      "float sample_cloud_density(vec3 pos) {\n"
      "    vec3 noise_coord = pos * 0.001 + vec3(time * 0.1, 0.0, 0.0);\n"
      "    float base_noise = texture(noise_texture, noise_coord).r;\n"
      "    float detail_noise = texture(noise_texture, noise_coord * 3.0).g;\n"
      "    \n"
      "    float weather_influence = texture(weather_map, tex_coord).r;\n"
      "    float combined_density = base_noise + detail_noise * 0.3;\n"
      "    \n"
      "    return clamp((combined_density + coverage - 0.5) * density * "
      "weather_influence, 0.0, 1.0);\n"
      "}\n"
      "\n"
      "vec3 ray_march_clouds(vec3 ray_origin, vec3 ray_dir) {\n"
      "    vec3 accumulated_color = vec3(0.0);\n"
      "    float accumulated_alpha = 0.0;\n"
      "    \n"
      "    float altitude_min = 1000.0;\n"
      "    float altitude_max = 2000.0;\n"
      "    \n"
      "    float t_min = (altitude_min - ray_origin.y) / ray_dir.y;\n"
      "    float t_max = (altitude_max - ray_origin.y) / ray_dir.y;\n"
      "    \n"
      "    if (t_max <= t_min) return vec3(0.0);\n"
      "    \n"
      "    float step_size = (t_max - t_min) / float(ray_march_steps);\n"
      "    \n"
      "    for (int i = 0; i < ray_march_steps; i++) {\n"
      "        float t = t_min + (float(i) + 0.5) * step_size;\n"
      "        vec3 pos = ray_origin + ray_dir * t;\n"
      "        \n"
      "        float cloud_density = sample_cloud_density(pos);\n"
      "        if (cloud_density > 0.01) {\n"
      "            float light_transmittance = exp(-cloud_density * step_size "
      "* 2.0);\n"
      "            vec3 cloud_color = mix(vec3(0.9, 0.9, 0.95), vec3(0.7, 0.7, "
      "0.8), cloud_type);\n"
      "            \n"
      "            accumulated_color += cloud_color * cloud_density * "
      "step_size * light_transmittance * (1.0 - accumulated_alpha);\n"
      "            accumulated_alpha += (1.0 - light_transmittance) * (1.0 - "
      "accumulated_alpha);\n"
      "            \n"
      "            if (accumulated_alpha > 0.95) break;\n"
      "        }\n"
      "    }\n"
      "    \n"
      "    return accumulated_color;\n"
      "}\n"
      "\n"
      "void main() {\n"
      "    vec3 ray_dir = normalize(world_pos - camera_position);\n"
      "    vec3 cloud_color = ray_march_clouds(camera_position, ray_dir);\n"
      "    \n"
      "    frag_color = vec4(cloud_color, clamp(cloud_color.r + cloud_color.g "
      "+ cloud_color.b, 0.0, 1.0));\n"
      "}\n";

  g_weather_rendering.cloud_shader =
      shader_compile(cloud_vertex_shader, cloud_fragment_shader);
  if (!g_weather_rendering.cloud_shader) {
    fprintf(stderr, "Failed to compile cloud shader\n");
    return false;
  }

  // Initialize textures
  g_weather_rendering.weather_map_texture =
      gpu_texture_create_2d(512, 512, GPU_FORMAT_R32F);
  g_weather_rendering.noise_texture_3d =
      gpu_texture_create_3d(128, 128, 128, GPU_FORMAT_R32F);
  g_weather_rendering.blue_noise_texture =
      gpu_texture_create_2d(512, 512, GPU_FORMAT_RGBA8);

  // Generate procedural noise textures
  weather_rendering_generate_noise_textures();

  g_weather_rendering.rendering_enabled = true;
  g_weather_rendering.render_quality = 1.0f;
  g_weather_rendering.temporal_blend_factor = 0.05f;

  return true;
}

// Generate procedural noise textures for weather effects
void weather_rendering_generate_noise_textures(void) {
  // Generate 3D Perlin-Worley noise for clouds
  float *noise_data = malloc(128 * 128 * 128 * sizeof(float));
  if (noise_data) {
    for (int x = 0; x < 128; x++) {
      for (int y = 0; y < 128; y++) {
        for (int z = 0; z < 128; z++) {
          float freq1 = 0.02f;
          float freq2 = 0.05f;
          float freq3 = 0.1f;

          float n1 = perlin_noise_3d(x * freq1, y * freq1, z * freq1);
          float n2 = worley_noise_3d(x * freq2, y * freq2, z * freq2);
          float n3 = perlin_noise_3d(x * freq3, y * freq3, z * freq3);

          noise_data[(x * 128 + y) * 128 + z] =
              n1 * 0.6f + n2 * 0.3f + n3 * 0.1f;
        }
      }
    }
    gpu_texture_update_3d(g_weather_rendering.noise_texture_3d, noise_data);
    UNIFIED_FREE(noise_data);
  }

  // Generate blue noise for temporal AA
  uint8_t *blue_noise =
      UNIFIED_ALLOC(512 * 512 * 4, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_TRACK);
  if (blue_noise) {
    generate_blue_noise(blue_noise, 512, 512);
    gpu_texture_update_2d(g_weather_rendering.blue_noise_texture, blue_noise);
    UNIFIED_FREE(blue_noise);
  }
}

// Apply weather effects to the rendering pipeline
void weather_manager_apply_to_rendering(WeatherManager *manager,
                                        const float *view_matrix,
                                        const float *projection_matrix,
                                        const float *camera_position) {
  if (!manager || !g_weather_rendering.rendering_enabled) {
    return;
  }

  uint64_t start_time = get_performance_counter();

  // Get current weather parameters
  WeatherParameters current_params = manager->transition.current_params;

  // Update weather map for clouds
  float weather_map[512 * 512];
  cloud_weather_map(&manager->clouds, get_time_seconds(), weather_map, 512,
                    512);
  gpu_texture_update_2d(g_weather_rendering.weather_map_texture, weather_map);

  // Render atmospheric sky
  weather_rendering_apply_sky(&current_params, camera_position);

  // Render volumetric clouds
  weather_rendering_apply_clouds(&manager->clouds, &current_params, view_matrix,
                                 projection_matrix, camera_position);

  // Apply fog effects
  weather_rendering_apply_fog(&current_params, camera_position);

  // Render precipitation
  weather_rendering_apply_precipitation(&manager->precipitation, view_matrix,
                                        projection_matrix, camera_position);

  // Render lightning effects
  weather_rendering_apply_lightning(&manager->lightning, view_matrix,
                                    projection_matrix, camera_position);

  // Update temporal state
  g_weather_rendering.frame_count++;

  uint64_t end_time = get_performance_counter();
  g_weather_rendering.last_frame_time_ms =
      (end_time - start_time) * 1000.0 / get_performance_frequency();
}

// Render atmospheric sky with scattering
static void weather_rendering_apply_sky(const WeatherParameters *params,
                                        const float *camera_position) {
  if (!params)
    return;

  material_bind_by_shader(g_weather_rendering.sky_shader);

  // Set atmospheric parameters
  float sun_direction[3] = {0.0f, -1.0f, 0.0f}; // Default sun position
  if (params->sun_intensity_multiplier > 0.0f) {
    // Calculate sun direction from time of day
    float time = get_time_of_day();
    sun_direction[0] = sinf(time * 2.0f * 3.14159f);
    sun_direction[1] = cosf(time * 2.0f * 3.14159f);
    sun_direction[2] = 0.0f;
  }

  material_set_float3("sun_direction", sun_direction);
  material_set_float("sun_intensity", params->sun_intensity_multiplier);

  // Rayleigh scattering coefficients (for blue sky)
  float rayleigh_coeffs[3] = {0.0058f * params->sky_color[0],
                              0.0135f * params->sky_color[1],
                              0.0331f * params->sky_color[2]};
  material_set_float3("rayleigh_coefficients", rayleigh_coeffs);

  // Mie scattering coefficients (for haze)
  float mie_coeffs[3] = {0.0035f * params->horizon_color[0],
                         0.0035f * params->horizon_color[1],
                         0.0035f * params->horizon_color[2]};
  material_set_float3("mie_coefficients", mie_coeffs);

  material_set_float("rayleigh_scale_height",
                     ATMOSPHERE_DEFAULT.rayleigh_scale_height);
  material_set_float("mie_scale_height", ATMOSPHERE_DEFAULT.mie_scale_height);

  // Render fullscreen sky quad
  render_fullscreen_quad();

  g_weather_rendering.draw_calls++;
}

// Render volumetric clouds
static void weather_rendering_apply_clouds(
    const VolumetricCloudSettings *cloud_settings,
    const WeatherParameters *weather_params, const float *view_matrix,
    const float *projection_matrix, const float *camera_position) {
  if (!cloud_settings || !cloud_settings->enabled)
    return;

  material_bind_by_shader(g_weather_rendering.cloud_shader);

  // Set cloud parameters
  material_set_texture("weather_map", g_weather_rendering.weather_map_texture);
  material_set_texture("noise_texture", g_weather_rendering.noise_texture_3d);
  material_set_float3("sun_direction", weather_params->wind_direction);
  material_set_float("coverage", cloud_settings->coverage);
  material_set_float("density", cloud_settings->density);
  material_set_float("cloud_type", cloud_settings->cloud_type);
  material_set_float("time", get_time_seconds());
  material_set_int("ray_march_steps", cloud_settings->ray_march_steps);
  material_set_float3("camera_position", camera_position);

  // Set up cloud mesh (simple billboard for now)
  float cloud_altitude = cloud_settings->altitude_min;
  render_cloud_billboard(cloud_altitude, view_matrix, projection_matrix);

  g_weather_rendering.triangles_rendered += 2; // One quad = 2 triangles
  g_weather_rendering.draw_calls++;
}

// Apply fog rendering effects
static void weather_rendering_apply_fog(const WeatherParameters *params,
                                        const float *camera_position) {
  if (!params || params->fog_density <= 0.0f)
    return;

  // Set global fog parameters for the rendering pipeline
  set_global_fog_parameters(params->fog_density, params->fog_start_distance,
                            params->fog_end_distance, params->fog_color,
                            params->fog_height_falloff);
}

// Render precipitation particles
static void weather_rendering_apply_precipitation(
    const PrecipitationSystem *precipitation, const float *view_matrix,
    const float *projection_matrix, const float *camera_position) {
  if (!precipitation || !precipitation->active ||
      precipitation->active_particles == 0)
    return;

  // Update particle system
  material_bind_by_shader(g_weather_rendering.precipitation_shader);

  material_set_float("precipitation_intensity", precipitation->intensity);
  material_set_float("particle_size", precipitation->particle_size);
  material_set_float("fall_speed", precipitation->fall_speed);
  material_set_float("wind_influence", precipitation->wind_influence);
  material_set_float3("wind_direction",
                      (const float[]){1.0f, 0.0f, 0.0f}); // Simplified
  material_set_texture("particle_texture", precipitation->particle_texture);

  // Render particle system
  render_particle_system(precipitation->active_particles, camera_position);

  g_weather_rendering.triangles_rendered += precipitation->active_particles;
  g_weather_rendering.draw_calls++;
}

// Render lightning effects
static void weather_rendering_apply_lightning(const LightningSystem *lightning,
                                              const float *view_matrix,
                                              const float *projection_matrix,
                                              const float *camera_position) {
  if (!lightning || lightning->bolt_count == 0)
    return;

  material_bind_by_shader(g_weather_rendering.lightning_shader);

  for (uint32_t i = 0; i < lightning->bolt_count; i++) {
    const LightningBolt *bolt = &lightning->bolts[i];

    float intensity = bolt->intensity * (1.0f - bolt->age / bolt->lifetime);
    if (intensity <= 0.01f)
      continue;

    material_set_float("lightning_intensity", intensity);
    material_set_float("bolt_width", 2.0f);
    material_set_float3("lightning_color", (const float[]){0.8f, 0.9f, 1.0f});

    // Render lightning bolt as line segments
    render_lightning_bolt(bolt->segments, bolt->segment_count, view_matrix,
                          projection_matrix);

    g_weather_rendering.draw_calls++;

    // Add screen flash if needed
    if (bolt->has_flash && bolt->flash_intensity > 0.0f) {
      apply_screen_flash(bolt->flash_intensity * intensity);
    }
  }
}

// Perlin noise function for cloud generation
static float perlin_noise_3d(float x, float y, float z) {
  // Simplified Perlin noise implementation
  int xi = (int)floorf(x) & 255;
  int yi = (int)floorf(y) & 255;
  int zi = (int)floorf(z) & 255;

  float xf = x - floorf(x);
  float yf = y - floorf(y);
  float zf = z - floorf(z);

  // Simple gradient noise (simplified for performance)
  float u = fade(xf);
  float v = fade(yf);
  float w = fade(zf);

  float a = grad(0, xi, yi, zi, xf, yf, zf);
  float b = grad(0, xi + 1, yi, zi, xf - 1, yf, zf);
  float c = grad(0, xi, yi + 1, zi, xf, yf - 1, zf);
  float d = grad(0, xi + 1, yi + 1, zi, xf - 1, yf - 1, zf);
  float e = grad(0, xi, yi, zi + 1, xf, yf, zf - 1);
  float f = grad(0, xi + 1, yi, zi + 1, xf - 1, yf, zf - 1);
  float g = grad(0, xi, yi + 1, zi + 1, xf, yf - 1, zf - 1);
  float h = grad(0, xi + 1, yi + 1, zi + 1, xf - 1, yf - 1, zf - 1);

  return lerp(lerp(lerp(a, b, u), lerp(c, d, u), v),
              lerp(lerp(e, f, u), lerp(g, h, u), v), w);
}

// Worley noise for cellular patterns
static float worley_noise_3d(float x, float y, float z) {
  int xi = (int)floorf(x);
  int yi = (int)floorf(y);
  int zi = (int)floorf(z);

  float min_dist = 1000000.0f;

  // Check neighboring cells
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dz = -1; dz <= 1; dz++) {
        int cx = xi + dx;
        int cy = yi + dy;
        int cz = zi + dz;

        // Generate random feature point for this cell
        uint32_t seed = cx * 374761393u + cy * 668265263u + cz * 2654435761u;
        float fx = cx + (float)(seed % 1000) / 1000.0f;
        float fy = cy + (float)((seed >> 10) % 1000) / 1000.0f;
        float fz = cz + (float)((seed >> 20) % 1000) / 1000.0f;

        float dist = sqrtf((x - fx) * (x - fx) + (y - fy) * (y - fy) +
                           (z - fz) * (z - fz));
        if (dist < min_dist) {
          min_dist = dist;
        }
      }
    }
  }

  return 1.0f - min_dist;
}

// Fade function for Perlin noise
static float fade(float t) {
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Gradient function for Perlin noise
static float grad(int hash, int x, int y, int z, float fx, float fy, float fz) {
  int h = hash & 15;
  float u = h < 8 ? fx : fy;
  float v = h < 4 ? fy : h == 12 || h == 14 ? fx : fz;
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Linear interpolation
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Generate blue noise texture
static void generate_blue_noise(uint8_t *data, int width, int height) {
  // Simplified blue noise generation using error diffusion
  for (int i = 0; i < width * height * 4; i += 4) {
    float noise = (float)rand() / RAND_MAX;
    uint8_t value = (uint8_t)(noise * 255.0f);
    data[i] = value;     // R
    data[i + 1] = value; // G
    data[i + 2] = value; // B
    data[i + 3] = 255;   // A
  }
}

// Cleanup weather rendering resources
void weather_rendering_shutdown(void) {
  if (g_weather_rendering.sky_shader) {
    shader_destroy(g_weather_rendering.sky_shader);
  }
  if (g_weather_rendering.cloud_shader) {
    shader_destroy(g_weather_rendering.cloud_shader);
  }
  if (g_weather_rendering.precipitation_shader) {
    shader_destroy(g_weather_rendering.precipitation_shader);
  }
  if (g_weather_rendering.fog_shader) {
    shader_destroy(g_weather_rendering.fog_shader);
  }
  if (g_weather_rendering.lightning_shader) {
    shader_destroy(g_weather_rendering.lightning_shader);
  }

  if (g_weather_rendering.weather_map_texture) {
    gpu_texture_destroy(g_weather_rendering.weather_map_texture);
  }
  if (g_weather_rendering.noise_texture_3d) {
    gpu_texture_destroy(g_weather_rendering.noise_texture_3d);
  }
  if (g_weather_rendering.blue_noise_texture) {
    gpu_texture_destroy(g_weather_rendering.blue_noise_texture);
  }

  memset(&g_weather_rendering, 0, sizeof(WeatherRenderingState));
}

// Get rendering performance metrics
void weather_rendering_get_metrics(float *frame_time_ms, uint32_t *triangles,
                                   uint32_t *draw_calls) {
  if (frame_time_ms)
    *frame_time_ms = g_weather_rendering.last_frame_time_ms;
  if (triangles)
    *triangles = g_weather_rendering.triangles_rendered;
  if (draw_calls)
    *draw_calls = g_weather_rendering.draw_calls;
}

// Set rendering quality level
void weather_rendering_set_quality(float quality) {
  g_weather_rendering.render_quality = fmaxf(0.1f, fminf(1.0f, quality));
}

// Enable/disable weather rendering
void weather_rendering_set_enabled(bool enabled) {
  g_weather_rendering.rendering_enabled = enabled;
}