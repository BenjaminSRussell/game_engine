#ifndef EXPLOSION_EFFECTS_H
#define EXPLOSION_EFFECTS_H

#include "../../3d_rendering.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for particle system
typedef struct ParticleSystem ParticleSystem;

typedef struct {
    Vec3 origin;
    float radius;
    float strength;
    float duration;
    float current_time;
    // Particles
    ParticleSystem* debris_system;
    ParticleSystem* spark_system;
    ParticleSystem* smoke_system;
} ExplosionEffect;

/**
 * @brief Trigger an explosion at a location
 * @param origin World position
 * @param radius Radius of shockwave
 * @param strength Intensity
 */
void explosion_spawn(Vec3 origin, float radius, float strength);

/**
 * @brief Render explosion effects
 * @param effect Explosion instance
 * @param camera_view View matrix
 * @param camera_proj Projection matrix
 */
void render_explosion(ExplosionEffect* effect, const Mat4* camera_view, const Mat4* camera_proj);

// Internal
void spawn_debris(ExplosionEffect* effect, Vec3 origin, float intensity);
void render_shockwave(Vec3 center, float radius, float thickness);

#ifdef __cplusplus
}
#endif

#endif // EXPLOSION_EFFECTS_H
