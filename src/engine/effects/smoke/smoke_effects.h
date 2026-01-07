#ifndef SMOKE_EFFECTS_H
#define SMOKE_EFFECTS_H

#include "../../3d_rendering.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ResourceHandle density_texture; // 3D texture handle
    Vec3 extent;
    Vec3 position;
    Vec3 color;
    float scattering_factor;
    float absorption_factor;
} SmokeVolume;

// Ray marching constants
#define SMOKE_RAYMARCH_STEPS 64
#define SMOKE_DENSITY_THRESHOLD 0.01f

/**
 * @brief Initialize smoke rendering system
 */
void smoke_rendering_init(void);

/**
 * @brief Render smoke volume using raymarching
 * @param volume Smoke volume data
 * @param ray_origin Camera position or ray start
 * @param ray_dir Generalized ray direction
 * @param max_dist Maximum ray distance
 * @return Accumulated color and alpha in Vec4
 */
Vec4 raymarch_smoke(SmokeVolume* volume, Vec3 ray_origin, Vec3 ray_dir, float max_dist);

// Internal functions
float sample_density_field(ResourceHandle density_texture, Vec3 position);
float trace_shadow(Vec3 position, Vec3 light_dir);
Vec3 compute_smoke_lighting(SmokeVolume* volume, Vec3 position, float density, Vec3 light_dir);

#ifdef __cplusplus
}
#endif

#endif // SMOKE_EFFECTS_H
