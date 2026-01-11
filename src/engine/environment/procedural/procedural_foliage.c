#include "include/environment/procedural_foliage.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CANDIDATES 30
#define GRID_CELL_SIZE 5.0f // Optimization grid size

// Internal helper for random float [0, 1]
static f32 random_f32(void) { return (f32)rand() / (f32)RAND_MAX; }

static f32 random_range(f32 min, f32 max) {
  return min + random_f32() * (max - min);
}

ProceduralFoliageSpawner *foliage_spawner_create(void) {
  ProceduralFoliageSpawner *spawner =
      (ProceduralFoliageSpawner *)calloc(1, sizeof(ProceduralFoliageSpawner));
  if (spawner) {
    spawner->type_count = 0;
    spawner->random_seed = 12345;
    spawner->tile_size = 64;
    // Defaults
    spawner->width = 100.0f;
    spawner->length = 100.0f;
  }
  return spawner;
}

void foliage_spawner_add_type(ProceduralFoliageSpawner *spawner,
                              FoliageType type) {
  if (spawner && spawner->type_count < MAX_FOLIAGE_TYPES) {
    spawner->types[spawner->type_count++] = type;
  }
}

// Check if a point is valid based on slope and height constraints
static bool is_valid_location(ProceduralFoliageSpawner *spawner,
                              FoliageType *type, f32 x, f32 z, f32 *out_y,
                              Vec3 *out_normal) {
  if (!spawner->get_height || !spawner->get_normal) {
    // Assume flat plane at y=0 if no terrain provider
    *out_y = 0.0f;
    *out_normal = (Vec3){0, 1, 0};

    // Simple height check against 0
    if (0 < type->min_height || 0 > type->max_height)
      return false;
    return true;
  }

  f32 y = spawner->get_height(x, z, spawner->terrain_user_data);

  // Height constraint
  if (y < type->min_height || y > type->max_height) {
    return false;
  }

  Vec3 normal;
  spawner->get_normal(x, z, &normal, spawner->terrain_user_data);

  // Slope constraint (dot product with up vector)
  // 1.0 = flat (0 degrees), 0.0 = vertical (90 degrees)
  f32 slope_dot = normal.y;
  f32 angle_deg = acosf(slope_dot) * (180.0f / 3.14159f);

  if (angle_deg < type->min_slope || angle_deg > type->max_slope) {
    return false;
  }

  *out_y = y;
  *out_normal = normal;
  return true;
}

void foliage_spawner_simulate(ProceduralFoliageSpawner *spawner, u32 steps,
                              void (*callback)(const char *mesh, Vec3 pos,
                                               Vec3 scale, f32 rot)) {
  if (!spawner || !callback)
    return;

  srand(spawner->random_seed);

  // For each type, perform simulation
  for (u32 t = 0; t < spawner->type_count; t++) {
    FoliageType *type = &spawner->types[t];

    // Calculate target count based on density
    f32 area = spawner->width * spawner->length;
    u32 target_count = (u32)(area * type->density);

    // Safety cap
    if (target_count > 100000)
      target_count = 100000;

    // Simple rejection sampling (Poisson Disc approximation)
    // In a real implementation, we'd use a spatial hash to check min_dist
    // (radius) efficient neighbor lookup. Here we use a simpler approach for
    // "steps" iterations or target count.

    u32 placed_count = 0;
    u32 attempts = 0;
    u32 max_attempts = target_count * 5;

    while (placed_count < target_count && attempts < max_attempts) {
      attempts++;

      // Random point in area
      f32 tx = random_f32();
      f32 tz = random_f32();

      f32 world_x = spawner->origin.x + tx * spawner->width;
      f32 world_z = spawner->origin.z + tz * spawner->length; // Assuming Y-up

      f32 y = 0;
      Vec3 normal = {0, 1, 0};

      if (is_valid_location(spawner, type, world_x, world_z, &y, &normal)) {
        // Success - generate instance data
        Vec3 pos = {world_x, y, world_z};

        // Scale
        f32 s_base = random_range(type->min_scale, type->max_scale);
        Vec3 scale = {s_base, s_base, s_base};
        if (!type->lock_scale_xy) {
          scale.y = random_range(type->min_scale, type->max_scale);
        }

        // Rotation (Y-axis / Yaw)
        f32 rot = 0.0f;
        if (type->random_yaw) {
          rot = random_range(0, 360.0f);
        }

        // Emit
        callback(type->mesh_path, pos, scale, rot);
        placed_count++;
      }
    }
  }
}
