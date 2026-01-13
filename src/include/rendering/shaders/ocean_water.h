/**
 * =================================================================================================
 *                              WATER OCEAN SHADER HEADER
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 *
 * Header file for the ocean water shader system with FFT waves, caustics, 
 * foam, and underwater effects.
 *
 * =================================================================================================
 */

#ifndef OCEAN_WATER_H
#define OCEAN_WATER_H

#include <stdbool.h>
#include <stdint.h>
#include <math/math.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */
#define OCEAN_MAX_WAVE_LAYERS 8
#define OCEAN_MAX_FFT_RESOLUTION 1024
#define OCEAN_PATCH_SIZE 100.0f
#define OCEAN_SEA_LEVEL 0.0f

/* =================================================================================================
 *                                    MATH UTILITIES
 * =================================================================================================
 */
static inline float saturate(float x) {
  return fmaxf(0.0f, fminf(1.0f, x));
}

static inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

static inline float dot(const float a[3], const float b[3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline float min(float a, float b) {
  return (a < b) ? a : b;
}

static inline float max(float a, float b) {
  return (a > b) ? a : b;
}

/* =================================================================================================
 *                                    OCEAN STRUCTURES
 * =================================================================================================
 */

typedef struct OceanWaveLayer {
  float direction[2];
  float wavelength;
  float amplitude;
  float steepness;
  float speed;
  float phase_offset;
} OceanWaveLayer;

typedef struct OceanParameters {
  // Wave layers (Gerstner waves)
  OceanWaveLayer wave_layers[OCEAN_MAX_WAVE_LAYERS];
  uint32_t wave_layer_count;
  
  // FFT waves
  bool use_fft_waves;
  uint32_t fft_resolution;
  float fft_wind_speed;
  float fft_wind_direction[2];
  float fft_fetch;
  float fft_spread_blend;
  
  // Colors
  float shallow_color[4];
  float deep_color[4];
  float scatter_color[3];
  float foam_color[3];
  
  // Depth
  float depth_max;
  float depth_falloff;
  float shore_depth;
  
  // Reflections
  float reflection_strength;
  float fresnel_power;
  float distortion_strength;
  bool use_ssr;
  bool use_planar_reflection;
  uint32_t reflection_texture;
  
  // Refraction
  float refraction_strength;
  float absorption[3];
  float scattering;
  
  // Foam
  float foam_threshold;
  float foam_intensity;
  float foam_scale;
  uint32_t foam_texture;
  
  // Caustics
  bool enable_caustics;
  float caustics_strength;
  float caustics_scale;
  float caustics_speed;
  uint32_t caustics_texture;
  
  // Underwater
  bool enable_underwater;
  float underwater_fog_density;
  float underwater_fog_color[3];
} OceanParameters;

typedef struct FFTOceanState {
  uint32_t spectrum_texture;
  uint32_t displacement_texture;
  uint32_t normal_texture;
  uint32_t folding_texture;
  uint32_t fft_resolution;
  float time;
  float patch_size;
  bool is_initialized;
} FFTOceanState;

/* =================================================================================================
 *                                    GERSTNER WAVES
 * =================================================================================================
 */
void ocean_calculate_gerstner_displacement(const OceanWaveLayer* wave_layer, 
                                          const float position[3], 
                                          float time, 
                                          float displacement[3]);

void ocean_calculate_gerstner_normal(const OceanWaveLayer* wave_layer,
                                    const float position[3],
                                    float time,
                                    float normal[3]);

/* =================================================================================================
 *                                    FFT WAVE SPECTRUM
 * =================================================================================================
 */
float ocean_phillips_spectrum(float k, const float wind[2], float wind_speed, float fetch);

float ocean_jonswap_spectrum(float k, float wind_speed, float fetch, float alpha, float gamma);

float ocean_directional_spreading(float theta, float theta_mean, float s);

/* =================================================================================================
 *                                    FFT IMPLEMENTATION
 * =================================================================================================
 */
bool ocean_fft_init(FFTOceanState* state, uint32_t resolution, float patch_size);

void ocean_fft_update(FFTOceanState* state, float wind_speed, 
                     const float wind_direction[2], float time);

/* =================================================================================================
 *                                    FOAM GENERATION
 * =================================================================================================
 */
float ocean_calculate_jacobian(float dx, float dz);

float ocean_generate_foam(const float position[3], float depth, 
                         float jacobian, float foam_threshold);

/* =================================================================================================
 *                                    COLOR BLENDING
 * =================================================================================================
 */
void ocean_depth_color_blend(float depth, float depth_max, float falloff,
                            const float shallow_color[4], 
                            const float deep_color[4],
                            float output[4]);

float ocean_fresnel_reflection(float cos_theta, float fresnel_power);

/* =================================================================================================
 *                                    REFLECTIONS
 * =================================================================================================
 */
void ocean_sample_ssr(const float position[3], const float normal[3],
                     const float view_dir[3], float reflection_strength,
                     float reflection_color[3]);

void ocean_sample_planar_reflection(uint32_t reflection_texture,
                                  const float position[3],
                                  const float normal[3],
                                  float reflection_color[3]);

/* =================================================================================================
 *                                    REFRACTION
 * =================================================================================================
 */
void ocean_refract_with_chromatic_aberration(const float incident[3],
                                            const float normal[3],
                                            float ior,
                                            float refracted[3],
                                            float dispersion);

/* =================================================================================================
 *                                    CAUSTICS
 * =================================================================================================
 */
float ocean_project_caustics(const float position[3], const float light_dir[3],
                            float time, float caustics_scale, float caustics_speed);

/* =================================================================================================
 *                                    UNDERWATER EFFECTS
 * =================================================================================================
 */
bool ocean_is_camera_underwater(const float camera_pos[3], float ocean_height);

void ocean_underwater_fog(float distance, float fog_density,
                          const float fog_color[3],
                          const float view_dir[3],
                          const float light_dir[3],
                          float fog_result[4]);

void ocean_underwater_distortion(const float position[3], float time,
                                float distortion_strength,
                                float distorted_position[3]);

/* =================================================================================================
 *                                    TESSELLATION AND LOD
 * =================================================================================================
 */
float ocean_calculate_tessellation(float distance, float wave_height, float max_tess);

uint32_t ocean_calculate_lod(float distance, float max_lod_distance);

/* =================================================================================================
 *                                    INFINITE OCEAN GRID
 * =================================================================================================
 */
void ocean_infinite_grid_positions(const float camera_pos[3], float patch_size,
                                  uint32_t grid_size,
                                  float patch_positions[/*grid_size*grid_size*/][3]);

/* =================================================================================================
 *                                    BUOYANCY SAMPLING
 * =================================================================================================
 */
float ocean_sample_height(const float position[3], float time,
                         const OceanParameters* ocean_params);

void ocean_sample_normal(const float position[3], float time,
                        const OceanParameters* ocean_params,
                        float normal[3]);

/* =================================================================================================
 *                                    OCEAN API
 * =================================================================================================
 */
bool ocean_shader_init(OceanParameters* ocean_params);

void ocean_shader_shutdown(OceanParameters* ocean_params);

void ocean_shader_update(OceanParameters* ocean_params, float time,
                        float wind_speed, const float wind_direction[2]);

void ocean_shader_render(const OceanParameters* ocean_params,
                        const float view_matrix[16], 
                        const float proj_matrix[16],
                        const float camera_pos[3],
                        float time);

float ocean_get_height_at(const OceanParameters* ocean_params,
                         const float position[3],
                         float time);

void ocean_get_normal_at(const OceanParameters* ocean_params,
                        const float position[3],
                        float time,
                        float normal[3]);

void ocean_set_wind(OceanParameters* ocean_params, float wind_speed,
                   const float wind_direction[2]);

void ocean_set_time_scale(OceanParameters* ocean_params, float time_scale);

#endif // OCEAN_WATER_H
