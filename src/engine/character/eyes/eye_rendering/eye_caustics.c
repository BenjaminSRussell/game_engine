#include <character/eyes/eye_rendering/eye_rendering.h>
#include <common.h>
#include <math/vec3.h>

// Simulated corno-iris interface refraction/caustics facade
vec3_t calculate_cornea_caustics(vec3_t L, vec3_t N, float pupil_dilation) {
  // cornea caustics are caused by light refracting through the cornea
  // and hitting the iris.

  float dot_ln = fmaxf(0.0f, vec3_dot(L, N));

  // Fake caustic focus peak
  float caustic = powf(dot_ln, 8.0f) * (1.0f - pupil_dilation);

  // Return a yellowish caustic light spike
  return vec3(caustic * 1.0f, caustic * 0.9f, caustic * 0.5f);
}
// Disabled for build fix
#include <common.h>
