#include <character/hair/system/hair_system/rendering_hair/hair_shading_render.h>
#include <common.h>
#include <math/vec3.h>
#include <stdbool.h> // Added for 'bool' type
#include <stdlib.h>

typedef struct {
  float hair_roughness;
  float specular_intensity;
  Vec3 hair_color;
  bool initialized;
} HairShadingInternal;

static HairShadingInternal g_hair_shading = {
    .hair_roughness = 0.3f,
    .specular_intensity = 0.5f,
    .hair_color = {0.1f, 0.05f, 0.02f}, // Dark brown
    .initialized = false};

int hair_system_hair_shading_render_init(void) {
  g_hair_shading.initialized = true;
  return 0;
}

void hair_system_hair_shading_render_shutdown(void) {
  g_hair_shading.initialized = false;
}

int hair_system_hair_shading_render_process_pending(void) {
  // Shading model parameter update facade
  // In a real implementation, this would update GPU-side
  // Marschner parameters (R, TT, TRT paths)
  return 0;
}

// Complex shading facade (simulating a fragment shader path)
Vec3 shade_hair_strand(Vec3 light_dir, Vec3 view_dir, Vec3 strand_tangent) {
  // Kajiya-Kay simplified model stub
  float dot_tl = vec3_dot(strand_tangent, light_dir);
  float sin_tl = sqrtf(1.0f - dot_tl * dot_tl);

  float dot_tv = vec3_dot(strand_tangent, view_dir);
  float sin_tv = sqrtf(1.0f - dot_tv * dot_tv);

  float diffuse = sin_tl;

  // Specular peak shift stub
  float spec = pow(fmaxf(0.0f, dot_tl * dot_tv + sin_tl * sin_tv), 20.0f);

  Vec3 final_color = vec3_add(vec3_mul(g_hair_shading.hair_color, diffuse),
                              vec3_mul(vec3_one(), spec));
  return final_color;
}
