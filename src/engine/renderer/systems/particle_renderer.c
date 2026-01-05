// Particle rendering implementation
#include <core/logger.h>
#include <math.h>
#include <renderer/particle_renderer.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather_particles.h>
#include <world/plant_vfx.h>

// Default configuration
static const ParticleRendererConfig DEFAULT_CONFIG = {
    .enable_instancing = true,
    .enable_sorting = true,
    .enable_soft_particles = true,
    .enable_depth_fade = true,
    .max_particles_per_batch = 1024,
    .particle_size_scale = 1.0f,
    .brightness_scale = 1.0f,
    .enable_glow = true,
    .glow_intensity = 0.5f};

// Texture file mappings
static const char *PARTICLE_TEXTURE_FILES[PARTICLE_TEXTURE_COUNT] = {
    "textures/particles/pollen.png", "textures/particles/spore.png",
    "textures/particles/seed.png",   "textures/particles/petal.png",
    "textures/particles/rain.png",   "textures/particles/snow.png",
    "textures/particles/dust.png",   "textures/particles/bloom.png",
    "textures/particles/generic.png"};

// Particle colors for different types
static const Vec3 PARTICLE_COLORS[PARTICLE_TEXTURE_COUNT] = {
    {1.0f, 0.9f, 0.3f}, // Pollen - yellow
    {0.6f, 0.4f, 0.8f}, // Spore - purple
    {0.8f, 0.6f, 0.4f}, // Seed - brown
    {1.0f, 0.6f, 0.8f}, // Petal - pink
    {0.7f, 0.8f, 1.0f}, // Rain - blue
    {1.0f, 1.0f, 1.0f}, // Snow - white
    {0.8f, 0.7f, 0.6f}, // Dust - tan
    {1.0f, 0.8f, 0.2f}, // Bloom - orange
    {0.8f, 0.8f, 0.8f}  // Generic - gray
};

// Comparison function for depth sorting
static int compare_particles(const void *a, const void *b) {
  const f32 *depth_a = (const f32 *)a;
  const f32 *depth_b = (const f32 *)b;
  return (*depth_a > *depth_b) ? 1 : -1;
}

void particle_renderer_init(ParticleRenderer *renderer, VulkanRenderer *vulkan,
                            u32 max_particles) {
  if (!renderer)
    return;

  memset(renderer, 0, sizeof(ParticleRenderer));
  renderer->renderer = vulkan;
  renderer->max_vertices = max_particles;
  renderer->config = DEFAULT_CONFIG;

  // Allocate vertex buffer
  renderer->vertex_buffer =
      (ParticleVertex *)malloc(max_particles * sizeof(ParticleVertex));
  if (!renderer->vertex_buffer) {
    LOG_ERROR("Failed to allocate particle vertex buffer");
    return;
  }

  // Allocate sorting indices
  renderer->sorted_indices = (u32 *)malloc(max_particles * sizeof(u32));
  if (!renderer->sorted_indices) {
    LOG_ERROR("Failed to allocate particle sorting indices");
    free(renderer->vertex_buffer);
    return;
  }

  // Initialize GPU buffers (these would be actual Vulkan calls)
  // For now, we just reserve space
  renderer->vbo = 0;
  renderer->vao = 0;
  renderer->shader_program = 0;
  renderer->instance_buffer = 0;

  // Initialize texture handles
  for (u32 i = 0; i < PARTICLE_TEXTURE_COUNT; i++) {
    renderer->textures[i] = 0;
  }

  renderer->vertex_count = 0;
  renderer->instance_count = 0;
  renderer->initialized = true;

  LOG_INFO("Particle renderer initialized with %u max particles",
           max_particles);
}

void particle_renderer_free(ParticleRenderer *renderer) {
  if (!renderer)
    return;

  if (renderer->vertex_buffer) {
    free(renderer->vertex_buffer);
    renderer->vertex_buffer = NULL;
  }

  if (renderer->sorted_indices) {
    free(renderer->sorted_indices);
    renderer->sorted_indices = NULL;
  }

  // Free GPU resources (would be actual Vulkan cleanup)
  // glDeleteBuffers(1, &renderer->vbo);
  // glDeleteVertexArrays(1, &renderer->vao);
  // glDeleteProgram(renderer->shader_program);

  memset(renderer, 0, sizeof(ParticleRenderer));
}

void particle_renderer_load_textures(ParticleRenderer *renderer) {
  if (!renderer || !renderer->initialized)
    return;

  LOG_INFO("Loading particle textures");

  for (u32 i = 0; i < PARTICLE_TEXTURE_COUNT; i++) {
    particle_renderer_load_texture(renderer, (ParticleTextureType)i,
                                   PARTICLE_TEXTURE_FILES[i]);
  }
}

bool particle_renderer_load_texture(ParticleRenderer *renderer,
                                    ParticleTextureType type,
                                    const char *filepath) {
  if (!renderer || !filepath || type >= PARTICLE_TEXTURE_COUNT)
    return false;

  // In a real implementation, this would load the texture from file
  // For now, just create a placeholder handle
  renderer->textures[type] = type + 1;

  LOG_DEBUG("Loaded particle texture: %s (handle: %u)", filepath,
            renderer->textures[type]);

  return true;
}

void particle_renderer_begin_batch(ParticleRenderer *renderer) {
  if (!renderer || !renderer->initialized)
    return;

  renderer->vertex_count = 0;
  renderer->instance_count = 0;
}

void particle_renderer_add_particle(ParticleRenderer *renderer, Vec3 position,
                                    f32 size, Vec3 color, f32 brightness,
                                    f32 rotation, ParticleTextureType texture,
                                    f32 lifetime_ratio) {
  if (!renderer || renderer->vertex_count >= renderer->max_vertices)
    return;

  ParticleVertex *vertex = &renderer->vertex_buffer[renderer->vertex_count];

  vertex->position = position;
  vertex->size = size * renderer->config.particle_size_scale;
  vertex->color = color;
  vertex->brightness = brightness * renderer->config.brightness_scale;
  vertex->rotation = rotation;
  vertex->texture_id = texture;
  vertex->lifetime_ratio = lifetime_ratio;
  vertex->padding = 0;

  renderer->vertex_count++;

  if (renderer->config.enable_instancing) {
    renderer->instance_count++;
  }
}

void particle_renderer_end_batch(ParticleRenderer *renderer) {
  if (!renderer || !renderer->initialized)
    return;

  // Sort particles by depth if enabled
  if (renderer->config.enable_sorting && renderer->vertex_count > 0) {
    particle_renderer_sort_particles(renderer, vec3(0.0f, 0.0f, 0.0f));
  }

  // In a real implementation, this would:
  // 1. Upload vertex buffer to GPU
  // 2. Set up instancing
  // 3. Issue draw calls

  LOG_DEBUG("Particle batch: %u vertices, %u instances", renderer->vertex_count,
            renderer->instance_count);
}

void particle_renderer_render_plant_particles(ParticleRenderer *renderer,
                                              const PlantParticle *particles,
                                              u32 particle_count) {
  if (!renderer || !particles || particle_count == 0)
    return;

  particle_renderer_begin_batch(renderer);

  for (u32 i = 0; i < particle_count; i++) {
    const PlantParticle *p = &particles[i];

    if (!p->active)
      continue;

    // Get texture and color for this particle type
    ParticleTextureType texture =
        particle_renderer_get_texture_for_type(p->type);
    Vec3 color = particle_renderer_get_color_for_type(p->type);

    // Calculate lifetime ratio (0 = new, 1 = dead)
    f32 lifetime_ratio = 1.0f - (p->lifetime / p->max_lifetime);

    particle_renderer_add_particle(renderer, p->position, p->size, color,
                                   p->brightness, p->rotation, texture,
                                   lifetime_ratio);
  }

  particle_renderer_end_batch(renderer);
}

void particle_renderer_render_weather_particles(
    ParticleRenderer *renderer, const WeatherParticle *particles,
    u32 particle_count) {
  if (!renderer || !particles || particle_count == 0)
    return;

  particle_renderer_begin_batch(renderer);

  for (u32 i = 0; i < particle_count; i++) {
    const WeatherParticle *p = &particles[i];

    // Weather particles are always active (type 0-3)
    ParticleTextureType texture = PARTICLE_TEXTURE_RAIN;
    Vec3 color = vec3(1.0f, 1.0f, 1.0f);

    // Map weather particle type to texture
    if (p->type == 0) { // Rain
      texture = PARTICLE_TEXTURE_RAIN;
      color = vec3(0.7f, 0.8f, 1.0f);
    } else if (p->type == 1) { // Snow
      texture = PARTICLE_TEXTURE_SNOW;
      color = vec3(1.0f, 1.0f, 1.0f);
    } else if (p->type == 2) { // Fog
      texture = PARTICLE_TEXTURE_GENERIC;
      color = vec3(0.9f, 0.9f, 0.9f);
    } else if (p->type == 3) { // Hail
      texture = PARTICLE_TEXTURE_SNOW;
      color = vec3(0.95f, 0.95f, 1.0f);
    }

    // Calculate lifetime ratio
    f32 lifetime_ratio = 1.0f - (p->life / 5.0f); // Assuming 5s default life
    if (lifetime_ratio < 0.0f)
      lifetime_ratio = 0.0f;
    if (lifetime_ratio > 1.0f)
      lifetime_ratio = 1.0f;

    particle_renderer_add_particle(renderer, p->position, p->size, color, 1.0f,
                                   0.0f, texture, lifetime_ratio);
  }

  particle_renderer_end_batch(renderer);
}

void particle_renderer_sort_particles(ParticleRenderer *renderer,
                                      Vec3 camera_position) {
  if (!renderer || renderer->vertex_count == 0)
    return;

  // Calculate depth for each particle
  f32 *depths = (f32 *)malloc(renderer->vertex_count * sizeof(f32));
  if (!depths)
    return;

  for (u32 i = 0; i < renderer->vertex_count; i++) {
    Vec3 diff = vec3_sub(renderer->vertex_buffer[i].position, camera_position);
    depths[i] = vec3_length(diff);
    renderer->sorted_indices[i] = i;
  }

  // Sort indices by depth (back to front)
  qsort(renderer->sorted_indices, renderer->vertex_count, sizeof(u32),
        compare_particles);

  free(depths);
}

ParticleTextureType particle_renderer_get_texture_for_type(u32 particle_type) {
  // Map internal particle types to texture types
  switch (particle_type) {
  case 0: // PLANT_PARTICLE_POLLEN
    return PARTICLE_TEXTURE_POLLEN;
  case 1: // PLANT_PARTICLE_SPORE
    return PARTICLE_TEXTURE_SPORE;
  case 2: // PLANT_PARTICLE_SEED
    return PARTICLE_TEXTURE_SEED;
  case 3: // PLANT_PARTICLE_NECTAR
    return PARTICLE_TEXTURE_BLOOM;
  case 4: // PLANT_PARTICLE_PETAL
    return PARTICLE_TEXTURE_PETAL;
  case 5: // PLANT_PARTICLE_LEAF_DUST
    return PARTICLE_TEXTURE_DUST;
  case 6: // PLANT_PARTICLE_VINE_GROWTH
    return PARTICLE_TEXTURE_GENERIC;
  case 7: // PLANT_PARTICLE_MUSHROOM_SPORE
    return PARTICLE_TEXTURE_SPORE;
  case 8: // PLANT_PARTICLE_BLOOM
    return PARTICLE_TEXTURE_BLOOM;
  default:
    return PARTICLE_TEXTURE_GENERIC;
  }
}

Vec3 particle_renderer_get_color_for_type(u32 particle_type) {
  ParticleTextureType texture =
      particle_renderer_get_texture_for_type(particle_type);
  return PARTICLE_COLORS[texture];
}

void particle_renderer_set_config(ParticleRenderer *renderer,
                                  const ParticleRendererConfig *config) {
  if (!renderer || !config)
    return;
  renderer->config = *config;
}

ParticleRendererConfig particle_renderer_get_default_config(void) {
  return DEFAULT_CONFIG;
}

ParticleRenderStats
particle_renderer_get_stats(const ParticleRenderer *renderer) {
  ParticleRenderStats stats = {0};

  if (!renderer)
    return stats;

  stats.particles_rendered = renderer->vertex_count;
  stats.batches = (renderer->vertex_count > 0) ? 1 : 0;
  stats.draw_calls = stats.batches;

  if (stats.batches > 0) {
    stats.average_batch_size =
        (f32)stats.particles_rendered / (f32)stats.batches;
  }

  return stats;
}
