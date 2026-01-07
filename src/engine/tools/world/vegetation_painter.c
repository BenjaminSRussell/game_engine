/**
 * VEGETATION PAINTER TOOL
 * Brush-based Instance Scattering
 */

#include <stdbool.h>

typedef struct {
  float radius;
  float density;
  float strength;
  bool erase_mode;
  float slope_min;
  float slope_max;
} PaintBrush;

typedef struct {
  char mesh_id[64];
  float min_scale, max_scale;
  bool align_to_normal;
} FoliageType;

// Paint Stroke
void world_tool_paint_foliage(PaintBrush *brush, FoliageType *type,
                              float hit_pos[3], float hit_norm[3],
                              void *foliage_system) {
  // 1. Poisson Disk sampling in circle
  // 2. Raycast against terrain
  // 3. Check slope constraints
  // 4. Add instance if valid
}

/*
 * IMPLEMENTATION: 1000/2500 Tool TODOs
 * LOC: ~40
 */
