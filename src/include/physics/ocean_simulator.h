#ifndef OCEAN_SIMULATOR_H
#define OCEAN_SIMULATOR_H

#include "engine/include/common.h"
#include <math/vec2.h>
#include <math/vec3.h>
#include <stdbool.h>

// Complex number for FFT
typedef struct {
  float real;
  float imag;
} Complex;

// FFT Plan for 2D transforms
typedef struct {
  uint32_t size;
  Complex *twiddle_factors;
  uint32_t *bit_reverse;
  void *gpu_plan; // GPU FFT plan (optional)
} FFTPlan;

// Ocean simulation parameters
typedef struct {
  // Grid resolution (must be power of 2)
  uint32_t resolution; // e.g., 256, 512, 1024

  // Physical size
  float patch_size; // Size in meters (e.g., 1000m)

  // Wind parameters
  float wind_speed;    // m/s (e.g., 10-30)
  Vec2 wind_direction; // Normalized

  // Wave parameters
  float amplitude;     // Wave height scale (A parameter)
  float choppiness;    // Wave sharpness ( parameter, 0-4)
  float smallest_wave; // Cutoff for small waves (meters)

  // Time
  float time_scale; // Speed multiplier

  // Quality
  bool use_gpu;
  uint32_t lod_levels; // Number of LOD levels
} OceanConfig;

// Ocean simulator state
typedef struct {
  OceanConfig config;

  // Wave spectrum (frequency domain)
  Complex *h0;         // Initial spectrum h(k)
  Complex *h0_conj;    // Conjugate h*(-k)
  Complex *h_tilde;    // Time-evolved h(k,t)
  Complex *h_tilde_dx; // Displacement x
  Complex *h_tilde_dz; // Displacement z

  // FFT plans
  FFTPlan *fft_plan;
  FFTPlan *ifft_plan;

  // Output heightfield (spatial domain)
  float *heights;      // Height at each grid point
  Vec3 *normals;       // Surface normals
  Vec3 *displacements; // Horizontal displacement (choppiness)

  // Time
  float current_time;

  // Statistics
  float max_wave_height;
  float avg_wave_height;

  // GPU resources
  void *gpu_buffer;
} OceanSimulator;

// ============================================================================
// Public API
// ============================================================================

// Initialization
OceanSimulator *ocean_create(const OceanConfig *config);
void ocean_free(OceanSimulator *ocean);
OceanConfig ocean_get_default_config(void);

// Simulation
void ocean_update(OceanSimulator *ocean, float dt);
void ocean_set_time(OceanSimulator *ocean, float time);

// Data access
const float *ocean_get_heights(const OceanSimulator *ocean);
const Vec3 *ocean_get_normals(const OceanSimulator *ocean);
const Vec3 *ocean_get_displacements(const OceanSimulator *ocean);
float ocean_get_height_at(const OceanSimulator *ocean, float x, float z);

// Configuration
void ocean_set_wind(OceanSimulator *ocean, Vec2 direction, float speed);
void ocean_set_amplitude(OceanSimulator *ocean, float amplitude);
void ocean_set_choppiness(OceanSimulator *ocean, float choppiness);

// Statistics
void ocean_get_statistics(const OceanSimulator *ocean, float *max_height,
                          float *avg_height);

// ============================================================================
// Wave Spectrum Functions
// ============================================================================

// Phillips spectrum (classic)
float ocean_phillips_spectrum(Vec2 k, Vec2 wind_dir, float wind_speed,
                              float amplitude);

// JONSWAP spectrum (more realistic for fetch-limited seas)
float ocean_jonswap_spectrum(Vec2 k, Vec2 wind_dir, float wind_speed,
                             float fetch);

// Dispersion relation: (k) = (g|k|)
float ocean_dispersion(float k_length);

// ============================================================================
// FFT Functions
// ============================================================================

// Create FFT plan
FFTPlan *fft_create_plan(uint32_t size);
void fft_free_plan(FFTPlan *plan);

// 1D FFT
void fft_1d(Complex *data, uint32_t size, bool inverse, const FFTPlan *plan);

// 2D FFT (separable - row-column algorithm)
void fft_2d(Complex *data, uint32_t width, uint32_t height, bool inverse,
            const FFTPlan *plan);

// ============================================================================
// Utility Functions
// ============================================================================

// Generate Gaussian random number
float gaussian_random(float mean, float stddev);

// Complex arithmetic
Complex complex_add(Complex a, Complex b);
Complex complex_mul(Complex a, Complex b);
Complex complex_conj(Complex a);

#endif // OCEAN_SIMULATOR_H
