#include "audio/spatial/distance_attenuation.h"
#include "core/core.h"
#include <include/math/math_all.h>
#include <string.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                          CUSTOM DISTANCE ATTENUATION - AGENT_AUDIO_2
 * =================================================================================================
 *
 * PURPOSE: Build custom distance attenuation curves with various falloff models.
 * =================================================================================================
 */

// -------------------------------------------------------------------------------------------------
// GLOBAL DISTANCE ATTENUATION STATE
// -------------------------------------------------------------------------------------------------

static DistanceAttenuationSystem g_attenuation_system = {0};
static bool g_attenuation_initialized = false;

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Distance attenuation system implementation
// -------------------------------------------------------------------------------------------------

bool DistanceAttenuation_Init(DistanceAttenuationSystem* system, u32 sample_rate) {
    if (!system) return false;
    
    system->sample_rate = sample_rate;
    system->curve_count = 6;  // Support 6 different attenuation curves
    
    // Allocate curve definitions
    system->curves = malloc(sizeof(AttenuationCurve) * system->curve_count);
    if (!system->curves) {
        return false;
    }
    
    // Initialize standard attenuation curves
    system->curves[0] = (AttenuationCurve){
        ATTENUATION_INVERSE, "Inverse Square", 0.5f, 50.0f, 1.0f
    };
    system->curves[1] = (AttenuationCurve){
        ATTENUATION_INVERSE_SQUARE, "Inverse Square", 1.0f, 100.0f, 1.0f
    };
    system->curves[2] = (AttenuationCurve){
        ATTENUATION_LINEAR, "Linear", 2.0f, 30.0f, 1.0f
    };
    system->curves[3] = (AttenuationCurve){
        ATTENUATION_EXPONENTIAL, "Exponential", 1.0f, 40.0f, 2.0f
    };
    system->curves[4] = (AttenuationCurve){
        ATTENUATION_LOGARITHMIC, "Logarithmic", 0.5f, 60.0f, 0.5f
    };
    system->curves[5] = (AttenuationCurve){
        ATTENUATION_CUSTOM, "Custom", 1.0f, 25.0f, 1.5f
    };
    
    // Initialize custom curve parameters
    system->custom_curve.points = malloc(sizeof(CurvePoint) * 16);  // Support 16 custom points
    system->custom_curve.point_count = 0;
    
    if (!system->custom_curve.points) {
        free(system->curves);
        return false;
    }
    
    // Set default curve
    system->current_curve_type = ATTENUATION_INVERSE_SQUARE;
    system->master_volume = 1.0f;
    system->distance_scale = 1.0f;
    
    // Initialize air absorption parameters
    system->air_absorption_enabled = true;
    system->air_absorption_factor = 0.001f;  // Absorption per meter
    system->reference_frequency = 1000.0f;   // 1kHz reference
    
    system->initialized = true;
    printf("Distance Attenuation System initialized: %d curves\n", system->curve_count);
    return true;
}

void DistanceAttenuation_Shutdown(DistanceAttenuationSystem* system) {
    if (system && system->initialized) {
        if (system->curves) {
            free(system->curves);
            system->curves = NULL;
        }
        if (system->custom_curve.points) {
            free(system->custom_curve.points);
            system->custom_curve.points = NULL;
        }
        system->initialized = false;
        printf("Distance Attenuation System shutdown\n");
    }
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Standard attenuation curves
// -------------------------------------------------------------------------------------------------

f32 DistanceAttenuation_CalculateInverse(DistanceAttenuationSystem* system, f32 distance) {
    // Inverse distance law: gain = min_distance / (min_distance + rolloff * (distance - min_distance))
    AttenuationCurve* curve = &system->curves[system->current_curve_type];
    
    if (distance <= curve->min_distance) {
        return 1.0f;
    }
    
    f32 rolloff_distance = distance - curve->min_distance;
    f32 attenuation = curve->min_distance / (curve->min_distance + curve->rolloff_factor * rolloff_distance);
    
    return fmaxf(0.0f, attenuation);
}

f32 DistanceAttenuation_CalculateInverseSquare(DistanceAttenuationSystem* system, f32 distance) {
    // Inverse square law: gain = (min_distance / distance)^2
    AttenuationCurve* curve = &system->curves[system->current_curve_type];
    
    if (distance <= curve->min_distance) {
        return 1.0f;
    }
    
    f32 ratio = curve->min_distance / distance;
    f32 attenuation = ratio * ratio;  // Square the ratio
    
    return fmaxf(0.0f, attenuation);
}

f32 DistanceAttenuation_CalculateLinear(DistanceAttenuationSystem* system, f32 distance) {
    // Linear attenuation: gain = 1.0 - (distance - min_distance) / (max_distance - min_distance)
    AttenuationCurve* curve = &system->curves[system->current_curve_type];
    
    if (distance <= curve->min_distance) {
        return 1.0f;
    }
    
    if (distance >= curve->max_distance) {
        return 0.0f;
    }
    
    f32 normalized_distance = (distance - curve->min_distance) / (curve->max_distance - curve->min_distance);
    f32 attenuation = 1.0f - normalized_distance;
    
    return fmaxf(0.0f, attenuation);
}

f32 DistanceAttenuation_CalculateExponential(DistanceAttenuationSystem* system, f32 distance) {
    // Exponential attenuation: gain = exp(-rolloff * (distance - min_distance))
    AttenuationCurve* curve = &system->curves[system->current_curve_type];
    
    if (distance <= curve->min_distance) {
        return 1.0f;
    }
    
    f32 excess_distance = distance - curve->min_distance;
    f32 attenuation = expf(-curve->rolloff_factor * excess_distance);
    
    return fmaxf(0.0f, attenuation);
}

f32 DistanceAttenuation_CalculateLogarithmic(DistanceAttenuationSystem* system, f32 distance) {
    // Logarithmic attenuation: gain = 20 * log10(min_distance / distance)
    AttenuationCurve* curve = &system->curves[system->current_curve_type];
    
    if (distance <= curve->min_distance) {
        return 1.0f;
    }
    
    f32 ratio = curve->min_distance / distance;
    f32 db_attenuation = 20.0f * log10f(ratio) * curve->rolloff_factor;
    f32 attenuation = powf(10.0f, db_attenuation / 20.0f);
    
    return fmaxf(0.0f, attenuation);
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Custom curve interpolation
// -------------------------------------------------------------------------------------------------

bool DistanceAttenuation_AddCustomPoint(DistanceAttenuationSystem* system, f32 distance, f32 gain) {
    if (!system || !system->initialized || system->custom_curve.point_count >= 16) {
        return false;
    }
    
    // Add point to custom curve
    system->custom_curve.points[system->custom_curve.point_count] = (CurvePoint){distance, gain};
    system->custom_curve.point_count++;
    
    // Sort points by distance
    for (u32 i = system->custom_curve.point_count - 1; i > 0; i--) {
        if (system->custom_curve.points[i].distance < system->custom_curve.points[i - 1].distance) {
            // Swap
            CurvePoint temp = system->custom_curve.points[i];
            system->custom_curve.points[i] = system->custom_curve.points[i - 1];
            system->custom_curve.points[i - 1] = temp;
        }
    }
    
    printf("Added custom attenuation point: distance=%.2f, gain=%.3f\n", distance, gain);
    return true;
}

f32 DistanceAttenuation_CalculateCustom(DistanceAttenuationSystem* system, f32 distance) {
    if (system->custom_curve.point_count < 2) {
        return 1.0f;  // Not enough points for interpolation
    }
    
    // Find surrounding points for interpolation
    u32 lower_index = 0;
    u32 upper_index = system->custom_curve.point_count - 1;
    
    for (u32 i = 0; i < system->custom_curve.point_count - 1; i++) {
        if (distance >= system->custom_curve.points[i].distance && 
            distance <= system->custom_curve.points[i + 1].distance) {
            lower_index = i;
            upper_index = i + 1;
            break;
        }
    }
    
    // Handle out-of-range distances
    if (distance < system->custom_curve.points[0].distance) {
        return system->custom_curve.points[0].gain;
    }
    if (distance > system->custom_curve.points[system->custom_curve.point_count - 1].distance) {
        return system->custom_curve.points[system->custom_curve.point_count - 1].gain;
    }
    
    // Linear interpolation between points
    CurvePoint* lower = &system->custom_curve.points[lower_index];
    CurvePoint* upper = &system->custom_curve.points[upper_index];
    
    f32 distance_range = upper->distance - lower->distance;
    if (distance_range <= 0.0f) {
        return lower->gain;
    }
    
    f32 t = (distance - lower->distance) / distance_range;
    f32 gain = lower->gain + t * (upper->gain - lower->gain);
    
    return fmaxf(0.0f, fminf(1.0f, gain));
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Air absorption modeling
// -------------------------------------------------------------------------------------------------

f32 DistanceAttenuation_CalculateAirAbsorption(DistanceAttenuationSystem* system, f32 distance, f32 frequency) {
    if (!system->air_absorption_enabled || distance <= 0.0f) {
        return 1.0f;
    }
    
    // Calculate frequency-dependent air absorption
    // Based on ISO 9613-1 standard approximation
    f32 freq_factor = frequency / system->reference_frequency;
    f32 absorption_coefficient = system->air_absorption_factor * freq_factor;
    
    // Exponential decay due to air absorption
    f32 air_absorption_gain = expf(-absorption_coefficient * distance);
    
    return air_absorption_gain;
}

void DistanceAttenuation_ApplyAirAbsorption(DistanceAttenuationSystem* system, const f32* input, f32* output, 
                                        u32 frames, f32 distance, f32 frequency) {
    if (!system || !input || !output) {
        return;
    }
    
    f32 air_absorption_gain = DistanceAttenuation_CalculateAirAbsorption(system, distance, frequency);
    
    for (u32 i = 0; i < frames; i++) {
        output[i] = input[i] * air_absorption_gain;
    }
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Main attenuation processing
// -------------------------------------------------------------------------------------------------

f32 DistanceAttenuation_CalculateGain(DistanceAttenuationSystem* system, f32 distance) {
    if (!system || !system->initialized) {
        return 1.0f;
    }
    
    // Apply distance scaling
    f32 scaled_distance = distance * system->distance_scale;
    
    // Calculate base attenuation based on current curve type
    f32 base_gain = 1.0f;
    
    switch (system->current_curve_type) {
        case ATTENUATION_INVERSE:
            base_gain = DistanceAttenuation_CalculateInverse(system, scaled_distance);
            break;
        case ATTENUATION_INVERSE_SQUARE:
            base_gain = DistanceAttenuation_CalculateInverseSquare(system, scaled_distance);
            break;
        case ATTENUATION_LINEAR:
            base_gain = DistanceAttenuation_CalculateLinear(system, scaled_distance);
            break;
        case ATTENUATION_EXPONENTIAL:
            base_gain = DistanceAttenuation_CalculateExponential(system, scaled_distance);
            break;
        case ATTENUATION_LOGARITHMIC:
            base_gain = DistanceAttenuation_CalculateLogarithmic(system, scaled_distance);
            break;
        case ATTENUATION_CUSTOM:
            base_gain = DistanceAttenuation_CalculateCustom(system, scaled_distance);
            break;
        default:
            base_gain = DistanceAttenuation_CalculateInverseSquare(system, scaled_distance);
            break;
    }
    
    // Apply master volume
    f32 final_gain = base_gain * system->master_volume;
    
    // Clamp to valid range
    return fmaxf(0.0f, fminf(1.0f, final_gain));
}

void DistanceAttenuation_ProcessAudio(DistanceAttenuationSystem* system, const f32* input, f32* output, 
                                u32 frames, f32 distance, f32 frequency) {
    if (!system || !input || !output) {
        return;
    }
    
    // Calculate distance gain
    f32 distance_gain = DistanceAttenuation_CalculateGain(system, distance);
    
    // Calculate air absorption
    f32 air_absorption_gain = DistanceAttenuation_CalculateAirAbsorption(system, distance, frequency);
    
    // Apply both effects
    f32 total_gain = distance_gain * air_absorption_gain;
    
    for (u32 i = 0; i < frames; i++) {
        output[i] = input[i] * total_gain;
    }
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Curve management
// -------------------------------------------------------------------------------------------------

void DistanceAttenuation_SetCurveType(DistanceAttenuationSystem* system, AttenuationType type) {
    if (!system || !system->initialized) {
        return;
    }
    
    system->current_curve_type = type;
    printf("Set attenuation curve type: %d\n", type);
}

void DistanceAttenuation_SetMasterVolume(DistanceAttenuationSystem* system, f32 volume) {
    if (!system || !system->initialized) {
        return;
    }
    
    system->master_volume = fmaxf(0.0f, fminf(2.0f, volume));  // Clamp 0-2
}

void DistanceAttenuation_SetDistanceScale(DistanceAttenuationSystem* system, f32 scale) {
    if (!system || !system->initialized) {
        return;
    }
    
    system->distance_scale = fmaxf(0.1f, fminf(10.0f, scale));  // Clamp 0.1-10
}

void DistanceAttenuation_SetAirAbsorption(DistanceAttenuationSystem* system, bool enabled, f32 factor) {
    if (!system || !system->initialized) {
        return;
    }
    
    system->air_absorption_enabled = enabled;
    system->air_absorption_factor = fmaxf(0.0f, fminf(0.1f, factor));  // Clamp 0-0.1
}

void DistanceAttenuation_ClearCustomCurve(DistanceAttenuationSystem* system) {
    if (!system || !system->initialized) {
        return;
    }
    
    system->custom_curve.point_count = 0;
    printf("Cleared custom attenuation curve\n");
}

// -------------------------------------------------------------------------------------------------
// MAIN DISTANCE ATTENUATION INTERFACE
// -------------------------------------------------------------------------------------------------

void DistanceAttenuation_CreateSystem(void) {
    if (!g_attenuation_initialized) {
        DistanceAttenuation_Init(&g_attenuation_system, 48000);  // 48kHz
        g_attenuation_initialized = true;
        printf("Global Distance Attenuation System created\n");
    }
}

DistanceAttenuationSystem* DistanceAttenuation_GetSystem(void) {
    if (!g_attenuation_initialized) {
        DistanceAttenuation_CreateSystem();
    }
    return &g_attenuation_system;
}

void DistanceAttenuation_ProcessAudioWithDistance(const f32* input, f32* output, u32 frames, 
                                             f32 distance, f32 frequency) {
    DistanceAttenuationSystem* system = DistanceAttenuation_GetSystem();
    if (!system) {
        memcpy(output, input, frames * sizeof(f32));
        return;
    }
    
    DistanceAttenuation_ProcessAudio(system, input, output, frames, distance, frequency);
}
