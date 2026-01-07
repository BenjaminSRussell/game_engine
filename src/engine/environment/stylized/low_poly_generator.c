#include "environment/stylized/low_poly_generator.h"
#include <stdlib.h>
#include <include/math/math.h>

typedef struct {
    float x, y, z;
} Vec3;

void low_poly_gen_flat_shade(void *mesh) {
    // Split vertices for flat shading
    // For each triangle:
    //   Duplicate vertices
    //   Compute face normal: normalize(cross(v1-v0, v2-v0))
    //   Assign normal to all 3 vertices
}

void low_poly_gen_jitter(void *mesh, float intensity) {
    // Randomize vertex positions
    // For each vertex v:
    //   v.x += random(-1, 1) * intensity
    //   v.y += random(-1, 1) * intensity
    //   v.z += random(-1, 1) * intensity
}

void low_poly_gen_triangulate(void *mesh) {
    // Ensure consistent triangulation for quads
    // For each quad (v0, v1, v2, v3):
    //   d1 = dist(v0, v2)
    //   d2 = dist(v1, v3)
    //   if (d1 < d2) split (v0, v2) else split (v1, v3)
}
