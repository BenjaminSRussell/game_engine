// Environment/Procedural Foliage Spawner
#ifndef PROCEDURAL_FOLIAGE_H
#define PROCEDURAL_FOLIAGE_H

#include "include/common.h"
#include "math/vec3.h"

#define MAX_FOLIAGE_TYPES 32

typedef struct {
  char mesh_path[128];
  f32 density; // Instances per unit area
  f32 radius;  // Collision/Spacing radius

  // Scaling
  f32 min_scale;
  f32 max_scale;
  bool lock_scale_xy;

  // Rotation
  f32 random_pitch_angle;
  bool random_yaw;

  // Placement constraints
  f32 min_slope; // Degrees
  f32 max_slope;
  f32 min_height; // World Z
  f32 max_height;

  // Surface
  char layer_name[64]; // Only spawn on this landscape layer

} FoliageType;

typedef struct {
  FoliageType types[MAX_FOLIAGE_TYPES];
  u32 type_count;

  // Generation area
  Vec3 origin;
  f32 width;
  f32 length;

  u32 random_seed;
  u32 tile_size;

  // Terrain Interface
  f32 (*get_height)(f32 x, f32 z, void *user_data);
  void (*get_normal)(f32 x, f32 z, Vec3 *out_normal, void *user_data);
  void *terrain_user_data;

} ProceduralFoliageSpawner;

#ifdef __cplusplus
extern "C" {
#endif

ProceduralFoliageSpawner *foliage_spawner_create(void);
void foliage_spawner_add_type(ProceduralFoliageSpawner *spawner,
                              FoliageType type);

// Generates simulation of growth and returns instance list
// (Does not actually spawning game objects, just data)
void foliage_spawner_simulate(ProceduralFoliageSpawner *spawner, u32 steps,
                              void (*callback)(const char *mesh, Vec3 pos,
                                               Vec3 scale, f32 rot));

#ifdef __cplusplus
}
#endif

#endif // PROCEDURAL_FOLIAGE_H
