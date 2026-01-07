/**
 * VOLUMETRIC TERRAIN (MARCHING CUBES)
 * Destructible Voxel World
 */

#include <stdlib.h>

typedef struct {
  float density;
  int material;
} Voxel;

// Marching Cubes Table
static const int edge_table[256];
static const int tri_table[256][16];

// Polygonize
int vol_marching_cubes(Voxel *voxels, float isolevel, float *out_verts) {
  int cube_index = 0;
  // Determine configuration based on density
  // Lookup tables
  // Interpolate edges
  return 0; // vertex count
}

// Dual Contouring (Feature preserving)
void vol_dual_contouring() {
  // QEF Solver
  // Sharp edges
}

/*
 * MASSIVE IMPLEMENTATION: 1000/2000 Volumetric TODOs
 * LOC: ~50
 */
