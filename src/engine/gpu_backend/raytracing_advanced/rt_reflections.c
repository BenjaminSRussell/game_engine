// Batch stub implementations for raytracing systems
#include "renderer/raytracing_advanced/rt_reflections.h"
void rt_reflections_init() {}
void rt_reflections_trace(void *scene, void *camera, void *output) {}

#include "renderer/raytracing_advanced/path_tracer.h"  
void path_tracer_init() {}
void path_tracer_render(void *scene, void *output, int samples) {}

#include "renderer/raytracing_advanced/denoising.h"
void denoiser_init() {}
void denoiser_filter(void *noisy, void *albedo, void *normals, void *output) {}

#include "renderer/raytracing_advanced/acceleration_structure.h"
void accel_build_bvh(void *triangles, int count, void *bvh_output) {}
void accel_refit(void *bvh, void *updated_triangles) {}

#include "renderer/raytracing_advanced/shader_binding_table.h"
void sbt_create(void *ray_gen, void *miss, void *hit, void *sbt_output) {}
