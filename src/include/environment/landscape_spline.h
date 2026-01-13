#ifndef LANDSCAPE_SPLINE_H
#define LANDSCAPE_SPLINE_H

#include "include/common.h"
#include "math/vec3.h"

#define SPLINE_MAX_POINTS 128

typedef struct {
  Vec3 position;
  Vec3 tangent_in;
  Vec3 tangent_out;
  f32 width;
  f32 roll;
} SplineControlPoint;

typedef struct {
  SplineControlPoint points[SPLINE_MAX_POINTS];
  u32 point_count;
  bool closed_loop;

  // Mesh generation
  f32 segment_length;          // Tessellation detail
  char mesh_profile_path[128]; // Path to profile mesh (optional)

} LandscapeSpline;

#ifdef __cplusplus
extern "C" {
#endif

LandscapeSpline *landscape_spline_create(void);
void landscape_spline_add_point(LandscapeSpline *spline, Vec3 pos, f32 width);
void landscape_spline_destroy(LandscapeSpline *spline);

// Evaluation
Vec3 landscape_spline_eval_position(LandscapeSpline *spline, f32 t);
Vec3 landscape_spline_eval_tangent(LandscapeSpline *spline, f32 t);

// Mesh Generation (Generates vertices for a strip/road)
void landscape_spline_generate_mesh(LandscapeSpline *spline,
                                    void (*callback)(Vec3 p1, Vec3 p2, Vec3 p3,
                                                     Vec3 normal));

#ifdef __cplusplus
}
#endif

#endif // LANDSCAPE_SPLINE_H
