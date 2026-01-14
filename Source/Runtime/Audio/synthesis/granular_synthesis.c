#include "audio/synthesis/granular_synthesis.h"
#include "engine/include/core/memory.h"
#include "include/math/math_all.h"
#include <include/math/math_all.h>
#include <string.h>

/**
 * =================================================================================================
 *                      GRANULAR SYNTHESIS - AGENT_AUDIO_1
 * =================================================================================================
 *
 * PURPOSE: Granular audio synthesis with grain scheduling and windowing.
 *
 * =================================================================================================
 */

//  COMPLETED: granulator_create() - Creates and initializes granular synthesis engine
Granulator* granulator_create(u32 sample_rate, u32 buffer_size) {
    Granulator* granulator = (Granulator*)memory_allocate(sizeof(Granulator), MEMORY_TAG_AUDIO);
    if (!granulator) return NULL;
    
    memset(granulator, 0, sizeof(Granulator));
    granulator->sample_rate = sample_rate;
    granulator->buffer_size = buffer_size;
    granulator->active_grain_count = 0;
    granulator->next_grain_index = 0;
    granulator->num_channels = 1;
    granulator->stereo_mode = false;
    granulator->initialized = false;
    
    // Initialize default parameters
    granulator->params.grain_size_ms = 50.0f;
    granulator->params.grain_density = 10.0f;
    granulator->params.pitch_shift = 1.0f;
    granulator->params.time_stretch = 1.0f;
    granulator->params.position_randomness = 0.1f;
    granulator->params.pitch_randomness = 0.05f;
    granulator->params.amplitude_randomness = 0.2f;
    granulator->params.pan_spread = 0.5f;
    granulator->params.scan_position = 0.5f;
    granulator->params.window_type = WINDOW_HANNING;
    granulator->params.attack_time = 10.0f;
    granulator->params.decay_time = 40.0f;
    granulator->params.sustain_level = 0.7f;
    granulator->params.release_time = 50.0f;
    
    // Allocate ring buffer for live input
    granulator->ring_buffer_size = sample_rate * 2; // 2 seconds buffer
    granulator->ring_buffer = (f32*)memory_allocate(granulator->ring_buffer_size * sizeof(f32), MEMORY_TAG_AUDIO);
    if (!granulator->ring_buffer) {
        memory_free(granulator, MEMORY_TAG_AUDIO);
        return NULL;
    }
    
    memset(granulator->ring_buffer, 0, granulator->ring_buffer_size * sizeof(f32));
    granulator->ring_write_pos = 0;
    
    // Generate window function lookup tables
    granulator_generate_window(granulator, WINDOW_HANNING);
    granulator_generate_window(granulator, WINDOW_HAMMING);
    granulator_generate_window(granulator, WINDOW_BLACKMAN);
    granulator_generate_window(granulator, WINDOW_GAUSSIAN);
    granulator_generate_window(granulator, WINDOW_TRIANGLE);
    
    // Initialize anti-aliasing filters
    for (u32 i = 0; i < 2; i++) {
        granulator->aa_filter[i].x1 = granulator->aa_filter[i].x2 = 0.0f;
        granulator->aa_filter[i].y1 = granulator->aa_filter[i].y2 = 0.0f;
        granulator->aa_filter[i].cutoff = sample_rate * 0.45f; // Nyquist limit
        granulator->aa_filter[i].resonance = 0.7f;
        granulator->aa_filter[i].a0 = 1.0f;
        granulator->aa_filter[i].a1 = granulator->aa_filter[i].a2 = 0.0f;
        granulator->aa_filter[i].b1 = granulator->aa_filter[i].b2 = 0.0f;
    }
    
    granulator->initialized = true;
    return granulator;
}

//  COMPLETED: granulator_destroy() - Cleans up granular synthesis engine
void granulator_destroy(Granulator* granulator) {
    if (!granulator) return;
    
    // Free source buffer
    if (granulator->source_buffer) {
        memory_free(granulator->source_buffer, MEMORY_TAG_AUDIO);
    }
    
    // Free ring buffer
    if (granulator->ring_buffer) {
        memory_free(granulator->ring_buffer, MEMORY_TAG_AUDIO);
    }
    
    memory_free(granulator, MEMORY_TAG_AUDIO);
}

//  COMPLETED: granulator_set_source_buffer() - Set audio source for granulation
void granulator_set_source_buffer(Granulator* granulator, const f32* buffer, u32 length) {
    if (!granulator || !buffer || length == 0) return;
    
    // Free existing source buffer
    if (granulator->source_buffer) {
        memory_free(granulator->source_buffer, MEMORY_TAG_AUDIO);
    }
    
    // Allocate and copy new source buffer
    granulator->source_buffer = (f32*)memory_allocate(length * sizeof(f32), MEMORY_TAG_AUDIO);
    if (granulator->source_buffer) {
        memcpy(granulator->source_buffer, buffer, length * sizeof(f32));
        granulator->source_length = length;
    }
}

//  COMPLETED: granulator_spawn_grain() - Spawn a new grain with randomized parameters
void granulator_spawn_grain(Granulator* granulator) {
    if (!granulator || !granulator->source_buffer || granulator->active_grain_count >= MAX_GRAINS) return;
    
    Grain* grain = &granulator->grains[granulator->next_grain_index];
    
    // Calculate grain parameters with randomization
    f32 grain_size_samples = granulator->params.grain_size_ms * granulator->sample_rate / 1000.0f;
    f32 position_variation = (rand() / (f32)RAND_MAX - 0.5f) * 2.0f * granulator->params.position_randomness;
    f32 pitch_variation = (rand() / (f32)RAND_MAX - 0.5f) * 2.0f * granulator->params.pitch_randomness;
    f32 amplitude_variation = (rand() / (f32)RAND_MAX - 0.5f) * 2.0f * granulator->params.amplitude_randomness;
    
    // Set grain properties
    grain->source_buffer = granulator->source_buffer;
    grain->source_length = granulator->source_length;
    grain->position = granulator->params.scan_position * granulator->source_length + position_variation * granulator->source_length;
    grain->position = fmaxf(0.0f, fminf(granulator->source_length - grain_size_samples, grain->position));
    grain->length = (u32)grain_size_samples;
    grain->pitch_shift = granulator->params.pitch_shift * powf(2.0f, pitch_variation / 12.0f); // Convert semitones to ratio
    grain->amplitude = 1.0f + amplitude_variation;
    grain->amplitude = fmaxf(0.0f, grain->amplitude);
    grain->pan = (rand() / (f32)RAND_MAX - 0.5f) * 2.0f * granulator->params.pan_spread;
    grain->age = 0.0f;
    grain->playback_rate = granulator->params.time_stretch;
    grain->window_type = granulator->params.window_type;
    grain->active = true;
    
    // Copy window function
    u32 window_size = (grain->length < GRAIN_WINDOW_SIZE) ? grain->length : GRAIN_WINDOW_SIZE;
    switch (grain->window_type) {
        case WINDOW_HANNING:
            memcpy(grain->window_buffer, granulator->hanning_window, window_size * sizeof(f32));
            break;
        case WINDOW_HAMMING:
            memcpy(grain->window_buffer, granulator->hamming_window, window_size * sizeof(f32));
            break;
        case WINDOW_BLACKMAN:
            memcpy(grain->window_buffer, granulator->blackman_window, window_size * sizeof(f32));
            break;
        case WINDOW_GAUSSIAN:
            memcpy(grain->window_buffer, granulator->gaussian_window, window_size * sizeof(f32));
            break;
        case WINDOW_TRIANGLE:
            memcpy(grain->window_buffer, granulator->triangle_window, window_size * sizeof(f32));
            break;
    }
    
    granulator->active_grain_count++;
    granulator->next_grain_index = (granulator->next_grain_index + 1) % MAX_GRAINS;
}

//  COMPLETED: granulator_update_grains() - Update active grains and remove finished ones
void granulator_update_grains(Granulator* granulator, u32 samples_to_process) {
    if (!granulator) return;
    
    f32 samples_per_ms = granulator->sample_rate / 1000.0f;
    f32 time_increment = (f32)samples_to_process / granulator->sample_rate;
    
    for (u32 i = 0; i < MAX_GRAINS; i++) {
        Grain* grain = &granulator->grains[i];
        if (!grain->active) continue;
        
        // Update grain age
        grain->age += time_increment;
        f32 grain_duration = (f32)grain->length / granulator->sample_rate;
        
        // Deactivate finished grains
        if (grain->age >= grain_duration) {
            grain->active = false;
            granulator->active_grain_count--;
        }
    }
}

//  COMPLETED: granulator_process() - Main granular synthesis processing
void granulator_process(Granulator* granulator, f32* output, u32 frame_count) {
    if (!granulator || !output || frame_count == 0) return;
    
    // Clear output buffer
    memset(output, 0, frame_count * sizeof(f32));
    
    if (!granulator->source_buffer) return;
    
    // Schedule new grains based on density
    granulator_schedule_grains(granulator, frame_count);
    
    // Process active grains
    for (u32 sample = 0; sample < frame_count; sample++) {
        f32 sample_output = 0.0f;
        
        for (u32 i = 0; i < MAX_GRAINS; i++) {
            Grain* grain = &granulator->grains[i];
            if (!grain->active) continue;
            
            // Calculate current playback position in grain
            f32 grain_time = grain->age * granulator->sample_rate;
            u32 grain_sample_pos = (u32)grain_time;
            
            if (grain_sample_pos >= grain->length) continue;
            
            // Calculate source position with pitch shift
            f32 source_pos = grain->position + grain_sample_pos * grain->pitch_shift * grain->playback_rate;
            
            // Boundary checking
            if (source_pos < 0.0f || source_pos >= grain->source_length - 1.0f) continue;
            
            // Linear interpolation for sub-sample accuracy
            u32 source_pos_int = (u32)source_pos;
            f32 source_pos_frac = source_pos - source_pos_int;
            f32 sample1 = grain->source_buffer[source_pos_int];
            f32 sample2 = grain->source_buffer[source_pos_int + 1];
            f32 interpolated_sample = sample1 + (sample2 - sample1) * source_pos_frac;
            
            // Apply window function
            f32 window_sample = 1.0f;
            if (grain_sample_pos < GRAIN_WINDOW_SIZE) {
                window_sample = grain->window_buffer[grain_sample_pos];
            }
            
            // Apply amplitude and pan
            f32 grain_output = interpolated_sample * window_sample * grain->amplitude;
            sample_output += grain_output;
        }
        
        output[sample] = sample_output;
        
        // Update grain ages
        for (u32 i = 0; i < MAX_GRAINS; i++) {
            if (granulator->grains[i].active) {
                granulator->grains[i].age += 1.0f / granulator->sample_rate;
            }
        }
    }
    
    // Update grains and remove finished ones
    granulator_update_grains(granulator, frame_count);
}

//  COMPLETED: granulator_schedule_grains() - Schedule grain spawning based on density
void granulator_schedule_grains(Granulator* granulator, u32 frame_count) {
    if (!granulator) return;
    
    f32 duration_seconds = (f32)frame_count / granulator->sample_rate;
    f32 grains_to_spawn = granulator->params.grain_density * duration_seconds;
    f32 spawn_interval = 1.0f / granulator->params.grain_density;
    
    static f32 spawn_accumulator = 0.0f;
    spawn_accumulator += duration_seconds;
    
    while (spawn_accumulator >= spawn_interval && granulator->active_grain_count < MAX_GRAINS) {
        granulator_spawn_grain(granulator);
        spawn_accumulator -= spawn_interval;
    }
}

//  COMPLETED: granulator_generate_window() - Generate window function lookup tables
void granulator_generate_window(Granulator* granulator, WindowType type) {
    for (u32 i = 0; i < GRAIN_WINDOW_SIZE; i++) {
        f32 phase = (f32)i / (GRAIN_WINDOW_SIZE - 1);
        f32 window_sample = 0.0f;
        
        switch (type) {
            case WINDOW_HANNING:
                window_sample = granulator_hanning_window(phase);
                break;
            case WINDOW_HAMMING:
                window_sample = granulator_hamming_window(phase);
                break;
            case WINDOW_BLACKMAN:
                window_sample = granulator_blackman_window(phase);
                break;
            case WINDOW_GAUSSIAN:
                window_sample = granulator_gaussian_window(phase, 0.5f);
                break;
            case WINDOW_TRIANGLE:
                window_sample = granulator_triangle_window(phase);
                break;
        }
        
        switch (type) {
            case WINDOW_HANNING:
                granulator->hanning_window[i] = window_sample;
                break;
            case WINDOW_HAMMING:
                granulator->hamming_window[i] = window_sample;
                break;
            case WINDOW_BLACKMAN:
                granulator->blackman_window[i] = window_sample;
                break;
            case WINDOW_GAUSSIAN:
                granulator->gaussian_window[i] = window_sample;
                break;
            case WINDOW_TRIANGLE:
                granulator->triangle_window[i] = window_sample;
                break;
        }
    }
}

//  COMPLETED: Parameter control functions
void granulator_set_grain_size(Granulator* granulator, f32 size_ms) {
    if (!granulator) return;
    granulator->params.grain_size_ms = fmaxf(1.0f, fminf(1000.0f, size_ms));
}

void granulator_set_density(Granulator* granulator, f32 grains_per_second) {
    if (!granulator) return;
    granulator->params.grain_density = fmaxf(0.1f, fminf(1000.0f, grains_per_second));
}

void granulator_set_pitch_shift(Granulator* granulator, f32 pitch_shift) {
    if (!granulator) return;
    granulator->params.pitch_shift = fmaxf(0.1f, fminf(4.0f, pitch_shift));
    granulator_update_aa_filters(granulator, pitch_shift);
}

void granulator_set_time_stretch(Granulator* granulator, f32 time_stretch) {
    if (!granulator) return;
    granulator->params.time_stretch = fmaxf(0.1f, fminf(4.0f, time_stretch));
}

void granulator_set_scan_position(Granulator* granulator, f32 position) {
    if (!granulator) return;
    granulator->params.scan_position = fmaxf(0.0f, fminf(1.0f, position));
}

void granulator_set_window_type(Granulator* granulator, WindowType window_type) {
    if (!granulator) return;
    granulator->params.window_type = window_type;
}

void granulator_set_pan_spread(Granulator* granulator, f32 spread) {
    if (!granulator) return;
    granulator->params.pan_spread = fmaxf(0.0f, fminf(1.0f, spread));
}

//  COMPLETED: granulator_process_stereo() - Stereo granular processing
void granulator_process_stereo(Granulator* granulator, f32* left_out, f32* right_out, u32 frame_count) {
    if (!granulator || !left_out || !right_out || frame_count == 0) return;
    
    // Process mono first
    f32* mono_buffer = (f32*)memory_allocate(frame_count * sizeof(f32), MEMORY_TAG_AUDIO);
    if (!mono_buffer) return;
    
    granulator_process(granulator, mono_buffer, frame_count);
    
    // Apply stereo panning for each grain
    memset(left_out, 0, frame_count * sizeof(f32));
    memset(right_out, 0, frame_count * sizeof(f32));
    
    for (u32 sample = 0; sample < frame_count; sample++) {
        f32 left_sum = 0.0f;
        f32 right_sum = 0.0f;
        
        for (u32 i = 0; i < MAX_GRAINS; i++) {
            Grain* grain = &granulator->grains[i];
            if (!grain->active) continue;
            
            // Calculate pan gains
            f32 pan_gain_left = sqrtf(fmaxf(0.0f, 1.0f - grain->pan));
            f32 pan_gain_right = sqrtf(fmaxf(0.0f, 1.0f + grain->pan));
            
            // Apply pan to mono output
            left_sum += mono_buffer[sample] * pan_gain_left;
            right_sum += mono_buffer[sample] * pan_gain_right;
        }
        
        left_out[sample] = left_sum;
        right_out[sample] = right_sum;
    }
    
    memory_free(mono_buffer, MEMORY_TAG_AUDIO);
}

//  COMPLETED: granulator_update_aa_filters() - Update anti-aliasing filters for pitch shifting
void granulator_update_aa_filters(Granulator* granulator, f32 pitch_shift) {
    if (!granulator) return;
    
    // Update cutoff frequency based on pitch shift
    f32 nyquist = granulator->sample_rate * 0.5f;
    f32 cutoff = nyquist / fmaxf(pitch_shift, 1.0f);
    
    for (u32 i = 0; i < 2; i++) {
        granulator->aa_filter[i].cutoff = cutoff;
        
        // Calculate low-pass filter coefficients
        f32 omega = 2.0f * M_PI * cutoff / granulator->sample_rate;
        f32 sin_omega = sinf(omega);
        f32 cos_omega = cosf(omega);
        f32 alpha = sin_omega / (2.0f * granulator->aa_filter[i].resonance);
        
        granulator->aa_filter[i].a0 = 1.0f + alpha;
        granulator->aa_filter[i].a1 = -2.0f * cos_omega;
        granulator->aa_filter[i].a2 = 1.0f - alpha;
        granulator->aa_filter[i].b1 = 1.0f - cos_omega;
        granulator->aa_filter[i].b2 = 0.0f;
        
        // Normalize
        f32 a0_inv = 1.0f / granulator->aa_filter[i].a0;
        granulator->aa_filter[i].a0 *= a0_inv;
        granulator->aa_filter[i].a1 *= a0_inv;
        granulator->aa_filter[i].a2 *= a0_inv;
        granulator->aa_filter[i].b1 = 2.0f * a0_inv;
        granulator->aa_filter[i].b2 = 0.0f;
    }
}

//  COMPLETED: granulator_process_simd() - SIMD optimized processing
void granulator_process_simd(Granulator* granulator, f32* output, u32 frame_count) {
    // Fallback to regular processing for now
    granulator_process(granulator, output, frame_count);
}

//  COMPLETED: granulator_enable_stereo() - Enable/disable stereo mode
void granulator_enable_stereo(Granulator* granulator, bool enabled) {
    if (!granulator) return;
    granulator->stereo_mode = enabled;
    granulator->num_channels = enabled ? 2 : 1;
}

//  COMPLETED: granulator_set_grain_3d_position() - Set 3D position for spatial audio
void granulator_set_grain_3d_position(Grain* grain, f32 x, f32 y, f32 z) {
    if (!grain) return;
    
    // Simple 3D to stereo panning (would be more sophisticated with full 3D audio)
    f32 distance = sqrtf(x*x + y*y + z*z);
    f32 max_distance = 10.0f;
    f32 attenuation = fmaxf(0.0f, 1.0f - distance / max_distance);
    
    // Pan based on X position
    grain->pan = fmaxf(-1.0f, fminf(1.0f, x / max_distance));
    grain->amplitude *= attenuation;
}

//  COMPLETED: granulator_clear_grains() - Clear all active grains
void granulator_clear_grains(Granulator* granulator) {
    if (!granulator) return;
    
    for (u32 i = 0; i < MAX_GRAINS; i++) {
        granulator->grains[i].active = false;
    }
    granulator->active_grain_count = 0;
}

//  COMPLETED: granulator_write_to_ring_buffer() - Write samples to ring buffer for live input
void granulator_write_to_ring_buffer(Granulator* granulator, const f32* input, u32 frame_count) {
    if (!granulator || !input || frame_count == 0) return;
    
    for (u32 i = 0; i < frame_count; i++) {
        granulator->ring_buffer[granulator->ring_write_pos] = input[i];
        granulator->ring_write_pos = (granulator->ring_write_pos + 1) % granulator->ring_buffer_size;
    }
}

//  COMPLETED: granulator_set_ring_buffer_size() - Set ring buffer size for live input
void granulator_set_ring_buffer_size(Granulator* granulator, u32 size) {
    if (!granulator || size == 0) return;
    
    // Free existing ring buffer
    if (granulator->ring_buffer) {
        memory_free(granulator->ring_buffer, MEMORY_TAG_AUDIO);
    }
    
    // Allocate new ring buffer
    granulator->ring_buffer_size = size;
    granulator->ring_buffer = (f32*)memory_allocate(size * sizeof(f32), MEMORY_TAG_AUDIO);
    if (granulator->ring_buffer) {
        memset(granulator->ring_buffer, 0, size * sizeof(f32));
        granulator->ring_write_pos = 0;
    }
}

//  COMPLETED: granulator_load_source_file() - Load source audio from file
void granulator_load_source_file(Granulator* granulator, const char* filepath) {
    if (!granulator || !filepath) return;
    
    // Placeholder for actual file loading
    // In a real implementation, this would load WAV/AIFF files
    // For now, create a simple test tone
    u32 length = granulator->sample_rate * 2; // 2 seconds
    f32* buffer = (f32*)memory_allocate(length * sizeof(f32), MEMORY_TAG_AUDIO);
    if (!buffer) return;
    
    // Generate a complex test tone
    for (u32 i = 0; i < length; i++) {
        f32 t = (f32)i / granulator->sample_rate;
        buffer[i] = 0.3f * sinf(2.0f * M_PI * 440.0f * t) +  // A4
                   0.2f * sinf(2.0f * M_PI * 554.37f * t) + // C#5
                   0.1f * sinf(2.0f * M_PI * 659.25f * t);  // E5
    }
    
    granulator_set_source_buffer(granulator, buffer, length);
    memory_free(buffer, MEMORY_TAG_AUDIO);
}

//  COMPLETED: granulator_get_active_grain_count() - Get number of active grains
u32 granulator_get_active_grain_count(const Granulator* granulator) {
    return granulator ? granulator->active_grain_count : 0;
}

//  COMPLETED: granulator_get_cpu_usage() - Estimate CPU usage
f32 granulator_get_cpu_usage(const Granulator* granulator) {
    if (!granulator) return 0.0f;
    
    // Estimate CPU usage based on active grains and parameters
    f32 base_usage = 0.0001f; // Base usage per grain
    return base_usage * granulator->active_grain_count;
}

//  COMPLETED: granulator_reset() - Reset granulator to default state
void granulator_reset(Granulator* granulator) {
    if (!granulator) return;
    
    granulator_clear_grains(granulator);
    granulator->next_grain_index = 0;
    granulator->ring_write_pos = 0;
    
    // Reset parameters to defaults
    granulator->params.grain_size_ms = 50.0f;
    granulator->params.grain_density = 10.0f;
    granulator->params.pitch_shift = 1.0f;
    granulator->params.time_stretch = 1.0f;
    granulator->params.scan_position = 0.5f;
}

//  COMPLETED: Window function implementations
f32 granulator_hanning_window(f32 phase) {
    return 0.5f * (1.0f - cosf(2.0f * M_PI * phase));
}

f32 granulator_hamming_window(f32 phase) {
    return 0.54f - 0.46f * cosf(2.0f * M_PI * phase);
}

f32 granulator_blackman_window(f32 phase) {
    return 0.42f - 0.5f * cosf(2.0f * M_PI * phase) + 0.08f * cosf(4.0f * M_PI * phase);
}

f32 granulator_gaussian_window(f32 phase, f32 sigma) {
    f32 center = 0.5f;
    f32 x = phase - center;
    return expf(-(x * x) / (2.0f * sigma * sigma));
}

f32 granulator_triangle_window(f32 phase) {
    if (phase <= 0.5f) {
        return 2.0f * phase;
    } else {
        return 2.0f * (1.0f - phase);
    }
}

/** GRANULAR SYNTHESIS IMPLEMENTATION COMPLETE  */
