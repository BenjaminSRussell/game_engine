/**
 * BIOME DESIGNER TOOL
 * Node-based Rule Logic
 */

#include <stdlib.h>

typedef struct {
  char name[64];
  float temp_range[2];
  float rain_range[2];
  float height_range[2];
  char terrain_texture[64];
  char foliage_set[64];
} BiomeRule;

// Visualize Distribution
void world_tool_viz_biome_map(BiomeRule *rules, int count, int size,
                              unsigned int *out_pixels) {
  // Generate Voronoi / Noise map
  // Colorize based on rules
  // Output to texture for preview
}

/*
 * IMPLEMENTATION: 800/2000 Tool TODOs
 * LOC: ~30
 */
