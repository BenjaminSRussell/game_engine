#ifndef SPATIAL_REVERB_ZONES_H
#define SPATIAL_REVERB_ZONES_H

#include <common.h>
#include <math/math.h>

// -------------------------------------------------------------------------------------------------
// REVERB ROOM TYPES
// -------------------------------------------------------------------------------------------------

typedef enum {
    REVERB_ROOM_SMALL,      // Small room (RT60 ~0.8s)
    REVERB_ROOM_MEDIUM,     // Medium room (RT60 ~1.5s)
    REVERB_ROOM_LARGE,      // Large room (RT60 ~2.5s)
    REVERB_ROOM_HALL,       // Concert hall (RT60 ~4.0s)
    REVERB_ROOM_CATHEDRAL, // Cathedral (RT60 ~6.0s)
    REVERB_ROOM_OUTDOOR    // Outdoor (no reverb)
} ReverbRoomType;

// -------------------------------------------------------------------------------------------------
// BOUNDING BOX FOR ZONES
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 min;  // Minimum corner of bounding box
    Vec3 max;  // Maximum corner of bounding box
} BoundingBox;

// -------------------------------------------------------------------------------------------------
// REVERB PARAMETERS
// -------------------------------------------------------------------------------------------------

typedef struct {
    ReverbRoomType room_type;
    f32 rt60;              // Reverberation time (60dB decay time)
    f32 pre_delay;          // Pre-delay time in milliseconds
    f32 early_reflection;   // Early reflection level (0.0-1.0)
    f32 late_reverb;        // Late reverb level (0.0-1.0)
    f32 damping;           // High frequency damping (0.0-1.0)
    f32 room_size;         // Room size multiplier (0.5-2.0)
    char impulse_response_path[256];  // Path to custom impulse response file
} ReverbParameters;

// -------------------------------------------------------------------------------------------------
// REVERB ZONE DEFINITION
// -------------------------------------------------------------------------------------------------

typedef struct {
    char name[64];               // Zone name for identification
    BoundingBox bounds;          // Zone boundaries in world space
    ReverbParameters reverb_params; // Reverb parameters for this zone
    f32 priority;               // Zone priority for blending (higher = more important)
    bool enabled;                // Whether zone is active
} ReverbZone;

// -------------------------------------------------------------------------------------------------
// CONVOLUTION REVERB PROCESSOR
// -------------------------------------------------------------------------------------------------

typedef struct {
    u32 sample_rate;
    u32 buffer_size;
    u32 impulse_length;           // Length of impulse response
    
    f32* impulse_response;       // Impulse response buffer
    f32* convolution_buffer;     // Circular buffer for convolution
    u32 buffer_index;           // Current position in circular buffer
    
    ReverbParameters current_params; // Current reverb parameters
    bool initialized;
} ConvolutionReverb;

// -------------------------------------------------------------------------------------------------
// REVERB ZONE SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    // Configuration
    u32 max_zones;
    u32 zone_count;
    u32 sample_rate;
    u32 buffer_size;
    
    // Zone definitions
    ReverbZone* zones;
    
    // Reverb processors (stereo)
    ConvolutionReverb* processors;  // [0] = Left, [1] = Right
    
    // Mixing buffers
    f32* wet_buffer;    // Processed reverb signal
    f32* dry_buffer;    // Original dry signal
    f32* zone_buffer;   // Temporary zone processing buffer
    
    // Blending parameters
    f32 blend_distance;      // Distance for zone blending
    u32 max_blend_zones;   // Maximum zones to blend simultaneously
    f32 global_reverb_level; // Global reverb level multiplier
    
    bool initialized;
} ReverbZoneSystem;

// -------------------------------------------------------------------------------------------------
// REVERB ZONE SYSTEM FUNCTIONS
// -------------------------------------------------------------------------------------------------

bool ReverbZones_Init(ReverbZoneSystem* system, u32 max_zones, u32 sample_rate, u32 buffer_size);
void ReverbZones_Shutdown(ReverbZoneSystem* system);

// ✅ COMPLETED: Zone management
bool ReverbZones_AddZone(ReverbZoneSystem* system, const ReverbZone* zone);
void ReverbZones_RemoveZone(ReverbZoneSystem* system, u32 zone_index);
ReverbZone* ReverbZones_FindZone(ReverbZoneSystem* system, const Vec3 position);
bool ReverbZones_IsPositionInZone(const ReverbZone* zone, const Vec3 position);

// ✅ COMPLETED: Zone blending system
void ReverbZones_CalculateBlendWeights(ReverbZoneSystem* system, const Vec3 listener_pos, 
                                     u32* zone_indices, f32* weights, u32* count);
void ReverbZones_ProcessWithBlending(ReverbZoneSystem* system, const Vec3 listener_pos,
                                  const f32* input_left, const f32* input_right,
                                  f32* output_left, f32* output_right, u32 frames);

// ✅ COMPLETED: Convolution reverb implementation
bool ConvolutionReverb_Init(ConvolutionReverb* reverb, u32 sample_rate, u32 buffer_size);
void ConvolutionReverb_Process(ConvolutionReverb* reverb, const f32* input, f32* output, u32 frames);
void ConvolutionReverb_SetParameters(ConvolutionReverb* reverb, const ReverbParameters* params);
void ConvolutionReverb_LoadIR(ConvolutionReverb* reverb, const char* ir_path);
void ConvolutionReverb_Cleanup(ConvolutionReverb* reverb);
void ConvolutionReverb_GenerateImpulse(ConvolutionReverb* reverb, ReverbRoomType room_type);
void ConvolutionReverb_GenerateDefaultImpulse(ConvolutionReverb* reverb, ReverbRoomType room_type);

// Main interface
void ReverbZones_CreateSystem(void);
ReverbZoneSystem* ReverbZones_GetSystem(void);
void ReverbZones_ProcessAudio(const Vec3 listener_pos, const f32* input_left, const f32* input_right,
                           f32* output_left, f32* output_right, u32 frames);

#endif // SPATIAL_REVERB_ZONES_H
