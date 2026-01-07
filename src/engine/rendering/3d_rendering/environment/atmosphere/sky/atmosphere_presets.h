#ifndef ATMOSPHERE_PRESETS_H
#define ATMOSPHERE_PRESETS_H

#include "atmosphere_scattering.h"

/**
 * @file atmosphere_presets.h
 * @brief Pre-configured atmospheric presets
 * 
 * Provides ready-to-use atmospheric configurations for various scenarios
 */

// =============================================================================
// Atmosphere Presets
// =============================================================================

/**
 * Get Earth atmosphere preset (clear day)
 */
static inline void atm_preset_earth_clear(ScatteringCoefficients* coeffs, AtmosphereLayerParams* profiles) {
    if (coeffs) {
        *coeffs = atm_get_earth_coefficients();
    }
    
    if (profiles) {
        atm_get_earth_density_profiles(profiles);
    }
}

/**
 * Get Earth atmosphere preset (hazy/polluted)
 */
static inline void atm_preset_earth_hazy(ScatteringCoefficients* coeffs, AtmosphereLayerParams* profiles) {
    if (coeffs) {
        *coeffs = atm_get_earth_coefficients();
        // Increase Mie scattering for haze
        coeffs->mie *= 2.5f;
    }
    
    if (profiles) {
        atm_get_earth_density_profiles(profiles);
        // Lower Mie scale height for thicker haze near ground
        profiles->mie.exp_scale = 0.8f;
    }
}

/**
 * Get Mars atmosphere preset
 * Thin CO2 atmosphere with reddish tint
 */
static inline void atm_preset_mars(ScatteringCoefficients* coeffs, AtmosphereLayerParams* profiles) {
    if (coeffs) {
        // Much thinner atmosphere
        coeffs->rayleigh = simd_make_float3(0.001f, 0.0008f, 0.0006f);
        coeffs->mie = 0.004f;
        coeffs->ozone = simd_make_float3(0.0f, 0.0f, 0.0f); // No ozone on Mars
    }
    
    if (profiles) {
        // Mars atmosphere is much thinner
        profiles->rayleigh.exp_term = 1.0f;
        profiles->rayleigh.exp_scale = 11.1f; // km
        profiles->rayleigh.linear_term = 0.0f;
        profiles->rayleigh.constant_term = 0.0f;
        profiles->rayleigh.width = 50.0f;
        
        profiles->mie.exp_term = 1.0f;
        profiles->mie.exp_scale = 5.0f;
        profiles->mie.linear_term = 0.0f;
        profiles->mie.constant_term = 0.0f;
        profiles->mie.width = 50.0f;
        
        // No ozone
        profiles->ozone.exp_term = 0.0f;
        profiles->ozone.exp_scale = 1.0f;
        profiles->ozone.linear_term = 0.0f;
        profiles->ozone.constant_term = 0.0f;
        profiles->ozone.width = 0.0f;
    }
}

/**
 * Get alien atmosphere preset (blue/cyan world)
 */
static inline void atm_preset_alien_blue(ScatteringCoefficients* coeffs, AtmosphereLayerParams* profiles) {
    if (coeffs) {
        // Strong blue scattering
        coeffs->rayleigh = simd_make_float3(10e-3f, 20e-3f, 50e-3f);
        coeffs->mie = 25e-3f;
        coeffs->ozone = simd_make_float3(0.0f, 0.0f, 0.0f);
    }
    
    if (profiles) {
        atm_get_earth_density_profiles(profiles);
    }
}

/**
 * Get alien atmosphere preset (purple/magenta world)
 */
static inline void atm_preset_alien_purple(ScatteringCoefficients* coeffs, AtmosphereLayerParams* profiles) {
    if (coeffs) {
        // Purple atmosphere (strong red and blue, weak green)
        coeffs->rayleigh = simd_make_float3(15e-3f, 3e-3f, 18e-3f);
        coeffs->mie = 20e-3f;
        coeffs->ozone = simd_make_float3(0.0f, 0.0f, 0.0f);
    }
    
    if (profiles) {
        atm_get_earth_density_profiles(profiles);
        // Thicker atmosphere
        profiles->rayleigh.exp_scale = 12.0f;
    }
}

/**
 * Get alien atmosphere preset (toxic green world)
 */
static inline void atm_preset_alien_toxic(ScatteringCoefficients* coeffs, AtmosphereLayerParams* profiles) {
    if (coeffs) {
        // Green-dominated atmosphere
        coeffs->rayleigh = simd_make_float3(3e-3f, 25e-3f, 5e-3f);
        coeffs->mie = 30e-3f;
        coeffs->ozone = simd_make_float3(5e-3f, 0e-3f, 8e-3f);
    }
    
    if (profiles) {
        atm_get_earth_density_profiles(profiles);
        // Dense low-altitude haze
        profiles->mie.exp_scale = 0.6f;
    }
}

// =============================================================================
// Time of Day Sun Positions
// =============================================================================

/**
 * Calculate sun direction for a given time of day
 * 
 * @param time_of_day Hours since midnight [0, 24]
 * @param latitude Latitude in degrees [-90, 90]
 * @param day_of_year Day of year [0, 365]
 * @return Normalized sun direction vector
 */
static inline simd_float3 atm_sun_direction_from_time(float time_of_day, float latitude, float day_of_year) {
    // Simplified solar position calculation
    // For production use, implement proper astronomical calculations
    
    // Hour angle: 360° / 24h = 15° per hour
    // 12:00 = 0° (sun at zenith), 18:00 = 90° (sunset)
    float hour_angle_deg = (time_of_day - 12.0f) * 15.0f;
    float hour_angle_rad = hour_angle_deg * (ATM_PI / 180.0f);
    
    // Solar declination (simplified, ignores day of year for now)
    float declination_rad = 0.0f; // Equinox
    
    // Latitude in radians
    float lat_rad = latitude * (ATM_PI / 180.0f);
    
    // Calculate sun altitude (angle above horizon)
    float sin_altitude = sinf(lat_rad) * sinf(declination_rad) +
                         cosf(lat_rad) * cosf(declination_rad) * cosf(hour_angle_rad);
    float altitude_rad = asinf(sin_altitude);
    
    // Calculate sun azimuth
    float cos_azimuth = (sinf(declination_rad) - sinf(lat_rad) * sin_altitude) /
                        (cosf(lat_rad) * cosf(altitude_rad));
    float azimuth_rad = acosf(fmaxf(-1.0f, fminf(1.0f, cos_azimuth)));
    
    if (hour_angle_rad > 0.0f) {
        azimuth_rad = 2.0f * ATM_PI - azimuth_rad;
    }
    
    // Convert to Cartesian coordinates
    // Y is up
    float sun_y = sinf(altitude_rad);
    float horizontal_dist = cosf(altitude_rad);
    float sun_x = horizontal_dist * sinf(azimuth_rad);
    float sun_z = horizontal_dist * cosf(azimuth_rad);
    
    simd_float3 sun_dir = simd_make_float3(sun_x, sun_y, sun_z);
    return simd_normalize(sun_dir);
}

/**
 * Get sun intensity for time of day
 * Sun intensity decreases near sunrise/sunset
 * 
 * @param sun_direction Normalized sun direction vector
 * @return RGB sun intensity
 */
static inline simd_float3 atm_sun_intensity_from_direction(simd_float3 sun_direction) {
    float sun_elevation_sin = sun_direction.y; // Y component = sin(elevation)
    
    // Sun intensity falls off near horizon
    float intensity_scale = fmaxf(0.0f, sun_elevation_sin);
    intensity_scale = powf(intensity_scale, 0.4f); // Gentle falloff
    
    // Base sun intensity (white light)
    float base_intensity = 10.0f;
    
    return simd_make_float3(1.0f, 1.0f, 1.0f) * (base_intensity * intensity_scale);
}

// =============================================================================
// Preset Helper Functions
// =============================================================================

/**
 * Blend between two atmosphere presets
 * 
 * @param coeffs_a First preset coefficients
 * @param coeffs_b Second preset coefficients
 * @param t Blend factor [0, 1]
 * @param out_coeffs Output blended coefficients
 */
static inline void atm_blend_coefficients(
    ScatteringCoefficients coeffs_a,
    ScatteringCoefficients coeffs_b,
    float t,
    ScatteringCoefficients* out_coeffs
) {
    if (!out_coeffs) return;
    
    t = fmaxf(0.0f, fminf(1.0f, t));
    
    out_coeffs->rayleigh = coeffs_a.rayleigh * (1.0f - t) + coeffs_b.rayleigh * t;
    out_coeffs->mie = coeffs_a.mie * (1.0f - t) + coeffs_b.mie * t;
    out_coeffs->ozone = coeffs_a.ozone * (1.0f - t) + coeffs_b.ozone * t;
}

#endif // ATMOSPHERE_PRESETS_H
