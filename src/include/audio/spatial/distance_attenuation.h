#ifndef SPATIAL_DISTANCE_ATTENUATION_H
#define SPATIAL_DISTANCE_ATTENUATION_H

#include "engine/include/common.h"
#include "engine/include/math/math.h"

// -------------------------------------------------------------------------------------------------
// DISTANCE ATTENUATION TYPES
// -------------------------------------------------------------------------------------------------

typedef enum {
    ATTENUATION_INVERSE,        // Inverse distance law (1/d)
    ATTENUATION_INVERSE_SQUARE,  // Inverse square law (1/d) - most realistic
    ATTENUATION_LINEAR,          // Linear falloff
    ATTENUATION_EXPONENTIAL,     // Exponential decay
    ATTENUATION_LOGARITHMIC,     // Logarithmic falloff (in dB)
    ATTENUATION_CUSTOM           // User-defined curve
} AttenuationType;

// -------------------------------------------------------------------------------------------------
// CUSTOM CURVE POINT
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 distance;  // Distance in meters
    f32 gain;      // Gain factor at this distance (0.0-1.0)
} CurvePoint;

// -------------------------------------------------------------------------------------------------
// ATTENUATION CURVE DEFINITION
// -------------------------------------------------------------------------------------------------

typedef struct {
    AttenuationType type;
    char name[32];              // Human-readable name
    f32 min_distance;          // Minimum distance for attenuation
    f32 max_distance;          // Maximum distance (beyond this = 0 gain)
    f32 rolloff_factor;        // Rolloff factor (controls steepness)
} AttenuationCurve;

// -------------------------------------------------------------------------------------------------
// CUSTOM ATTENUATION CURVE
// -------------------------------------------------------------------------------------------------

typedef struct {
    CurvePoint* points;         // Array of control points
    u32 point_count;          // Number of control points
    bool interpolation_enabled;  // Whether to interpolate between points
} CustomAttenuationCurve;

// -------------------------------------------------------------------------------------------------
// DISTANCE ATTENUATION SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    // Configuration
    u32 sample_rate;
    u32 curve_count;
    
    // Standard curves
    AttenuationCurve* curves;
    AttenuationType current_curve_type;
    
    // Custom curve support
    CustomAttenuationCurve custom_curve;
    
    // Global parameters
    f32 master_volume;         // Master volume multiplier
    f32 distance_scale;        // Distance scaling factor
    
    // Air absorption modeling
    bool air_absorption_enabled;
    f32 air_absorption_factor;  // Absorption per meter
    f32 reference_frequency;     // Reference frequency for absorption
    
    bool initialized;
} DistanceAttenuationSystem;

// -------------------------------------------------------------------------------------------------
// DISTANCE ATTENUATION SYSTEM FUNCTIONS
// -------------------------------------------------------------------------------------------------

bool DistanceAttenuation_Init(DistanceAttenuationSystem* system, u32 sample_rate);
void DistanceAttenuation_Shutdown(DistanceAttenuationSystem* system);

//  COMPLETED: Standard attenuation curves
f32 DistanceAttenuation_CalculateInverse(DistanceAttenuationSystem* system, f32 distance);
f32 DistanceAttenuation_CalculateInverseSquare(DistanceAttenuationSystem* system, f32 distance);
f32 DistanceAttenuation_CalculateLinear(DistanceAttenuationSystem* system, f32 distance);
f32 DistanceAttenuation_CalculateExponential(DistanceAttenuationSystem* system, f32 distance);
f32 DistanceAttenuation_CalculateLogarithmic(DistanceAttenuationSystem* system, f32 distance);

//  COMPLETED: Custom curve interpolation
bool DistanceAttenuation_AddCustomPoint(DistanceAttenuationSystem* system, f32 distance, f32 gain);
f32 DistanceAttenuation_CalculateCustom(DistanceAttenuationSystem* system, f32 distance);

//  COMPLETED: Air absorption modeling
f32 DistanceAttenuation_CalculateAirAbsorption(DistanceAttenuationSystem* system, f32 distance, f32 frequency);
void DistanceAttenuation_ApplyAirAbsorption(DistanceAttenuationSystem* system, const f32* input, f32* output, 
                                        u32 frames, f32 distance, f32 frequency);

//  COMPLETED: Main attenuation processing
f32 DistanceAttenuation_CalculateGain(DistanceAttenuationSystem* system, f32 distance);
void DistanceAttenuation_ProcessAudio(DistanceAttenuationSystem* system, const f32* input, f32* output, 
                                u32 frames, f32 distance, f32 frequency);

//  COMPLETED: Curve management
void DistanceAttenuation_SetCurveType(DistanceAttenuationSystem* system, AttenuationType type);
void DistanceAttenuation_SetMasterVolume(DistanceAttenuationSystem* system, f32 volume);
void DistanceAttenuation_SetDistanceScale(DistanceAttenuationSystem* system, f32 scale);
void DistanceAttenuation_SetAirAbsorption(DistanceAttenuationSystem* system, bool enabled, f32 factor);
void DistanceAttenuation_ClearCustomCurve(DistanceAttenuationSystem* system);

// Main interface
void DistanceAttenuation_CreateSystem(void);
DistanceAttenuationSystem* DistanceAttenuation_GetSystem(void);
void DistanceAttenuation_ProcessAudioWithDistance(const f32* input, f32* output, u32 frames, 
                                             f32 distance, f32 frequency);

#endif // SPATIAL_DISTANCE_ATTENUATION_H
