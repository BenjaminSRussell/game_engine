#include <common.h>
#include <math/vec3.h>
#include <stdlib.h>

typedef struct {
  float wind_strength;
  float wind_speed;
  Vec3 wind_direction;
  float grass_height;
  uint32_t density_per_meter;
} GrassSystemParams;

static GrassSystemParams g_grass_params = {.wind_strength = 0.5f,
                                           .wind_speed = 1.2f,
                                           .wind_direction = {1.0f, 0.0f, 0.0f},
                                           .grass_height = 0.6f,
                                           .density_per_meter = 16};

void grass_init(void) {
  // Metal buffer allocation stub for instances
}

void grass_update(float delta_time) {
  // Update wind animation phase
}

void grass_render(void) {
  // GPU Instancing Facade
  // 1. Culling against frustum
  // 2. Sorting based on LOD distance
  // 3. Indirect draw call for blade geometry
  // 4. Vertex shader-based wind displacement
}

void grass_set_wind(float strength, Vec3 direction) {
  g_grass_params.wind_strength = strength;
  g_grass_params.wind_direction = direction;
}
