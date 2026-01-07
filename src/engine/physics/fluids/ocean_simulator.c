// physics/fluids/ocean_simulator.c - FFT-based ocean wave simulation
#include <core/logger.h>
#include <core/memory.h>
#include <include/math/math.h>
#include <physics/ocean_simulator.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846f
#define GRAVITY 9.81f

// ============================================================================
// Complex Number Operations
// ============================================================================

Complex complex_add(Complex a, Complex b) {
  return (Complex){a.real + b.real, a.imag + b.imag};
}

Complex complex_mul(Complex a, Complex b) {
  return (Complex){a.real * b.real - a.imag * b.imag,
                   a.real * b.imag + a.imag * b.real};
}

Complex complex_conj(Complex a) { return (Complex){a.real, -a.imag}; }

Complex complex_scale(Complex a, float s) {
  return (Complex){a.real * s, a.imag * s};
}

// ============================================================================
// FFT Implementation (Cooley-Tukey Algorithm)
// ============================================================================

// Bit-reverse permutation
static void fft_bit_reverse(Complex *data, uint32_t size) {
  uint32_t bits = 0;
  uint32_t n = size;
  while (n > 1) {
    bits++;
    n >>= 1;
  }

  for (uint32_t i = 0; i < size; i++) {
    uint32_t j = 0;
    for (uint32_t b = 0; b < bits; b++) {
      j = (j << 1) | ((i >> b) & 1);
    }

    if (j > i) {
      Complex temp = data[i];
      data[i] = data[j];
      data[j] = temp;
    }
  }
}

// 1D FFT (in-place)
void fft_1d(Complex *data, uint32_t size, bool inverse, const FFTPlan *plan) {
  if (!data || size == 0)
    return;

  // Bit-reverse permutation
  fft_bit_reverse(data, size);

  // Cooley-Tukey FFT
  float sign = inverse ? 1.0f : -1.0f;

  for (uint32_t s = 2; s <= size; s *= 2) {
    float angle = sign * 2.0f * PI / s;
    Complex w = {cosf(angle), sinf(angle)};

    for (uint32_t k = 0; k < size; k += s) {
      Complex wn = {1.0f, 0.0f};

      for (uint32_t j = 0; j < s / 2; j++) {
        Complex t = complex_mul(wn, data[k + j + s / 2]);
        Complex u = data[k + j];

        data[k + j] = complex_add(u, t);
        data[k + j + s / 2] = complex_add(u, complex_scale(t, -1.0f));

        wn = complex_mul(wn, w);
      }
    }
  }

  // Normalize for inverse transform
  if (inverse) {
    float scale = 1.0f / size;
    for (uint32_t i = 0; i < size; i++) {
      data[i] = complex_scale(data[i], scale);
    }
  }
}

// 2D FFT (separable - row-column algorithm)
void fft_2d(Complex *data, uint32_t width, uint32_t height, bool inverse,
            const FFTPlan *plan) {
  // Transform rows
  for (uint32_t y = 0; y < height; y++) {
    fft_1d(&data[y * width], width, inverse, plan);
  }

  // Transform columns
  Complex *column = MALLOC(height * sizeof(Complex));
  for (uint32_t x = 0; x < width; x++) {
    // Extract column
    for (uint32_t y = 0; y < height; y++) {
      column[y] = data[y * width + x];
    }

    // Transform column
    fft_1d(column, height, inverse, plan);

    // Put back
    for (uint32_t y = 0; y < height; y++) {
      data[y * width + x] = column[y];
    }
  }
  FREE(column);
}

// Create FFT plan
FFTPlan *fft_create_plan(uint32_t size) {
  FFTPlan *plan = CALLOC(1, sizeof(FFTPlan));
  plan->size = size;
  // Additional precomputation could go here
  return plan;
}

void fft_free_plan(FFTPlan *plan) {
  if (plan)
    FREE(plan);
}

// ============================================================================
// Random Number Generation
// ============================================================================

// Box-Muller transform for Gaussian random numbers
float gaussian_random(float mean, float stddev) {
  static bool has_spare = false;
  static float spare;

  if (has_spare) {
    has_spare = false;
    return mean + stddev * spare;
  }

  has_spare = true;

  float u, v, s;
  do {
    u = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    v = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    s = u * u + v * v;
  } while (s >= 1.0f || s == 0.0f);

  s = sqrtf(-2.0f * logf(s) / s);
  spare = v * s;
  return mean + stddev * u * s;
}

// ============================================================================
// Wave Spectrum Functions
// ============================================================================

// Phillips spectrum
float ocean_phillips_spectrum(Vec2 k, Vec2 wind_dir, float wind_speed,
                              float amplitude) {
  float k_length = sqrtf(k.x * k.x + k.y * k.y);
  if (k_length < 0.0001f)
    return 0.0f;

  float k_length2 = k_length * k_length;
  float k_length4 = k_length2 * k_length2;

  // Largest wave from wind
  float L = wind_speed * wind_speed / GRAVITY;
  float L2 = L * L;

  // Wind direction alignment
  float k_dot_w = (k.x * wind_dir.x + k.y * wind_dir.y) / k_length;
  float k_dot_w2 = k_dot_w * k_dot_w;

  // Phillips spectrum
  float ph = amplitude * expf(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2;

  // Suppress waves smaller than L/1000
  float l = L / 1000.0f;
  ph *= expf(-k_length2 * l * l);

  return ph;
}

// Dispersion relation: ω(k) = √(g|k|)
float ocean_dispersion(float k_length) { return sqrtf(GRAVITY * k_length); }

// ============================================================================
// Ocean Simulator
// ============================================================================

OceanConfig ocean_get_default_config(void) {
  OceanConfig config = {0};
  config.resolution = 256;
  config.patch_size = 1000.0f;
  config.wind_speed = 15.0f;
  config.wind_direction = (Vec2){1.0f, 0.0f};
  config.amplitude = 0.0002f;
  config.choppiness = 1.5f;
  config.smallest_wave = 0.001f;
  config.time_scale = 1.0f;
  config.use_gpu = false;
  config.lod_levels = 1;
  return config;
}

OceanSimulator *ocean_create(const OceanConfig *config) {
  if (!config || config->resolution == 0) {
    LOG_ERROR("Invalid ocean configuration");
    return NULL;
  }

  // Check resolution is power of 2
  uint32_t res = config->resolution;
  if ((res & (res - 1)) != 0) {
    LOG_ERROR("Ocean resolution must be power of 2");
    return NULL;
  }

  OceanSimulator *ocean = CALLOC(1, sizeof(OceanSimulator));
  ocean->config = *config;

  uint32_t size = res * res;

  // Allocate spectrum arrays
  ocean->h0 = CALLOC(size, sizeof(Complex));
  ocean->h0_conj = CALLOC(size, sizeof(Complex));
  ocean->h_tilde = CALLOC(size, sizeof(Complex));
  ocean->h_tilde_dx = CALLOC(size, sizeof(Complex));
  ocean->h_tilde_dz = CALLOC(size, sizeof(Complex));

  // Allocate output arrays
  ocean->heights = CALLOC(size, sizeof(float));
  ocean->normals = CALLOC(size, sizeof(Vec3));
  ocean->displacements = CALLOC(size, sizeof(Vec3));

  // Create FFT plans
  ocean->fft_plan = fft_create_plan(res);
  ocean->ifft_plan = fft_create_plan(res);

  // Initialize spectrum
  Vec2 wind_dir = config->wind_direction;
  float wind_len = sqrtf(wind_dir.x * wind_dir.x + wind_dir.y * wind_dir.y);
  if (wind_len > 0.0001f) {
    wind_dir.x /= wind_len;
    wind_dir.y /= wind_len;
  }

  for (uint32_t y = 0; y < res; y++) {
    for (uint32_t x = 0; x < res; x++) {
      uint32_t idx = y * res + x;

      // Wave vector k
      Vec2 k = {(2.0f * PI * (x - res / 2)) / config->patch_size,
                (2.0f * PI * (y - res / 2)) / config->patch_size};

      // Phillips spectrum
      float ph = ocean_phillips_spectrum(k, wind_dir, config->wind_speed,
                                         config->amplitude);

      // Initial wave amplitude h0(k)
      float er = gaussian_random(0.0f, 1.0f);
      float ei = gaussian_random(0.0f, 1.0f);
      ocean->h0[idx].real = er * sqrtf(ph / 2.0f);
      ocean->h0[idx].imag = ei * sqrtf(ph / 2.0f);

      // Conjugate for h0(-k)
      Vec2 k_neg = {-k.x, -k.y};
      float ph_neg = ocean_phillips_spectrum(
          k_neg, wind_dir, config->wind_speed, config->amplitude);
      float er_neg = gaussian_random(0.0f, 1.0f);
      float ei_neg = gaussian_random(0.0f, 1.0f);
      ocean->h0_conj[idx].real = er_neg * sqrtf(ph_neg / 2.0f);
      ocean->h0_conj[idx].imag = -ei_neg * sqrtf(ph_neg / 2.0f);
    }
  }

  ocean->current_time = 0.0f;

  LOG_INFO("Created ocean simulator: %ux%u, patch size: %.1fm", res, res,
           config->patch_size);

  return ocean;
}

void ocean_free(OceanSimulator *ocean) {
  if (!ocean)
    return;

  FREE(ocean->h0);
  FREE(ocean->h0_conj);
  FREE(ocean->h_tilde);
  FREE(ocean->h_tilde_dx);
  FREE(ocean->h_tilde_dz);
  FREE(ocean->heights);
  FREE(ocean->normals);
  FREE(ocean->displacements);

  fft_free_plan(ocean->fft_plan);
  fft_free_plan(ocean->ifft_plan);

  FREE(ocean);
}

void ocean_update(OceanSimulator *ocean, float dt) {
  if (!ocean)
    return;

  ocean->current_time += dt * ocean->config.time_scale;
  ocean_set_time(ocean, ocean->current_time);
}

void ocean_set_time(OceanSimulator *ocean, float time) {
  if (!ocean)
    return;

  uint32_t res = ocean->config.resolution;
  float patch_size = ocean->config.patch_size;

  // Update wave amplitudes based on time
  for (uint32_t y = 0; y < res; y++) {
    for (uint32_t x = 0; x < res; x++) {
      uint32_t idx = y * res + x;

      // Wave vector k
      Vec2 k = {(2.0f * PI * (x - res / 2)) / patch_size,
                (2.0f * PI * (y - res / 2)) / patch_size};
      float k_length = sqrtf(k.x * k.x + k.y * k.y);

      // Dispersion
      float omega = ocean_dispersion(k_length);

      // Time evolution: h̃(k,t) = h0(k)e^(iωt) + h0*(-k)e^(-iωt)
      float cos_wt = cosf(omega * time);
      float sin_wt = sinf(omega * time);

      Complex exp_iwt = {cos_wt, sin_wt};
      Complex exp_niwt = {cos_wt, -sin_wt};

      Complex h0_exp = complex_mul(ocean->h0[idx], exp_iwt);
      Complex h0c_exp = complex_mul(ocean->h0_conj[idx], exp_niwt);

      ocean->h_tilde[idx] = complex_add(h0_exp, h0c_exp);

      // Displacement (choppiness)
      if (k_length > 0.0001f) {
        ocean->h_tilde_dx[idx] =
            complex_mul(ocean->h_tilde[idx], (Complex){0.0f, -k.x / k_length});
        ocean->h_tilde_dz[idx] =
            complex_mul(ocean->h_tilde[idx], (Complex){0.0f, -k.y / k_length});
      } else {
        ocean->h_tilde_dx[idx] = (Complex){0.0f, 0.0f};
        ocean->h_tilde_dz[idx] = (Complex){0.0f, 0.0f};
      }
    }
  }

  // Inverse FFT to get spatial domain
  fft_2d(ocean->h_tilde, res, res, true, ocean->ifft_plan);
  fft_2d(ocean->h_tilde_dx, res, res, true, ocean->ifft_plan);
  fft_2d(ocean->h_tilde_dz, res, res, true, ocean->ifft_plan);

  // Extract heights and displacements
  float max_height = 0.0f;
  float sum_height = 0.0f;

  for (uint32_t y = 0; y < res; y++) {
    for (uint32_t x = 0; x < res; x++) {
      uint32_t idx = y * res + x;

      // Height
      ocean->heights[idx] = ocean->h_tilde[idx].real;

      // Displacement (choppiness)
      ocean->displacements[idx].x =
          ocean->h_tilde_dx[idx].real * ocean->config.choppiness;
      ocean->displacements[idx].y = ocean->heights[idx];
      ocean->displacements[idx].z =
          ocean->h_tilde_dz[idx].real * ocean->config.choppiness;

      // Statistics
      float h = fabsf(ocean->heights[idx]);
      if (h > max_height)
        max_height = h;
      sum_height += h;
    }
  }

  ocean->max_wave_height = max_height;
  ocean->avg_wave_height = sum_height / (res * res);

  // Calculate normals (finite differences)
  float dx = patch_size / res;
  for (uint32_t y = 0; y < res; y++) {
    for (uint32_t x = 0; x < res; x++) {
      uint32_t idx = y * res + x;

      // Neighboring heights
      uint32_t x_next = (x + 1) % res;
      uint32_t y_next = (y + 1) % res;

      float h_x = ocean->heights[y * res + x_next] - ocean->heights[idx];
      float h_y = ocean->heights[y_next * res + x] - ocean->heights[idx];

      // Normal = (-dh/dx, 1, -dh/dy) normalized
      Vec3 normal = {-h_x / dx, 1.0f, -h_y / dx};
      float len = sqrtf(normal.x * normal.x + normal.y * normal.y +
                        normal.z * normal.z);
      if (len > 0.0001f) {
        normal.x /= len;
        normal.y /= len;
        normal.z /= len;
      }

      ocean->normals[idx] = normal;
    }
  }
}

// Data access
const float *ocean_get_heights(const OceanSimulator *ocean) {
  return ocean ? ocean->heights : NULL;
}

const Vec3 *ocean_get_normals(const OceanSimulator *ocean) {
  return ocean ? ocean->normals : NULL;
}

const Vec3 *ocean_get_displacements(const OceanSimulator *ocean) {
  return ocean ? ocean->displacements : NULL;
}

float ocean_get_height_at(const OceanSimulator *ocean, float x, float z) {
  if (!ocean)
    return 0.0f;

  uint32_t res = ocean->config.resolution;
  float patch_size = ocean->config.patch_size;

  // Wrap coordinates
  x = fmodf(x, patch_size);
  z = fmodf(z, patch_size);
  if (x < 0)
    x += patch_size;
  if (z < 0)
    z += patch_size;

  // Grid coordinates
  float gx = (x / patch_size) * res;
  float gz = (z / patch_size) * res;

  uint32_t ix = (uint32_t)gx;
  uint32_t iz = (uint32_t)gz;

  // Bilinear interpolation
  float fx = gx - ix;
  float fz = gz - iz;

  uint32_t ix1 = (ix + 1) % res;
  uint32_t iz1 = (iz + 1) % res;

  float h00 = ocean->heights[iz * res + ix];
  float h10 = ocean->heights[iz * res + ix1];
  float h01 = ocean->heights[iz1 * res + ix];
  float h11 = ocean->heights[iz1 * res + ix1];

  float h0 = h00 * (1.0f - fx) + h10 * fx;
  float h1 = h01 * (1.0f - fx) + h11 * fx;

  return h0 * (1.0f - fz) + h1 * fz;
}

// Configuration
void ocean_set_wind(OceanSimulator *ocean, Vec2 direction, float speed) {
  if (!ocean)
    return;
  ocean->config.wind_direction = direction;
  ocean->config.wind_speed = speed;
  // Would need to regenerate spectrum for this to take effect
}

void ocean_set_amplitude(OceanSimulator *ocean, float amplitude) {
  if (ocean)
    ocean->config.amplitude = amplitude;
}

void ocean_set_choppiness(OceanSimulator *ocean, float choppiness) {
  if (ocean)
    ocean->config.choppiness = choppiness;
}

// Statistics
void ocean_get_statistics(const OceanSimulator *ocean, float *max_height,
                          float *avg_height) {
  if (!ocean)
    return;
  if (max_height)
    *max_height = ocean->max_wave_height;
  if (avg_height)
    *avg_height = ocean->avg_wave_height;
}
