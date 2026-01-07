#ifndef SPATIAL_AUDIO_H
#define SPATIAL_AUDIO_H

#include <common.h>
#include "include/math/math.h"

// -------------------------------------------------------------------------------------------------
// SPATIAL AUDIO SYSTEM - COMPREHENSIVE ENHANCEMENTS
// -------------------------------------------------------------------------------------------------

typedef enum {
    DISTANCE_MODEL_INVERSE,
    DISTANCE_MODEL_INVERSE_SQUARE,
    DISTANCE_MODEL_LINEAR,
    DISTANCE_MODEL_EXPONENTIAL,
    DISTANCE_MODEL_CUSTOM
} DistanceModel;

typedef enum {
    REVERB_ZONE_OUTDOOR,
    REVERB_ZONE_ROOM,
    REVERB_ZONE_HALL,
    REVERB_ZONE_CAVE,
    REVERB_ZONE_UNDERWATER,
    REVERB_ZONE_CUSTOM
} ReverbZoneType;

typedef enum {
    AUDIO_CATEGORY_AMBIENT,
    AUDIO_CATEGORY_EFFECTS,
    AUDIO_CATEGORY_MUSIC,
    AUDIO_CATEGORY_DIALOGUE,
    AUDIO_CATEGORY_UI,
    AUDIO_CATEGORY_COUNT
} AudioCategory;

typedef enum {
    PROCEDURAL_TYPE_FOOTSTEP,
    PROCEDURAL_TYPE_WIND,
    PROCEDURAL_TYPE_IMPACT,
    PROCEDURAL_TYPE_ENGINE,
    PROCEDURAL_TYPE_WATER,
    PROCEDURAL_TYPE_FIRE
} ProceduralAudioType;

// -------------------------------------------------------------------------------------------------
// HRTF BINAURAL AUDIO SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32* left_ear_impulse;
    f32* right_ear_impulse;
    u32 impulse_length;
    f32 azimuth;
    f32 elevation;
    f32 delay_left;
    f32 delay_right;
} HRTFImpulse;

typedef struct {
    HRTFImpulse* impulses;
    u32 impulse_count;
    u32 sample_rate;
    bool loaded;
    f32 head_radius;
} HRTFDatabase;

typedef struct {
    HRTFDatabase database;
    f32* left_convolution_buffer;
    f32* right_convolution_buffer;
    u32 buffer_size;
    u32 convolution_pos;
    bool enabled;
} HRTFProcessor;

// -------------------------------------------------------------------------------------------------
// SOUND PROPAGATION SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 start_point;
    Vec3 end_point;
    Vec3 reflection_point;
    f32 distance;
    f32 attenuation;
    u32 bounce_count;
    bool has_direct_path;
    bool has_reflection;
} PropagationRay;

typedef struct {
    PropagationRay* rays;
    u32 max_rays;
    u32 ray_count;
    f32 max_distance;
    f32 reflection_coefficient;
    bool enabled;
} PropagationSystem;

// -------------------------------------------------------------------------------------------------
// OCCLUSION AND OBSTRUCTION SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 source_pos;
    Vec3 listener_pos;
    f32 occlusion_factor;
    f32 obstruction_factor;
    u32 ray_count;
    f32 lowpass_freq;
    bool is_occluded;
    u32 last_update_frame;
} OcclusionData;

typedef struct {
    OcclusionData* occlusion_data;
    u32 max_sources;
    u32 rays_per_source;
    f32 cache_duration;
    bool enabled;
} OcclusionSystem;

// -------------------------------------------------------------------------------------------------
// REVERB ZONE SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 position;
    Vec3 size;
    ReverbZoneType type;
    f32 decay_time;
    f32 damping;
    f32 pre_delay;
    f32 early_reflections;
    f32 late_reverb;
    f32 diffusion;
    f32 room_size;
} ReverbZone;

typedef struct {
    ReverbZone* zones;
    u32 zone_count;
    u32 max_zones;
    ReverbZone* current_zone;
    f32 transition_time;
    f32 current_transition;
    bool enabled;
} ReverbZoneSystem;

// -------------------------------------------------------------------------------------------------
// DISTANCE ATTENUATION MODELS
// -------------------------------------------------------------------------------------------------

typedef struct {
    DistanceModel model;
    f32 min_distance;
    f32 max_distance;
    f32 rolloff_factor;
    f32 custom_curve[16];
    f32 air_absorption;
} DistanceAttenuation;

// -------------------------------------------------------------------------------------------------
// DOPPLER EFFECT SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 previous_source_pos;
    Vec3 previous_listener_pos;
    f32 previous_time;
    f32 doppler_factor;
    f32 speed_of_sound;
    f32 min_pitch_shift;
    f32 max_pitch_shift;
    bool enabled;
} DopplerProcessor;

// -------------------------------------------------------------------------------------------------
// PROCEDURAL AUDIO GENERATION
// -------------------------------------------------------------------------------------------------

typedef struct {
    ProceduralAudioType type;
    f32 frequency;
    f32 amplitude;
    f32 phase;
    u32 sample_rate;
    f32* buffer;
    u32 buffer_size;
    u32 buffer_pos;
    bool active;
} ProceduralAudio;

typedef struct {
    ProceduralAudio* generators;
    u32 max_generators;
    u32 active_count;
    bool enabled;
} ProceduralAudioSystem;

// -------------------------------------------------------------------------------------------------
// ADAPTIVE MUSIC SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32* stems;
    u32 stem_count;
    u32 stem_length;
    f32 intensity;
    f32 target_intensity;
    f32 transition_speed;
    u32 current_beat;
    u32 beats_per_minute;
    bool is_transitioning;
} AdaptiveMusic;

typedef struct {
    AdaptiveMusic* tracks;
    u32 track_count;
    u32 max_tracks;
    AudioCategory current_category;
    f32 global_intensity;
    bool enabled;
} AdaptiveMusicSystem;

// -------------------------------------------------------------------------------------------------
// AUDIO LOD SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    u32 source_id;
    Vec3 position;
    f32 distance;
    f32 importance;
    u32 update_frequency;
    u32 frames_since_update;
    bool is_virtual;
    bool is_active;
    AudioCategory category;
} AudioVoice;

typedef struct {
    AudioVoice* voices;
    u32 max_voices;
    u32 active_voices;
    u32 virtual_voices;
    u32 max_active_voices;
    f32 cull_distance;
    f32 importance_threshold;
    bool enabled;
} AudioLODSystem;

// -------------------------------------------------------------------------------------------------
// AUDIO DEBUGGING SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 source_pos;
    Vec3 listener_pos;
    f32 level;
    bool is_occluded;
    bool is_playing;
    AudioCategory category;
    f32 distance;
} DebugAudioSource;

typedef struct {
    DebugAudioSource* sources;
    u32 max_sources;
    u32 source_count;
    bool show_rays;
    bool show_zones;
    bool show_levels;
    bool show_occlusion;
    bool enabled;
} AudioDebugSystem;

// -------------------------------------------------------------------------------------------------
// MAIN SPATIAL AUDIO SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    // Core components
    HRTFProcessor hrtf;
    PropagationSystem propagation;
    OcclusionSystem occlusion;
    ReverbZoneSystem reverb_zones;
    DopplerProcessor doppler;
    ProceduralAudioSystem procedural;
    AdaptiveMusicSystem adaptive_music;
    AudioLODSystem lod;
    AudioDebugSystem debug;
    
    // Global state
    Vec3 listener_position;
    Quat listener_orientation;
    Vec3 listener_velocity;
    f32 master_volume;
    u32 sample_rate;
    u32 buffer_size;
    u32 current_frame;
    
    // Distance attenuation
    DistanceAttenuation distance_models[AUDIO_CATEGORY_COUNT];
    
    // Performance tracking
    f32 processing_time_ms;
    u32 active_sources;
    u32 virtualized_sources;
    
    bool initialized;
} SpatialAudioSystem;

// -------------------------------------------------------------------------------------------------
// FUNCTION DECLARATIONS
// -------------------------------------------------------------------------------------------------

// System initialization and shutdown
bool SpatialAudio_Initialize(u32 sample_rate, u32 buffer_size);
void SpatialAudio_Shutdown(void);
void SpatialAudio_Update(f32 delta_time);

// Listener control
void SpatialAudio_SetListenerPosition(const Vec3 position);
void SpatialAudio_SetListenerOrientation(const Quat orientation);
void SpatialAudio_SetListenerVelocity(const Vec3 velocity);
void SpatialAudio_GetListenerState(Vec3* position, Quat* orientation, Vec3* velocity);

// Source management
u32 SpatialAudio_AddSource(const Vec3 position, AudioCategory category);
void SpatialAudio_RemoveSource(u32 source_id);
void SpatialAudio_SetSourcePosition(u32 source_id, const Vec3 position);
void SpatialAudio_SetSourceVelocity(u32 source_id, const Vec3 velocity);
void SpatialAudio_SetSourceGain(u32 source_id, f32 gain);
void SpatialAudio_SetSourcePitch(u32 source_id, f32 pitch);

// HRTF Binaural Audio
bool HRTF_Initialize(HRTFProcessor* processor, u32 sample_rate, u32 buffer_size);
void HRTF_Shutdown(HRTFProcessor* processor);
void HRTF_Process(HRTFProcessor* processor, const f32* input, f32* left_output, f32* right_output, u32 frames, const Vec3 source_pos, const Vec3 listener_pos, const Quat listener_orient);
bool HRTF_LoadDatabase(HRTFProcessor* processor, const char* dataset_path);

// Sound Propagation
bool Propagation_Initialize(PropagationSystem* system, u32 max_rays);
void Propagation_Shutdown(PropagationSystem* system);
void Propagation_Process(PropagationSystem* system, const Vec3 source_pos, const Vec3 listener_pos);
f32 Propagation_GetAttenuation(const PropagationSystem* system, u32 ray_index);

// Occlusion and Obstruction
bool Occlusion_Initialize(OcclusionSystem* system, u32 max_sources);
void Occlusion_Shutdown(OcclusionSystem* system);
void Occlusion_Update(OcclusionSystem* system, u32 source_id, const Vec3 source_pos, const Vec3 listener_pos);
f32 Occlusion_GetFactor(const OcclusionSystem* system, u32 source_id);
f32 Occlusion_GetLowpassFreq(const OcclusionSystem* system, u32 source_id);

// Reverb Zone System
bool ReverbZones_Initialize(ReverbZoneSystem* system, u32 max_zones);
void ReverbZones_Shutdown(ReverbZoneSystem* system);
u32 ReverbZones_AddZone(ReverbZoneSystem* system, const Vec3 position, const Vec3 size, ReverbZoneType type);
void ReverbZones_Update(ReverbZoneSystem* system, const Vec3 listener_pos);
void ReverbZones_GetParameters(const ReverbZoneSystem* system, f32* decay_time, f32* damping, f32* pre_delay);

// Distance Attenuation
void DistanceAttenuation_SetModel(DistanceAttenuation* attenuation, DistanceModel model, f32 min_dist, f32 max_dist, f32 rolloff);
f32 DistanceAttenuation_Calculate(const DistanceAttenuation* attenuation, f32 distance);

// Doppler Effect
bool Doppler_Initialize(DopplerProcessor* processor);
void Doppler_Shutdown(DopplerProcessor* processor);
f32 Doppler_CalculateShift(DopplerProcessor* processor, const Vec3 source_pos, const Vec3 source_vel, const Vec3 listener_pos, const Vec3 listener_vel, f32 current_time);

// Procedural Audio
bool Procedural_Initialize(ProceduralAudioSystem* system, u32 max_generators);
void Procedural_Shutdown(ProceduralAudioSystem* system);
u32 Procedural_CreateGenerator(ProceduralAudioSystem* system, ProceduralAudioType type, f32 frequency, f32 amplitude);
void Procedural_SetParameters(u32 generator_id, f32 frequency, f32 amplitude);
void Procedural_Generate(ProceduralAudioSystem* system, f32* output, u32 frames);

// Adaptive Music
bool AdaptiveMusic_Initialize(AdaptiveMusicSystem* system, u32 max_tracks);
void AdaptiveMusic_Shutdown(AdaptiveMusicSystem* system);
u32 AdaptiveMusic_AddTrack(AdaptiveMusicSystem* system, const f32* stems, u32 stem_count, u32 stem_length);
void AdaptiveMusic_SetIntensity(AdaptiveMusicSystem* system, u32 track_id, f32 intensity);
void AdaptiveMusic_Process(AdaptiveMusicSystem* system, f32* output, u32 frames);

// Audio LOD
bool AudioLOD_Initialize(AudioLODSystem* system, u32 max_voices, u32 max_active);
void AudioLOD_Shutdown(AudioLODSystem* system);
void AudioLOD_Update(AudioLODSystem* system, const Vec3 listener_pos);
bool AudioLOD_IsSourceActive(const AudioLODSystem* system, u32 source_id);

// Audio Debugging
bool AudioDebug_Initialize(AudioDebugSystem* system, u32 max_sources);
void AudioDebug_Shutdown(AudioDebugSystem* system);
void AudioDebug_AddSource(AudioDebugSystem* system, u32 source_id, const Vec3 position, AudioCategory category, f32 level);
void AudioDebug_UpdateSource(AudioDebugSystem* system, u32 source_id, f32 level, bool is_occluded);
void AudioDebug_Render(AudioDebugSystem* system);

// Main processing function
void SpatialAudio_Process(f32* input_buffer, f32* left_output, f32* right_output, u32 frames, u32 source_id);

// Utility functions
f32 SpatialAudio_CalculateDistance(const Vec3 pos1, const Vec3 pos2);
void SpatialAudio_CalculateSphericalCoordinates(const Vec3 source_pos, const Vec3 listener_pos, const Quat listener_orient, f32* azimuth, f32* elevation, f32* distance);

// Global access
extern SpatialAudioSystem* g_spatial_audio;

#endif // SPATIAL_AUDIO_H
