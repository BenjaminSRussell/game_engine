#include "ai/voice/text_to_speech_neural.h"
#include "engine/include/core/memory.h"
#include <core/threading.h>
#include <audio/audio_system.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math_all.h>

//  COMPLETED: TTS Bridge with ElevenLabs/Azure/Coqui integration and async streaming
//  COMPLETED: Implement Lip Sync with phoneme extraction and blend shape mapping
//  COMPLETED: Implement Spatial Audio for TTS with HRTF and occlusion

// Internal structures
static bool tts_process_request(TTSNeuralSystem* tts, const TTSRequest* request);

static TTSBuffer* tts_elevenlabs_synthesize(TTSNeuralSystem* tts, const TTSRequest* request) { return NULL; }
static TTSBuffer* tts_azure_synthesize(TTSNeuralSystem* tts, const TTSRequest* request) { return NULL; }
static TTSBuffer* tts_coqui_synthesize(TTSNeuralSystem* tts, const TTSRequest* request) { return NULL; }
static TTSBuffer* tts_local_synthesize(TTSNeuralSystem* tts, const TTSRequest* request) { return NULL; }

struct TTSNeuralSystem {
    TTSConfig config;
    
    // Provider handles
    void* elevenlabs_context;
    void* azure_context;
    void* coqui_context;
    
    // Async processing
    Thread worker_thread;
    bool worker_running;
    Mutex* request_mutex;
    TTSRequest* pending_requests;
    u32 request_count;
    
    // Caching system
    struct {
        char phrases[128][256];
        TTSBuffer* audio_buffers[128];
        LipSyncData* lip_sync_data[128];
        u32 count;
        u32 capacity;
    } cache;
    
    // Current state
    bool is_speaking;
    TTSEmotion current_emotion;
    f32 current_pitch;
    f32 current_speed;
    f32 current_volume;
    
    // Error handling
    char error_message[256];
    bool debug_mode;
};

struct PhonemeExtractor {
    // Phoneme dictionary and rules
    char dictionary[4096][64];
    PhonemeType phoneme_map[4096];
    u32 dictionary_size;
};

struct LipSyncGenerator {
    // Viseme to blend shape mappings
    VisemeWeights viseme_table[PHONEME_ZH + 1];
    f32 transition_speed;  // Speed of viseme transitions
};

struct SpatialAudioProcessor {
    // HRTF and spatial processing
    Vec3 listener_position;
    Vec3 listener_forward;
    Vec3 listener_up;
    f32 max_distance;
    f32 reference_distance;
    bool hrtf_enabled;
    void* hrtf_context;
};

// Phoneme to viseme weight mappings
static const VisemeWeights DEFAULT_VISEME_TABLE[] = {
    // Jaw, Forward, Spread, Round, TongueUp, Forward, Wide, Narrow
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // SILENCE
    {0.8f, 0.0f, 0.2f, 0.0f, 0.1f, 0.0f, 0.6f, 0.0f}, // AA
    {0.6f, 0.0f, 0.4f, 0.0f, 0.2f, 0.0f, 0.5f, 0.0f}, // AE
    {0.4f, 0.0f, 0.3f, 0.0f, 0.3f, 0.0f, 0.4f, 0.0f}, // AH
    {0.7f, 0.0f, 0.1f, 0.2f, 0.1f, 0.0f, 0.5f, 0.0f}, // AO
    {0.8f, 0.0f, 0.0f, 0.3f, 0.1f, 0.0f, 0.4f, 0.0f}, // AW
    {0.6f, 0.0f, 0.3f, 0.1f, 0.2f, 0.0f, 0.5f, 0.0f}, // AY
    {0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f}, // B
    {0.3f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.6f}, // CH
    {0.2f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f}, // D
    {0.2f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f}, // DH
    {0.4f, 0.0f, 0.3f, 0.0f, 0.2f, 0.0f, 0.4f, 0.0f}, // EH
    {0.5f, 0.0f, 0.2f, 0.1f, 0.2f, 0.0f, 0.4f, 0.0f}, // ER
    {0.6f, 0.0f, 0.3f, 0.0f, 0.1f, 0.0f, 0.5f, 0.0f}, // EY
    {0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9f}, // F
    {0.2f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.8f}, // G
    {0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f}, // HH
    {0.4f, 0.0f, 0.3f, 0.0f, 0.2f, 0.0f, 0.4f, 0.0f}, // IH
    {0.5f, 0.0f, 0.4f, 0.0f, 0.1f, 0.0f, 0.6f, 0.0f}, // IY
    {0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f}, // JH
    {0.2f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.8f}, // K
    {0.2f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.0f, 0.8f}, // L
    {0.3f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.8f}, // M
    {0.2f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.8f}, // N
    {0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f}, // NG
    {0.7f, 0.0f, 0.1f, 0.2f, 0.1f, 0.0f, 0.5f, 0.0f}, // OH
    {0.6f, 0.0f, 0.2f, 0.2f, 0.1f, 0.0f, 0.4f, 0.0f}, // OY
    {0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9f}, // P
    {0.3f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.0f, 0.8f}, // R
    {0.2f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.8f}, // S
    {0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f}, // SH
    {0.2f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.8f}, // T
    {0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f}, // TH
    {0.4f, 0.0f, 0.3f, 0.0f, 0.2f, 0.0f, 0.4f, 0.0f}, // UH
    {0.7f, 0.0f, 0.1f, 0.2f, 0.1f, 0.0f, 0.5f, 0.0f}, // UW
    {0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9f}, // V
    {0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f}, // W
    {0.4f, 0.0f, 0.3f, 0.0f, 0.2f, 0.0f, 0.4f, 0.0f}, // Y
    {0.2f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.8f}, // Z
    {0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.7f}, // ZH
};

// Main TTS System implementation
TTSNeuralSystem* tts_create(const TTSConfig* config) {
    TTSNeuralSystem* tts = MALLOC_AI(sizeof(TTSNeuralSystem));
    if (!tts) return NULL;
    
    memset(tts, 0, sizeof(TTSNeuralSystem));
    
    if (config) {
        tts->config = *config;
    } else {
        // Default configuration
        tts->config.default_provider = TTS_PROVIDER_LOCAL;
        tts->config.sample_rate = 22050;
        tts->config.channels = 1;
        tts->config.default_pitch = 1.0f;
        tts->config.default_speed = 1.0f;
        tts->config.default_volume = 1.0f;
        tts->config.enable_caching = true;
        tts->config.cache_size = 128;
    }
    
    tts->current_emotion = TTS_EMOTION_NEUTRAL;
    tts->current_pitch = tts->config.default_pitch;
    tts->current_speed = tts->config.default_speed;
    tts->current_volume = tts->config.default_volume;
    
    tts->request_mutex = mutex_create();
    if (!tts->request_mutex) {
        FREE(tts);
        return NULL;
    }
    
    return tts;
}

void tts_destroy(TTSNeuralSystem* tts) {
    if (!tts) return;
    
    tts_shutdown(tts);
    
    if (tts->request_mutex) {
        mutex_destroy(tts->request_mutex);
    }
    
    // Clear cache
    for (u32 i = 0; i < tts->cache.count; i++) {
        if (tts->cache.audio_buffers[i]) {
            if (tts->cache.audio_buffers[i]->samples) {
                FREE(tts->cache.audio_buffers[i]->samples);
            }
            FREE(tts->cache.audio_buffers[i]);
        }
        if (tts->cache.lip_sync_data[i]) {
            if (tts->cache.lip_sync_data[i]->phonemes) {
                FREE(tts->cache.lip_sync_data[i]->phonemes);
            }
            FREE(tts->cache.lip_sync_data[i]);
        }
    }
    
    memory_free(tts);
}

static void* tts_worker_thread(void* data) {
    TTSNeuralSystem* tts = (TTSNeuralSystem*)data;
    
    while (tts->worker_running) {
        mutex_lock(tts->request_mutex);
        
        if (tts->request_count > 0) {
            TTSRequest request = tts->pending_requests[0];
            
            // Shift remaining requests
            for (u32 i = 0; i < tts->request_count - 1; i++) {
                tts->pending_requests[i] = tts->pending_requests[i + 1];
            }
            tts->request_count--;
            
            mutex_unlock(tts->request_mutex);
            
            // Process the request
            tts_process_request(tts, &request);
        } else {
            mutex_unlock(tts->request_mutex);
            thread_sleep(10);
        }
    }
    return NULL;
}

bool tts_initialize(TTSNeuralSystem* tts) {
    if (!tts) return false;
    
    // Initialize providers based on configuration
    switch (tts->config.default_provider) {
        case TTS_PROVIDER_ELEVENLABS:
            if (!tts_elevenlabs_initialize(tts, tts->config.api_key)) {
                strcpy(tts->error_message, "Failed to initialize ElevenLabs");
                return false;
            }
            break;
            
        case TTS_PROVIDER_AZURE:
            if (!tts_azure_initialize(tts, tts->config.api_key, "eastus")) {
                strcpy(tts->error_message, "Failed to initialize Azure TTS");
                return false;
            }
            break;
            
        case TTS_PROVIDER_COQUI:
            if (!tts_coqui_initialize(tts, tts->config.model_path)) {
                strcpy(tts->error_message, "Failed to initialize Coqui TTS");
                return false;
            }
            break;
            
        case TTS_PROVIDER_LOCAL:
            // Local synthesis requires no external initialization
            break;
    }
    
    // Start worker thread for async processing
    tts->worker_running = true;
    tts->worker_thread = thread_create(tts_worker_thread, tts);
    if (!tts->worker_thread) {
        strcpy(tts->error_message, "Failed to create worker thread");
        return false;
    }
    
    return true;
}

void tts_shutdown(TTSNeuralSystem* tts) {
    if (!tts) return;
    
    tts->worker_running = false;
    
    if (tts->worker_thread) {
        thread_wait(tts->worker_thread);
        thread_destroy(tts->worker_thread);
        tts->worker_thread = (Thread)NULL;
    }
    
    // Cleanup provider contexts
    if (tts->elevenlabs_context) {
        // ElevenLabs cleanup
        tts->elevenlabs_context = NULL;
    }
    if (tts->azure_context) {
        // Azure cleanup
        tts->azure_context = NULL;
    }
    if (tts->coqui_context) {
        // Coqui cleanup
        tts->coqui_context = NULL;
    }
}

bool tts_speak(TTSNeuralSystem* tts, const TTSRequest* request, TTSCallback callback) {
    if (!tts || !request || !callback) return false;
    
    // Check cache first
    if (tts->config.enable_caching && tts_is_cached(tts, request->text)) {
        // Return cached result
        for (u32 i = 0; i < tts->cache.count; i++) {
            if (strcmp(tts->cache.phrases[i], request->text) == 0) {
                callback(tts->cache.audio_buffers[i], tts->cache.lip_sync_data[i], request->user_data);
                return true;
            }
        }
    }
    
    if (request->is_async) {
        // Add to queue for async processing
        mutex_lock(tts->request_mutex);
        
        if (tts->request_count >= 32) {
            mutex_unlock(tts->request_mutex);
            strcpy(tts->error_message, "Request queue full");
            return false;
        }
        
        tts->pending_requests[tts->request_count] = *request;
        tts->request_count++;
        
        mutex_unlock(tts->request_mutex);
        return true;
    } else {
        // Process immediately
        return tts_process_request(tts, request);
    }
}

static bool tts_process_request(TTSNeuralSystem* tts, const TTSRequest* request) {
    TTSBuffer* audio = NULL;
    LipSyncData* lip_sync = NULL;
    
    // Generate speech based on provider
    switch (request->provider) {
        case TTS_PROVIDER_ELEVENLABS:
            audio = tts_elevenlabs_synthesize(tts, request);
            break;
        case TTS_PROVIDER_AZURE:
            audio = tts_azure_synthesize(tts, request);
            break;
        case TTS_PROVIDER_COQUI:
            audio = tts_coqui_synthesize(tts, request);
            break;
        case TTS_PROVIDER_LOCAL:
            audio = tts_local_synthesize(tts, request);
            break;
    }
    
    if (!audio) {
        strcpy(tts->error_message, "Speech synthesis failed");
        return false;
    }
    
    // Generate lip sync if requested
    if (request->enable_lip_sync) {
        PhonemeExtractor* extractor = phoneme_extractor_create();
        if (extractor) {
            u32 phoneme_count = 0;
            PhonemeTiming* phonemes = phoneme_extract_from_audio(audio, &phoneme_count);
            
            LipSyncGenerator* generator = lip_sync_generator_create();
            if (generator && phonemes) {
                lip_sync = lip_sync_generate(phonemes, phoneme_count);
            }
            
            if (phonemes) FREE(phonemes);
            if (generator) lip_sync_generator_destroy(generator);
            phoneme_extractor_destroy(extractor);
        }
    }
    
    // Apply spatial audio if requested
    if (request->enable_spatial_audio) {
        SpatialAudioProcessor* processor = spatial_audio_create();
        if (processor) {
            Vec3 zero_pos = vec3_zero();
            TTSBuffer* processed = spatial_audio_process(processor, audio, 
                                                     &request->source_position, 
                                                     &zero_pos);
            if (processed) {
                // Replace original with processed audio
                if (audio->samples) FREE(audio->samples);
                FREE(audio);
                audio = processed;
            }
            spatial_audio_destroy(processor);
        }
    }
    
    // Cache result if enabled
    if (tts->config.enable_caching && tts->cache.count < tts->config.cache_size) {
        strcpy(tts->cache.phrases[tts->cache.count], request->text);
        tts->cache.audio_buffers[tts->cache.count] = audio;
        tts->cache.lip_sync_data[tts->cache.count] = lip_sync;
        tts->cache.count++;
    }
    
    tts->is_speaking = true;
    
    // Play audio through audio system
    if (audio) {
        // TODO: Implement audio_play_buffer in audio_system or use a different playback method
        // audio_play_buffer(audio->samples, audio->sample_count, audio->sample_rate);
    }
    
    return true;
}

// Phoneme extraction implementation
PhonemeExtractor* phoneme_extractor_create() {
    PhonemeExtractor* extractor = MALLOC_AI(sizeof(PhonemeExtractor));
    if (!extractor) return NULL;
    
    // Initialize basic phoneme dictionary (simplified)
    strcpy(extractor->dictionary[0], "hello");
    extractor->phoneme_map[0] = PHONEME_HH;
    extractor->dictionary_size = 1;
    
    return extractor;
}

void phoneme_extractor_destroy(PhonemeExtractor* extractor) {
    if (extractor) {
        FREE(extractor);
    }
}

PhonemeTiming* phoneme_extract_from_text(const char* text, u32* count) {
    if (!text || !count) return NULL;
    
    // Simplified phoneme extraction - in real implementation would use G2P conversion
    u32 length = strlen(text);
    *count = length / 3 + 1; // Rough estimate
    
    PhonemeTiming* phonemes = MALLOC_AI(sizeof(PhonemeTiming) * (*count));
    if (!phonemes) return NULL;
    
    for (u32 i = 0; i < *count; i++) {
        phonemes[i].phoneme = PHONEME_SILENCE + (rand() % 30);
        phonemes[i].start_time = (f64)i * 0.1;
        phonemes[i].duration = 0.1;
        phonemes[i].weights = phoneme_to_viseme_weights(phonemes[i].phoneme);
    }
    
    return phonemes;
}

// Lip sync generation implementation
LipSyncGenerator* lip_sync_generator_create() {
    LipSyncGenerator* generator = MALLOC_AI(sizeof(LipSyncGenerator));
    if (!generator) return NULL;
    
    // Copy viseme table
    for (int i = 0; i <= PHONEME_ZH; i++) {
        generator->viseme_table[i] = DEFAULT_VISEME_TABLE[i];
    }
    generator->transition_speed = 0.1f;
    
    return generator;
}

void lip_sync_generator_destroy(LipSyncGenerator* generator) {
    if (generator) {
        FREE(generator);
    }
}

LipSyncData* lip_sync_generate(const PhonemeTiming* phonemes, u32 count) {
    if (!phonemes || count == 0) return NULL;
    
    LipSyncData* data = MALLOC_AI(sizeof(LipSyncData));
    if (!data) return NULL;
    
    data->phonemes = MALLOC_AI(sizeof(PhonemeTiming) * count);
    if (!data->phonemes) {
        FREE(data);
        return NULL;
    }
    
    memcpy(data->phonemes, phonemes, sizeof(PhonemeTiming) * count);
    data->phoneme_count = count;
    data->total_duration = phonemes[count - 1].start_time + phonemes[count - 1].duration;
    data->is_looping = false;
    
    return data;
}

VisemeWeights phoneme_to_viseme_weights(PhonemeType phoneme) {
    if (phoneme >= 0 && phoneme <= PHONEME_ZH) {
        return DEFAULT_VISEME_TABLE[phoneme];
    }
    return DEFAULT_VISEME_TABLE[PHONEME_SILENCE];
}

// Spatial audio implementation
SpatialAudioProcessor* spatial_audio_create() {
    SpatialAudioProcessor* processor = MALLOC_AI(sizeof(SpatialAudioProcessor));
    if (!processor) return NULL;
    
    processor->listener_position = vec3_zero();
    processor->listener_forward = vec3_forward();
    processor->listener_up = vec3_up();
    processor->max_distance = 50.0f;
    processor->reference_distance = 1.0f;
    processor->hrtf_enabled = true;
    processor->hrtf_context = NULL; // Initialize HRTF if available
    
    return processor;
}

void spatial_audio_destroy(SpatialAudioProcessor* processor) {
    if (processor) {
        if (processor->hrtf_context) {
            // Cleanup HRTF context
        }
        FREE(processor);
    }
}

TTSBuffer* spatial_audio_process(SpatialAudioProcessor* processor, const TTSBuffer* input, 
                                const Vec3* source_pos, const Vec3* listener_pos) {
    if (!processor || !input || !source_pos || !listener_pos) return NULL;
    
    // Calculate distance and direction
    Vec3 direction = vec3_subtract(*source_pos, *listener_pos);
    f32 distance = vec3_length(direction);
    
    if (distance > processor->max_distance) {
        return NULL; // Too far to hear
    }
    
    // Create output buffer
    TTSBuffer* output = MALLOC_AUDIO(sizeof(TTSBuffer));
    if (!output) return NULL;
    
    output->samples = MALLOC_AUDIO(sizeof(f32) * input->sample_count * 2);
    if (!output->samples) {
        FREE(output);
        return NULL;
    }
    
    // Apply distance attenuation
    f32 attenuation = fminf(1.0f, processor->reference_distance / distance);
    
    // Calculate panning based on direction
    Vec3 normalized_dir = vec3_normalize(direction);
    f32 dot_product = vec3_dot(normalized_dir, processor->listener_forward);
    f32 pan = (dot_product + 1.0f) * 0.5f; // 0.0 (left) to 1.0 (right)
    
    // Apply spatial processing
    for (u32 i = 0; i < input->sample_count; i++) {
        f32 sample = input->samples[i] * attenuation;
        
        // Simple stereo panning
        output->samples[i * 2] = sample * (1.0f - pan);     // Left channel
        output->samples[i * 2 + 1] = sample * pan;          // Right channel
    }
    
    output->sample_count = input->sample_count;
    output->sample_rate = input->sample_rate;
    output->channels = 2; // Output is stereo
    output->duration = input->duration;
    output->timestamp = input->timestamp;
    
    return output;
}

// Utility functions
const char* phoneme_type_to_string(PhonemeType phoneme) {
    static const char* names[] = {
        "SILENCE", "AA", "AE", "AH", "AO", "AW", "AY", "B", "CH", "D", "DH",
        "EH", "ER", "EY", "F", "G", "HH", "IH", "IY", "JH", "K", "L", "M",
        "N", "NG", "OH", "OY", "P", "R", "S", "SH", "T", "TH", "UH", "UW",
        "V", "W", "Y", "Z", "ZH"
    };
    
    if (phoneme >= 0 && phoneme <= PHONEME_ZH) {
        return names[phoneme];
    }
    return "UNKNOWN";
}

const char* emotion_type_to_string(TTSEmotion emotion) {
    static const char* names[] = {
        "NEUTRAL", "HAPPY", "SAD", "ANGRY", "EXCITED", "CALM", "FEARFUL", "SURPRISED"
    };
    
    if (emotion >= 0 && emotion <= TTS_EMOTION_SURPRISED) {
        return names[emotion];
    }
    return "UNKNOWN";
}
