#ifndef AUDIO_SURROUND_H
#define AUDIO_SURROUND_H

#include <common.h>
#include <math/math.h>

// -------------------------------------------------------------------------------------------------
// 7.1.2 SURROUND SOUND CONFIGURATION
// -------------------------------------------------------------------------------------------------

typedef enum {
    SPEAKER_FRONT_LEFT = 0,
    SPEAKER_FRONT_RIGHT,
    SPEAKER_FRONT_CENTER,
    SPEAKER_LFE,
    SPEAKER_SIDE_LEFT,
    SPEAKER_SIDE_RIGHT,
    SPEAKER_REAR_LEFT,
    SPEAKER_REAR_RIGHT,
    SPEAKER_TOP_LEFT,
    SPEAKER_TOP_RIGHT,
    SPEAKER_COUNT
} SpeakerChannel;

typedef struct {
    Vec3 position;
    f32 gain;
    f32 distance;
    f32 delay_ms;
    bool active;
} SpeakerConfig;

typedef struct {
    SpeakerConfig speakers[SPEAKER_COUNT];
    f32 master_gain;
    u32 sample_rate;
    u32 buffer_size;
    bool height_channels_enabled;
} SurroundConfig;

// -------------------------------------------------------------------------------------------------
// VECTOR BASE AMPLITUDE PANNING (VBAP)
// -------------------------------------------------------------------------------------------------

typedef struct {
    Vec3 speaker_positions[SPEAKER_COUNT];
    u32 triangle_count;
    u32 triangles[SPEAKER_COUNT * 8][3];  // Max triangles for speaker setup
    f32 gain_matrix[SPEAKER_COUNT][3];
} VBAPSetup;

typedef struct {
    Vec3 source_position;
    f32 gains[SPEAKER_COUNT];
    u32 active_triangle;
    f32 pan_gains[3];
} VBAPRenderer;

// -------------------------------------------------------------------------------------------------
// OBJECT-BASED AUDIO (ATMOS STYLE)
// -------------------------------------------------------------------------------------------------

typedef struct {
    u32 id;
    Vec3 position;
    Vec3 velocity;
    f32 gain;
    f32 size;  // Source size for diffusion
    bool active;
    void* audio_data;
    size_t audio_size;
    u32 sample_rate;
} AudioObject;

typedef struct {
    AudioObject* objects;
    u32 max_objects;
    u32 active_count;
    f32 listener_position[3];
    f32 listener_orientation[4];  // Quaternion
    f32 render_distance;
} ObjectAudioRenderer;

// -------------------------------------------------------------------------------------------------
// FUNCTION DECLARATIONS
// -------------------------------------------------------------------------------------------------

// 7.1.2 Configuration
void Audio_InitSurroundConfig(SurroundConfig* config);
void Audio_Setup71Speakers(SurroundConfig* config);
bool Audio_ValidateSpeakerSetup(const SurroundConfig* config);
void Audio_PrintSpeakerSetup(const SurroundConfig* config);

// VBAP Functions
bool Audio_InitVBAP(VBAPSetup* setup, const SurroundConfig* config);
void Audio_CalculateVBAPGains(VBAPSetup* setup, VBAPRenderer* renderer, const Vec3 source_pos);
void Audio_ApplyVBAPGains(const VBAPRenderer* renderer, f32* output_buffer, const f32* input_buffer, u32 frames);

// Object-Based Audio
bool Audio_InitObjectRenderer(ObjectAudioRenderer* renderer, u32 max_objects);
u32 Audio_AddAudioObject(ObjectAudioRenderer* renderer, const AudioObject* object);
void Audio_UpdateAudioObject(ObjectAudioRenderer* renderer, u32 object_id, const AudioObject* object);
void Audio_RemoveAudioObject(ObjectAudioRenderer* renderer, u32 object_id);
void Audio_RenderObjectAudio(ObjectAudioRenderer* renderer, f32* output_buffer, u32 frames);

// Main surround mixing
void Audio_MixSurround(f32* output_buffer, const f32* input_buffer, u32 frames, const SurroundConfig* config);
void Audio_UpdateListener(ObjectAudioRenderer* renderer, const Vec3 position, const Quat orientation);

#endif // AUDIO_SURROUND_H
