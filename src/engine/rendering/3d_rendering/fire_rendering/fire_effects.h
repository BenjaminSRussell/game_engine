#ifndef FIRE_EFFECTS_H
#define FIRE_EFFECTS_H

#include "../../3d_rendering.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for particle system
typedef struct ParticleSystem ParticleSystem;

typedef struct {
    ParticleSystem* flames;
    ParticleSystem* embers;
    ResourceHandle noise_3d;
    Vec3 base_color;
    Vec3 tip_color;
    float intensity;
    Vec3 position;
} FireEffect;

/**
 * @brief Initialize the fire simulation subsystem
 */
void fire_simulation_init(void);

/**
 * @brief Update fire simulation
 * @param delta_time Time step
 */
void fire_simulation_update(float delta_time);

/**
 * @brief Render a fire effect
 * @param fire The fire effect instance
 * @param camera_view Camera view matrix
 * @param camera_proj Camera projection matrix
 */
void render_fire(FireEffect* fire, const Mat4* camera_view, const Mat4* camera_proj);

// Internal functions for composition
void simulate_fire_particles(FireEffect* fire, float delta_time);
void apply_fire_shading(FireEffect* fire);
void apply_heat_distortion(Vec3 position, float intensity);

#ifdef __cplusplus
}
#endif

#endif // FIRE_EFFECTS_H
