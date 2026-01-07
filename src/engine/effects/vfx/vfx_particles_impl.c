/**
 * =================================================================================================
 *                              VFX PARTICLE SYSTEM IMPLEMENTATION
 *                              Agent: AGENT_VFX_1
 * =================================================================================================
 */

#include "effects/vfx/vfx_particles.h"
#include <include/math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    INTERNAL STRUCTURES
 * =================================================================================================
 */

typedef struct Particle {
  float x, y, z;
  float vx, vy, vz;
  float life, max_life;
  float r, g, b, a;
  float size;
  float rotation;
} Particle;

typedef enum EmitterType {
  EMITTER_POINT,
  EMITTER_SPHERE,
  EMITTER_CONE,
  EMITTER_BOX,
  EMITTER_MESH,
  EMITTER_RING,
  EMITTER_EDGE,
  EMITTER_TEXTURE
} EmitterType;

struct ParticleEmitter {
  uint32_t id;
  EmitterType type;
  Particle *particles;
  uint32_t count;
  uint32_t capacity;
  bool active;
  float emission_rate;
  float emission_accumulator;

  // Transform
  float px, py, pz;

  // Modules
  bool mod_size_life;
  bool mod_color_life;
  bool mod_velocity_life;
  bool mod_rotation_life;
  bool mod_noise;
  bool mod_collision;
  bool mod_lights;
  bool mod_trails;
};

static struct {
  ParticleEmitter *emitters;
  uint32_t emitter_count;
  uint32_t emitter_capacity;
  bool initialized;
} g_vfx_manager = {0};

/* =================================================================================================
 *                                    PARTICLE EMITTER TYPES
 * =================================================================================================
 */

ParticleEmitter *emitter_create(EmitterType type) {
  ParticleEmitter *emitter =
      (ParticleEmitter *)calloc(1, sizeof(ParticleEmitter));
  emitter->type = type;
  emitter->capacity = 100;
  emitter->particles = (Particle *)calloc(emitter->capacity, sizeof(Particle));
  emitter->active = true;
  return emitter;
}

void *emitter_point_create(void) { return emitter_create(EMITTER_POINT); }
void *emitter_sphere_create(float radius) {
  return emitter_create(EMITTER_SPHERE);
}
void *emitter_cone_create(float angle, float height) {
  return emitter_create(EMITTER_CONE);
}
void *emitter_box_create(float x, float y, float z) {
  return emitter_create(EMITTER_BOX);
}
void *emitter_mesh_create(void *mesh) { return emitter_create(EMITTER_MESH); }
void *emitter_ring_create(float radius, float thickness) {
  return emitter_create(EMITTER_RING);
}
void *emitter_edge_create(float length) { return emitter_create(EMITTER_EDGE); }
void *emitter_texture_create(void *texture) {
  return emitter_create(EMITTER_TEXTURE);
}

void emitter_spawn_particle(void *emitter) {
  ParticleEmitter *e = (ParticleEmitter *)emitter;
  if (e->count < e->capacity) {
    Particle *p = &e->particles[e->count++];
    p->life = 1.0f;
    p->max_life = 1.0f;
  }
}

void emitter_spawn_burst(void *emitter, int count) {
  for (int i = 0; i < count; i++)
    emitter_spawn_particle(emitter);
}

void emitter_update(void *emitter, float dt) {
  ParticleEmitter *e = (ParticleEmitter *)emitter;
  if (!e->active)
    return;

  // Emit
  e->emission_accumulator += e->emission_rate * dt;
  while (e->emission_accumulator >= 1.0f) {
    emitter_spawn_particle(emitter);
    e->emission_accumulator -= 1.0f;
  }

  // Update
  for (uint32_t i = 0; i < e->count; i++) {
    Particle *p = &e->particles[i];
    p->life -= dt;
    p->x += p->vx * dt;
    p->y += p->vy * dt;
    p->z += p->vz * dt;

    // Compact
    if (p->life <= 0) {
      *p = e->particles[--e->count];
      i--;
    }
  }
}

void emitter_simulate_gpu(void *emitter) {}
void emitter_simulate_cpu(void *emitter) {}
void emitter_sort_particles(void *emitter) {}
void emitter_compact_buffer(void *emitter) {}
void emitter_prewarm(void *emitter, float dt) {}
void emitter_serialize(void *emitter, const char *path) {}
void emitter_deserialize(void *emitter, const char *path) {}

/* =================================================================================================
 *                                    FORCE FIELDS
 * =================================================================================================
 */

void force_gravity(void *emitter, float g) {}
void force_wind(void *emitter, float x, float y, float z) {}
void force_vortex(void *emitter, float strength, float axis_x, float axis_y,
                  float axis_z) {}
void force_turbulence(void *emitter, float strength) {}
void force_drag(void *emitter, float coeff) {}
void force_attractor(void *emitter, float x, float y, float z, float strength) {
}
void force_repulsor(void *emitter, float x, float y, float z, float strength) {}
void force_vector_field(void *emitter, void *field) {}
void force_curl_noise(void *emitter, float strength) {}
void force_combine(void *e1, void *e2) {}

/* =================================================================================================
 *                                    PARTICLE MODULES
 * =================================================================================================
 */

void module_size_over_lifetime(void *emitter, void *curve) {}
void module_color_over_lifetime(void *emitter, void *gradient) {}
void module_velocity_over_lifetime(void *emitter, void *curve) {}
void module_rotation_over_lifetime(void *emitter, void *curve) {}
void module_noise(void *emitter, float strength) {}
void module_collision(void *emitter, float restitution) {}
void module_sub_emitter(void *emitter, void *sub_emitter, int trigger) {}
void module_texture_sheet(void *emitter, int tiles_x, int tiles_y) {}
void module_trails(void *emitter, float lifetime) {}
void module_lights(void *emitter, float range, float intensity) {}
void module_ribbons(void *emitter, float width) {}

/* =================================================================================================
 *                                    PARTICLE RENDERING
 * =================================================================================================
 */

void render_billboard(void *emitter) {}
void render_stretched_billboard(void *emitter, float scale) {}
void render_horizontal_billboard(void *emitter) {}
void render_mesh_particles(void *emitter) {}
void render_trails(void *emitter) {}
void render_ribbons(void *emitter) {}
void render_soft_particles(void *emitter, float depth_fade) {}
void render_distortion(void *emitter) {}
void render_lit_particles(void *emitter) {}
void render_instanced(void *emitter) {}
void render_gpu_indirect(void *emitter) {}

/* =================================================================================================
 *                                    EFFECT PRESETS
 * =================================================================================================
 */

void *preset_fire_create(void) { return emitter_point_create(); }
void *preset_smoke_create(void) { return emitter_point_create(); }
void *preset_explosion_create(void) { return emitter_sphere_create(10.0f); }
void *preset_sparks_create(void) { return emitter_point_create(); }
void *preset_dust_create(void) { return emitter_box_create(10, 10, 10); }
void *preset_rain_create(void) { return emitter_box_create(50, 50, 50); }
void *preset_snow_create(void) { return emitter_box_create(50, 50, 50); }
void *preset_magic_create(void) { return emitter_point_create(); }
void *preset_blood_create(void) { return emitter_point_create(); }
void *preset_water_splash_create(void) { return emitter_cone_create(45, 5); }
void *preset_footstep_dust_create(void) { return emitter_point_create(); }
void *preset_muzzle_flash_create(void) { return emitter_point_create(); }
void *preset_impact_create(void) { return emitter_point_create(); }
void *preset_healing_create(void) { return emitter_ring_create(2, 5); }
void *preset_buff_create(void) { return emitter_ring_create(2, 5); }
void *preset_debuff_create(void) { return emitter_ring_create(2, 5); }
void *preset_level_up_create(void) { return emitter_ring_create(2, 10); }
void *preset_portal_create(void) { return emitter_ring_create(5, 5); }
void *preset_beam_create(void) { return emitter_edge_create(10); }

/* =================================================================================================
 *                                    VFX MANAGER
 * =================================================================================================
 */

void vfx_manager_init(void) { g_vfx_manager.initialized = true; }
void vfx_manager_shutdown(void) {}
void vfx_manager_update(float dt) {}
void vfx_manager_render(void) {}
void *vfx_spawn_effect(const char *name) { return NULL; }
void *vfx_spawn_at_position(const char *name, float x, float y, float z) {
  return NULL;
}
void *vfx_spawn_attached(const char *name, void *entity) { return NULL; }
void vfx_stop_effect(void *effect) {}
void vfx_stop_all(void) {}
void vfx_pool_prewarm(const char *name, int count) {}
void vfx_culling_update(void) {}
void vfx_lod_update(void) {}
void vfx_budget_control(float max_ms) {}
