/**
 * =================================================================================================
 *                          PBR COOK-TORRANCE BRDF IMPLEMENTATION
 *                          Phase 2: Rendering Excellence
 * =================================================================================================
 *
 * PURPOSE: Physically-based rendering using Cook-Torrance BRDF model
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// PBR Material properties
typedef struct {
  float albedo[3];   // Base color (sRGB)
  float metallic;    // 0 = dielectric, 1 = metal
  float roughness;   // 0 = smooth, 1 = rough
  float ao;          // Ambient occlusion
  float normal[3];   // Normal map value
  float emission[3]; // Emissive color
  float emission_strength;
  float anisotropy; // Anisotropic roughness
  float clearcoat;  // Clearcoat layer strength
  float clearcoat_roughness;
  float subsurface; // Subsurface scattering strength
  float subsurface_color[3];
  float sheen; // Sheen for cloth
  float sheen_tint;
} PBRMaterial;

// Light source
typedef struct {
  float position[3];
  float color[3];
  float intensity;
  int type; // 0=point, 1=directional, 2=spot
  float direction[3];
  float inner_cone;
  float outer_cone;
} PBRLight;

// IBL (Image-Based Lighting) data
typedef struct {
  float *irradiance_sh[9]; // Spherical harmonics for diffuse
  uint32_t prefilter_cubemap;
  uint32_t brdf_lut;
  float max_reflection_lod;
} IBLData;

// -----------------------------------------------------------------------------
// Math Helpers
// -----------------------------------------------------------------------------

static inline float clamp01(float x) {
  return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

static inline float saturate(float x) { return clamp01(x); }

static inline float vec3_dot_inline(const float *a, const float *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void vec3_normalize_inline(float *v) {
  float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (len > 1e-6f) {
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
  }
}

static inline float lerp(float a, float b, float t) { return a + t * (b - a); }

static inline void vec3_lerp(float *out, const float *a, const float *b,
                             float t) {
  out[0] = lerp(a[0], b[0], t);
  out[1] = lerp(a[1], b[1], t);
  out[2] = lerp(a[2], b[2], t);
}

// -----------------------------------------------------------------------------
// GGX Normal Distribution Function (NDF)
// Trowbridge-Reitz GGX
// -----------------------------------------------------------------------------

float distribution_ggx(float n_dot_h, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float denom = n_dot_h * n_dot_h * (a2 - 1.0f) + 1.0f;
  denom = M_PI * denom * denom;
  return a2 / fmaxf(denom, 1e-7f);
}

// Anisotropic GGX
float distribution_ggx_aniso(float n_dot_h, float h_dot_x, float h_dot_y,
                             float roughness_x, float roughness_y) {
  float ax2 = roughness_x * roughness_x;
  float ay2 = roughness_y * roughness_y;
  float denom =
      h_dot_x * h_dot_x / ax2 + h_dot_y * h_dot_y / ay2 + n_dot_h * n_dot_h;
  return 1.0f / (M_PI * roughness_x * roughness_y * denom * denom);
}

// -----------------------------------------------------------------------------
// Geometry Function (Smith's method with GGX)
// -----------------------------------------------------------------------------

float geometry_schlick_ggx(float n_dot_v, float roughness) {
  float r = roughness + 1.0f;
  float k = (r * r) / 8.0f; // Direct lighting
  float denom = n_dot_v * (1.0f - k) + k;
  return n_dot_v / fmaxf(denom, 1e-7f);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
  float ggx1 = geometry_schlick_ggx(n_dot_v, roughness);
  float ggx2 = geometry_schlick_ggx(n_dot_l, roughness);
  return ggx1 * ggx2;
}

// -----------------------------------------------------------------------------
// Fresnel (Schlick approximation)
// -----------------------------------------------------------------------------

void fresnel_schlick(float *out, float cos_theta, const float *f0) {
  float one_minus_cos = 1.0f - cos_theta;
  float pow5 = one_minus_cos * one_minus_cos * one_minus_cos * one_minus_cos *
               one_minus_cos;
  out[0] = f0[0] + (1.0f - f0[0]) * pow5;
  out[1] = f0[1] + (1.0f - f0[1]) * pow5;
  out[2] = f0[2] + (1.0f - f0[2]) * pow5;
}

void fresnel_schlick_roughness(float *out, float cos_theta, const float *f0,
                               float roughness) {
  float one_minus_cos = 1.0f - cos_theta;
  float pow5 = one_minus_cos * one_minus_cos * one_minus_cos * one_minus_cos *
               one_minus_cos;
  float max_val = 1.0f - roughness;
  out[0] = f0[0] + (fmaxf(max_val, f0[0]) - f0[0]) * pow5;
  out[1] = f0[1] + (fmaxf(max_val, f0[1]) - f0[1]) * pow5;
  out[2] = f0[2] + (fmaxf(max_val, f0[2]) - f0[2]) * pow5;
}

// -----------------------------------------------------------------------------
// Cook-Torrance BRDF
// -----------------------------------------------------------------------------

void cook_torrance_brdf(float *out_radiance, const float *view_dir,
                        const float *light_dir, const float *normal,
                        const PBRMaterial *material, const float *light_color,
                        float light_intensity) {
  // Half vector
  float h[3] = {view_dir[0] + light_dir[0], view_dir[1] + light_dir[1],
                view_dir[2] + light_dir[2]};
  vec3_normalize_inline(h);

  // Dot products
  float n_dot_v = fmaxf(vec3_dot_inline(normal, view_dir), 0.0f);
  float n_dot_l = fmaxf(vec3_dot_inline(normal, light_dir), 0.0f);
  float n_dot_h = fmaxf(vec3_dot_inline(normal, h), 0.0f);
  float h_dot_v = fmaxf(vec3_dot_inline(h, view_dir), 0.0f);

  // F0 - reflectance at normal incidence
  float f0[3] = {0.04f, 0.04f, 0.04f}; // Dielectric default
  f0[0] = lerp(f0[0], material->albedo[0], material->metallic);
  f0[1] = lerp(f0[1], material->albedo[1], material->metallic);
  f0[2] = lerp(f0[2], material->albedo[2], material->metallic);

  // NDF
  float ndf = distribution_ggx(n_dot_h, material->roughness);

  // Geometry
  float g = geometry_smith(n_dot_v, n_dot_l, material->roughness);

  // Fresnel
  float f[3];
  fresnel_schlick(f, h_dot_v, f0);

  // Cook-Torrance specular
  float denom = 4.0f * n_dot_v * n_dot_l + 0.0001f;
  float specular_factor = ndf * g / denom;
  float specular[3] = {f[0] * specular_factor, f[1] * specular_factor,
                       f[2] * specular_factor};

  // kS (specular) = F, kD (diffuse) = 1 - kS
  float ks[3] = {f[0], f[1], f[2]};
  float kd[3] = {(1.0f - ks[0]) * (1.0f - material->metallic),
                 (1.0f - ks[1]) * (1.0f - material->metallic),
                 (1.0f - ks[2]) * (1.0f - material->metallic)};

  // Lambertian diffuse
  float diffuse[3] = {kd[0] * material->albedo[0] / M_PI,
                      kd[1] * material->albedo[1] / M_PI,
                      kd[2] * material->albedo[2] / M_PI};

  // Combine
  float radiance[3] = {light_color[0] * light_intensity,
                       light_color[1] * light_intensity,
                       light_color[2] * light_intensity};

  out_radiance[0] = (diffuse[0] + specular[0]) * radiance[0] * n_dot_l;
  out_radiance[1] = (diffuse[1] + specular[1]) * radiance[1] * n_dot_l;
  out_radiance[2] = (diffuse[2] + specular[2]) * radiance[2] * n_dot_l;
}

// -----------------------------------------------------------------------------
// Full PBR Lighting
// -----------------------------------------------------------------------------

void pbr_calculate_lighting(float *out_color, const float *world_pos,
                            const float *normal, const float *view_pos,
                            const PBRMaterial *material, const PBRLight *lights,
                            uint32_t light_count, const IBLData *ibl) {
  // View direction
  float view_dir[3] = {view_pos[0] - world_pos[0], view_pos[1] - world_pos[1],
                       view_pos[2] - world_pos[2]};
  vec3_normalize_inline(view_dir);

  // Accumulate from all lights
  float lo[3] = {0.0f, 0.0f, 0.0f};

  for (uint32_t i = 0; i < light_count; i++) {
    const PBRLight *light = &lights[i];
    float light_dir[3];
    float attenuation = 1.0f;

    if (light->type == 1) {
      // Directional
      light_dir[0] = -light->direction[0];
      light_dir[1] = -light->direction[1];
      light_dir[2] = -light->direction[2];
    } else {
      // Point/Spot
      light_dir[0] = light->position[0] - world_pos[0];
      light_dir[1] = light->position[1] - world_pos[1];
      light_dir[2] = light->position[2] - world_pos[2];

      float dist =
          sqrtf(light_dir[0] * light_dir[0] + light_dir[1] * light_dir[1] +
                light_dir[2] * light_dir[2]);
      attenuation = 1.0f / (dist * dist + 0.0001f);

      vec3_normalize_inline(light_dir);

      // Spot light cone
      if (light->type == 2) {
        float cos_theta = vec3_dot_inline(light_dir, light->direction);
        float epsilon = light->inner_cone - light->outer_cone;
        float spot = saturate((cos_theta - light->outer_cone) / epsilon);
        attenuation *= spot;
      }
    }

    // Cook-Torrance BRDF
    float contrib[3];
    cook_torrance_brdf(contrib, view_dir, light_dir, normal, material,
                       light->color, light->intensity * attenuation);

    lo[0] += contrib[0];
    lo[1] += contrib[1];
    lo[2] += contrib[2];
  }

  // Ambient (simple or IBL)
  float ambient[3];
  if (ibl) {
    // IBL ambient - simplified SH lookup
    float f0[3] = {0.04f, 0.04f, 0.04f};
    f0[0] = lerp(f0[0], material->albedo[0], material->metallic);
    f0[1] = lerp(f0[1], material->albedo[1], material->metallic);
    f0[2] = lerp(f0[2], material->albedo[2], material->metallic);

    float n_dot_v = fmaxf(vec3_dot_inline(normal, view_dir), 0.0f);
    float f[3];
    fresnel_schlick_roughness(f, n_dot_v, f0, material->roughness);

    float kd[3] = {(1.0f - f[0]) * (1.0f - material->metallic),
                   (1.0f - f[1]) * (1.0f - material->metallic),
                   (1.0f - f[2]) * (1.0f - material->metallic)};

    // Diffuse irradiance (simplified)
    float irradiance[3] = {0.3f, 0.3f, 0.4f};
    float diffuse[3] = {irradiance[0] * material->albedo[0],
                        irradiance[1] * material->albedo[1],
                        irradiance[2] * material->albedo[2]};

    ambient[0] = kd[0] * diffuse[0] * material->ao;
    ambient[1] = kd[1] * diffuse[1] * material->ao;
    ambient[2] = kd[2] * diffuse[2] * material->ao;
  } else {
    // Simple ambient
    ambient[0] = 0.03f * material->albedo[0] * material->ao;
    ambient[1] = 0.03f * material->albedo[1] * material->ao;
    ambient[2] = 0.03f * material->albedo[2] * material->ao;
  }

  // Final color
  out_color[0] =
      ambient[0] + lo[0] + material->emission[0] * material->emission_strength;
  out_color[1] =
      ambient[1] + lo[1] + material->emission[1] * material->emission_strength;
  out_color[2] =
      ambient[2] + lo[2] + material->emission[2] * material->emission_strength;
}

// -----------------------------------------------------------------------------
// Clearcoat Layer
// -----------------------------------------------------------------------------

void pbr_add_clearcoat(float *color, const float *normal, const float *view_dir,
                       const float *light_dir, float clearcoat,
                       float clearcoat_roughness) {
  if (clearcoat <= 0.0f)
    return;

  float h[3] = {view_dir[0] + light_dir[0], view_dir[1] + light_dir[1],
                view_dir[2] + light_dir[2]};
  vec3_normalize_inline(h);

  float n_dot_h = fmaxf(vec3_dot_inline(normal, h), 0.0f);
  float h_dot_v = fmaxf(vec3_dot_inline(h, view_dir), 0.0f);

  // Clearcoat uses a fixed IOR of 1.5 -> F0 = 0.04
  float f0 = 0.04f;
  float fresnel = f0 + (1.0f - f0) * powf(1.0f - h_dot_v, 5.0f);

  // GGX for clearcoat
  float d = distribution_ggx(n_dot_h, clearcoat_roughness);

  float coat_specular = fresnel * d * clearcoat;

  color[0] += coat_specular;
  color[1] += coat_specular;
  color[2] += coat_specular;
}

// -----------------------------------------------------------------------------
// Subsurface Scattering (Simplified)
// -----------------------------------------------------------------------------

void pbr_add_subsurface(float *color, const float *normal,
                        const float *view_dir, const float *light_dir,
                        float subsurface, const float *sss_color,
                        const float *light_color, float light_intensity) {
  if (subsurface <= 0.0f)
    return;

  // Wrap lighting for SSS
  float n_dot_l = vec3_dot_inline(normal, light_dir);
  float wrap = 0.5f;
  float wrapped = (n_dot_l + wrap) / (1.0f + wrap);
  wrapped = fmaxf(wrapped, 0.0f);

  // SSS contribution
  float sss_contrib = wrapped * subsurface * light_intensity;

  color[0] += sss_color[0] * light_color[0] * sss_contrib;
  color[1] += sss_color[1] * light_color[1] * sss_contrib;
  color[2] += sss_color[2] * light_color[2] * sss_contrib;
}

// -----------------------------------------------------------------------------
// Material Defaults
// -----------------------------------------------------------------------------

PBRMaterial pbr_material_default(void) {
  PBRMaterial mat = {0};
  mat.albedo[0] = 1.0f;
  mat.albedo[1] = 1.0f;
  mat.albedo[2] = 1.0f;
  mat.metallic = 0.0f;
  mat.roughness = 0.5f;
  mat.ao = 1.0f;
  mat.normal[0] = 0.0f;
  mat.normal[1] = 1.0f;
  mat.normal[2] = 0.0f;
  return mat;
}

PBRMaterial pbr_material_metal(float roughness) {
  PBRMaterial mat = pbr_material_default();
  mat.metallic = 1.0f;
  mat.roughness = roughness;
  return mat;
}

PBRMaterial pbr_material_plastic(float r, float g, float b, float roughness) {
  PBRMaterial mat = pbr_material_default();
  mat.albedo[0] = r;
  mat.albedo[1] = g;
  mat.albedo[2] = b;
  mat.metallic = 0.0f;
  mat.roughness = roughness;
  return mat;
}

PBRMaterial pbr_material_skin(void) {
  PBRMaterial mat = pbr_material_default();
  mat.albedo[0] = 0.9f;
  mat.albedo[1] = 0.7f;
  mat.albedo[2] = 0.6f;
  mat.roughness = 0.4f;
  mat.subsurface = 0.5f;
  mat.subsurface_color[0] = 1.0f;
  mat.subsurface_color[1] = 0.3f;
  mat.subsurface_color[2] = 0.2f;
  return mat;
}

PBRMaterial pbr_material_car_paint(float r, float g, float b) {
  PBRMaterial mat = pbr_material_default();
  mat.albedo[0] = r;
  mat.albedo[1] = g;
  mat.albedo[2] = b;
  mat.metallic = 0.9f;
  mat.roughness = 0.1f;
  mat.clearcoat = 1.0f;
  mat.clearcoat_roughness = 0.05f;
  return mat;
}
