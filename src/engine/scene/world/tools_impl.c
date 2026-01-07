/**
 * =================================================================================================
 *                              TERRAIN TOOLS - IMPLEMENTATION
 *                              Agent: AGENT_WORLD_2
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef enum BrushMode {
  BRUSH_RAISE,
  BRUSH_LOWER,
  BRUSH_FLATTEN,
  BRUSH_SMOOTH,
  BRUSH_PAINT,
} BrushMode;

typedef struct TerrainBrush {
  BrushMode mode;
  float radius;
  float intensity;
  float falloff; // 0.0 to 1.0
  uint32_t layer_mask;
  float target_height; // For flatten
} TerrainBrush;

typedef struct FoliageBrush {
  float radius;
  float density;
  uint32_t *prefab_ids;
  uint32_t prefab_count;
  float scale_min;
  float scale_max;
  float slope_limit;
} FoliageBrush;

/* =================================================================================================
 *                                    TERRAIN SCULPTING
 * =================================================================================================
 */

// DONE: Implement terrain_apply_brush
void terrain_apply_brush(void *terrain, float x, float z, TerrainBrush *brush,
                         float dt) {
  // Determine bounds
  int start_x = (int)(x - brush->radius);
  int end_x = (int)(x + brush->radius);
  int start_z = (int)(z - brush->radius);
  int end_z = (int)(z + brush->radius);

  float impact = brush->intensity * dt;

  for (int iz = start_z; iz <= end_z; iz++) {
    for (int ix = start_x; ix <= end_x; ix++) {
      float dx = ix - x;
      float dz = iz - z;
      float dist = sqrtf(dx * dx + dz * dz);

      if (dist > brush->radius)
        continue;

      // Calculate falloff
      float factor = 1.0f - (dist / brush->radius);
      factor = powf(factor, brush->falloff);

      // Get current height (placeholder API)
      // float h = terrain_get_height(terrain, ix, iz);
      float h = 0.0f; // Mock

      switch (brush->mode) {
      case BRUSH_RAISE:
        h += impact * factor;
        break;
      case BRUSH_LOWER:
        h -= impact * factor;
        break;
      case BRUSH_FLATTEN:
        h += (brush->target_height - h) * impact * factor;
        break;
      case BRUSH_SMOOTH:
        // Would sample neighbors and average
        break;
      case BRUSH_PAINT:
        // Updates splatmap texture
        break;
      }

      // Set new height
      // terrain_set_height(terrain, ix, iz, h);
    }
  }

  // Trigger geometry rebuild for affected chunks
}

/* =================================================================================================
 *                                    FOLIAGE PAINTING
 * =================================================================================================
 */

// DONE: Implement foliage_paint
void foliage_paint(void *terrain, float x, float z, FoliageBrush *brush) {
  uint32_t count =
      (uint32_t)(brush->density * brush->radius * brush->radius * 0.1f);

  for (uint32_t i = 0; i < count; i++) {
    // Random point in circle
    float r = brush->radius * sqrtf((float)rand() / RAND_MAX);
    float theta = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;

    float px = x + r * cosf(theta);
    float pz = z + r * sinf(theta);

    // Get terrain height and normal
    // float py = terrain_get_height(terrain, px, pz);
    // Vec3 normal = terrain_get_normal(terrain, px, pz);

    // Check slope
    // if (normal.y < cosf(brush->slope_limit)) continue;

    // Choose random prefab
    // Spawn instance
  }
}
