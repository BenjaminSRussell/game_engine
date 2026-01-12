#ifndef SPATIAL_AUDIO_HRTF_H
#define SPATIAL_AUDIO_HRTF_H

#include <common.h>
#include <math/math.h>

// -------------------------------------------------------------------------------------------------
// HRTF (HEAD-RELATED TRANSFER FUNCTION) SYSTEM
// -------------------------------------------------------------------------------------------------

typedef enum {
    HRTF_DATASET_GENERIC,
    HRTF_DATASET_KEMAR,
    HRTF_DATASET_CIPIC,
    HRTF_DATASET_LISTEN,
    HRTF_DATASET_CUSTOM
} HRTFDataset;

typedef struct {
    f32 azimuth;      // Horizontal angle (-180 to +180 degrees)
    f32 elevation;    // Vertical angle (-90 to +90 degrees)
    f32* left_ear_impulse;   // Left ear impulse response
    f32* right_ear_impulse;  // Right ear impulse response
    u32 impulse_length;     // Length of impulse response
    f32 delay_left;          // Interaural time difference (samples)
    f32 delay_right;         // Interaural time difference (samples)
} HRTFMeasurement;

typedef struct {
    HRTFDataset dataset_type;
    HRTFMeasurement* measurements;
    u32 measurement_count;
    u32 impulse_length;
    u32 sample_rate;
    bool loaded;
    char dataset_path[256];
} HRTFDatabase;

// -------------------------------------------------------------------------------------------------
// SPATIAL AUDIO PROCESSOR
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 source_position;
    Vec3 listener_position;
    Quat listener_orientation;
    f32 distance;
    f32 azimuth;
    f32 elevation;
    f32 gain;
    f32 occlusion_factor;
    bool is_behind_listener;
} SpatialAudioParams;

typedef struct {
    HRTFDatabase hrtf_database;
    SpatialAudioParams params;
    
    // Convolution buffers
    f32* left_convolution_buffer;
    f32* right_convolution_buffer;
    u32 convolution_buffer_size;
    
    // Current HRTF measurement
    HRTFMeasurement* current_hrtf_left;
    HRTFMeasurement* current_hrtf_right;
    
    // Filter states for real-time processing
    f32* left_filter_state;
    f32* right_filter_state;
    
    // Distance attenuation
    f32 min_distance;
    f32 max_distance;
    f32 rolloff_factor;
    
    // Occlusion and obstruction
    f32 occlusion_lowpass_freq;
    f32 obstruction_factor;
    
    bool initialized;
} SpatialAudioProcessor;

// -------------------------------------------------------------------------------------------------
// BAFFLING AND SHADOWING EFFECTS
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 head_radius;           // Average head radius (8.75 cm)
    f32 shoulder_width;       // Shoulder width for body shadowing
    f32 torso_height;          // Torso height for low-frequency shadowing
    f32 pinna_offset;          // Pinna (outer ear) offset calculations
} AnthropometricData;

typedef struct {
    AnthropometricData anthropometry;
    f32 head_shadow_gain;      // High-frequency shadowing by head
    f32 shoulder_shadow_gain;  // Low-frequency shadowing by shoulders
    f32 torso_shadow_gain;     // Very low-frequency shadowing by torso
    f32 pinna_notch_freq;      // Pinna notch frequency (~8kHz)
    f32 pinna_boost_freq;      // Pinna boost frequency (~3kHz)
} BafflingModel;

// -------------------------------------------------------------------------------------------------
// DYNAMIC CROSSFADE SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 crossfade_duration;    // Crossfade time between HRTF measurements
    f32 current_crossfade_time;
    HRTFMeasurement* prev_hrtf_left;
    HRTFMeasurement* prev_hrtf_right;
    HRTFMeasurement* target_hrtf_left;
    HRTFMeasurement* target_hrtf_right;
    bool crossfading;
} HRTFCrossfade;

// -------------------------------------------------------------------------------------------------
// NEAR-FIELD EFFECTS
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 near_field_threshold;  // Distance threshold for near-field effects
    f32 near_field_gain_boost; // Gain boost for close sources
    f32 low_freq_boost;        // Low-frequency boost for proximity
    f32 proximity_effect;      // Proximity effect strength
} NearFieldModel;

// -------------------------------------------------------------------------------------------------
// FUNCTION DECLARATIONS
// -------------------------------------------------------------------------------------------------

// HRTF Database Management
bool HRTF_LoadDatabase(HRTFDatabase* database, const char* dataset_path, HRTFDataset dataset_type);
bool HRTF_LoadGenericDataset(HRTFDatabase* database);
bool HRTF_LoadKEMARDataset(HRTFDatabase* database);
bool HRTF_LoadCustomDataset(HRTFDatabase* database, const char* file_path);
void HRTF_UnloadDatabase(HRTFDatabase* database);
HRTFMeasurement* HRTF_FindMeasurement(const HRTFDatabase* database, f32 azimuth, f32 elevation);

// ✅ COMPLETED: HRTF Interpolation Functions
void HRTF_InterpolateMeasurements(const HRTFDatabase* database, f32 azimuth, f32 elevation, HRTFMeasurement* result);
void HRTF_BarycentricInterpolation(const HRTFMeasurement* v1, const HRTFMeasurement* v2, const HRTFMeasurement* v3,
                                  f32 w1, f32 w2, f32 w3, HRTFMeasurement* result);

// Spatial Audio Processor
bool SpatialAudio_Init(SpatialAudioProcessor* processor, u32 sample_rate, u32 buffer_size);
void SpatialAudio_Shutdown(SpatialAudioProcessor* processor);
void SpatialAudio_SetListener(SpatialAudioProcessor* processor, const Vec3 position, const Quat orientation);
void SpatialAudio_SetSource(SpatialAudioProcessor* processor, const Vec3 position, f32 gain);
void SpatialAudio_SetDistanceModel(SpatialAudioProcessor* processor, f32 min_distance, f32 max_distance, f32 rolloff);
void SpatialAudio_SetOcclusion(SpatialAudioProcessor* processor, f32 occlusion_factor);
void SpatialAudio_Process(SpatialAudioProcessor* processor, const f32* input, f32* left_output, f32* right_output, u32 frames);

// HRTF Processing
void HRTF_CalculateParameters(const Vec3 source_pos, const Vec3 listener_pos, const Quat listener_orient, f32* azimuth, f32* elevation, f32* distance);
void HRTF_ApplyMeasurement(SpatialAudioProcessor* processor, const HRTFMeasurement* hrtf_left, const HRTFMeasurement* hrtf_right);
void HRTF_ConvolveWithImpulse(const f32* input, const f32* impulse, f32* output, u32 input_frames, u32 impulse_length);
void HRTF_ApplyInterauralTimeDifference(f32* left_output, f32* right_output, f32 itd_samples, u32 frames);

// Baffling and Shadowing
void Baffling_Init(BafflingModel* model);
void Baffling_ApplyHeadShadow(BafflingModel* model, f32* left_output, f32* right_output, f32 azimuth, u32 frames);
void Baffling_ApplyShoulderShadow(BafflingModel* model, f32* left_output, f32* right_output, f32 azimuth, f32 elevation, u32 frames);
void Baffling_ApplyPinnaEffects(BafflingModel* model, f32* left_output, f32* right_output, f32 elevation, u32 frames);

// Crossfading
void HRTF_InitCrossfade(HRTFCrossfade* crossfade, f32 duration);
void HRTF_StartCrossfade(HRTFCrossfade* crossfade, HRTFMeasurement* prev_left, HRTFMeasurement* prev_right, HRTFMeasurement* target_left, HRTFMeasurement* target_right);
void HRTF_UpdateCrossfade(HRTFCrossfade* crossfade, f32* left_output, f32* right_output, u32 frames);
bool HRTF_IsCrossfading(const HRTFCrossfade* crossfade);

// Near-field Effects
void NearField_Init(NearFieldModel* model);
void NearField_ApplyProximityEffect(NearFieldModel* model, f32* left_output, f32* right_output, f32 distance, u32 frames);
void NearField_ApplyLowFreqBoost(NearFieldModel* model, f32* left_output, f32* right_output, f32 distance, u32 frames);

// Utility Functions
f32 SpatialAudio_CalculateDistanceAttenuation(f32 distance, f32 min_distance, f32 max_distance, f32 rolloff_factor);
f32 SpatialAudio_CalculateInterauralTimeDifference(f32 azimuth, f32 head_radius);
void SpatialAudio_CalculateInterauralLevelDifference(f32 azimuth, f32* left_gain, f32* right_gain);
bool SpatialAudio_IsSourceBehind(const Vec3 source_pos, const Quat listener_orient);

// Main interface functions
void SpatialAudio_CreateHRTFSystem(void);
void SpatialAudio_Process3DAudio(f32* input_buffer, f32* left_output, f32* right_output, u32 frames);

#endif // SPATIAL_AUDIO_HRTF_H
