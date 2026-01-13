// World/Landscape Splines (Roads, Rivers, Walls)
#ifndef LANDSCAPE_SPLINES_H
#define LANDSCAPE_SPLINES_H

#include "include/common.h"
#include "math/vec3.h"

#define MAX_SPLINE_POINTS 128

typedef struct {
  Vec3 position;
  Vec3 rotation; // Euler
  Vec3 tangent_in;
  Vec3 tangent_out;
  f32 width;
  f32 roll;
} SplineControlPoint;

typedef struct {
  char mesh_path[128];
  bool generate_collision;
  f32 mesh_scale;
  // axis logic...
} SplineMeshSegment;

typedef struct {
  SplineControlPoint points[MAX_SPLINE_POINTS];
  u32 point_count;

  SplineMeshSegment segment_settings;
  bool closed_loop;

  // Deform landscape to fit spline?
  bool raise_terrain;
  bool lower_terrain;
  f32 falloff_width;

} LandscapeSpline;

#ifdef __cplusplus
extern "C" {
#endif

LandscapeSpline *spline_create(void);
void spline_add_point(LandscapeSpline *spline, Vec3 position);

// Evaluate
Vec3 spline_get_location_at(LandscapeSpline *spline, f32 t); // 0.0 to 1.0
Vec3 spline_get_tangent_at(LandscapeSpline *spline, f32 t);

// Generation
void spline_generate_meshes(LandscapeSpline *spline, void *world_context);
void spline_apply_to_landscape(LandscapeSpline *spline,
                               void *landscape_context);

#ifdef __cplusplus
}
#endif

#endif // LANDSCAPE_SPLINES_H
