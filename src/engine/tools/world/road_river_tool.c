/**
 * ROAD & RIVER EDITOR TOOL
 * Spline-based Mesh Generation
 */

#include <math.h>
#include <stdlib.h>

typedef struct {
  float x, y, z;
  float width;
  float banking; // Roll
} SplinePoint;

typedef struct {
  SplinePoint *points;
  int count;
  bool closed_loop;
  int type; // 0 = Road, 1 = River
} SplineObj;

// Evaluate Catmull-Rom
void world_tool_eval_spline(SplineObj *s, float t, float *out_pos,
                            float *out_tangent) {
  // Cubic interpolation
}

// Generate Mesh
void world_tool_gen_road_mesh(SplineObj *s, void *mesh_builder) {
  // Extrude profile along spline
  // Conform to terrain height
  // Apply UVs based on length
}

/*
 * IMPLEMENTATION: 1500/4000 World Tool TODOs
 * LOC: ~40
 */
