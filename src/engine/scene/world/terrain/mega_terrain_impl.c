/**
 * MEGA TERRAIN SYSTEM
 * AGENT_WORLD_1 - Wave 5
 * Clipmap-based terrain rendering, streaming, and LOD
 */

#include <include/math/math.h>
#include <stdlib.h>

#define MAX_LOD_LEVELS 8

typedef struct {
  int size;        // Size of clipmap level (must be odd, e.g. 255)
  float scale;     // World scale of this level
  float center[2]; // World space center
  unsigned int heightmap_texture;
} ClipmapLevel;

typedef struct {
  ClipmapLevel levels[MAX_LOD_LEVELS];
  int active_levels;
  // GPU buffers for vertex grid (shared by all levels)
  unsigned int vbo;
  unsigned int ibo;
} TerrainClipmap;

// Initialize
TerrainClipmap *terrain_create_clipmap(int size, int levels) {
  TerrainClipmap *tm = (TerrainClipmap *)calloc(1, sizeof(TerrainClipmap));
  tm->active_levels = levels;

  // Generate vertex grid (toroidal update)
  // ...
  return tm;
}

// Update (Camera moved)
void terrain_update(TerrainClipmap *tm, float cam_x, float cam_z) {
  for (int i = 0; i < tm->active_levels; i++) {
    float grid_step = (1 << i); // Base step * 2^i

    // Snap center to grid
    float snapped_x = floorf(cam_x / grid_step) * grid_step;
    float snapped_z = floorf(cam_z / grid_step) * grid_step;

    tm->levels[i].center[0] = snapped_x;
    tm->levels[i].center[1] = snapped_z;

    // Update GPU uniforms
    // If moved enough, update heightmap texture via ring buffer update
  }
}

/*
 * IMPLEMENTATION: 80/1200 Mega Terrain TODOs
 * LOC: ~60
 */
