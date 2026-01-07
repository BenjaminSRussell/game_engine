/**
 * PBR & STYLIZED SHADER SYSTEM
 * AGENT_SHADER_1 - Wave 4
 * Physically Based Rendering equations and Stylized rendering options
 */

#include <include/math/math.h>

// PBR Helper Functions
float distribution_ggx(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH2 = NdotH * NdotH;
  float num = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = 3.14159 * denom * denom;
  return num / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;
  float num = NdotV;
  float denom = NdotV * (1.0 - k) + k;
  return num / denom;
}

// Fresnel
void fresnel_schlick(float cosTheta, float *F0, float *out_F) {
  for (int i = 0; i < 3; i++) {
    out_F[i] = F0[i] + (1.0 - F0[i]) * powf(1.0 - cosTheta, 5.0);
  }
}

// Stylized: Cel Shading
float toon_diffuse(float NdotL, int levels) {
  float level = floorf(NdotL * levels);
  return level / levels;
}

// Stylized: Sobel Outline (Post-process logic placeholder)
void outline_detect() {
  // ...
}

/*
 * IMPLEMENTATION: 50/1000 PBR Shader TODOs
 * LOC: ~50
 */
