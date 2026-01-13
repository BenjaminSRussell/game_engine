// Particle rendering system - draws particles using billboards and instancing
// Handles all particle type rendering with optimization for large particle
// counts
#ifndef PARTICLE_RENDERER_H
#define PARTICLE_RENDERER_H

#include <common.h>
#include <math/vec2.h>
#include <math/vec3.h>

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;
struct PlantParticle;
struct WeatherParticle;

// Particle vertex data (for GPU instancing)
typedef struct {
  Vec3 position;
  f32 size;
  Vec3 color;
  f32 brightness;
  f32 rotation;
  u32 texture_id;
  f32 lifetime_ratio; // 0.0 = just created, 1.0 = dead
  u32 padding;
} ParticleVertex;

// Particle texture types
typedef enum {
  PARTICLE_TEXTURE_POLLEN = 0,
  PARTICLE_TEXTURE_SPORE = 1,
  PARTICLE_TEXTURE_SEED = 2,
  PARTICLE_TEXTURE_PETAL = 3,
  PARTICLE_TEXTURE_RAIN = 4,
  PARTICLE_TEXTURE_SNOW = 5,
  PARTICLE_TEXTURE_DUST = 6,
  PARTICLE_TEXTURE_BLOOM = 7,
  PARTICLE_TEXTURE_GENERIC = 8,
  PARTICLE_TEXTURE_COUNT = 9
} ParticleTextureType;

// Rendering configuration
typedef struct {
  bool enable_instancing;
  bool enable_sorting;
  bool enable_soft_particles;
  bool enable_depth_fade;
  u32 max_particles_per_batch;
  f32 particle_size_scale;
  f32 brightness_scale;
  bool enable_glow;
  f32 glow_intensity;
} ParticleRendererConfig;

// Particle renderer system
typedef struct {
  VulkanRenderer *renderer;
  ParticleVertex *vertex_buffer;
  u32 max_vertices;
  u32 vertex_count;
  ParticleRendererConfig config;

  // Texture handles
  u32 textures[PARTICLE_TEXTURE_COUNT];

  // GPU buffers
  u32 vbo; // Vertex buffer object
  u32 vao; // Vertex array object
  u32 shader_program;

  // Instancing
  u32 instance_buffer;
  u32 instance_count;

  // Depth sorting
  u32 *sorted_indices;

  bool initialized;
} ParticleRenderer;

// Initialization and cleanup
void particle_renderer_init(ParticleRenderer *renderer, VulkanRenderer *vulkan,
                            u32 max_particles);
void particle_renderer_free(ParticleRenderer *renderer);

// Loading textures
void particle_renderer_load_textures(ParticleRenderer *renderer);
bool particle_renderer_load_texture(ParticleRenderer *renderer,
                                    ParticleTextureType type,
                                    const char *filepath);

// Rendering plant particles
void particle_renderer_render_plant_particles(ParticleRenderer *renderer,
                                              const struct PlantParticle *particles,
                                              u32 particle_count);

// Rendering weather particles
void particle_renderer_render_weather_particles(
    ParticleRenderer *renderer, const struct WeatherParticle *particles,
    u32 particle_count);

// Batch rendering
void particle_renderer_begin_batch(ParticleRenderer *renderer);
void particle_renderer_add_particle(ParticleRenderer *renderer, Vec3 position,
                                    f32 size, Vec3 color, f32 brightness,
                                    f32 rotation, ParticleTextureType texture,
                                    f32 lifetime_ratio);
void particle_renderer_end_batch(ParticleRenderer *renderer);

// Configuration
void particle_renderer_set_config(ParticleRenderer *renderer,
                                  const ParticleRendererConfig *config);
ParticleRendererConfig particle_renderer_get_default_config(void);

// Depth sorting (for transparency)
void particle_renderer_sort_particles(ParticleRenderer *renderer,
                                      Vec3 camera_position);

// Utility functions
ParticleTextureType particle_renderer_get_texture_for_type(u32 particle_type);
Vec3 particle_renderer_get_color_for_type(u32 particle_type);

// Statistics
typedef struct {
  u32 particles_rendered;
  u32 batches;
  u32 draw_calls;
  f32 average_batch_size;
} ParticleRenderStats;

ParticleRenderStats
particle_renderer_get_stats(const ParticleRenderer *renderer);

#endif // PARTICLE_RENDERER_H
