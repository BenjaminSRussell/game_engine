// Graphics API Bridge Implementation
// Connects Swift frontend to C graphics backend

#include "../include/assets/gltf_loader.h"
#include "../include/core/logger.h"
#include "../include/platform/graphics_api_bridge.h"
#include "include/rendering/gpu_material.h"
#include "include/rendering/gpu_particles.h"
#include "include/rendering/vulkan.h"
#include <string.h>

// Global renderer (would be properly managed in production)
static VulkanRenderer *g_renderer = NULL;
static ParticleSystem *g_particle_system = NULL;

// Storage for emitters (simplified)
#define MAX_EMITTERS 256
static struct {
  uint32_t id;
  bool active;
  float position[3];
} g_emitters[MAX_EMITTERS];
static uint32_t g_next_emitter_id = 1;

// ============================================================================
// Dynamic Resolution API
// ============================================================================

void graphics_set_render_scale(float scale) {
  if (g_renderer) {
    vulkan_set_render_scale(g_renderer, scale);
    LOG_INFO("Render scale set to %.2fx", scale);
  }
}

float graphics_get_render_scale(void) {
  if (g_renderer) {
    return g_renderer->render_scale;
  }
  return 1.0f;
}

void graphics_get_render_resolution(uint32_t *width, uint32_t *height) {
  if (g_renderer && width && height) {
    *width = g_renderer->render_extent.width;
    *height = g_renderer->render_extent.height;
  }
}

// ============================================================================
// GPU Particle System API
// ============================================================================

uint32_t graphics_particle_create_emitter(const ParticleEmitterConfig *config) {
  if (!config)
    return 0;

  // Find free slot
  for (uint32_t i = 0; i < MAX_EMITTERS; i++) {
    if (!g_emitters[i].active) {
      g_emitters[i].id = g_next_emitter_id++;
      g_emitters[i].active = true;
      g_emitters[i].position[0] = 0.0f;
      g_emitters[i].position[1] = 0.0f;
      g_emitters[i].position[2] = 0.0f;

      LOG_INFO("Created particle emitter #%u", g_emitters[i].id);
      return g_emitters[i].id;
    }
  }

  LOG_ERROR("Max particle emitters reached");
  return 0;
}

void graphics_particle_update_emitter(uint32_t emitter_id,
                                      const ParticleEmitterConfig *config) {
  // TODO: Update emitter properties in particle system
  LOG_INFO("Updated particle emitter #%u", emitter_id);
}

void graphics_particle_set_position(uint32_t emitter_id, float x, float y,
                                    float z) {
  for (uint32_t i = 0; i < MAX_EMITTERS; i++) {
    if (g_emitters[i].id == emitter_id && g_emitters[i].active) {
      g_emitters[i].position[0] = x;
      g_emitters[i].position[1] = y;
      g_emitters[i].position[2] = z;
      return;
    }
  }
}

void graphics_particle_set_active(uint32_t emitter_id, bool active) {
  for (uint32_t i = 0; i < MAX_EMITTERS; i++) {
    if (g_emitters[i].id == emitter_id) {
      g_emitters[i].active = active;
      return;
    }
  }
}

void graphics_particle_destroy_emitter(uint32_t emitter_id) {
  for (uint32_t i = 0; i < MAX_EMITTERS; i++) {
    if (g_emitters[i].id == emitter_id) {
      g_emitters[i].active = false;
      LOG_INFO("Destroyed particle emitter #%u", emitter_id);
      return;
    }
  }
}

uint32_t graphics_particle_get_count(uint32_t emitter_id) {
  // TODO: Query actual particle system
  return 0;
}

// ============================================================================
// GLTF Model Loading API
// ============================================================================

static GLTFLoadResult g_gltf_results[64];
static uint32_t g_next_gltf_id = 1;

uint32_t graphics_gltf_load(const char *filepath, GLTFLoadResult *result) {
  if (!filepath || !result) {
    if (result)
      strcpy(result->error, "Invalid parameters");
    return 0;
  }

  GLTFLoadResult loaded = gltf_load(filepath);

  if (!loaded.success) {
    strcpy(result->error, loaded.error);
    return 0;
  }

  uint32_t id = g_next_gltf_id++;
  g_gltf_results[id % 64] = loaded;

  result->mesh_count = loaded.mesh_count;
  result->material_count = loaded.material_count;
  // result->mesh_names member removed from struct


  LOG_INFO("Loaded GLTF: %s (%u meshes, %u materials)", filepath,
           loaded.mesh_count, loaded.material_count);

  return id;
}

void graphics_gltf_unload(uint32_t model_id) {
  if (model_id > 0 && model_id < 64) {
    gltf_free(&g_gltf_results[model_id]);
    LOG_INFO("Unloaded GLTF model #%u", model_id);
  }
}

uint32_t graphics_gltf_get_mesh(uint32_t model_id, uint32_t mesh_index) {
  // TODO: Return mesh handle from loaded GLTF
  return 0;
}

// ============================================================================
// PBR Material API
// ============================================================================

uint32_t graphics_material_create_pbr(const PBRMaterialConfig *config) {
  // TODO: Create material through GPUMaterialManager
  LOG_INFO("Created PBR material");
  return 1; // Placeholder
}

void graphics_material_update(uint32_t material_id,
                              const PBRMaterialConfig *config) {
  LOG_INFO("Updated material #%u", material_id);
}

uint32_t graphics_material_from_gltf(uint32_t gltf_model_id,
                                     uint32_t material_index) {
  // TODO: Extract material from GLTF and register
  return 0;
}

void graphics_material_destroy(uint32_t material_id) {
  LOG_INFO("Destroyed material #%u", material_id);
}

// ============================================================================
// Post-Processing Effects API
// ============================================================================

static BloomConfig g_bloom = {1.0f, 0.04f, 1.0f, true};
static TAAConfig g_taa = {0.9f, 1.0f, true, true};
static SSRConfig g_ssr = {100.0f, 64, 0.5f, 0.8f, true};
static VolumetricFogConfig g_fog = {
    0.01f, 0.5f, 0.1f, {0.5f, 0.5f, 0.6f}, false};

void graphics_bloom_set_config(const BloomConfig *config) {
  if (config) {
    g_bloom = *config;
    LOG_INFO("Bloom: threshold=%.2f, intensity=%.2f, %s", config->threshold,
             config->intensity, config->enabled ? "ON" : "OFF");
  }
}

void graphics_bloom_get_config(BloomConfig *config) {
  if (config)
    *config = g_bloom;
}

void graphics_taa_set_config(const TAAConfig *config) {
  if (config) {
    g_taa = *config;
    LOG_INFO("TAA: blend=%.2f, clamp=%.2f, %s", config->blend_factor,
             config->variance_clamp, config->enabled ? "ON" : "OFF");
  }
}

void graphics_taa_get_config(TAAConfig *config) {
  if (config)
    *config = g_taa;
}

void graphics_ssr_set_config(const SSRConfig *config) {
  if (config) {
    g_ssr = *config;
    LOG_INFO("SSR: distance=%.1f, steps=%u, %s", config->max_distance,
             config->max_steps, config->enabled ? "ON" : "OFF");
  }
}

void graphics_ssr_get_config(SSRConfig *config) {
  if (config)
    *config = g_ssr;
}

void graphics_fog_set_config(const VolumetricFogConfig *config) {
  if (config) {
    g_fog = *config;
    LOG_INFO("Volumetric Fog: density=%.3f, %s", config->density,
             config->enabled ? "ON" : "OFF");
  }
}

void graphics_fog_get_config(VolumetricFogConfig *config) {
  if (config)
    *config = g_fog;
}

// ============================================================================
// IBL API
// ============================================================================

uint32_t graphics_ibl_load_environment(const char *hdr_filepath) {
  if (!hdr_filepath)
    return 0;
  LOG_INFO("Loading environment: %s", hdr_filepath);
  return 1; // Placeholder
}

void graphics_ibl_set_active(uint32_t environment_id) {
  LOG_INFO("Set active environment #%u", environment_id);
}

void graphics_ibl_set_intensity(float intensity) {
  LOG_INFO("Environment intensity: %.2f", intensity);
}

void graphics_ibl_unload(uint32_t environment_id) {
  LOG_INFO("Unloaded environment #%u", environment_id);
}

// ============================================================================
// General Graphics Settings
// ============================================================================

static GraphicsSettings g_settings = {true, false, 0, 1.0f, 2.2f};

void graphics_set_settings(const GraphicsSettings *settings) {
  if (settings) {
    g_settings = *settings;
    LOG_INFO("Graphics settings updated: MSAA=%ux, HDR=%s, VSync=%s",
             settings->msaa_samples, settings->hdr ? "ON" : "OFF",
             settings->vsync ? "ON" : "OFF");
  }
}

void graphics_get_settings(GraphicsSettings *settings) {
  if (settings)
    *settings = g_settings;
}

// ============================================================================
// Debug Visualization
// ============================================================================

void graphics_debug_show_wireframe(bool enabled) {
  LOG_INFO("Wireframe: %s", enabled ? "ON" : "OFF");
}

void graphics_debug_show_normals(bool enabled) {
  LOG_INFO("Normals: %s", enabled ? "ON" : "OFF");
}

void graphics_debug_show_particles(bool enabled) {
  LOG_INFO("Particle debug: %s", enabled ? "ON" : "OFF");
}

void graphics_debug_show_lighting_only(bool enabled) {
  LOG_INFO("Lighting only: %s", enabled ? "ON" : "OFF");
}
