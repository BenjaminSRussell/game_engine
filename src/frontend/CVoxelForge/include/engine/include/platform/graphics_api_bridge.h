// Graphics API Bridge - C API exports for Swift frontend
// Exposes all high-fidelity graphics features to VoxelForgeStudio

#ifndef GRAPHICS_API_BRIDGE_H
#define GRAPHICS_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Dynamic Resolution API
// ============================================================================

// Set render scale (0.25 = quarter res, 1.0 = native, 2.0 = supersampling)
void graphics_set_render_scale(float scale);

// Get current render scale
float graphics_get_render_scale(void);

// Get current render resolution
void graphics_get_render_resolution(uint32_t *width, uint32_t *height);

// ============================================================================
// GPU Particle System API
// ============================================================================

typedef struct {
  uint32_t max_particles;
  float emission_rate;
  float lifetime;
  float start_size;
  float end_size;
  float start_color[4]; // RGBA
  float end_color[4];
} ParticleEmitterConfig;

// Create particle emitter
uint32_t graphics_particle_create_emitter(const ParticleEmitterConfig *config);

// Update emitter configuration
void graphics_particle_update_emitter(uint32_t emitter_id,
                                      const ParticleEmitterConfig *config);

// Set emitter position
void graphics_particle_set_position(uint32_t emitter_id, float x, float y,
                                    float z);

// Enable/disable emitter
void graphics_particle_set_active(uint32_t emitter_id, bool active);

// Destroy emitter
void graphics_particle_destroy_emitter(uint32_t emitter_id);

// Get particle count
uint32_t graphics_particle_get_count(uint32_t emitter_id);

// ============================================================================
// GLTF Model Loading API
// ============================================================================

typedef struct {
  uint32_t mesh_count;
  uint32_t material_count;
  char **mesh_names;
  char error_message[256];
} GLTFLoadResult;

// Load GLTF/GLB file
uint32_t graphics_gltf_load(const char *filepath, GLTFLoadResult *result);

// Unload GLTF model
void graphics_gltf_unload(uint32_t model_id);

// Get mesh by index from loaded model
uint32_t graphics_gltf_get_mesh(uint32_t model_id, uint32_t mesh_index);

// ============================================================================
// PBR Material API
// ============================================================================

typedef struct {
  float metallic;
  float roughness;
  float ao;
  float normal_strength;
  float albedo[4]; // RGBA
  float emissive[3];
  char albedo_texture[256];
  char normal_texture[256];
  char metallic_roughness_texture[256];
  char ao_texture[256];
} PBRMaterialConfig;

// Create PBR material
uint32_t graphics_material_create_pbr(const PBRMaterialConfig *config);

// Update material properties
void graphics_material_update(uint32_t material_id,
                              const PBRMaterialConfig *config);

// Load material from GLTF
uint32_t graphics_material_from_gltf(uint32_t gltf_model_id,
                                     uint32_t material_index);

// Destroy material
void graphics_material_destroy(uint32_t material_id);

// ============================================================================
// Post-Processing Effects API
// ============================================================================

// Bloom configuration
typedef struct {
  float threshold; // HDR brightness threshold (default: 1.0)
  float intensity; // Bloom strength (default: 0.04)
  float spread;    // Blur spread (default: 1.0)
  bool enabled;
} BloomConfig;

void graphics_bloom_set_config(const BloomConfig *config);
void graphics_bloom_get_config(BloomConfig *config);

// TAA configuration
typedef struct {
  float blend_factor;   // History weight 0-1 (default: 0.9)
  float variance_clamp; // Neighborhood clamp (default: 1.0)
  bool jitter_enabled;  // Enable camera jitter
  bool enabled;
} TAAConfig;

void graphics_taa_set_config(const TAAConfig *config);
void graphics_taa_get_config(TAAConfig *config);

// SSR configuration
typedef struct {
  float max_distance; // Max ray distance (default: 100.0)
  uint32_t max_steps; // Ray march steps (default: 64)
  float thickness;    // Ray thickness (default: 0.5)
  float fade_start;   // Edge fade start (default: 0.8)
  bool enabled;
} SSRConfig;

void graphics_ssr_set_config(const SSRConfig *config);
void graphics_ssr_get_config(SSRConfig *config);

// Volumetric Fog configuration
typedef struct {
  float density;        // Fog density (default: 0.01)
  float scattering;     // Light scattering (default: 0.5)
  float height_falloff; // Height-based falloff (default: 0.1)
  float color[3];       // RGB fog color
  bool enabled;
} VolumetricFogConfig;

void graphics_fog_set_config(const VolumetricFogConfig *config);
void graphics_fog_get_config(VolumetricFogConfig *config);

// ============================================================================
// IBL (Environment Lighting) API
// ============================================================================

// Load HDR environment map
uint32_t graphics_ibl_load_environment(const char *hdr_filepath);

// Set active environment map
void graphics_ibl_set_active(uint32_t environment_id);

// Set environment intensity
void graphics_ibl_set_intensity(float intensity);

// Unload environment map
void graphics_ibl_unload(uint32_t environment_id);

// ============================================================================
// General Graphics Settings
// ============================================================================

typedef struct {
  bool vsync;
  bool hdr;
  uint32_t msaa_samples; // 0, 2, 4, 8
  float exposure;        // HDR exposure
  float gamma;           // Gamma correction
} GraphicsSettings;

void graphics_set_settings(const GraphicsSettings *settings);
void graphics_get_settings(GraphicsSettings *settings);

// ============================================================================
// Debug Visualization
// ============================================================================

void graphics_debug_show_wireframe(bool enabled);
void graphics_debug_show_normals(bool enabled);
void graphics_debug_show_particles(bool enabled);
void graphics_debug_show_lighting_only(bool enabled);

#ifdef __cplusplus
}
#endif

#endif // GRAPHICS_API_BRIDGE_H
