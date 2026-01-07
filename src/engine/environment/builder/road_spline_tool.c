#include "environment/builder/road_spline_tool.h"
#include <include/math/math.h>
#include <stdlib.h>

typedef struct {
    float points[100][3];
    int count;
} Spline;

void road_spline_extrude_mesh(Spline *spline, float width, void **out_mesh) {
    // Generate mesh along spline
    for (int i = 0; i < spline->count - 1; i++) {
        // Compute tangent, bitangent
        // Left/Right vertices = current_point +/- bitangent * width
        // Add Triangle Strip
        // UVs based on length accumulated
    }
}

void road_spline_conform_terrain(Spline *spline, void *terrain) {
    // Flatten terrain under road
    // For each point on spline:
    //   Scan terrain heightmap in radius
    //   Lerp height towards road height
    //   Add falloff (shoulders)
}

void road_spline_generate_intersections(void *spline_list, int *intersections) {
    // Detect where splines cross
    // 1. Find intersection point
    // 2. Remove spline segments near intersection
    // 3. Generate specialized junction mesh (T-junction/Cross)
    // 4. Stitch UVs
}
