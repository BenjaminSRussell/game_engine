#ifndef ATMOSPHERE_SCATTERING_H
#define ATMOSPHERE_SCATTERING_H

#include <simd/simd.h>
#include <math.h>
#include <stdbool.h>

/**
 * @file atmosphere_scattering.h
 * @brief Atmospheric scattering physics calculations
 * 
 * Implements physically-based Rayleigh and Mie scattering for realistic
 * atmospheric rendering. Based on research by Bruneton & Neyret (2008)
 * and Hillaire (2020).
 */

// Physical constants
#define ATM_PI 3.14159265358979323846
#define ATM_SPEED_OF_LIGHT 299792458.0  // m/s
#define ATM_EARTH_RADIUS 6360.0         // km
#define ATM_ATMOSPHERE_HEIGHT 100.0     // km

// Wavelengths for RGB channels (in micrometers)
#define ATM_WAVELENGTH_R 0.680  // 680nm (red)
#define ATM_WAVELENGTH_G 0.550  // 550nm (green)
#define ATM_WAVELENGTH_B 0.440  // 440nm (blue)

/**
 * Density profile for atmospheric layers
 * Supports exponential decay with optional linear component
 */
typedef struct {
    float exp_term;      // Exponential coefficient
    float exp_scale;     // Scale height for exponential term (km)
    float linear_term;   // Linear coefficient
    float constant_term; // Constant offset
    float width;         // Layer width (km)
} AtmosphereDensityProfile;

/**
 * Complete atmospheric layer parameters
 */
typedef struct {
    AtmosphereDensityProfile rayleigh;
    AtmosphereDensityProfile mie;
    AtmosphereDensityProfile ozone;
} AtmosphereLayerParams;

/**
 * Scattering coefficients structure
 */
typedef struct {
    simd_float3 rayleigh;  // Rayleigh scattering coefficient (m^-1)
    float mie;             // Mie scattering coefficient (m^-1)
    simd_float3 ozone;     // Ozone absorption coefficient (m^-1)
} ScatteringCoefficients;

// =============================================================================
// Rayleigh Scattering
// =============================================================================

/**
 * Calculate Rayleigh scattering coefficient for a given wavelength
 * 
 * Formula: β_R(λ) = (8π³(n²-1)²)/(3Nλ⁴)
 * where n is refractive index, N is molecular density, λ is wavelength
 * 
 * @param wavelength Wavelength in micrometers
 * @param refractive_index Refractive index of air (typically 1.00029)
 * @param molecular_density Number density at sea level (molecules/m³)
 * @return Rayleigh scattering coefficient (m^-1)
 */
float atm_rayleigh_coefficient(float wavelength, float refractive_index, float molecular_density);

/**
 * Calculate RGB Rayleigh scattering coefficients
 * 
 * Uses standard wavelengths: R=680nm, G=550nm, B=440nm
 * 
 * @return RGB Rayleigh coefficients (m^-1)
 */
simd_float3 atm_rayleigh_coefficient_rgb(void);

/**
 * Rayleigh phase function
 * 
 * Formula: P_R(θ) = 3/(16π) * (1 + cos²θ)
 * 
 * @param cos_theta Cosine of scattering angle
 * @return Phase function value
 */
float atm_rayleigh_phase(float cos_theta);

// =============================================================================
// Mie Scattering
// =============================================================================

/**
 * Calculate Mie scattering coefficient
 * 
 * @param turbidity Atmospheric turbidity (typically 2.0-4.0)
 * @param beta_base Base Mie coefficient at sea level (m^-1)
 * @return Mie scattering coefficient (m^-1)
 */
float atm_mie_coefficient(float turbidity, float beta_base);

/**
 * Henyey-Greenstein phase function (approximates Mie scattering)
 * 
 * Formula: P_M(θ) = (1-g²) / (4π(1+g²-2g·cosθ)^(3/2))
 * 
 * @param cos_theta Cosine of scattering angle
 * @param g Asymmetry factor [-1, 1] (typically 0.76-0.8 for Earth)
 * @return Phase function value
 */
float atm_mie_phase_hg(float cos_theta, float g);

/**
 * Cornette-Shanks phase function (improved Mie approximation)
 * Better than Henyey-Greenstein for forward scattering
 * 
 * @param cos_theta Cosine of scattering angle
 * @param g Asymmetry factor
 * @return Phase function value
 */
float atm_mie_phase_cornette_shanks(float cos_theta, float g);

// =============================================================================
// Ozone Absorption
// =============================================================================

/**
 * Calculate ozone absorption coefficient (RGB)
 * Ozone absorbs primarily in the red and UV spectrum
 * 
 * @return RGB ozone absorption coefficients (m^-1)
 */
simd_float3 atm_ozone_absorption_rgb(void);

// =============================================================================
// Density Profiles
// =============================================================================

/**
 * Evaluate atmospheric density at a given altitude
 * 
 * @param altitude Height above surface (km)
 * @param profile Density profile parameters
 * @return Density [0, 1]
 */
float atm_density_profile_eval(float altitude, AtmosphereDensityProfile profile);

/**
 * Get standard Earth atmosphere density profiles
 * 
 * @param out_params Output parameter structure
 */
void atm_get_earth_density_profiles(AtmosphereLayerParams* out_params);

// =============================================================================
// Optical Depth & Transmittance
// =============================================================================

/**
 * Calculate optical depth along a ray through the atmosphere
 * 
 * @param origin Ray origin (km from planet center)
 * @param direction Ray direction (normalized)
 * @param max_distance Maximum integration distance (km)
 * @param planet_radius Planet radius (km)
 * @param atmo_height Atmosphere height (km)
 * @param coeffs Scattering coefficients
 * @param profiles Density profiles
 * @param num_samples Number of integration samples
 * @return RGB optical depth
 */
simd_float3 atm_optical_depth(
    simd_float3 origin,
    simd_float3 direction,
    float max_distance,
    float planet_radius,
    float atmo_height,
    ScatteringCoefficients coeffs,
    AtmosphereLayerParams profiles,
    int num_samples
);

/**
 * Calculate transmittance from optical depth
 * 
 * Formula: T = exp(-τ)
 * 
 * @param optical_depth RGB optical depth
 * @return RGB transmittance [0, 1]
 */
simd_float3 atm_transmittance_from_optical_depth(simd_float3 optical_depth);

// =============================================================================
// Combined Scattering
// =============================================================================

/**
 * Calculate combined in-scattering (Rayleigh + Mie + multi-scattering)
 * 
 * @param view_origin View ray origin
 * @param view_dir View direction
 * @param sun_dir Sun direction
 * @param max_distance Maximum raymarch distance
 * @param planet_radius Planet radius
 * @param atmo_height Atmosphere height
 * @param coeffs Scattering coefficients
 * @param profiles Density profiles
 * @param num_samples Number of samples
 * @return RGB inscattering color
 */
simd_float3 atm_combined_scattering(
    simd_float3 view_origin,
    simd_float3 view_dir,
    simd_float3 sun_dir,
    float max_distance,
    float planet_radius,
    float atmo_height,
    ScatteringCoefficients coeffs,
    AtmosphereLayerParams profiles,
    int num_samples
);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Ray-sphere intersection
 * 
 * @param origin Ray origin
 * @param direction Ray direction (normalized)
 * @param sphere_center Sphere center
 * @param sphere_radius Sphere radius
 * @param out_t1 First intersection distance (optional)
 * @param out_t2 Second intersection distance (optional)
 * @return true if intersection exists
 */
bool atm_ray_sphere_intersect(
    simd_float3 origin,
    simd_float3 direction,
    simd_float3 sphere_center,
    float sphere_radius,
    float* out_t1,
    float* out_t2
);

/**
 * Get default scattering coefficients for Earth
 * 
 * @return Earth scattering coefficients
 */
ScatteringCoefficients atm_get_earth_coefficients(void);

/**
 * Validate atmospheric parameters
 * 
 * @param coeffs Scattering coefficients to validate
 * @param planet_radius Planet radius
 * @param atmo_height Atmosphere height
 * @return true if parameters are valid
 */
bool atm_validate_parameters(ScatteringCoefficients coeffs, float planet_radius, float atmo_height);

#endif // ATMOSPHERE_SCATTERING_H
