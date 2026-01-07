#include "phillips_spectrum.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Random Number Generation
// =============================================================================

static uint32_t rng_state = 123456789;

static void phillips_seed_random(uint32_t seed) {
    rng_state = seed;
}

static float phillips_random_uniform(void) {
    rng_state = rng_state * 1664525 + 1013904223;
    return (float)rng_state / (float)UINT32_MAX;
}

float phillips_gaussian_random(void) {
    // Box-Muller transform
    float u1 = phillips_random_uniform();
    float u2 = phillips_random_uniform();
    
    // Avoid log(0)
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float r = sqrtf(-2.0f * logf(u1));
    float theta = 2.0f * M_PI * u2;
    
    return r * cosf(theta);
}

// =============================================================================
// Phillips Spectrum Math
// =============================================================================

void phillips_compute_wave_vector(
    int n,
    int m,
    uint32_t resolution,
    float tile_size,
    float* out_kx,
    float* out_kz
) {
    // k = 2π/L * (n, m) where n,m ∈ [-N/2, N/2-1]
    float kx = (2.0f * M_PI / tile_size) * (float)n;
    float kz = (2.0f * M_PI / tile_size) * (float)m;
    
    if (out_kx) *out_kx = kx;
    if (out_kz) *out_kz = kz;
}

float phillips_dispersion(float kx, float kz, float gravity) {
    // ω(k) = sqrt(g|k|)
    float k_mag = sqrtf(kx * kx + kz * kz);
    
    if (k_mag < 1e-8f) {
        return 0.0f;
    }
    
    return sqrtf(gravity * k_mag);
}

float phillips_spectrum_value(
    float kx,
    float kz,
    const PhillipsSpectrumParams* params
) {
    float k_mag = sqrtf(kx * kx + kz * kz);
    
    // Avoid division by zero
    if (k_mag < 1e-8f) {
        return 0.0f;
    }
    
    // Normalize wave vector
    float k_norm_x = kx / k_mag;
    float k_norm_z = kz / k_mag;
    
    // Normalize wind direction
    float wind_mag = sqrtf(params->wind_direction_x * params->wind_direction_x +
                           params->wind_direction_z * params->wind_direction_z);
    
    if (wind_mag < 1e-8f) {
        wind_mag = 1.0f;  // Fallback
    }
    
    float wind_norm_x = params->wind_direction_x / wind_mag;
    float wind_norm_z = params->wind_direction_z / wind_mag;
    
    // Dot product: k̂ · ŵ
    float k_dot_w = k_norm_x * wind_norm_x + k_norm_z * wind_norm_z;
    
    // Directional spreading: |k̂ · ŵ|²
    float directional = k_dot_w * k_dot_w;
    
    // Largest wave for this wind speed
    // L = V² / g
    float L = (params->wind_speed * params->wind_speed) / OCEAN_GRAVITY;
    
    // Phillips spectrum:
    // P(k) = A * (|k̂·ŵ|² / |k|⁴) * exp(-1/(kL)²) * exp(-k²l²)
    
    float k2 = k_mag * k_mag;
    float k4 = k2 * k2;
    
    // Main term
    float amplitude_term = params->amplitude;
    float directional_term = directional;
    float wave_term = 1.0f / k4;
    
    // Large wave suppression: exp(-1/(kL)²)
    float kL = k_mag * L;
    float large_wave_term = expf(-1.0f / (kL * kL + 1e-8f));
    
    // Small wave suppression: exp(-k²l²)
    float l = params->wave_suppression;
    float small_wave_term = expf(-k2 * l * l);
    
    // Combine
    float P = amplitude_term * directional_term * wave_term * 
              large_wave_term * small_wave_term;
    
    return P;
}

// =============================================================================
// Spectrum Initialization
// =============================================================================

bool phillips_spectrum_initialize(
    OceanFFTSystem* ocean,
    OceanFFTCascade* cascade,
    const PhillipsSpectrumParams* params,
    uint32_t resolution
) {
    if (!ocean || !cascade || !params) {
        return false;
    }
    
    printf("[Phillips] Initializing spectrum: resolution=%u, tile_size=%.1f\n",
           resolution, cascade->tile_size);
    printf("[Phillips] Wind: speed=%.1f m/s, direction=(%.2f, %.2f)\n",
           params->wind_speed, params->wind_direction_x, params->wind_direction_z);
    
    // Seed random generator (use cascade tile size as seed for variation)
    phillips_seed_random((uint32_t)(cascade->tile_size * 12345.0f));
    
    // Allocate temporary CPU buffers for h0(k) and h0*(-k)
    // These will be uploaded to GPU textures
    // Format: complex numbers stored as (real, imaginary) pairs
    
    size_t pixel_count = resolution * resolution;
    float* h0_data = (float*)malloc(pixel_count * 2 * sizeof(float));  // RG32F
    float* h0_conj_data = (float*)malloc(pixel_count * 2 * sizeof(float));
    float* omega_data = (float*)malloc(pixel_count * sizeof(float));  // R32F
    
    if (!h0_data || !h0_conj_data || !omega_data) {
        fprintf(stderr, "[Phillips] Error: Failed to allocate temporary buffers\n");
        free(h0_data);
        free(h0_conj_data);
        free(omega_data);
        return false;
    }
    
    int half_res = (int)resolution / 2;
    
    // Generate spectrum for all wave vectors
    for (int m = -half_res; m < half_res; m++) {
        for (int n = -half_res; n < half_res; n++) {
            // Map to texture coordinates [0, resolution)
            uint32_t tex_x = (n + half_res) % resolution;
            uint32_t tex_y = (m + half_res) % resolution;
            uint32_t index = tex_y * resolution + tex_x;
            
            // Compute wave vector k
            float kx, kz;
            phillips_compute_wave_vector(n, m, resolution, cascade->tile_size, &kx, &kz);
            
            // Compute Phillips spectrum P(k)
            float P_k = phillips_spectrum_value(kx, kz, params);
            
            // Generate h0(k) = (1/sqrt(2)) * sqrt(P(k)) * (ξr + i*ξi)
            // where ξr, ξi are independent Gaussian random variables
            
            float xi_r = phillips_gaussian_random();
            float xi_i = phillips_gaussian_random();
            
            float amplitude = sqrtf(P_k / 2.0f);  // Includes 1/√2 normalization
            
            float h0_real = amplitude * xi_r;
            float h0_imag = amplitude * xi_i;
            
            // Store h0(k)
            h0_data[index * 2 + 0] = h0_real;
            h0_data[index * 2 + 1] = h0_imag;
            
            // Compute h0*(-k) = conjugate of h0(-k)
            // We need to look up h0 at (-n, -m)
            uint32_t conj_tex_x = (-n + half_res) % resolution;
            uint32_t conj_tex_y = (-m + half_res) % resolution;
            uint32_t conj_index = conj_tex_y * resolution + conj_tex_x;
            
            // For now, store the conjugate relationship
            // h0_conj(k) = conjugate(h0(-k))
            // This will be filled in second pass
            
            // Compute dispersion ω(k)
            float omega = phillips_dispersion(kx, kz, OCEAN_GRAVITY);
            omega_data[index] = omega;
        }
    }
    
    // Second pass: fill h0_conj using conjugate symmetry
    for (uint32_t y = 0; y < resolution; y++) {
        for (uint32_t x = 0; x < resolution; x++) {
            uint32_t index = y * resolution + x;
            
            // Map back to wave vector indices
            int n = (int)x - half_res;
            int m = (int)y - half_res;
            
            // Find h0(-k)
            uint32_t conj_x = (uint32_t)((-n + half_res) % (int)resolution);
            uint32_t conj_y = (uint32_t)((-m + half_res) % (int)resolution);
            uint32_t conj_index = conj_y * resolution + conj_x;
            
            // h0*(-k) = conjugate(h0(-k))
            float h0_minus_k_real = h0_data[conj_index * 2 + 0];
            float h0_minus_k_imag = h0_data[conj_index * 2 + 1];
            
            h0_conj_data[index * 2 + 0] = h0_minus_k_real;   // Real part stays same
            h0_conj_data[index * 2 + 1] = -h0_minus_k_imag;  // Imaginary part negated
        }
    }
    
    // TODO: Upload h0_data, h0_conj_data, omega_data to GPU textures
    // cascade->h0_texture
    // cascade->h0_conj_texture
    // cascade->omega_texture
    
    printf("[Phillips] Spectrum generated successfully\n");
    
    // Free temporary buffers
    free(h0_data);
    free(h0_conj_data);
    free(omega_data);
    
    return true;
}

bool phillips_spectrum_update_params(
    OceanFFTSystem* ocean,
    OceanFFTCascade* cascade,
    const PhillipsSpectrumParams* params
) {
    // Simply reinitialize with new parameters
    return phillips_spectrum_initialize(ocean, cascade, params, ocean->config.fft_resolution);
}

void phillips_spectrum_release(OceanFFTCascade* cascade) {
    // Textures are released by ocean_fft.c
    // This function is a placeholder for any cascade-specific cleanup
    (void)cascade;
}
