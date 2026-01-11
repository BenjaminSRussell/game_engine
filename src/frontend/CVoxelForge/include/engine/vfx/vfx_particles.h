/**
 * =================================================================================================
 *                              VFX PARTICLE SYSTEM
 *                              Agent: AGENT_VFX_1
 * =================================================================================================
 */

#ifndef VFX_PARTICLES_H
#define VFX_PARTICLES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct ParticleEmitter ParticleEmitter;

/* =================================================================================================
 *                                    PARTICLE EMITTER TYPES
 * =================================================================================================
 */

void *emitter_point_create(void);
void *emitter_sphere_create(float radius);
void *emitter_cone_create(float angle, float height);
void *emitter_box_create(float x, float y, float z);
void *emitter_mesh_create(void *mesh);
void *emitter_ring_create(float radius, float thickness);
void *emitter_edge_create(float length);
void *emitter_texture_create(void *texture);
void emitter_spawn_particle(void *emitter);
void emitter_spawn_burst(void *emitter, int count);
void emitter_update(void *emitter, float dt);
void emitter_simulate_gpu(void *emitter);
void emitter_simulate_cpu(void *emitter);
void emitter_sort_particles(void *emitter);
void emitter_compact_buffer(void *emitter);
void emitter_prewarm(void *emitter, float dt);
void emitter_serialize(void *emitter, const char *path);
void emitter_deserialize(void *emitter, const char *path);

/* =================================================================================================
 *                                    FORCE FIELDS
 * =================================================================================================
 */

void force_gravity(void *emitter, float g);
void force_wind(void *emitter, float x, float y, float z);
void force_vortex(void *emitter, float strength, float axis_x, float axis_y,
                  float axis_z);
void force_turbulence(void *emitter, float strength);
void force_drag(void *emitter, float coeff);
void force_attractor(void *emitter, float x, float y, float z, float strength);
void force_repulsor(void *emitter, float x, float y, float z, float strength);
void force_vector_field(void *emitter, void *field);
void force_curl_noise(void *emitter, float strength);
void force_combine(void *e1, void *e2);

/* =================================================================================================
 *                                    PARTICLE MODULES
 * =================================================================================================
 */

void module_size_over_lifetime(void *emitter, void *curve);
void module_color_over_lifetime(void *emitter, void *gradient);
void module_velocity_over_lifetime(void *emitter, void *curve);
void module_rotation_over_lifetime(void *emitter, void *curve);
void module_noise(void *emitter, float strength);
void module_collision(void *emitter, float restitution);
void module_sub_emitter(void *emitter, void *sub_emitter, int trigger);
void module_texture_sheet(void *emitter, int tiles_x, int tiles_y);
void module_trails(void *emitter, float lifetime);
void module_lights(void *emitter, float range, float intensity);
void module_ribbons(void *emitter, float width);

/* =================================================================================================
 *                                    PARTICLE RENDERING
 * =================================================================================================
 */

void render_billboard(void *emitter);
void render_stretched_billboard(void *emitter, float scale);
void render_horizontal_billboard(void *emitter);
void render_mesh_particles(void *emitter);
void render_trails(void *emitter);
void render_ribbons(void *emitter);
void render_soft_particles(void *emitter, float depth_fade);
void render_distortion(void *emitter);
void render_lit_particles(void *emitter);
void render_instanced(void *emitter);
void render_gpu_indirect(void *emitter);

/* =================================================================================================
 *                                    EFFECT PRESETS
 * =================================================================================================
 */

void *preset_fire_create(void);
void *preset_smoke_create(void);
void *preset_explosion_create(void);
void *preset_sparks_create(void);
void *preset_dust_create(void);
void *preset_rain_create(void);
void *preset_snow_create(void);
void *preset_magic_create(void);
void *preset_blood_create(void);
void *preset_water_splash_create(void);
void *preset_footstep_dust_create(void);
void *preset_muzzle_flash_create(void);
void *preset_impact_create(void);
void *preset_healing_create(void);
void *preset_buff_create(void);
void *preset_debuff_create(void);
void *preset_level_up_create(void);
void *preset_portal_create(void);
void *preset_beam_create(void);

/* =================================================================================================
 *                                    VFX MANAGER
 * =================================================================================================
 */

void vfx_manager_init(void);
void vfx_manager_shutdown(void);
void vfx_manager_update(float dt);
void vfx_manager_render(void);
void *vfx_spawn_effect(const char *name);
void *vfx_spawn_at_position(const char *name, float x, float y, float z);
void *vfx_spawn_attached(const char *name, void *entity);
void vfx_stop_effect(void *effect);
void vfx_stop_all(void);
void vfx_pool_prewarm(const char *name, int count);
void vfx_culling_update(void);
void vfx_lod_update(void);
void vfx_budget_control(float max_ms);

#endif // VFX_PARTICLES_H
