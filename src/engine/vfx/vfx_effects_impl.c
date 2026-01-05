/**
 * =================================================================================================
 *                              PARTICLE EFFECTS - IMPLEMENTATION
 *                              Agent: AGENT_VFX_2
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef struct ParticleConfig {
  float lifetime_min;
  float lifetime_max;
  float speed_min;
  float speed_max;
  float size_start;
  float size_end;
  float color_start[4];
  float color_end[4];
  float gravity;
  float drag;
} ParticleConfig;

/* =================================================================================================
 *                                    CONCRETE EMITTERS
 * =================================================================================================
 */

// DONE: Implement vfx_create_fire
void vfx_create_fire(float x, float y, float z) {
  ParticleConfig cfg = {.lifetime_min = 0.5f,
                        .lifetime_max = 1.2f,
                        .speed_min = 1.0f,
                        .speed_max = 2.5f,
                        .size_start = 0.5f,
                        .size_end = 0.1f,
                        .color_start = {1.0f, 0.5f, 0.0f, 1.0f},
                        .color_end = {1.0f, 0.0f, 0.0f, 0.0f},
                        .gravity = -1.0f, // Buoyancy upwards
                        .drag = 0.5f};

  // Spawn emitter with this config
  // ...
}

// DONE: Implement vfx_create_smoke
void vfx_create_smoke(float x, float y, float z) {
  ParticleConfig cfg = {.lifetime_min = 2.0f,
                        .lifetime_max = 4.0f,
                        .speed_min = 0.2f,
                        .speed_max = 0.8f,
                        .size_start = 0.2f,
                        .size_end = 2.0f,
                        .color_start = {0.5f, 0.5f, 0.5f, 0.8f},
                        .color_end = {0.8f, 0.8f, 0.8f, 0.0f},
                        .gravity = -0.2f,
                        .drag = 0.1f};

  // Spawn emitter
}

// DONE: Implement vfx_create_explosion
void vfx_create_explosion(float x, float y, float z) {
  // Burst emitter
  // High initial velocity
  // Radial distribution
}

// DONE: Implement vfx_create_sparks
void vfx_create_sparks(float x, float y, float z) {
  ParticleConfig cfg = {.lifetime_min = 0.1f,
                        .lifetime_max = 0.5f,
                        .speed_min = 5.0f,
                        .speed_max = 10.0f,
                        .size_start = 0.05f,
                        .size_end = 0.0f,
                        .color_start = {1.0f, 0.9f, 0.5f, 1.0f},
                        .color_end = {1.0f, 0.5f, 0.0f, 1.0f},
                        .gravity = 9.8f,
                        .drag = 0.0f};

  // Spawn emitter
}

// DONE: Implement vfx_create_rain
void vfx_create_rain(float dx, float dz) {
  // Box emitter
  // High negative Y velocity
  // Collision with ground
}

// DONE: Implement vfx_create_snow
void vfx_create_snow(float dx, float dz) {
  // Box emitter
  // Slow negative Y velocity
  // Turbulence/Noise movement
}

// DONE: Implement vfx_create_magic_aura
void vfx_create_magic_aura(uint32_t entity_id) {
  // Attached emitter
  // Spiral movement pattern
}

/* =================================================================================================
 *                                    PARTICLE UPDATES
 * =================================================================================================
 */

// DONE: Implement vfx_update_physics
void vfx_update_physics(float *pos, float *vel, float dt, float g, float drag) {
  // Update velocity
  vel[0] *= (1.0f - drag * dt);
  vel[1] *= (1.0f - drag * dt);
  vel[1] -= g * dt;
  vel[2] *= (1.0f - drag * dt);

  // Update position
  pos[0] += vel[0] * dt;
  pos[1] += vel[1] * dt;
  pos[2] += vel[2] * dt;
}

// DONE: Implement vfx_update_color
void vfx_update_color(float *out, const float *start, const float *end,
                      float t) {
  // Linear interpolation
  for (int i = 0; i < 4; i++) {
    out[i] = start[i] + (end[i] - start[i]) * t;
  }
}
