#ifndef LIQUID_EFFECTS_H
#define LIQUID_EFFECTS_H

#include "../../3d_rendering.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Vec3 min_bounds;
    Vec3 max_bounds;
    float water_level;
    Vec3 surface_color;
    float wave_height;
    float wave_speed;
    ResourceHandle normal_map_0;
    ResourceHandle normal_map_1;
} LiquidBody;

typedef struct ParticleSystem ParticleSystem;

/**
 * @brief Initialize liquid rendering
 */
void liquid_rendering_init(void);

/**
 * @brief Render liquid surface
 * @param liquid Liquid body parameters
 * @param camera_view View matrix
 * @param camera_proj Projection matrix
 */
void render_liquid_surface(LiquidBody* liquid, const Mat4* camera_view, const Mat4* camera_proj);

/**
 * @brief Spawn splashes at a given location
 * @param position World position
 * @param force Impact force
 */
void spawn_liquid_splash(Vec3 position, float force);

#ifdef __cplusplus
}
#endif

#endif // LIQUID_EFFECTS_H
