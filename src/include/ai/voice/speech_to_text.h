#ifndef SPEECH_TO_TEXT_H
#define SPEECH_TO_TEXT_H

#include <core/types.h>
#include <core/logger.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Speech-to-Text System - Voice recognition with VAD and keyword spotting

// Forward declarations
typedef struct STTSystem STTSystem;
typedef struct MicrophoneBuffer MicrophoneBuffer;
typedef struct VoiceActivityDetector VoiceActivityDetector;
typedef struct KeywordSpotter KeywordSpotter;

// Audio buffer structure
typedef struct {
    f32* samples;           // Audio samples (32-bit float)
    u32 sample_count;       // Number of samples
    u32 sample_rate;        // Sample rate (Hz)
    u32 channels;           // Number of channels
    f64 timestamp;          // Capture timestamp
} AudioBuffer;

// Voice activity detection result
typedef struct {
    bool is_speaking;       // True if voice activity detected
    f32 confidence;         // Confidence level (0.0 - 1.0)
    f32 energy_level;       // Audio energy level
    f64 duration;           // Speaking duration
} VADResult;

// Keyword spotting result
typedef struct {
    bool keyword_detected;  // True if keyword detected
    char keyword[64];       // Detected keyword
    f32 confidence;         // Confidence level (0.0 - 1.0)
    f64 timestamp;          // Detection timestamp
} KeywordResult;

// Speech recognition result
typedef struct {
    char text[512];         // Recognized text
    f32 confidence;         // Confidence level (0.0 - 1.0)
    f64 processing_time;    // Processing time in milliseconds
    bool is_final;          // True if final result
} SpeechResult;

// Intent parsing result
typedef struct {
    char intent[64];        // Intent type (e.g., "move", "attack", "stop")
    char target[128];       // Target entity/object
    char action[64];        // Specific action
    Vec3 location;          // Location if specified
    f32 confidence;         // Confidence level (0.0 - 1.0)
    bool has_location;      // True if location was specified
} IntentResult;

// STT System configuration
typedef struct {
    u32 sample_rate;        // Audio sample rate (default: 16000)
    u32 buffer_size;        // Buffer size in samples (default: 1024)
    u32 channels;           // Number of channels (default: 1)
    f32 vad_threshold;      // VAD energy threshold (default: 0.01)
    f32 keyword_threshold;  // Keyword spotting threshold (default: 0.7)
    bool enable_whisper;     // Enable Whisper integration
    bool enable_keywords;   // Enable keyword spotting
    bool enable_intent_parsing; // Enable intent parsing
    char whisper_model_path[256]; // Path to Whisper model
} STTConfig;

// Microphone buffer management
MicrophoneBuffer* microphone_create(u32 sample_rate, u32 buffer_size);
void microphone_destroy(MicrophoneBuffer* mic);
bool microphone_start_capture(MicrophoneBuffer* mic);
bool microphone_stop_capture(MicrophoneBuffer* mic);
AudioBuffer* microphone_get_buffer(MicrophoneBuffer* mic);
void microphone_clear_buffer(MicrophoneBuffer* mic);

// Voice Activity Detection
VoiceActivityDetector* vad_create(f32 energy_threshold);
void vad_destroy(VoiceActivityDetector* vad);
VADResult vad_process(VoiceActivityDetector* vad, const AudioBuffer* buffer);
void vad_reset(VoiceActivityDetector* vad);

// Keyword Spotting
KeywordSpotter* keyword_spotter_create(const char* model_path, f32 threshold);
void keyword_spotter_destroy(KeywordSpotter* ks);
KeywordResult keyword_spotter_process(KeywordSpotter* ks, const AudioBuffer* buffer);
void keyword_spotter_add_keyword(KeywordSpotter* ks, const char* keyword);

// Speech Recognition (Whisper integration)
STTSystem* stt_create(const STTConfig* config);
void stt_destroy(STTSystem* stt);
SpeechResult stt_process_audio(STTSystem* stt, const AudioBuffer* buffer);
bool stt_is_ready(STTSystem* stt);
void stt_set_language(STTSystem* stt, const char* language_code);

// Intent Parsing
IntentResult stt_parse_intent(STTSystem* stt, const char* text);
void stt_add_context_entity(STTSystem* stt, const char* entity_name, const Vec3* location);
void stt_clear_context(STTSystem* stt);

// Main STT System API
bool stt_initialize(STTSystem* stt);
void stt_shutdown(STTSystem* stt);
void stt_update(STTSystem* stt, f64 delta_time);
bool stt_is_speaking(STTSystem* stt);
SpeechResult stt_get_last_result(STTSystem* stt);
KeywordResult stt_get_last_keyword(STTSystem* stt);
IntentResult stt_get_last_intent(STTSystem* stt);

// Utility functions
const char* stt_get_error_message(STTSystem* stt);
bool stt_save_audio(STTSystem* stt, const char* filename);
void stt_set_debug_mode(STTSystem* stt, bool enabled);

#ifdef __cplusplus
}
#endif

#endif // SPEECH_TO_TEXT_H
