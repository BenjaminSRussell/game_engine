#ifndef PHILLIPS_SPECTRUM_H
#define PHILLIPS_SPECTRUM_H

#include "environment/ocean/ocean_fft.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Phillips Spectrum Generation
// =============================================================================

/**
 * Initialize Phillips spectrum for a cascade
 * Generates h0(k), h0*(-k), and (k) textures
 * 
 * @param ocean Ocean system
 * @param cascade Cascade to initialize
 * @param params Spectrum parameters
 * @param resolution FFT resolution
 * @return true on success
 */
bool phillips_spectrum_initialize(
    OceanFFTSystem* ocean,
    OceanFFTCascade* cascade,
    const PhillipsSpectrumParams* params,
    uint32_t resolution
);

/**
 * Update spectrum parameters for existing cascade
 * Regenerates h0(k) with new parameters
 * 
 * @param ocean Ocean system
 * @param cascade Cascade to update
 * @param params New spectrum parameters
 * @return true on success
 */
bool phillips_spectrum_update_params(
    OceanFFTSystem* ocean,
    OceanFFTCascade* cascade,
    const PhillipsSpectrumParams* params
);

/**
 * Release spectrum resources for a cascade
 */
void phillips_spectrum_release(OceanFFTCascade* cascade);

// =============================================================================
// Spectrum Math Utilities
// =============================================================================

/**
 * Compute Phillips spectrum value P(k) for wave vector k
 * 
 * @param kx Wave vector X component
 * @param kz Wave vector Z component
 * @param params Spectrum parameters
 * @return Spectrum value
 */
float phillips_spectrum_value(
    float kx,
    float kz,
    const PhillipsSpectrumParams* params
);

/**
 * Compute dispersion relation (k) = sqrt(g|k|)
 * 
 * @param kx Wave vector X component
 * @param kz Wave vector Z component
 * @param gravity Gravity constant
 * @return Angular frequency
 */
float phillips_dispersion(float kx, float kz, float gravity);

/**
 * Generate Gaussian random number (Box-Muller transform)
 * 
 * @return Random value from N(0, 1)
 */
float phillips_gaussian_random(void);

/**
 * Compute wave vector k from grid indices
 * 
 * @param n Grid index X (-N/2 to N/2-1)
 * @param m Grid index Z (-N/2 to N/2-1)
 * @param resolution Grid resolution N
 * @param tile_size Physical tile size L
 * @param out_kx Output wave vector X
 * @param out_kz Output wave vector Z
 */
void phillips_compute_wave_vector(
    int n,
    int m,
    uint32_t resolution,
    float tile_size,
    float* out_kx,
    float* out_kz
);

#ifdef __cplusplus
}
#endif

#endif // PHILLIPS_SPECTRUM_H
