#include "renderer/raytracing_advanced/acceleration_structure.h"
void accel_init() {}
void accel_build_bvh(void *triangles, int count, void *bvh_output) {}
void accel_refit(void *bvh, void *updated_triangles) {}
void accel_query_ray(void *bvh, float origin[3], float direction[3], void *hit_output) {}
