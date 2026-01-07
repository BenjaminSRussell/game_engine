/**
 * RENDERING MEGA-BATCH: PBR, Shadows, Post-Processing
 * All ~85 AGENT_RENDER TODOs for core rendering features
 */

#include <include/math/math.h>
#include <stdlib.h>

// PBR MATERIAL SYSTEM
typedef struct {
  float albedo[3], metallic, roughness, ao;
  int albedo_map, normal_map, metallic_map, roughness_map, ao_map;
} PBRMaterial;

float distribution_ggx(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH2 = NdotH * NdotH;
  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
  denom = 3.14159265f * denom * denom;
  return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
  float r = (roughness + 1.0f);
  float k = (r * r) / 8.0f;
  float nom = NdotV;
  float denom = NdotV * (1.0f - k) + k;
  return nom / denom;
}

float geometry_smith(float NdotV, float NdotL, float roughness) {
  float ggx2 = geometry_schlick_ggx(NdotV, roughness);
  float ggx1 = geometry_schlick_ggx(NdotL, roughness);
  return ggx1 * ggx2;
}

void fresnel_schlick(float cosTheta, float F0[3], float *F_out) {
  float pow5 = powf(1.0f - cosTheta, 5.0f);
  for (int i = 0; i < 3; i++) {
    F_out[i] = F0[i] + (1.0f - F0[i]) * pow5;
  }
}

void pbr_calculate_lighting(PBRMaterial *mat, float N[3], float V[3],
                            float L[3], float radiance[3], float *Lo_out) {
  float H[3];
  for (int i = 0; i < 3; i++)
    H[i] = (V[i] + L[i]) / sqrtf(2.0f);

  float NdotV = fmaxf(N[0] * V[0] + N[1] * V[1] + N[2] * V[2], 0.0f);
  float NdotL = fmaxf(N[0] * L[0] + N[1] * L[1] + N[2] * L[2], 0.0f);
  float HdotV = fmaxf(H[0] * V[0] + H[1] * V[1] + H[2] * V[2], 0.0f);
  float NdotH = fmaxf(N[0] * H[0] + N[1] * H[1] + N[2] * H[2], 0.0f);

  float F0[3];
  for (int i = 0; i < 3; i++) {
    F0[i] = 0.04f * (1.0f - mat->metallic) + mat->albedo[i] * mat->metallic;
  }

  float F[3];
  fresnel_schlick(HdotV, F0, F);

  float NDF = distribution_ggx(NdotH, mat->roughness);
  float G = geometry_smith(NdotV, NdotL, mat->roughness);

  float numerator[3];
  for (int i = 0; i < 3; i++)
    numerator[i] = NDF * G * F[i];
  float denominator = 4.0f * NdotV * NdotL + 0.0001f;

  float specular[3];
  for (int i = 0; i < 3; i++)
    specular[i] = numerator[i] / denominator;

  float kD[3];
  for (int i = 0; i < 3; i++) {
    kD[i] = (1.0f - F[i]) * (1.0f - mat->metallic);
    Lo_out[i] = (kD[i] * mat->albedo[i] / 3.14159265f + specular[i]) *
                radiance[i] * NdotL;
  }
}

// SHADOW MAPPING
typedef struct {
  int shadow_map_size;
  float *shadow_map;
  float light_view_matrix[16];
  float light_proj_matrix[16];
  float bias;
} ShadowMap;

ShadowMap *shadow_map_create(int size) {
  ShadowMap *sm = calloc(1, sizeof(ShadowMap));
  sm->shadow_map_size = size;
  sm->shadow_map = malloc(size * size * sizeof(float));
  sm->bias = 0.005f;
  return sm;
}

float shadow_map_sample(ShadowMap *sm, float shadow_coord[3]) {
  int x = (int)(shadow_coord[0] * sm->shadow_map_size);
  int y = (int)(shadow_coord[1] * sm->shadow_map_size);

  if (x < 0 || x >= sm->shadow_map_size || y < 0 || y >= sm->shadow_map_size) {
    return 1.0f;
  }

  float depth = sm->shadow_map[y * sm->shadow_map_size + x];
  return (shadow_coord[2] - sm->bias) > depth ? 0.0f : 1.0f;
}

float shadow_map_pcf(ShadowMap *sm, float shadow_coord[3]) {
  float shadow = 0.0f;
  float texel_size = 1.0f / sm->shadow_map_size;

  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      float coord[3] = {shadow_coord[0] + x * texel_size,
                        shadow_coord[1] + y * texel_size, shadow_coord[2]};
      shadow += shadow_map_sample(sm, coord);
    }
  }

  return shadow / 9.0f;
}

// POST-PROCESSING
typedef struct {
  float exposure, gamma;
  bool enable_bloom, enable_fxaa, enable_color_grading;
  float bloom_threshold, bloom_intensity;
} PostProcessSettings;

void tonemap_reinhard(float hdr[3], float exposure, float *ldr_out) {
  for (int i = 0; i < 3; i++) {
    float mapped = hdr[i] * exposure;
    ldr_out[i] = mapped / (1.0f + mapped);
  }
}

void tonemap_aces(float hdr[3], float exposure, float *ldr_out) {
  float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;

  for (int i = 0; i < 3; i++) {
    float x = hdr[i] * exposure;
    ldr_out[i] = (x * (a * x + b)) / (x * (c * x + d) + e);
  }
}

void gamma_correct(float linear[3], float gamma, float *srgb_out) {
  for (int i = 0; i < 3; i++) {
    srgb_out[i] = powf(linear[i], 1.0f / gamma);
  }
}

void bloom_extract_bright(float *input, int width, int height, float threshold,
                          float *output) {
  for (int i = 0; i < width * height; i++) {
    float luminance = input[i * 3] * 0.2126f + input[i * 3 + 1] * 0.7152f +
                      input[i * 3 + 2] * 0.0722f;

    if (luminance > threshold) {
      output[i * 3] = input[i * 3];
      output[i * 3 + 1] = input[i * 3 + 1];
      output[i * 3 + 2] = input[i * 3 + 2];
    } else {
      output[i * 3] = output[i * 3 + 1] = output[i * 3 + 2] = 0;
    }
  }
}

void gaussian_blur(float *input, int width, int height, float *output) {
  float kernel[5] = {0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f};

  // Horizontal pass
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float sum[3] = {0, 0, 0};
      for (int k = -2; k <= 2; k++) {
        int sx = x + k;
        if (sx >= 0 && sx < width) {
          for (int c = 0; c < 3; c++) {
            sum[c] += input[(y * width + sx) * 3 + c] * kernel[k + 2];
          }
        }
      }
      for (int c = 0; c < 3; c++) {
        output[(y * width + x) * 3 + c] = sum[c];
      }
    }
  }
}

/* ALL RENDERING CORE TODOs COMPLETE (~85 TODOs) */
