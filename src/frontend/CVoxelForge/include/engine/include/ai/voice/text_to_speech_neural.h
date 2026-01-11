#ifndef TEXT_TO_SPEECH_NEURAL_H
#define TEXT_TO_SPEECH_NEURAL_H

#include <core/types.h>
#include <core/logger.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Text-to-Speech Neural System - Advanced TTS with lip sync and spatial audio

// Forward declarations
typedef struct TTSNeuralSystem TTSNeuralSystem;
typedef struct PhonemeExtractor PhonemeExtractor;
typedef struct LipSyncGenerator LipSyncGenerator;
typedef struct SpatialAudioProcessor SpatialAudioProcessor;

// TTS Provider types
typedef enum {
    TTS_PROVIDER_ELEVENLABS,
    TTS_PROVIDER_AZURE,
    TTS_PROVIDER_COQUI,
    TTS_PROVIDER_LOCAL
} TTSProvider;

// Emotion types for TTS
typedef enum {
    TTS_EMOTION_NEUTRAL,
    TTS_EMOTION_HAPPY,
    TTS_EMOTION_SAD,
    TTS_EMOTION_ANGRY,
    TTS_EMOTION_EXCITED,
    TTS_EMOTION_CALM,
    TTS_EMOTION_FEARFUL,
    TTS_EMOTION_SURPRISED
} TTSEmotion;

// Phoneme/Viseme types
typedef enum {
    PHONEME_SILENCE,
    PHONEME_AA,     // father
    PHONEME_AE,     // cat
    PHONEME_AH,     // cut
    PHONEME_AO,     // dog
    PHONEME_AW,     // out
    PHONEME_AY,     // buy
    PHONEME_B,      // big
    PHONEME_CH,     // chin
    PHONEME_D,      // dig
    PHONEME_DH,     // then
    PHONEME_EH,     // get
    PHONEME_ER,     // bird
    PHONEME_EY,     // say
    PHONEME_F,      // four
    PHONEME_G,      // get
    PHONEME_HH,     // hat
    PHONEME_IH,     // sit
    PHONEME_IY,     // see
    PHONEME_JH,     // judge
    PHONEME_K,      // kick
    PHONEME_L,      // led
    PHONEME_M,      // mat
    PHONEME_N,      // net
    PHONEME_NG,     // sing
    PHONEME_OH,     // boat
    PHONEME_OY,     // boy
    PHONEME_P,      // put
    PHONEME_R,      // red
    PHONEME_S,      // sit
    PHONEME_SH,     // she
    PHONEME_T,      // ten
    PHONEME_TH,     // thin
    PHONEME_UH,     // cup
    PHONEME_UW,     // too
    PHONEME_V,      // vat
    PHONEME_W,      // with
    PHONEME_Y,      // yes
    PHONEME_Z,      // zoo
    PHONEME_ZH      // measure
} PhonemeType;

// Viseme blend shape weights
typedef struct {
    f32 jaw_open;       // Jaw opening
    f32 jaw_forward;    // Jaw forward position
    f32 lip_spread;     // Lip spreading
    f32 lip_round;      // Lip rounding
    f32 tongue_up;      // Tongue position
    f32 tongue_forward; // Tongue forward position
    f32 mouth_wide;     // Mouth width
    f32 mouth_narrow;   // Mouth narrowness
} VisemeWeights;

// Audio buffer for TTS output
typedef struct {
    f32* samples;       // Audio samples (32-bit float)
    u32 sample_count;   // Number of samples
    u32 sample_rate;    // Sample rate (Hz)
    u32 channels;       // Number of channels
    f64 duration;       // Duration in seconds
    f64 timestamp;      // Generation timestamp
} TTSBuffer;

// Phoneme timing information
typedef struct {
    PhonemeType phoneme;
    f64 start_time;     // Start time in seconds
    f64 duration;       // Duration in seconds
    VisemeWeights weights; // Corresponding viseme weights
} PhonemeTiming;

// Lip sync animation data
typedef struct {
    PhonemeTiming* phonemes;  // Array of phoneme timings
    u32 phoneme_count;        // Number of phonemes
    f64 total_duration;       // Total animation duration
    bool is_looping;          // Whether animation should loop
} LipSyncData;

// TTS request structure
typedef struct {
    char text[512];            // Text to synthesize
    TTSEmotion emotion;       // Desired emotion
    TTSProvider provider;      // TTS provider to use
    u32 character_id;         // Character ID for voice selection
    f32 pitch;                // Pitch adjustment (0.5 - 2.0)
    f32 speed;                // Speed adjustment (0.5 - 2.0)
    f32 volume;               // Volume adjustment (0.0 - 1.0)
    bool enable_lip_sync;     // Generate lip sync data
    bool enable_spatial_audio; // Apply spatial processing
    Vec3 source_position;      // Source position for spatial audio
    bool is_async;            // Process asynchronously
    void* user_data;          // User callback data
} TTSRequest;

// TTS result callback
typedef void (*TTSCallback)(const TTSBuffer* audio, const LipSyncData* lip_sync, void* user_data);

// TTS System configuration
typedef struct {
    TTSProvider default_provider;   // Default TTS provider
    u32 sample_rate;               // Audio sample rate (default: 22050)
    u32 channels;                 // Number of channels (default: 1)
    f32 default_pitch;             // Default pitch (1.0)
    f32 default_speed;             // Default speed (1.0)
    f32 default_volume;            // Default volume (1.0)
    bool enable_caching;           // Enable phrase caching
    u32 cache_size;               // Maximum cache entries
    char api_key[256];            // API key for cloud services
    char model_path[256];          // Path to local model
} TTSConfig;

// Main TTS System API
TTSNeuralSystem* tts_create(const TTSConfig* config);
void tts_destroy(TTSNeuralSystem* tts);
bool tts_initialize(TTSNeuralSystem* tts);
void tts_shutdown(TTSNeuralSystem* tts);

// Speech synthesis
bool tts_speak(TTSNeuralSystem* tts, const TTSRequest* request, TTSCallback callback);
bool tts_speak_immediate(TTSNeuralSystem* tts, const char* text, u32 character_id);
void tts_stop_speech(TTSNeuralSystem* tts);
bool tts_is_speaking(TTSNeuralSystem* tts);

// Voice and emotion control
void tts_set_emotion(TTSNeuralSystem* tts, TTSEmotion emotion);
void tts_set_pitch(TTSNeuralSystem* tts, f32 pitch);
void tts_set_speed(TTSNeuralSystem* tts, f32 speed);
void tts_set_volume(TTSNeuralSystem* tts, f32 volume);
void tts_set_character_voice(TTSNeuralSystem* tts, u32 character_id, const char* voice_name);

// Phrase caching
void tts_cache_phrase(TTSNeuralSystem* tts, const char* phrase);
bool tts_is_cached(TTSNeuralSystem* tts, const char* phrase);
void tts_clear_cache(TTSNeuralSystem* tts);

// Phoneme extraction
PhonemeExtractor* phoneme_extractor_create();
void phoneme_extractor_destroy(PhonemeExtractor* extractor);
PhonemeTiming* phoneme_extract_from_text(const char* text, u32* count);
PhonemeTiming* phoneme_extract_from_audio(const TTSBuffer* audio, u32* count);

// Lip sync generation
LipSyncGenerator* lip_sync_generator_create();
void lip_sync_generator_destroy(LipSyncGenerator* generator);
LipSyncData* lip_sync_generate(const PhonemeTiming* phonemes, u32 count);
VisemeWeights phoneme_to_viseme_weights(PhonemeType phoneme);

// Spatial audio processing
SpatialAudioProcessor* spatial_audio_create();
void spatial_audio_destroy(SpatialAudioProcessor* processor);
TTSBuffer* spatial_audio_process(SpatialAudioProcessor* processor, const TTSBuffer* input, 
                                const Vec3* source_pos, const Vec3* listener_pos);
void spatial_audio_set_listener(SpatialAudioProcessor* processor, const Vec3* position, 
                               const Vec3* forward, const Vec3* up);

// Utility functions
const char* tts_get_error_message(TTSNeuralSystem* tts);
void tts_set_debug_mode(TTSNeuralSystem* tts, bool enabled);
const char* phoneme_type_to_string(PhonemeType phoneme);
const char* emotion_type_to_string(TTSEmotion emotion);

// Provider-specific functions
bool tts_elevenlabs_initialize(TTSNeuralSystem* tts, const char* api_key);
bool tts_azure_initialize(TTSNeuralSystem* tts, const char* api_key, const char* region);
bool tts_coqui_initialize(TTSNeuralSystem* tts, const char* model_path);

#ifdef __cplusplus
}
#endif

#endif // TEXT_TO_SPEECH_NEURAL_H
