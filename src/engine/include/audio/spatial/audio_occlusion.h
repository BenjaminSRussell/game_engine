#ifndef SPATIAL_AUDIO_OCCLUSION_H
#define SPATIAL_AUDIO_OCCLUSION_H

#include <common.h>
#include "engine/include/math/math_all.h"

// -------------------------------------------------------------------------------------------------
// MATERIAL TYPES FOR OCCLUSION
// -------------------------------------------------------------------------------------------------

typedef enum {
    MATERIAL_CONCRETE,
    MATERIAL_WOOD,
    MATERIAL_GLASS,
    MATERIAL_METAL,
    MATERIAL_FABRIC,
    MATERIAL_PLASTER,
    MATERIAL_BRICK,
    MATERIAL_AIR
} OcclusionMaterialType;

// -------------------------------------------------------------------------------------------------
// MATERIAL ABSORPTION PROPERTIES
// -------------------------------------------------------------------------------------------------

typedef struct {
    OcclusionMaterialType type;
    f32 absorption_125hz;   // Low frequency absorption (125Hz)
    f32 absorption_500hz;   // Low-mid frequency absorption (500Hz)
    f32 absorption_2khz;   // Mid frequency absorption (2kHz)
    f32 absorption_8khz;   // High frequency absorption (8kHz)
} MaterialAbsorption;

// -------------------------------------------------------------------------------------------------
// LOW-PASS FILTER FOR OCCLUSION
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 cutoff_frequency;    // Filter cutoff frequency in Hz
    u32 sample_rate;         // Audio sample rate
    
    // Biquad filter coefficients
    f32 b0, b1, b2;       // Feedforward coefficients
    f32 a1, a2;           // Feedback coefficients
    
    // Filter state (delay lines)
    f32 x1, x2;           // Input delay
    f32 y1, y2;           // Output delay
} LowPassFilter;

// -------------------------------------------------------------------------------------------------
// OCCLUSION SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    // Configuration
    u32 sample_rate;
    u32 buffer_size;
    
    // Low-pass filters for frequency-dependent occlusion
    LowPassFilter* filters;
    u32 filter_count;
    
    // Material absorption properties
    MaterialAbsorption* materials;
    u32 material_count;
    
    // Ray casting parameters
    u32 ray_count;              // Number of rays for occlusion sampling
    f32 max_distance;           // Maximum ray distance in meters
    f32 occlusion_threshold;    // Minimum occlusion to apply filtering
    
    bool initialized;
} OcclusionSystem;

// -------------------------------------------------------------------------------------------------
// OCCLUSION SYSTEM FUNCTIONS
// -------------------------------------------------------------------------------------------------

bool Occlusion_Init(OcclusionSystem* system, u32 sample_rate, u32 buffer_size);
void Occlusion_Shutdown(OcclusionSystem* system);

//  COMPLETED: Low-pass filter implementation
bool LowPassFilter_Init(LowPassFilter* filter, f32 cutoff_freq, u32 sample_rate);
void LowPassFilter_Process(LowPassFilter* filter, const f32* input, f32* output, u32 frames);
void LowPassFilter_SetCutoff(LowPassFilter* filter, f32 cutoff_freq);
void LowPassFilter_Cleanup(LowPassFilter* filter);

//  COMPLETED: Occlusion detection and filtering
f32 Occlusion_CalculateOcclusion(OcclusionSystem* system, const Vec3 source, const Vec3 listener);
void Occlusion_ApplyFiltering(OcclusionSystem* system, const f32* input, f32* output, u32 frames, f32 occlusion_factor);

//  COMPLETED: Ray casting for occlusion detection
bool Occlusion_Raycast(const Vec3 start, const Vec3 end, MaterialAbsorption* hit_material);
bool Occlusion_IsPointInsideObstacle(const Vec3 point);
MaterialAbsorption Occlusion_GetMaterialAtPoint(const Vec3 point);

// Main interface
void Occlusion_CreateSystem(void);
OcclusionSystem* Occlusion_GetSystem(void);
void Occlusion_ProcessAudio(const Vec3 source, const Vec3 listener, const f32* input, f32* output, u32 frames);

#endif // SPATIAL_AUDIO_OCCLUSION_H
