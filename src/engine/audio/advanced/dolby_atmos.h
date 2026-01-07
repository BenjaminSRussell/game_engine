#ifndef DOLBY_ATMOS_H
#define DOLBY_ATMOS_H

#include "audio/audio_system.h"
#include "include/math/vec3.h"
#include "include/math/quat.h"
#include <stdint.h>
#include <stdbool.h>
#include <include/math/math.h>

// -------------------------------------------------------------------------------------------------
// DOLBY ATMOS CONSTANTS AND CONFIGURATION
// -------------------------------------------------------------------------------------------------

#define ATMOS_MAX_OBJECTS 128
#define ATMOS_MAX_BED_CHANNELS 12
#define ATMOS_SAMPLE_RATE 48000
#define ATMOS_BUFFER_SIZE 512
#define ATMOS_MAX_RENDER_DISTANCE 100.0f

// Speaker layout types
typedef enum {
    ATMOS_LAYOUT_5_1_2,   // 5.1.2 (Front L/R, C, LFE, Side L/R, Top L/R)
    ATMOS_LAYOUT_7_1_4,   // 7.1.4 (Front L/R, C, LFE, Side L/R, Rear L/R, Top Front L/R, Top Rear L/R)
    ATMOS_LAYOUT_9_1_6,   // 9.1.6 (Front L/R, C, LFE, Side L/R, Rear L/R, Front Wide L/R, Top Front L/R, Top Rear L/R)
    ATMOS_LAYOUT_BINAURAL // Headphone rendering
} AtmosLayout;

// Audio object types
typedef enum {
    ATMOS_OBJECT_STATIC,      // Fixed position
    ATMOS_OBJECT_DYNAMIC,     // Moving object
    ATMOS_OBJECT_DIALOGUE,    // Dialogue priority
    ATMOS_OBJECT_MUSIC,       // Music bed
    ATMOS_OBJECT_EFFECTS      // Sound effects
} AtmosObjectType;

// -------------------------------------------------------------------------------------------------
// AUDIO OBJECT STRUCTURES
// -------------------------------------------------------------------------------------------------

typedef struct {
    uint32_t id;
    AtmosObjectType type;
    Vec3 position;
    Vec3 velocity;
    float gain;
    float size;           // Object spread/size
    uint8_t priority;    // 0-255 priority
    bool active;
    void* audio_data;    // Pointer to audio buffer
    size_t data_size;
} AtmosAudioObject;

typedef struct {
    float azimuth;       // Horizontal angle (-180 to +180)
    float elevation;     // Vertical angle (-90 to +90)
    float distance;      // Distance from listener
    float gain;          // Calculated gain after distance attenuation
} AtmosObjectMetadata;

// -------------------------------------------------------------------------------------------------
// SPEAKER CONFIGURATION
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 position;
    Vec3 direction;
    float distance;      // Distance from listening position
    float delay;         // Delay in samples
    float gain;          // Channel gain
    bool active;
} AtmosSpeaker;

typedef struct {
    AtmosLayout layout;
    AtmosSpeaker speakers[ATMOS_MAX_BED_CHANNELS];
    uint8_t channel_count;
    bool headphone_mode;
} AtmosSpeakerConfig;

// -------------------------------------------------------------------------------------------------
// RENDERER STATE
// -------------------------------------------------------------------------------------------------

typedef struct {
    void* renderer_handle;
    void* sdk_handle;
    bool initialized;
    AtmosSpeakerConfig speaker_config;
    AtmosAudioObject objects[ATMOS_MAX_OBJECTS];
    uint32_t active_object_count;
    Vec3 listener_position;
    Quat listener_orientation;
    float master_gain;
    bool dynamic_range_enabled;
} AtmosRenderer;

// -------------------------------------------------------------------------------------------------
// METADATA STRUCTURES
// -------------------------------------------------------------------------------------------------

typedef struct {
    uint32_t object_id;
    float azimuth;
    float elevation;
    float distance;
    float gain;
    uint8_t priority;
    bool dialogue_enhancement;
} AtmosObjectMetadataBlock;

typedef struct {
    uint16_t sample_rate;
    uint8_t channel_count;
    float loudness;      // LUFS integrated loudness
    float peak_level;    // True peak dBTP
    bool dialogue_normalization;
    uint32_t object_count;
    AtmosObjectMetadataBlock objects[ATMOS_MAX_OBJECTS];
} AtmosMetadata;

// -------------------------------------------------------------------------------------------------
// CORE API FUNCTIONS
// -------------------------------------------------------------------------------------------------

// SDK Initialization (TASK_700-705)
bool Atmos_InitializeSDK(AtmosRenderer* renderer);
bool Atmos_CheckSDKCompatibility(void);
bool Atmos_LoadLibraries(void);
bool Atmos_ValidateLicense(void);
bool Atmos_ConfigurePlatform(AtmosRenderer* renderer);
void Atmos_SetErrorCallbacks(void);

// Speaker Configuration (TASK_710-716)
bool Atmos_DetectSpeakerLayout(AtmosSpeakerConfig* config);
bool Atmos_ConfigureBedChannels(AtmosSpeakerConfig* config);
bool Atmos_ConfigureHeightChannels(AtmosSpeakerConfig* config);
bool Atmos_EnableSpeakerVirtualization(AtmosSpeakerConfig* config);
bool Atmos_EnableBinauralMode(AtmosSpeakerConfig* config);
bool Atmos_ConfigureSpeakerDistances(AtmosSpeakerConfig* config);
bool Atmos_CalibrateRoom(AtmosSpeakerConfig* config);

// Object-Based Audio (TASK_720-728)
uint32_t Atmos_CreateAudioObject(AtmosRenderer* renderer, AtmosObjectType type);
bool Atmos_AssignAudioToObject(AtmosRenderer* renderer, uint32_t object_id, void* audio_data, size_t size);
bool Atmos_SetObjectPosition(AtmosRenderer* renderer, uint32_t object_id, Vec3 position);
bool Atmos_SetObjectSize(AtmosRenderer* renderer, uint32_t object_id, float size);
bool Atmos_SetObjectPriority(AtmosRenderer* renderer, uint32_t object_id, uint8_t priority);
bool Atmos_SetObjectGain(AtmosRenderer* renderer, uint32_t object_id, float gain);
bool Atmos_FadeObjectIn(AtmosRenderer* renderer, uint32_t object_id, float duration);
bool Atmos_FadeObjectOut(AtmosRenderer* renderer, uint32_t object_id, float duration);
bool Atmos_EnableObjectDucking(AtmosRenderer* renderer, uint32_t object_id, bool enable);
bool Atmos_GroupObjects(AtmosRenderer* renderer, uint32_t* object_ids, uint32_t count);

// Metadata Encoding (TASK_730-736)
bool Atmos_GenerateMetadata(AtmosRenderer* renderer, AtmosMetadata* metadata);
bool Atmos_EncodeObjectPositions(AtmosMetadata* metadata, AtmosAudioObject* objects, uint32_t count);
bool Atmos_EncodeDialogueEnhancement(AtmosMetadata* metadata, bool enable);
bool Atmos_EncodeDynamicRange(AtmosMetadata* metadata, float range);
bool Atmos_EncodeLoudnessNormalization(AtmosMetadata* metadata, float loudness);
bool Atmos_SetContentType(AtmosMetadata* metadata, const char* type);
bool Atmos_GenerateADMMetadata(AtmosMetadata* metadata, void* bwf_buffer, size_t* size);

// Rendering (TASK_740-747)
bool Atmos_RenderFrame(AtmosRenderer* renderer, float* output_buffer, size_t buffer_size);
bool Atmos_ApplyPanningLaws(AtmosRenderer* renderer, uint32_t object_id);
bool Atmos_ApplyDistanceAttenuation(AtmosRenderer* renderer, uint32_t object_id);
bool Atmos_ApplyDopplerEffect(AtmosRenderer* renderer, uint32_t object_id);
bool Atmos_RenderEarlyReflections(AtmosRenderer* renderer);
bool Atmos_RenderLateReverb(AtmosRenderer* renderer);
bool Atmos_ApplyOcclusion(AtmosRenderer* renderer, uint32_t object_id);
bool Atmos_ApplyDiffraction(AtmosRenderer* renderer, uint32_t object_id);

// Binaural Rendering (TASK_750-755)
bool Atmos_LoadHRTFDatabase(const char* hrtf_path);
bool Atmos_InterpolateHRTF(float azimuth, float elevation, float* hrtf_coeffs);
bool Atmos_EnableHeadTracking(AtmosRenderer* renderer, bool enable);
bool Atmos_GeneratePersonalizedHRTF(AtmosRenderer* renderer);
bool Atmos_SimulateRoomForHeadphones(AtmosRenderer* renderer);
bool Atmos_EnhanceExternalization(AtmosRenderer* renderer);
bool Atmos_RenderBinauralFrame(AtmosRenderer* renderer, float* left_buffer, float* right_buffer, size_t buffer_size);

// Dynamic Range Control (TASK_760-765)
bool Atmos_NormalizeDialogue(AtmosRenderer* renderer, float target_loudness);
bool Atmos_CompressDynamicRange(AtmosRenderer* renderer, float ratio);
bool Atmos_MeasureLoudness(AtmosRenderer* renderer, float* loudness);
bool Atmos_LimitTruePeak(AtmosRenderer* renderer, float threshold);
bool Atmos_EnableNightMode(AtmosRenderer* renderer, bool enable);
bool Atmos_EnhanceSpeechIntelligibility(AtmosRenderer* renderer);
void Atmos_UpdateDynamicRangeSettings(AtmosRenderer* renderer, float target_loudness, 
                                     float compression_ratio, float peak_limit);
void Atmos_GetDynamicRangeStats(AtmosRenderer* renderer, float* current_loudness, 
                               float* peak_level, float* dynamic_range);
float Atmos_MeasureObjectLoudness(AtmosRenderer* renderer, uint32_t object_index);

// Room Acoustics (TASK_770-775)
bool Atmos_GenerateRoomImpulseResponse(AtmosRenderer* renderer, float room_size, float reverb_time);
bool Atmos_SetWallMaterialProperties(AtmosRenderer* renderer, uint32_t wall_id, float absorption);
bool Atmos_RayTraceAcoustics(AtmosRenderer* renderer);
bool Atmos_ModelDiffuseReflections(AtmosRenderer* renderer);
bool Atmos_SimulateRoomModes(AtmosRenderer* renderer, float room_volume);
void Atmos_SetRoomDimensions(AtmosRenderer* renderer, float width, float height, float depth);
void Atmos_GetRoomStats(AtmosRenderer* renderer, float* volume, float* surface_area, float* rt60);
bool Atmos_ApplyRoomAcousticsToObject(AtmosRenderer* renderer, uint32_t object_index);

// Export & Streaming (TASK_780-785)
bool Atmos_ExportADM_BWF(AtmosRenderer* renderer, const char* filename);
bool Atmos_StreamToAtmosReceiver(AtmosRenderer* renderer, void* stream_handle);
bool Atmos_EncodeDolbyDigitalPlus(AtmosRenderer* renderer, void* output_buffer, size_t* size);
bool Atmos_EncodeTrueHD(AtmosRenderer* renderer, void* output_buffer, size_t* size);
bool Atmos_EnableMAT2_0(AtmosRenderer* renderer, bool enable);
bool Atmos_EnableBitstreamPassthrough(AtmosRenderer* renderer, bool enable);

// Optimization (TASK_790-794)
bool Atmos_OptimizePanningSIMD(AtmosRenderer* renderer);
bool Atmos_EnableMultithreadedRendering(AtmosRenderer* renderer, bool enable);
bool Atmos_EnableGPUAcceleration(AtmosRenderer* renderer, bool enable);
bool Atmos_CullInactiveObjects(AtmosRenderer* renderer);
bool Atmos_AdaptiveQualityControl(AtmosRenderer* renderer, float cpu_load);

// Debugging & Monitoring (TASK_800-805)
void Atmos_VisualizeObjectPositions(AtmosRenderer* renderer);
void Atmos_MonitorLoudness(AtmosRenderer* renderer);
bool Atmos_ValidateMetadata(AtmosMetadata* metadata);
void Atmos_MeterAudioPerObject(AtmosRenderer* renderer);
void Atmos_ProfilePerformance(AtmosRenderer* renderer);
bool Atmos_EnableABComparison(AtmosRenderer* renderer, bool enable);

// Utility Functions
void Atmos_UpdateListener(AtmosRenderer* renderer, Vec3 position, Quat orientation);
void Atmos_SetMasterGain(AtmosRenderer* renderer, float gain);
void Atmos_Shutdown(AtmosRenderer* renderer);
bool Atmos_IsInitialized(AtmosRenderer* renderer);

#endif // DOLBY_ATMOS_H
