// src/engine/rendering/audio_visual/audio_visual_sync.c
// Audio-Visual Synchronization - Real-time audio analysis and visual effects synchronization

#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Audio-Visual Synchronization Types
// ============================================================================

typedef enum {
    AUDIO_VISUAL_TYPE_SPECTRUM,
    AUDIO_VISUAL_TYPE_WAVEFORM,
    AUDIO_VISUAL_TYPE_BEAT_DETECTION,
    AUDIO_VISUAL_TYPE_FREQUENCY_BANDS,
    AUDIO_VISUAL_TYPE_VOLUME_METER,
    AUDIO_VISUAL_TYPE_PARTICLE_SYNC,
    AUDIO_VISUAL_TYPE_LIGHT_SYNC,
    AUDIO_VISUAL_TYPE_CAMERA_SHAKE,
    AUDIO_VISUAL_TYPE_POST_PROCESS,
    AUDIO_VISUAL_TYPE_COUNT
} AudioVisualType;

typedef struct {
    float frequency;
    float amplitude;
    float phase;
    float bandwidth;
} FrequencyBand;

typedef struct {
    float *spectrum_data;
    uint32_t spectrum_size;
    float *waveform_data;
    uint32_t waveform_size;
    float *frequency_bands;
    uint32_t band_count;
    float volume;
    float peak_volume;
    float beat_strength;
    float beat_frequency;
    float bass_energy;
    float mid_energy;
    float treble_energy;
    
    // FFT parameters
    uint32_t fft_size;
    float *fft_input;
    float *fft_output;
    float *window_function;
    
    // Beat detection
    float beat_threshold;
    float beat_sensitivity;
    float last_beat_time;
    float beat_interval;
    bool beat_detected;
    
    // Smoothing
    float smoothing_factor;
    float *smoothed_spectrum;
    float *smoothed_bands;
    
    // Performance
    bool enable_fft;
    bool enable_beat_detection;
    bool enable_smoothing;
    uint32_t sample_rate;
    uint32_t buffer_size;
    
    char name[256];
    bool initialized;
} AudioAnalyzer;

typedef struct {
    // Visual effects parameters
    float particle_intensity;
    float particle_speed;
    float particle_size;
    float particle_count;
    
    float light_intensity;
    float light_color[3];
    float light_pulse_speed;
    
    float camera_shake_intensity;
    float camera_shake_frequency;
    
    float post_process_intensity;
    float color_shift[3];
    float blur_amount;
    float distortion_amount;
    
    // Synchronization
    AudioVisualType sync_type;
    float sync_strength;
    float sync_delay;
    bool invert_sync;
    
    // Animation
    float animation_speed;
    float animation_amplitude;
    float animation_offset;
    
    char name[256];
    bool active;
} VisualEffect;

typedef struct {
    AudioAnalyzer *analyzers[16];
    uint32_t analyzer_count;
    uint32_t analyzer_capacity;
    
    VisualEffect *effects[64];
    uint32_t effect_count;
    uint32_t effect_capacity;
    
    // Global settings
    float master_volume;
    float global_sync_strength;
    float latency_compensation;
    bool enable_auto_sync;
    
    // Performance
    uint64_t total_analysis_time_ms;
    uint64_t total_sync_time_ms;
    uint32_t frames_processed;
    float average_fps;
    
    // Thread safety
    pthread_mutex_t av_mutex;
    
    bool initialized;
} AudioVisualSystem;

static AudioVisualSystem g_av_system = {0};

// ============================================================================
// Audio Analysis Functions
// ============================================================================

static void apply_window_function(float *data, uint32_t size, const float *window) {
    for (uint32_t i = 0; i < size; i++) {
        data[i] *= window[i];
    }
}

static void compute_fft(float *input, float *output, uint32_t size) {
    // Simplified FFT implementation (real FFT)
    // In a real implementation, use a proper FFT library like FFTW or KissFFT
    
    // Copy input to output
    memcpy(output, input, size * sizeof(float));
    
    // Apply window function
    if (g_av_system.analyzers[0]->window_function) {
        apply_window_function(output, size, g_av_system.analyzers[0]->window_function);
    }
    
    // Simplified DFT (for demonstration - use proper FFT in production)
    for (uint32_t k = 0; k < size / 2; k++) {
        float real = 0.0f;
        float imag = 0.0f;
        
        for (uint32_t n = 0; n < size; n++) {
            float angle = -2.0f * M_PI * k * n / size;
            real += input[n] * cosf(angle);
            imag += input[n] * sinf(angle);
        }
        
        output[k] = sqrtf(real * real + imag * imag) / size;
    }
}

static void create_hamming_window(float *window, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        window[i] = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (size - 1));
    }
}

static void analyze_frequency_bands(AudioAnalyzer *analyzer) {
    if (!analyzer || !analyzer->spectrum_data || !analyzer->frequency_bands) return;
    
    // Define frequency bands (simplified)
    float band_frequencies[] = {60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000};
    uint32_t band_count = sizeof(band_frequencies) / sizeof(band_frequencies[0]);
    
    for (uint32_t i = 0; i < band_count && i < analyzer->band_count; i++) {
        float band_energy = 0.0f;
        uint32_t start_bin = (uint32_t)(band_frequencies[i] * analyzer->fft_size / analyzer->sample_rate);
        uint32_t end_bin = (i < band_count - 1) ? 
                          (uint32_t)(band_frequencies[i + 1] * analyzer->fft_size / analyzer->sample_rate) : 
                          analyzer->spectrum_size / 2;
        
        if (start_bin >= analyzer->spectrum_size) start_bin = analyzer->spectrum_size - 1;
        if (end_bin >= analyzer->spectrum_size) end_bin = analyzer->spectrum_size - 1;
        
        for (uint32_t j = start_bin; j < end_bin; j++) {
            band_energy += analyzer->spectrum_data[j];
        }
        
        analyzer->frequency_bands[i] = band_energy / (end_bin - start_bin);
    }
    
    // Calculate bass, mid, treble energy
    analyzer->bass_energy = analyzer->frequency_bands[0] + analyzer->frequency_bands[1];
    analyzer->mid_energy = analyzer->frequency_bands[2] + analyzer->frequency_bands[3] + analyzer->frequency_bands[4];
    analyzer->treble_energy = analyzer->frequency_bands[5] + analyzer->frequency_bands[6] + analyzer->frequency_bands[7];
}

static void detect_beats(AudioAnalyzer *analyzer, float current_time) {
    if (!analyzer) return;
    
    // Simple beat detection based on bass energy
    float bass_energy = analyzer->bass_energy;
    
    // Calculate beat strength
    if (bass_energy > analyzer->beat_threshold) {
        analyzer->beat_strength = (bass_energy - analyzer->beat_threshold) / analyzer->beat_threshold;
        
        // Check if this is a new beat
        if (current_time - analyzer->last_beat_time > analyzer->beat_interval) {
            analyzer->beat_detected = true;
            analyzer->last_beat_time = current_time;
            analyzer->beat_frequency = 1.0f / (current_time - analyzer->last_beat_time);
        }
    } else {
        analyzer->beat_detected = false;
        analyzer->beat_strength = 0.0f;
    }
    
    // Adaptive threshold
    analyzer->beat_threshold = analyzer->beat_threshold * 0.98f + bass_energy * 0.02f;
}

static void smooth_audio_data(AudioAnalyzer *analyzer) {
    if (!analyzer || !analyzer->enable_smoothing) return;
    
    // Smooth spectrum data
    for (uint32_t i = 0; i < analyzer->spectrum_size; i++) {
        analyzer->smoothed_spectrum[i] = analyzer->smoothed_spectrum[i] * analyzer->smoothing_factor + 
                                        analyzer->spectrum_data[i] * (1.0f - analyzer->smoothing_factor);
    }
    
    // Smooth frequency bands
    for (uint32_t i = 0; i < analyzer->band_count; i++) {
        analyzer->smoothed_bands[i] = analyzer->smoothed_bands[i] * analyzer->smoothing_factor + 
                                     analyzer->frequency_bands[i] * (1.0f - analyzer->smoothing_factor);
    }
}

// ============================================================================
// Visual Effect Synchronization
// ============================================================================

static void sync_particle_effect(VisualEffect *effect, AudioAnalyzer *analyzer) {
    if (!effect || !analyzer) return;
    
    float sync_value = 0.0f;
    
    switch (effect->sync_type) {
        case AUDIO_VISUAL_TYPE_SPECTRUM:
            // Use average spectrum energy
            sync_value = 0.0f;
            for (uint32_t i = 0; i < analyzer->spectrum_size; i++) {
                sync_value += analyzer->smoothed_spectrum[i];
            }
            sync_value /= analyzer->spectrum_size;
            break;
            
        case AUDIO_VISUAL_TYPE_FREQUENCY_BANDS:
            // Use bass energy for particles
            sync_value = analyzer->bass_energy;
            break;
            
        case AUDIO_VISUAL_TYPE_BEAT_DETECTION:
            // Use beat strength
            sync_value = analyzer->beat_strength;
            break;
            
        case AUDIO_VISUAL_TYPE_VOLUME_METER:
            // Use volume
            sync_value = analyzer->volume;
            break;
            
        default:
            sync_value = 0.5f;
            break;
    }
    
    // Apply sync parameters
    if (effect->invert_sync) {
        sync_value = 1.0f - sync_value;
    }
    
    sync_value *= effect->sync_strength * g_av_system.global_sync_strength;
    
    // Update particle parameters
    effect->particle_intensity = sync_value;
    effect->particle_speed = 0.5f + sync_value * 2.0f;
    effect->particle_size = 1.0f + sync_value * 3.0f;
    effect->particle_count = (uint32_t)(100 + sync_value * 900);
}

static void sync_light_effect(VisualEffect *effect, AudioAnalyzer *analyzer) {
    if (!effect || !analyzer) return;
    
    float sync_value = 0.0f;
    
    switch (effect->sync_type) {
        case AUDIO_VISUAL_TYPE_FREQUENCY_BANDS:
            // Use different bands for RGB channels
            effect->light_color[0] = analyzer->bass_energy;     // Red = bass
            effect->light_color[1] = analyzer->mid_energy;      // Green = mid
            effect->light_color[2] = analyzer->treble_energy;  // Blue = treble
            sync_value = (effect->light_color[0] + effect->light_color[1] + effect->light_color[2]) / 3.0f;
            break;
            
        case AUDIO_VISUAL_TYPE_BEAT_DETECTION:
            sync_value = analyzer->beat_strength;
            // Flash white on beats
            if (analyzer->beat_detected) {
                effect->light_color[0] = effect->light_color[1] = effect->light_color[2] = 1.0f;
            } else {
                effect->light_color[0] = effect->light_color[1] = effect->light_color[2] = sync_value * 0.5f;
            }
            break;
            
        case AUDIO_VISUAL_TYPE_VOLUME_METER:
            sync_value = analyzer->volume;
            effect->light_color[0] = effect->light_color[1] = effect->light_color[2] = sync_value;
            break;
            
        default:
            sync_value = 0.5f;
            effect->light_color[0] = effect->light_color[1] = effect->light_color[2] = sync_value;
            break;
    }
    
    // Apply sync parameters
    if (effect->invert_sync) {
        sync_value = 1.0f - sync_value;
    }
    
    sync_value *= effect->sync_strength * g_av_system.global_sync_strength;
    
    // Update light parameters
    effect->light_intensity = sync_value;
    effect->light_pulse_speed = 1.0f + sync_value * 5.0f;
}

static void sync_camera_shake_effect(VisualEffect *effect, AudioAnalyzer *analyzer) {
    if (!effect || !analyzer) return;
    
    float sync_value = 0.0f;
    
    switch (effect->sync_type) {
        case AUDIO_VISUAL_TYPE_BEAT_DETECTION:
            sync_value = analyzer->beat_strength;
            break;
            
        case AUDIO_VISUAL_TYPE_FREQUENCY_BANDS:
            sync_value = analyzer->bass_energy;
            break;
            
        case AUDIO_VISUAL_TYPE_VOLUME_METER:
            sync_value = analyzer->volume;
            break;
            
        default:
            sync_value = 0.0f;
            break;
    }
    
    // Apply sync parameters
    if (effect->invert_sync) {
        sync_value = 1.0f - sync_value;
    }
    
    sync_value *= effect->sync_strength * g_av_system.global_sync_strength;
    
    // Update camera shake parameters
    effect->camera_shake_intensity = sync_value * 0.1f; // Max 0.1 units shake
    effect->camera_shake_frequency = 10.0f + sync_value * 40.0f; // 10-50 Hz
}

static void sync_post_process_effect(VisualEffect *effect, AudioAnalyzer *analyzer) {
    if (!effect || !analyzer) return;
    
    float sync_value = 0.0f;
    
    switch (effect->sync_type) {
        case AUDIO_VISUAL_TYPE_SPECTRUM:
            // Use high frequency energy for color shift
            sync_value = analyzer->treble_energy;
            effect->color_shift[0] = sync_value; // Red shift
            effect->color_shift[1] = analyzer->mid_energy * 0.5f; // Green shift
            effect->color_shift[2] = analyzer->bass_energy * 0.3f; // Blue shift
            break;
            
        case AUDIO_VISUAL_TYPE_FREQUENCY_BANDS:
            sync_value = (analyzer->bass_energy + analyzer->mid_energy + analyzer->treble_energy) / 3.0f;
            effect->color_shift[0] = analyzer->bass_energy;
            effect->color_shift[1] = analyzer->mid_energy;
            effect->color_shift[2] = analyzer->treble_energy;
            break;
            
        case AUDIO_VISUAL_TYPE_BEAT_DETECTION:
            sync_value = analyzer->beat_strength;
            // Pulse effect on beats
            if (analyzer->beat_detected) {
                effect->blur_amount = sync_value * 0.5f;
                effect->distortion_amount = sync_value * 0.2f;
            } else {
                effect->blur_amount *= 0.9f; // Fade out
                effect->distortion_amount *= 0.9f;
            }
            break;
            
        default:
            sync_value = 0.0f;
            effect->color_shift[0] = effect->color_shift[1] = effect->color_shift[2] = 0.0f;
            break;
    }
    
    // Apply sync parameters
    if (effect->invert_sync) {
        sync_value = 1.0f - sync_value;
    }
    
    sync_value *= effect->sync_strength * g_av_system.global_sync_strength;
    
    // Update post-process parameters
    effect->post_process_intensity = sync_value;
}

// ============================================================================
// Audio-Visual System API
// ============================================================================

bool audio_visual_system_init(uint32_t max_analyzers, uint32_t max_effects, uint32_t sample_rate, uint32_t buffer_size) {
    if (g_av_system.initialized) {
        LOG_WARN("Audio-visual system already initialized");
        return true;
    }
    
    memset(&g_av_system, 0, sizeof(AudioVisualSystem));
    
    g_av_system.analyzer_capacity = max_analyzers;
    g_av_system.effect_capacity = max_effects;
    g_av_system.sample_rate = sample_rate;
    g_av_system.buffer_size = buffer_size;
    
    g_av_system.master_volume = 1.0f;
    g_av_system.global_sync_strength = 1.0f;
    g_av_system.latency_compensation = 0.0f;
    g_av_system.enable_auto_sync = true;
    
    if (pthread_mutex_init(&g_av_system.av_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize audio-visual mutex");
        return false;
    }
    
    g_av_system.initialized = true;
    LOG_INFO("Audio-visual system initialized (analyzers: %u, effects: %u, sample_rate: %u, buffer_size: %u)",
             max_analyzers, max_effects, sample_rate, buffer_size);
    return true;
}

void audio_visual_system_shutdown(void) {
    if (!g_av_system.initialized)
        return;
    
    LOG_INFO("Shutting down audio-visual system");
    
    // Destroy all analyzers
    for (uint32_t i = 0; i < g_av_system.analyzer_count; i++) {
        if (g_av_system.analyzers[i]) {
            audio_analyzer_destroy(g_av_system.analyzers[i]);
        }
    }
    
    // Destroy all effects
    for (uint32_t i = 0; i < g_av_system.effect_count; i++) {
        if (g_av_system.effects[i]) {
            visual_effect_destroy(g_av_system.effects[i]);
        }
    }
    
    pthread_mutex_destroy(&g_av_system.av_mutex);
    
    memset(&g_av_system, 0, sizeof(AudioVisualSystem));
    
    LOG_INFO("Audio-visual system shutdown complete");
}

AudioAnalyzer *audio_analyzer_create(const char *name, uint32_t fft_size, bool enable_fft, bool enable_beat_detection, bool enable_smoothing) {
    if (!g_av_system.initialized || !name) {
        LOG_ERROR("Audio-visual system not initialized or invalid name");
        return NULL;
    }
    
    pthread_mutex_lock(&g_av_system.av_mutex);
    
    if (g_av_system.analyzer_count >= g_av_system.analyzer_capacity) {
        LOG_ERROR("Too many audio analyzers");
        pthread_mutex_unlock(&g_av_system.av_mutex);
        return NULL;
    }
    
    AudioAnalyzer *analyzer = calloc(1, sizeof(AudioAnalyzer));
    if (!analyzer) {
        LOG_ERROR("Failed to allocate audio analyzer");
        pthread_mutex_unlock(&g_av_system.av_mutex);
        return NULL;
    }
    
    strncpy(analyzer->name, name, sizeof(analyzer->name) - 1);
    analyzer->fft_size = fft_size;
    analyzer->enable_fft = enable_fft;
    analyzer->enable_beat_detection = enable_beat_detection;
    analyzer->enable_smoothing = enable_smoothing;
    analyzer->sample_rate = g_av_system.sample_rate;
    analyzer->buffer_size = g_av_system.buffer_size;
    
    // Allocate audio data buffers
    analyzer->spectrum_size = fft_size / 2;
    analyzer->spectrum_data = calloc(analyzer->spectrum_size, sizeof(float));
    analyzer->waveform_data = calloc(g_av_system.buffer_size, sizeof(float));
    analyzer->frequency_bands = calloc(10, sizeof(float)); // 10 frequency bands
    analyzer->smoothed_spectrum = calloc(analyzer->spectrum_size, sizeof(float));
    analyzer->smoothed_bands = calloc(10, sizeof(float));
    
    // Allocate FFT buffers
    analyzer->fft_input = calloc(fft_size, sizeof(float));
    analyzer->fft_output = calloc(fft_size, sizeof(float));
    analyzer->window_function = calloc(fft_size, sizeof(float));
    
    if (!analyzer->spectrum_data || !analyzer->waveform_data || !analyzer->frequency_bands || 
        !analyzer->smoothed_spectrum || !analyzer->smoothed_bands || 
        !analyzer->fft_input || !analyzer->fft_output || !analyzer->window_function) {
        LOG_ERROR("Failed to allocate audio analyzer buffers");
        free(analyzer->spectrum_data);
        free(analyzer->waveform_data);
        free(analyzer->frequency_bands);
        free(analyzer->smoothed_spectrum);
        free(analyzer->smoothed_bands);
        free(analyzer->fft_input);
        free(analyzer->fft_output);
        free(analyzer->window_function);
        free(analyzer);
        pthread_mutex_unlock(&g_av_system.av_mutex);
        return NULL;
    }
    
    // Create window function
    create_hamming_window(analyzer->window_function, fft_size);
    
    // Initialize beat detection parameters
    analyzer->beat_threshold = 0.1f;
    analyzer->beat_sensitivity = 0.5f;
    analyzer->beat_interval = 0.1f; // Minimum 100ms between beats
    
    // Initialize smoothing
    analyzer->smoothing_factor = 0.8f;
    
    g_av_system.analyzers[g_av_system.analyzer_count++] = analyzer;
    
    pthread_mutex_unlock(&g_av_system.av_mutex);
    
    LOG_INFO("Created audio analyzer: %s (fft_size: %u, fft: %s, beats: %s, smoothing: %s)",
             name, fft_size, enable_fft ? "yes" : "no", enable_beat_detection ? "yes" : "no", enable_smoothing ? "yes" : "no");
    return analyzer;
}

void audio_analyzer_destroy(AudioAnalyzer *analyzer) {
    if (!analyzer) return;
    
    pthread_mutex_lock(&g_av_system.av_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_av_system.analyzer_count; i++) {
        if (g_av_system.analyzers[i] == analyzer) {
            g_av_system.analyzers[i] = g_av_system.analyzers[g_av_system.analyzer_count - 1];
            g_av_system.analyzer_count--;
            break;
        }
    }
    
    // Free buffers
    free(analyzer->spectrum_data);
    free(analyzer->waveform_data);
    free(analyzer->frequency_bands);
    free(analyzer->smoothed_spectrum);
    free(analyzer->smoothed_bands);
    free(analyzer->fft_input);
    free(analyzer->fft_output);
    free(analyzer->window_function);
    
    free(analyzer);
    
    pthread_mutex_unlock(&g_av_system.av_mutex);
    
    LOG_DEBUG("Destroyed audio analyzer: %s", analyzer->name);
}

void audio_analyzer_process_audio(AudioAnalyzer *analyzer, const float *audio_data, uint32_t sample_count, float current_time) {
    if (!analyzer || !audio_data) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Copy audio data to input buffer
    uint32_t copy_size = (sample_count < analyzer->fft_size) ? sample_count : analyzer->fft_size;
    memcpy(analyzer->fft_input, audio_data, copy_size * sizeof(float));
    
    // Zero-pad if necessary
    for (uint32_t i = copy_size; i < analyzer->fft_size; i++) {
        analyzer->fft_input[i] = 0.0f;
    }
    
    // Update waveform data
    memcpy(analyzer->waveform_data, audio_data, sample_count * sizeof(float));
    
    // Compute FFT
    if (analyzer->enable_fft) {
        compute_fft(analyzer->fft_input, analyzer->fft_output, analyzer->fft_size);
        memcpy(analyzer->spectrum_data, analyzer->fft_output, analyzer->spectrum_size * sizeof(float));
    }
    
    // Analyze frequency bands
    analyze_frequency_bands(analyzer);
    
    // Calculate volume
    analyzer->volume = 0.0f;
    for (uint32_t i = 0; i < sample_count; i++) {
        analyzer->volume += fabsf(audio_data[i]);
    }
    analyzer->volume /= sample_count;
    
    // Update peak volume
    if (analyzer->volume > analyzer->peak_volume) {
        analyzer->peak_volume = analyzer->volume;
    }
    
    // Beat detection
    if (analyzer->enable_beat_detection) {
        detect_beats(analyzer, current_time);
    }
    
    // Smooth data
    if (analyzer->enable_smoothing) {
        smooth_audio_data(analyzer);
    }
    
    uint64_t end_time = get_time_nanos();
    g_av_system.total_analysis_time_ms += nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Processed audio: %s (%.2f ms, volume: %.3f, beat: %s)", 
             analyzer->name, g_av_system.total_analysis_time_ms, analyzer->volume, analyzer->beat_detected ? "yes" : "no");
}

VisualEffect *visual_effect_create(const char *name, AudioVisualType sync_type, float sync_strength) {
    if (!g_av_system.initialized || !name) {
        LOG_ERROR("Audio-visual system not initialized or invalid name");
        return NULL;
    }
    
    pthread_mutex_lock(&g_av_system.av_mutex);
    
    if (g_av_system.effect_count >= g_av_system.effect_capacity) {
        LOG_ERROR("Too many visual effects");
        pthread_mutex_unlock(&g_av_system.av_mutex);
        return NULL;
    }
    
    VisualEffect *effect = calloc(1, sizeof(VisualEffect));
    if (!effect) {
        LOG_ERROR("Failed to allocate visual effect");
        pthread_mutex_unlock(&g_av_system.av_mutex);
        return NULL;
    }
    
    strncpy(effect->name, name, sizeof(effect->name) - 1);
    effect->sync_type = sync_type;
    effect->sync_strength = sync_strength;
    effect->active = true;
    
    // Initialize default parameters
    effect->particle_intensity = 0.5f;
    effect->particle_speed = 1.0f;
    effect->particle_size = 1.0f;
    effect->particle_count = 100;
    
    effect->light_intensity = 1.0f;
    effect->light_color[0] = effect->light_color[1] = effect->light_color[2] = 1.0f;
    effect->light_pulse_speed = 1.0f;
    
    effect->camera_shake_intensity = 0.0f;
    effect->camera_shake_frequency = 10.0f;
    
    effect->post_process_intensity = 0.0f;
    effect->color_shift[0] = effect->color_shift[1] = effect->color_shift[2] = 0.0f;
    effect->blur_amount = 0.0f;
    effect->distortion_amount = 0.0f;
    
    effect->animation_speed = 1.0f;
    effect->animation_amplitude = 1.0f;
    effect->animation_offset = 0.0f;
    
    g_av_system.effects[g_av_system.effect_count++] = effect;
    
    pthread_mutex_unlock(&g_av_system.av_mutex);
    
    LOG_INFO("Created visual effect: %s (sync_type: %d, sync_strength: %.2f)", name, (int)sync_type, sync_strength);
    return effect;
}

void visual_effect_destroy(VisualEffect *effect) {
    if (!effect) return;
    
    pthread_mutex_lock(&g_av_system.av_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_av_system.effect_count; i++) {
        if (g_av_system.effects[i] == effect) {
            g_av_system.effects[i] = g_av_system.effects[g_av_system.effect_count - 1];
            g_av_system.effect_count--;
            break;
        }
    }
    
    free(effect);
    
    pthread_mutex_unlock(&g_av_system.av_mutex);
    
    LOG_DEBUG("Destroyed visual effect: %s", effect->name);
}

void audio_visual_system_update(float dt) {
    if (!g_av_system.initialized) return;
    
    uint64_t start_time = get_time_nanos();
    
    pthread_mutex_lock(&g_av_system.av_mutex);
    
    // Update all visual effects
    for (uint32_t i = 0; i < g_av_system.effect_count; i++) {
        VisualEffect *effect = g_av_system.effects[i];
        if (!effect->active) continue;
        
        // Find a suitable audio analyzer
        AudioAnalyzer *analyzer = (g_av_system.analyzer_count > 0) ? g_av_system.analyzers[0] : NULL;
        
        if (analyzer) {
            // Synchronize effect with audio
            switch (effect->sync_type) {
                case AUDIO_VISUAL_TYPE_PARTICLE_SYNC:
                    sync_particle_effect(effect, analyzer);
                    break;
                    
                case AUDIO_VISUAL_TYPE_LIGHT_SYNC:
                    sync_light_effect(effect, analyzer);
                    break;
                    
                case AUDIO_VISUAL_TYPE_CAMERA_SHAKE:
                    sync_camera_shake_effect(effect, analyzer);
                    break;
                    
                case AUDIO_VISUAL_TYPE_POST_PROCESS:
                    sync_post_process_effect(effect, analyzer);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    pthread_mutex_unlock(&g_av_system.av_mutex);
    
    uint64_t end_time = get_time_nanos();
    g_av_system.total_sync_time_ms += nanos_to_ms(end_time - start_time);
    g_av_system.frames_processed++;
    g_av_system.average_fps = 1000.0f / (g_av_system.total_sync_time_ms / (g_av_system.frames_processed + 1));
    
    LOG_DEBUG("Audio-visual update: %.2f ms, fps: %.1f", g_av_system.total_sync_time_ms, g_av_system.average_fps);
}

void audio_visual_system_set_global_sync_strength(float strength) {
    if (!g_av_system.initialized) return;
    
    g_av_system.global_sync_strength = strength;
    
    LOG_DEBUG("Updated global sync strength: %.2f", strength);
}

void audio_visual_system_set_master_volume(float volume) {
    if (!g_av_system.initialized) return;
    
    g_av_system.master_volume = volume;
    
    LOG_DEBUG("Updated master volume: %.2f", volume);
}

void audio_analyzer_get_spectrum_data(AudioAnalyzer *analyzer, float **spectrum_data, uint32_t *size) {
    if (!analyzer || !spectrum_data || !size) return;
    
    *spectrum_data = analyzer->smoothed_spectrum ? analyzer->smoothed_spectrum : analyzer->spectrum_data;
    *size = analyzer->spectrum_size;
}

void audio_analyzer_get_frequency_bands(AudioAnalyzer *analyzer, float **bands, uint32_t *count) {
    if (!analyzer || !bands || !count) return;
    
    *bands = analyzer->smoothed_bands ? analyzer->smoothed_bands : analyzer->frequency_bands;
    *count = analyzer->band_count;
}

void visual_effect_get_particle_parameters(VisualEffect *effect, float *intensity, float *speed, float *size, uint32_t *count) {
    if (!effect) return;
    
    if (intensity) *intensity = effect->particle_intensity;
    if (speed) *speed = effect->particle_speed;
    if (size) *size = effect->particle_size;
    if (count) *count = effect->particle_count;
}

void visual_effect_get_light_parameters(VisualEffect *effect, float *intensity, float *color, float *pulse_speed) {
    if (!effect) return;
    
    if (intensity) *intensity = effect->light_intensity;
    if (color) {
        color[0] = effect->light_color[0];
        color[1] = effect->light_color[1];
        color[2] = effect->light_color[2];
    }
    if (pulse_speed) *pulse_speed = effect->light_pulse_speed;
}

void audio_visual_system_get_stats(float *analysis_time, float *sync_time, uint32_t *frames_processed, float *average_fps) {
    if (!g_av_system.initialized) return;
    
    if (analysis_time) *analysis_time = g_av_system.total_analysis_time_ms;
    if (sync_time) *sync_time = g_av_system.total_sync_time_ms;
    if (frames_processed) *frames_processed = g_av_system.frames_processed;
    if (average_fps) *average_fps = g_av_system.average_fps;
    
    LOG_DEBUG("Audio-visual stats: analysis=%.2f ms, sync=%.2f ms, frames=%u, fps=%.1f",
             *analysis_time, *sync_time, *frames_processed, *average_fps);
}

bool audio_visual_system_is_initialized(void) {
    return g_av_system.initialized;
}
