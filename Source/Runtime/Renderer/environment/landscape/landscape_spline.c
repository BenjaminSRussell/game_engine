#include "include/environment/landscape_spline.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

LandscapeSpline *landscape_spline_create(void) {
  LandscapeSpline *spline =
      (LandscapeSpline *)calloc(1, sizeof(LandscapeSpline));
  if (spline) {
    spline->segment_length = 1.0f;
  }
  return spline;
}

void landscape_spline_destroy(LandscapeSpline *spline) {
  if (spline)
    free(spline);
}

void landscape_spline_add_point(LandscapeSpline *spline, Vec3 pos, f32 width) {
  if (!spline || spline->point_count >= SPLINE_MAX_POINTS)
    return;

  SplineControlPoint *pt = &spline->points[spline->point_count++];
  pt->position = pos;
  pt->width = width;

  // Auto-calculate tangents (Catmull-Rom style for now, simplified)
  // If we have enough points, update previous tangents
  if (spline->point_count >= 3) {
    // Update i-1 based on i-2 and i
    // Tangent = (Next - Prev) * 0.5
    u32 i = spline->point_count - 1;
    Vec3 prev = spline->points[i - 2].position;
    Vec3 curr = spline->points[i - 1].position;
    Vec3 next = spline->points[i].position;

    // Simple smoothing
    // This is a placeholder for full bezier/catmull-rom
  }
}

// Cubic Hermite Spline evaluation
static Vec3 hermite_eval(Vec3 p0, Vec3 p1, Vec3 m0, Vec3 m1, f32 t) {
  f32 t2 = t * t;
  f32 t3 = t2 * t;

  f32 h00 = 2 * t3 - 3 * t2 + 1;
  f32 h10 = t3 - 2 * t2 + t;
  f32 h01 = -2 * t3 + 3 * t2;
  f32 h11 = t3 - t2;

  Vec3 res;
  res.x = h00 * p0.x + h10 * m0.x + h01 * p1.x + h11 * m1.x;
  res.y = h00 * p0.y + h10 * m0.y + h01 * p1.y + h11 * m1.y;
  res.z = h00 * p0.z + h10 * m0.z + h01 * p1.z + h11 * m1.z;
  return res;
}

Vec3 landscape_spline_eval_position(LandscapeSpline *spline, f32 t) {
  if (!spline || spline->point_count < 2)
    return (Vec3){0, 0, 0};

  // Map t [0, 1] to segment indices
  // Simplification: linear interpolation for stub
  f32 total_segments = (f32)(spline->point_count - 1);
  f32 segment_t = t * total_segments;
  u32 index = (u32)segment_t;
  f32 local_t = segment_t - index;

  if (index >= spline->point_count - 1) {
    return spline->points[spline->point_count - 1].position;
  }

  Vec3 p0 = spline->points[index].position;
  Vec3 p1 = spline->points[index + 1].position;

  // Linear lerp for now
  Vec3 res;
  res.x = p0.x + local_t * (p1.x - p0.x);
  res.y = p0.y + local_t * (p1.y - p0.y);
  res.z = p0.z + local_t * (p1.z - p0.z);
  return res;
}

Vec3 landscape_spline_eval_tangent(LandscapeSpline *spline, f32 t) {
  if (!spline || spline->point_count < 2)
    return (Vec3){0, 1, 0};

  // Stub tangent
  return (Vec3){1, 0, 0};
}

void landscape_spline_generate_mesh(LandscapeSpline *spline,
                                    void (*callback)(Vec3 p1, Vec3 p2, Vec3 p3,
                                                     Vec3 normal)) {
  // Generate mesh strip (road) logic would go here
  // Walk along spline, extrude width/profile
  if (!spline || !callback)
    return;
}
