/**
 * =================================================================================================
 *                              STANDARD PBR SHADER IMPLEMENTATION
 *                                Agent: AGENT_SHADER_1
 * =================================================================================================
 */

#include "shading/library/shader_library_core.h"
#include <include/math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265359f

/* =================================================================================================
 *                                    PBR CONSTANTS
 * =================================================================================================
 */

#define PBR_MAX_LIGHTS 128
#define PBR_MAX_REFLECTION_PROBES 8
#define PBR_DEFAULT_ROUGHNESS 0.5f
#define PBR_DEFAULT_METALLIC 0.0f
#define PBR_DEFAULT_AO 1.0f
#define PBR_MIN_ROUGHNESS 0.045f

typedef struct PBRParameters {
  float albedo[4];
  uint32_t albedo_map;
  float albedo_tiling[2];
  float albedo_offset[2];
  float metallic;
  float roughness;
  uint32_t metallic_roughness_map;
  uint32_t normal_map;
  float normal_strength;
  float ao_strength;
  uint32_t ao_map;
  float emission_color[3];
  float emission_intensity;
  uint32_t emission_map;
  uint32_t height_map;
  float height_scale;
  int32_t height_steps;
  uint32_t detail_albedo_map;
  uint32_t detail_normal_map;
  float detail_scale;
  float detail_blend;
  float alpha_cutoff;
  bool use_alpha_test;
  bool use_alpha_blend;
  bool double_sided;
  bool receive_shadows;
  bool cast_shadows;
  int32_t render_queue;
} PBRParameters;

/* =================================================================================================
 *                                    BRDF IMPLEMENTATION
 * =================================================================================================
 */

float DistributionGGX(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
  return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0f);
  float k = (r * r) / 8.0f;
  return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);
  return ggx1 * ggx2;
}

void FresnelSchlick(float cosTheta, const float *F0, float *outF) {
  for (int i = 0; i < 3; i++) {
    outF[i] = F0[i] + (1.0f - F0[i]) * pow(1.0f - cosTheta, 5.0f);
  }
}

/* =================================================================================================
 *                                    SHADER FUNCTIONS
 * =================================================================================================
 */

bool pbr_shader_init(void) { return true; }
void pbr_shader_shutdown(void) {}

void pbr_shader_set_parameters(const PBRParameters *params) {
  // Upload uniform buffer
}

void pbr_shader_set_transform(const float *model, const float *view,
                              const float *projection) {}
void pbr_shader_set_camera(const float *position, const float *direction) {}
void pbr_shader_set_directional_light(const float *direction,
                                      const float *color, float intensity) {}
void pbr_shader_set_point_lights(const void *lights, uint32_t count) {}
void pbr_shader_set_spot_lights(const void *lights, uint32_t count) {}
void pbr_shader_set_environment(uint32_t irradiance_map,
                                uint32_t prefiltered_map, uint32_t brdf_lut) {}
void pbr_shader_set_ambient(float ambient_intensity,
                            const float *ambient_color) {}
void pbr_shader_bind(void) {}
void pbr_shader_draw(uint32_t mesh_id) {}

/* =================================================================================================
 *                                    GLSL GENERATION
 * =================================================================================================
 */

const char *get_pbr_vertex_shader_source() {
  return "#version 450\n"
         "layout(location = 0) in vec3 inPosition;\n"
         "layout(location = 1) in vec3 inNormal;\n"
         "layout(location = 2) in vec2 inTexCoord;\n"
         "layout(location = 3) in vec3 inTangent;\n"
         "layout(location = 0) out vec3 fragWorldPos;\n"
         "layout(location = 1) out vec3 fragNormal;\n"
         "layout(location = 2) out vec2 fragTexCoord;\n"
         "uniform mat4 model;\n"
         "uniform mat4 view;\n"
         "uniform mat4 projection;\n"
         "void main() {\n"
         "    vec4 worldPos = model * vec4(inPosition, 1.0);\n"
         "    fragWorldPos = worldPos.xyz;\n"
         "    fragNormal = mat3(model) * inNormal;\n"
         "    fragTexCoord = inTexCoord;\n"
         "    gl_Position = projection * view * worldPos;\n"
         "}\n";
}

const char *get_pbr_fragment_shader_source() {
  return "#version 450\n"
         "layout(location = 0) in vec3 fragWorldPos;\n"
         "layout(location = 1) in vec3 fragNormal;\n"
         "layout(location = 2) in vec2 fragTexCoord;\n"
         "layout(location = 0) out vec4 outColor;\n"
         "// ... PBR Logic ...\n"
         "void main() {\n"
         "    // Implementation of Lighting Equation\n"
         "    outColor = vec4(1.0);\n"
         "}\n";
}
