/**
 * @file atmosphere_scattering.c
 * @brief Atmospheric scattering physics implementations
 * @details Production-grade atmospheric scattering calculations
 */

#include "environment/atmosphere/sky/atmosphere_scattering.h"
#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// Physical Constants
// =============================================================================

// Avogadro's number
#define AVOGADRO 6.02214076e23

// Standard atmospheric pressure at sea level (Pa)
#define STD_PRESSURE 101325.0

// Standard temperature at sea level (K)
#define STD_TEMPERATURE 288.15

// Gas constant for dry air (J/(kg·K))
#define GAS_CONSTANT_AIR 287.058

// Molecular weight of air (kg/mol)
#define MOLECULAR_WEIGHT_AIR 0.0289644

// Depolarization factor for air
#define DEPOLARIZATION_FACTOR 0.035

// =============================================================================
// Rayleigh Scattering
// =============================================================================

float atm_rayleigh_coefficient(float wavelength, float refractive_index, float molecular_density) {
    // Wavelength in meters
    float lambda_m = wavelength * 1e-6;
    
    // Calculate (n² - 1)²
    float n_sq_minus_1 = (refractive_index * refractive_index - 1.0f);
    float n_sq_minus_1_sq = n_sq_minus_1 * n_sq_minus_1;
    
    // Depolarization factor correction
    float depol_factor = (6.0f + 3.0f * DEPOLARIZATION_FACTOR) / (6.0f - 7.0f * DEPOLARIZATION_FACTOR);
    
    // Rayleigh scattering formula
    // β_R = (8π³(n²-1)²)/(3Nλ⁴) * depolarization_factor
    float numerator = 8.0f * ATM_PI * ATM_PI * ATM_PI * n_sq_minus_1_sq * depol_factor;
    float denominator = 3.0f * molecular_density * lambda_m * lambda_m * lambda_m * lambda_m;
    
    return numerator / denominator;
}

simd_float3 atm_rayleigh_coefficient_rgb(void) {
    // Standard refractive index of air at STP
    float n = 1.00029;
    
    // Molecular number density at sea level
    // ρ = (P * M) / (R * T)
    // N = (ρ * N_A) / M
    float density_kg_m3 = (STD_PRESSURE * MOLECULAR_WEIGHT_AIR) / (GAS_CONSTANT_AIR * STD_TEMPERATURE);
    float molecular_density = (density_kg_m3 * AVOGADRO) / MOLECULAR_WEIGHT_AIR;
    
    // Calculate for each wavelength
    float r = atm_rayleigh_coefficient(ATM_WAVELENGTH_R, n, molecular_density);
    float g = atm_rayleigh_coefficient(ATM_WAVELENGTH_G, n, molecular_density);
    float b = atm_rayleigh_coefficient(ATM_WAVELENGTH_B, n, molecular_density);
    
    // Convert from m^-1 to km^-1 for our coordinate system
    return simd_make_float3(r * 1000.0f, g * 1000.0f, b * 1000.0f);
}

float atm_rayleigh_phase(float cos_theta) {
    // P_R(θ) = 3/(16π) * (1 + cos²θ)
    float cos_theta_sq = cos_theta * cos_theta;
    return (3.0f / (16.0f * ATM_PI)) * (1.0f + cos_theta_sq);
}

// =============================================================================
// Mie Scattering
// =============================================================================

float atm_mie_coefficient(float turbidity, float beta_base) {
    // Mie scattering increases with turbidity
    // Typical beta_base for Earth: 21e-6 m^-1 = 0.021 km^-1
    return beta_base * turbidity / 2.0f;
}

float atm_mie_phase_hg(float cos_theta, float g) {
    // Henyey-Greenstein phase function
    // P(θ) = (1-g²) / (4π(1+g²-2g·cosθ)^(3/2))
    
    float g_sq = g * g;
    float numerator = 1.0f - g_sq;
    float denominator_base = 1.0f + g_sq - 2.0f * g * cos_theta;
    float denominator = 4.0f * ATM_PI * powf(denominator_base, 1.5f);
    
    return numerator / denominator;
}

float atm_mie_phase_cornette_shanks(float cos_theta, float g) {
    // Cornette-Shanks phase function (improved over H-G)
    // P(θ) = 3(1-g²)(1+cos²θ) / (2(2+g²)(1+g²-2g·cosθ)^(3/2))
    
    float g_sq = g * g;
    float cos_theta_sq = cos_theta * cos_theta;
    
    float numerator = 3.0f * (1.0f - g_sq) * (1.0f + cos_theta_sq);
    float denominator_factor = 1.0f + g_sq - 2.0f * g * cos_theta;
    float denominator = 2.0f * (2.0f + g_sq) * powf(denominator_factor, 1.5f);
    
    return numerator / denominator;
}

// =============================================================================
// Ozone Absorption
// =============================================================================

simd_float3 atm_ozone_absorption_rgb(void) {
    // Ozone absorption coefficients (Chappuis band)
    // Ozone absorbs strongly in red and UV, weakly in green/blue
    // Values in m^-1, converted to km^-1
    
    // Approximate absorption cross-sections at our wavelengths
    float abs_r = 0.65e-6f * 1000.0f;  // Strong red absorption
    float abs_g = 0.18e-6f * 1000.0f;  // Medium green absorption
    float abs_b = 0.08e-6f * 1000.0f;  // Weak blue absorption
    
    return simd_make_float3(abs_r, abs_g, abs_b);
}

// =============================================================================
// Density Profiles
// =============================================================================

float atm_density_profile_eval(float altitude, AtmosphereDensityProfile profile) {
    if (altitude < 0.0f) return 0.0f;
    
    // Exponential term: exp(-altitude / scale_height)
    float exp_component = 0.0f;
    if (profile.exp_scale > 0.0f) {
        exp_component = profile.exp_term * expf(-altitude / profile.exp_scale);
    }
    
    // Linear term
    float linear_component = profile.linear_term * altitude;
    
    // Combined density
    float density = exp_component + linear_component + profile.constant_term;
    
    // Clamp to [0, 1]
    if (density < 0.0f) density = 0.0f;
    if (density > 1.0f) density = 1.0f;
    
    return density;
}

void atm_get_earth_density_profiles(AtmosphereLayerParams* out_params) {
    // Rayleigh density: exponential with 8km scale height
    out_params->rayleigh.exp_term = 1.0f;
    out_params->rayleigh.exp_scale = 8.0f;
    out_params->rayleigh.linear_term = 0.0f;
    out_params->rayleigh.constant_term = 0.0f;
    out_params->rayleigh.width = 100.0f;
    
    // Mie density: exponential with 1.2km scale height
    out_params->mie.exp_term = 1.0f;
    out_params->mie.exp_scale = 1.2f;
    out_params->mie.linear_term = 0.0f;
    out_params->mie.constant_term = 0.0f;
    out_params->mie.width = 100.0f;
    
    // Ozone density: tent function centered at 25km with 15km width
    // Using a simple exponential approximation
    out_params->ozone.exp_term = 1.0f;
    out_params->ozone.exp_scale = 15.0f;
    out_params->ozone.linear_term = 0.0f;
    out_params->ozone.constant_term = 0.0f;
    out_params->ozone.width = 50.0f;
}

// =============================================================================
// Optical Depth & Transmittance
// =============================================================================

simd_float3 atm_optical_depth(
    simd_float3 origin,
    simd_float3 direction,
    float max_distance,
    float planet_radius,
    float atmo_height,
    ScatteringCoefficients coeffs,
    AtmosphereLayerParams profiles,
    int num_samples
) {
    simd_float3 optical_depth = simd_make_float3(0.0f, 0.0f, 0.0f);
    
    if (num_samples <= 0) num_samples = 64;
    
    float step_size = max_distance / (float)num_samples;
    
    for (int i = 0; i < num_samples; i++) {
        // Sample point along ray
        float t = (float)i * step_size + step_size * 0.5f;
        simd_float3 sample_pos = origin + direction * t;
        
        // Calculate altitude
        float altitude = simd_length(sample_pos) - planet_radius;
        
        // Skip if below surface or above atmosphere
        if (altitude < 0.0f || altitude > atmo_height) continue;
        
        // Evaluate density at this altitude
        float rho_rayleigh = atm_density_profile_eval(altitude, profiles.rayleigh);
        float rho_mie = atm_density_profile_eval(altitude, profiles.mie);
        float rho_ozone = atm_density_profile_eval(altitude, profiles.ozone);
        
        // Accumulate optical depth
        // τ = Σ (β_scatter * ρ * ds)
        simd_float3 extinction_rayleigh = coeffs.rayleigh * rho_rayleigh;
        simd_float3 extinction_mie = simd_make_float3(coeffs.mie, coeffs.mie, coeffs.mie) * rho_mie;
        simd_float3 extinction_ozone = coeffs.ozone * rho_ozone;
        
        optical_depth += (extinction_rayleigh + extinction_mie + extinction_ozone) * step_size;
    }
    
    return optical_depth;
}

simd_float3 atm_transmittance_from_optical_depth(simd_float3 optical_depth) {
    // T = exp(-τ)
    return simd_make_float3(
        expf(-optical_depth.x),
        expf(-optical_depth.y),
        expf(-optical_depth.z)
    );
}

// =============================================================================
// Combined Scattering
// =============================================================================

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
) {
    simd_float3 inscatter = simd_make_float3(0.0f, 0.0f, 0.0f);
    
    if (num_samples <= 0) num_samples = 32;
    
    float step_size = max_distance / (float)num_samples;
    float cos_theta = simd_dot(view_dir, sun_dir);
    
    // Phase functions
    float phase_rayleigh = atm_rayleigh_phase(cos_theta);
    float phase_mie = atm_mie_phase_hg(cos_theta, 0.8f);
    
    for (int i = 0; i < num_samples; i++) {
        float t = (float)i * step_size + step_size * 0.5f;
        simd_float3 sample_pos = view_origin + view_dir * t;
        
        float altitude = simd_length(sample_pos) - planet_radius;
        if (altitude < 0.0f || altitude > atmo_height) continue;
        
        // Density at sample point
        float rho_rayleigh = atm_density_profile_eval(altitude, profiles.rayleigh);
        float rho_mie = atm_density_profile_eval(altitude, profiles.mie);
        
        // Transmittance from camera to sample (simplified - should use LUT)
        simd_float3 transmittance_view = simd_make_float3(1.0f, 1.0f, 1.0f);
        
        // Transmittance from sample to sun (simplified - should use LUT)
        simd_float3 transmittance_sun = simd_make_float3(1.0f, 1.0f, 1.0f);
        
        // In-scattering contribution
        simd_float3 scatter_rayleigh = coeffs.rayleigh * rho_rayleigh * phase_rayleigh;
        simd_float3 scatter_mie = simd_make_float3(coeffs.mie, coeffs.mie, coeffs.mie) * rho_mie * phase_mie;
        
        inscatter += (scatter_rayleigh + scatter_mie) * transmittance_view * transmittance_sun * step_size;
    }
    
    return inscatter;
}

// =============================================================================
// Utility Functions
// =============================================================================

bool atm_ray_sphere_intersect(
    simd_float3 origin,
    simd_float3 direction,
    simd_float3 sphere_center,
    float sphere_radius,
    float* out_t1,
    float* out_t2
) {
    simd_float3 oc = origin - sphere_center;
    
    float a = simd_dot(direction, direction);
    float b = 2.0f * simd_dot(oc, direction);
    float c = simd_dot(oc, oc) - sphere_radius * sphere_radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) {
        return false;
    }
    
    float sqrt_discriminant = sqrtf(discriminant);
    float t1 = (-b - sqrt_discriminant) / (2.0f * a);
    float t2 = (-b + sqrt_discriminant) / (2.0f * a);
    
    if (out_t1) *out_t1 = t1;
    if (out_t2) *out_t2 = t2;
    
    return true;
}

ScatteringCoefficients atm_get_earth_coefficients(void) {
    ScatteringCoefficients coeffs;
    
    // Rayleigh scattering (automatically in km^-1)
    coeffs.rayleigh = atm_rayleigh_coefficient_rgb();
    
    // Mie scattering (convert from m^-1 to km^-1)
    // Base value: 21e-6 m^-1
    coeffs.mie = 21e-6f * 1000.0f;
    
    // Ozone absorption
    coeffs.ozone = atm_ozone_absorption_rgb();
    
    return coeffs;
}

bool atm_validate_parameters(ScatteringCoefficients coeffs, float planet_radius, float atmo_height) {
    // Check for NaN or Inf
    if (isnan(coeffs.rayleigh.x) || isnan(coeffs.rayleigh.y) || isnan(coeffs.rayleigh.z)) return false;
    if (isnan(coeffs.mie) || isnan(planet_radius) || isnan(atmo_height)) return false;
    if (isinf(coeffs.rayleigh.x) || isinf(coeffs.rayleigh.y) || isinf(coeffs.rayleigh.z)) return false;
    if (isinf(coeffs.mie) || isinf(planet_radius) || isinf(atmo_height)) return false;
    
    // Check for negative values
    if (coeffs.rayleigh.x < 0.0f || coeffs.rayleigh.y < 0.0f || coeffs.rayleigh.z < 0.0f) return false;
    if (coeffs.mie < 0.0f) return false;
    if (planet_radius <= 0.0f || atmo_height <= 0.0f) return false;
    
    // Check reasonable ranges (Earth-like)
    if (planet_radius < 1000.0f || planet_radius > 100000.0f) return false; // 1000km - 100000km
    if (atmo_height < 10.0f || atmo_height > 1000.0f) return false; // 10km - 1000km
    
    return true;
}
