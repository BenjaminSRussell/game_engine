/**
 * =================================================================================================
 *                              SHADER UTILITIES - IMPLEMENTATION
 *                              Agent: AGENT_SHADER_1
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    SHADER MATH HELPERS
 * =================================================================================================
 */

// DONE: Implement vec3_normalize
void vec3_normalize(float *v) {
  float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (len > 0.0001f) {
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
  }
}

// DONE: Implement vec3_dot
float vec3_dot(const float *a, const float *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// DONE: Implement vec3_cross
void vec3_cross(float *out, const float *a, const float *b) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

// DONE: Implement vec3_reflect
void vec3_reflect(float *out, const float *incident, const float *normal) {
  float dot = vec3_dot(incident, normal);
  out[0] = incident[0] - 2.0f * dot * normal[0];
  out[1] = incident[1] - 2.0f * dot * normal[1];
  out[2] = incident[2] - 2.0f * dot * normal[2];
}

// DONE: Implement vec3_refract
bool vec3_refract(float *out, const float *incident, const float *normal,
                  float eta) {
  float dot = vec3_dot(incident, normal);
  float k = 1.0f - eta * eta * (1.0f - dot * dot);

  if (k < 0.0f)
    return false;

  float sqrt_k = sqrtf(k);
  out[0] = eta * incident[0] - (eta * dot + sqrt_k) * normal[0];
  out[1] = eta * incident[1] - (eta * dot + sqrt_k) * normal[1];
  out[2] = eta * incident[2] - (eta * dot + sqrt_k) * normal[2];

  return true;
}

// DONE: Implement mat4_multiply
void mat4_multiply(float *out, const float *a, const float *b) {
  float temp[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      temp[i * 4 + j] = 0;
      for (int k = 0; k < 4; k++) {
        temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
      }
    }
  }
  memcpy(out, temp, 16 * sizeof(float));
}

// DONE: Implement mat4_inverse
bool mat4_inverse(float *out, const float *m) {
  float inv[16];

  inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
           m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

  inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
           m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

  inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
           m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

  inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
            m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

  float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (fabsf(det) < 0.0001f)
    return false;

  det = 1.0f / det;

  for (int i = 0; i < 16; i++) {
    out[i] = inv[i] * det;
  }

  return true;
}

/* =================================================================================================
 *                                    PBR LIGHTING
 * =================================================================================================
 */

// DONE: Implement pbr_distribution_ggx
float pbr_distribution_ggx(float ndoth, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float denom = ndoth * ndoth * (a2 - 1.0f) + 1.0f;
  denom = 3.14159f * denom * denom;
  return a2 / denom;
}

// DONE: Implement pbr_geometry_schlick_ggx
float pbr_geometry_schlick_ggx(float ndotv, float roughness) {
  float r = roughness + 1.0f;
  float k = (r * r) / 8.0f;
  return ndotv / (ndotv * (1.0f - k) + k);
}

// DONE: Implement pbr_geometry_smith
float pbr_geometry_smith(float ndotv, float ndotl, float roughness) {
  float ggx1 = pbr_geometry_schlick_ggx(ndotv, roughness);
  float ggx2 = pbr_geometry_schlick_ggx(ndotl, roughness);
  return ggx1 * ggx2;
}

// DONE: Implement pbr_fresnel_schlick
void pbr_fresnel_schlick(float *out, float costheta, const float *f0) {
  float pow5 = powf(1.0f - costheta, 5.0f);
  out[0] = f0[0] + (1.0f - f0[0]) * pow5;
  out[1] = f0[1] + (1.0f - f0[1]) * pow5;
  out[2] = f0[2] + (1.0f - f0[2]) * pow5;
}

// DONE: Implement pbr_cook_torrance_brdf
void pbr_cook_torrance_brdf(float *out, const float *n, const float *v,
                            const float *l, const float *albedo, float metallic,
                            float roughness) {
  float h[3] = {(v[0] + l[0]) * 0.5f, (v[1] + l[1]) * 0.5f,
                (v[2] + l[2]) * 0.5f};
  vec3_normalize(h);

  float ndotv = fmaxf(vec3_dot(n, v), 0.0f);
  float ndotl = fmaxf(vec3_dot(n, l), 0.0f);
  float ndoth = fmaxf(vec3_dot(n, h), 0.0f);
  float vdoth = fmaxf(vec3_dot(v, h), 0.0f);

  // F0 for dielectrics and metals
  float f0[3] = {0.04f, 0.04f, 0.04f};
  f0[0] = f0[0] * (1.0f - metallic) + albedo[0] * metallic;
  f0[1] = f0[1] * (1.0f - metallic) + albedo[1] * metallic;
  f0[2] = f0[2] * (1.0f - metallic) + albedo[2] * metallic;

  // Cook-Torrance BRDF
  float ndf = pbr_distribution_ggx(ndoth, roughness);
  float g = pbr_geometry_smith(ndotv, ndotl, roughness);
  float f[3];
  pbr_fresnel_schlick(f, vdoth, f0);

  float numerator[3] = {ndf * g * f[0], ndf * g * f[1], ndf * g * f[2]};
  float denominator = 4.0f * ndotv * ndotl + 0.0001f;

  float specular[3] = {numerator[0] / denominator, numerator[1] / denominator,
                       numerator[2] / denominator};

  // Diffuse
  float kd[3] = {(1.0f - f[0]) * (1.0f - metallic),
                 (1.0f - f[1]) * (1.0f - metallic),
                 (1.0f - f[2]) * (1.0f - metallic)};

  float diffuse[3] = {kd[0] * albedo[0] / 3.14159f,
                      kd[1] * albedo[1] / 3.14159f,
                      kd[2] * albedo[2] / 3.14159f};

  out[0] = (diffuse[0] + specular[0]) * ndotl;
  out[1] = (diffuse[1] + specular[1]) * ndotl;
  out[2] = (diffuse[2] + specular[2]) * ndotl;
}

/* =================================================================================================
 *                                    POST PROCESSING
 * =================================================================================================
 */

// DONE: Implement post_tonemap_aces
void post_tonemap_aces(float *color) {
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;

  for (int i = 0; i < 3; i++) {
    color[i] =
        (color[i] * (a * color[i] + b)) / (color[i] * (c * color[i] + d) + e);
    color[i] = fmaxf(0.0f, fminf(1.0f, color[i]));
  }
}

// DONE: Implement post_tonemap_reinhard
void post_tonemap_reinhard(float *color, float exposure) {
  for (int i = 0; i < 3; i++) {
    color[i] *= exposure;
    color[i] = color[i] / (1.0f + color[i]);
  }
}

// DONE: Implement post_gamma_correct
void post_gamma_correct(float *color, float gamma) {
  for (int i = 0; i < 3; i++) {
    color[i] = powf(color[i], 1.0f / gamma);
  }
}

// DONE: Implement post_color_grade
void post_color_grade(float *color, float contrast, float saturation,
                      float brightness) {
  // Brightness
  color[0] += brightness;
  color[1] += brightness;
  color[2] += brightness;

  // Contrast
  color[0] = (color[0] - 0.5f) * contrast + 0.5f;
  color[1] = (color[1] - 0.5f) * contrast + 0.5f;
  color[2] = (color[2] - 0.5f) * contrast + 0.5f;

  // Saturation
  float gray = color[0] * 0.299f + color[1] * 0.587f + color[2] * 0.114f;
  color[0] = gray + (color[0] - gray) * saturation;
  color[1] = gray + (color[1] - gray) * saturation;
  color[2] = gray + (color[2] - gray) * saturation;
}

// DONE: Implement post_bloom_threshold
void post_bloom_threshold(float *out, const float *color, float threshold) {
  float brightness =
      color[0] * 0.2126f + color[1] * 0.7152f + color[2] * 0.0722f;

  if (brightness > threshold) {
    float soft = brightness - threshold;
    soft = soft / (soft + 1.0f);
    out[0] = color[0] * soft;
    out[1] = color[1] * soft;
    out[2] = color[2] * soft;
  } else {
    out[0] = out[1] = out[2] = 0;
  }
}

// DONE: Implement post_gaussian_blur
void post_gaussian_blur(float *image, uint32_t width, uint32_t height,
                        uint32_t channels, float sigma) {
  // Simplified 1D separable Gaussian blur
  int kernel_size = (int)(sigma * 3.0f) * 2 + 1;
  float *kernel = malloc(kernel_size * sizeof(float));
  float *temp = malloc(width * height * channels * sizeof(float));

  // Generate kernel
  float sum = 0;
  int half = kernel_size / 2;
  for (int i = 0; i < kernel_size; i++) {
    float x = i - half;
    kernel[i] = expf(-(x * x) / (2.0f * sigma * sigma));
    sum += kernel[i];
  }
  for (int i = 0; i < kernel_size; i++) {
    kernel[i] /= sum;
  }

  // Horizontal pass
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      for (uint32_t c = 0; c < channels; c++) {
        float sum = 0;
        for (int k = 0; k < kernel_size; k++) {
          int sx = (int)x + k - half;
          if (sx >= 0 && sx < (int)width) {
            sum += image[(y * width + sx) * channels + c] * kernel[k];
          }
        }
        temp[(y * width + x) * channels + c] = sum;
      }
    }
  }

  // Vertical pass
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      for (uint32_t c = 0; c < channels; c++) {
        float sum = 0;
        for (int k = 0; k < kernel_size; k++) {
          int sy = (int)y + k - half;
          if (sy >= 0 && sy < (int)height) {
            sum += temp[(sy * width + x) * channels + c] * kernel[k];
          }
        }
        image[(y * width + x) * channels + c] = sum;
      }
    }
  }

  free(kernel);
  free(temp);
}

/* =================================================================================================
 *                                    SHADOW MAPPING
 * =================================================================================================
 */

// DONE: Implement shadow_pcf_sample
float shadow_pcf_sample(float *shadow_map, uint32_t width, uint32_t height,
                        float u, float v, float compare_depth,
                        uint32_t samples) {
  float shadow = 0;
  float texel_size = 1.0f / width;

  for (uint32_t y = 0; y < samples; y++) {
    for (uint32_t x = 0; x < samples; x++) {
      float offset_x = (x - samples / 2.0f) * texel_size;
      float offset_y = (y - samples / 2.0f) * texel_size;

      uint32_t sx = (uint32_t)((u + offset_x) * width);
      uint32_t sy = (uint32_t)((v + offset_y) * height);

      if (sx < width && sy < height) {
        float depth = shadow_map[sy * width + sx];
        shadow += (compare_depth > depth + 0.001f) ? 0.0f : 1.0f;
      }
    }
  }

  return shadow / (samples * samples);
}

// DONE: Implement shadow_vsm_sample
float shadow_vsm_sample(float *moments, float compare_depth) {
  float mean = moments[0];
  float mean_sq = moments[1];

  if (compare_depth <= mean)
    return 1.0f;

  float variance = mean_sq - mean * mean;
  variance = fmaxf(variance, 0.00001f);

  float d = compare_depth - mean;
  float p_max = variance / (variance + d * d);

  return p_max;
}

/* =================================================================================================
 *                                    TEXTURE UTILITIES
 * =================================================================================================
 */

// DONE: Implement texture_sample_bilinear
void texture_sample_bilinear(float *out, const float *texture, uint32_t width,
                             uint32_t height, uint32_t channels, float u,
                             float v) {
  u = u - floorf(u);
  v = v - floorf(v);

  float x = u * (width - 1);
  float y = v * (height - 1);

  uint32_t x0 = (uint32_t)x;
  uint32_t y0 = (uint32_t)y;
  uint32_t x1 = (x0 + 1) % width;
  uint32_t y1 = (y0 + 1) % height;

  float fx = x - x0;
  float fy = y - y0;

  for (uint32_t c = 0; c < channels; c++) {
    float c00 = texture[(y0 * width + x0) * channels + c];
    float c10 = texture[(y0 * width + x1) * channels + c];
    float c01 = texture[(y1 * width + x0) * channels + c];
    float c11 = texture[(y1 * width + x1) * channels + c];

    float c0 = c00 * (1.0f - fx) + c10 * fx;
    float c1 = c01 * (1.0f - fx) + c11 * fx;

    out[c] = c0 * (1.0f - fy) + c1 * fy;
  }
}

// DONE: Implement texture_generate_mipmaps
void texture_generate_mipmaps(float **mips, uint32_t *widths, uint32_t *heights,
                              uint32_t channels, uint32_t mip_count) {
  for (uint32_t mip = 1; mip < mip_count; mip++) {
    uint32_t src_w = widths[mip - 1];
    uint32_t src_h = heights[mip - 1];
    uint32_t dst_w = widths[mip] = src_w / 2;
    uint32_t dst_h = heights[mip] = src_h / 2;

    mips[mip] = malloc(dst_w * dst_h * channels * sizeof(float));

    for (uint32_t y = 0; y < dst_h; y++) {
      for (uint32_t x = 0; x < dst_w; x++) {
        for (uint32_t c = 0; c < channels; c++) {
          float sum = 0;
          sum += mips[mip - 1][(y * 2 * src_w + x * 2) * channels + c];
          sum += mips[mip - 1][(y * 2 * src_w + x * 2 + 1) * channels + c];
          sum += mips[mip - 1][(y * 2 + 1 * src_w + x * 2) * channels + c];
          sum += mips[mip - 1][(y * 2 + 1 * src_w + x * 2 + 1) * channels + c];

          mips[mip][(y * dst_w + x) * channels + c] = sum * 0.25f;
        }
      }
    }
  }
}

/* =================================================================================================
 *                                    ANIMATION
 * =================================================================================================
 */

// DONE: Implement anim_lerp_vec3
void anim_lerp_vec3(float *out, const float *a, const float *b, float t) {
  out[0] = a[0] + (b[0] - a[0]) * t;
  out[1] = a[1] + (b[1] - a[1]) * t;
  out[2] = a[2] + (b[2] - a[2]) * t;
}

// DONE: Implement anim_slerp_quat
void anim_slerp_quat(float *out, const float *a, const float *b, float t) {
  float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];

  float b_copy[4] = {b[0], b[1], b[2], b[3]};
  if (dot < 0) {
    dot = -dot;
    b_copy[0] = -b[0];
    b_copy[1] = -b[1];
    b_copy[2] = -b[2];
    b_copy[3] = -b[3];
  }

  if (dot > 0.9995f) {
    // Linear interpolation for very close quaternions
    out[0] = a[0] + (b_copy[0] - a[0]) * t;
    out[1] = a[1] + (b_copy[1] - a[1]) * t;
    out[2] = a[2] + (b_copy[2] - a[2]) * t;
    out[3] = a[3] + (b_copy[3] - a[3]) * t;
  } else {
    float theta = acosf(dot);
    float sin_theta = sinf(theta);
    float wa = sinf((1.0f - t) * theta) / sin_theta;
    float wb = sinf(t * theta) / sin_theta;

    out[0] = a[0] * wa + b_copy[0] * wb;
    out[1] = a[1] * wa + b_copy[1] * wb;
    out[2] = a[2] * wa + b_copy[2] * wb;
    out[3] = a[3] * wa + b_copy[3] * wb;
  }

  // Normalize
  float len = sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] +
                    out[3] * out[3]);
  if (len > 0.0001f) {
    out[0] /= len;
    out[1] /= len;
    out[2] /= len;
    out[3] /= len;
  }
}

// DONE: Implement anim_cubic_bezier
float anim_cubic_bezier(float t, float p0, float p1, float p2, float p3) {
  float u = 1.0f - t;
  float tt = t * t;
  float uu = u * u;
  float uuu = uu * u;
  float ttt = tt * t;

  return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
}
