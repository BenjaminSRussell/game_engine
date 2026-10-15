#ifndef ATMOSPHERE_LUT_H
#define ATMOSPHERE_LUT_H

#include <stddef.h>
#include <stdint.h>

/**
 * CPU atmosphere lookup tables.
 * Transmittance is 256x64 and sky-view is 192x108, matching atmosphere_common.h
 * and atmosphere_system_t. Distances are kilometers; scattering coefficients
 * consumed from atmosphere_scattering.h are m^-1.
 *
 * sun_phi / sun_theta are radians. theta is the sun zenith angle from +Y.
 */

void atmosphere_lut_init(void);
void atmosphere_lut_shutdown(void);

void atmosphere_lut_generate_transmittance(void);
void atmosphere_lut_generate_sky_view(float sun_phi, float sun_theta);

uint32_t atmosphere_lut_version(void);
size_t atmosphere_lut_memory_bytes(void);

void atmosphere_lut_transmittance_size(int *out_width, int *out_height);
void atmosphere_lut_sky_view_size(int *out_width, int *out_height);

const float *atmosphere_lut_transmittance_data(void);
const float *atmosphere_lut_sky_view_data(void);

/* RGBA. Alpha is 1 when the tables are initialized, else 0. */
void atmosphere_lut_sample_transmittance(float u, float v, float out_rgba[4]);
void atmosphere_lut_sample_sky_view(float u, float v, float sun_phi,
                                   float sun_theta, float out_rgba[4]);

#endif /* ATMOSPHERE_LUT_H */
