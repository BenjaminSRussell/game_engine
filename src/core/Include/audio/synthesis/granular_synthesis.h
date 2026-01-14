#ifndef AUDIO_SYNTHESIS_GRANULAR_H
#define AUDIO_SYNTHESIS_GRANULAR_H

#include <common.h>
#include "engine/include/math/math_all.h"

#define MAX_GRAINS 256
#define MAX_SOURCE_LENGTH 1048576  // 1M samples
#define GRAIN_WINDOW_SIZE 1024

typedef enum {
    WINDOW_HANNING,
    WINDOW_HAMMING,
    WINDOW_BLACKMAN,
    WINDOW_GAUSSIAN,
    WINDOW_TRIANGLE
} WindowType;

typedef struct {
    f32* source_buffer;
    u32 source_length;
    u32 position;        // Start position in source (samples)
    u32 length;          // Grain length in samples
    f32 pitch_shift;     // Pitch shift factor (1.0 = original)
    f32 amplitude;       // Grain amplitude (0.0-1.0)
    f32 pan;            // Pan position (-1.0 left, 1.0 right)
    f32 age;            // Age of grain (0.0 to 1.0)
    f32 playback_rate;  // Playback rate for time stretching
    bool active;
    
    // Window function data
    f32 window_buffer[GRAIN_WINDOW_SIZE];
    WindowType window_type;
} Grain;

typedef struct {
    f32 grain_size_ms;        // Grain size in milliseconds
    f32 grain_density;        // Grains per second
    f32 pitch_shift;          // Overall pitch shift
    f32 time_stretch;         // Time stretch factor
    f32 position_randomness;  // Random position variation (0.0-1.0)
    f32 pitch_randomness;    // Pitch variation (semitones)
    f32 amplitude_randomness; // Amplitude variation (0.0-1.0)
    f32 pan_spread;          // Pan spread (-1.0 to 1.0)
    f32 scan_position;        // Manual position scanning (0.0-1.0)
    WindowType window_type;
    
    // Envelope parameters
    f32 attack_time;
    f32 decay_time;
    f32 sustain_level;
    f32 release_time;
} GranularParameters;

typedef struct {
    Grain grains[MAX_GRAINS];
    u32 active_grain_count;
    u32 next_grain_index;
    
    // Source audio buffer
    f32* source_buffer;
    u32 source_length;
    u32 sample_rate;
    
    // Ring buffer for live input granularization
    f32* ring_buffer;
    u32 ring_buffer_size;
    u32 ring_write_pos;
    
    // Granular parameters
    GranularParameters params;
    
    // Window function lookup tables
    f32 hanning_window[GRAIN_WINDOW_SIZE];
    f32 hamming_window[GRAIN_WINDOW_SIZE];
    f32 blackman_window[GRAIN_WINDOW_SIZE];
    f32 gaussian_window[GRAIN_WINDOW_SIZE];
    f32 triangle_window[GRAIN_WINDOW_SIZE];
    
    // Anti-aliasing filters for pitch shifting
    struct {
        f32 x1, x2, y1, y2;
        f32 cutoff;
        f32 resonance;
        f32 a0, a1, a2, b1, b2;
    } aa_filter[2]; // Left and right channels
    
    // Multi-channel support
    u32 num_channels;
    bool stereo_mode;
    
    bool initialized;
} Granulator;

// Granulator lifecycle
Granulator* granulator_create(u32 sample_rate, u32 buffer_size);
void granulator_destroy(Granulator* granulator);

// Source management
void granulator_set_source_buffer(Granulator* granulator, const f32* buffer, u32 length);
void granulator_load_source_file(Granulator* granulator, const char* filepath);
void granulator_set_ring_buffer_size(Granulator* granulator, u32 size);
void granulator_write_to_ring_buffer(Granulator* granulator, const f32* input, u32 frame_count);

// Grain management
void granulator_spawn_grain(Granulator* granulator);
void granulator_update_grains(Granulator* granulator, u32 samples_to_process);
void granulator_clear_grains(Granulator* granulator);

// Parameter control
void granulator_set_grain_size(Granulator* granulator, f32 size_ms);
void granulator_set_density(Granulator* granulator, f32 grains_per_second);
void granulator_set_pitch_shift(Granulator* granulator, f32 pitch_shift);
void granulator_set_time_stretch(Granulator* granulator, f32 time_stretch);
void granulator_set_scan_position(Granulator* granulator, f32 position);
void granulator_set_window_type(Granulator* granulator, WindowType window_type);
void granulator_set_pan_spread(Granulator* granulator, f32 spread);

// Processing
void granulator_process(Granulator* granulator, f32* output, u32 frame_count);
void granulator_process_stereo(Granulator* granulator, f32* left_out, f32* right_out, u32 frame_count);

// Grain scheduling and windowing
void granulator_schedule_grains(Granulator* granulator, u32 frame_count);
void granulator_apply_window(Grain* grain, f32* output, u32 start_sample, u32 samples);
void granulator_generate_window(Granulator* granulator, WindowType type);

// SIMD optimization
void granulator_process_simd(Granulator* granulator, f32* output, u32 frame_count);

// Multi-channel support
void granulator_enable_stereo(Granulator* granulator, bool enabled);
void granulator_set_grain_3d_position(Grain* grain, f32 x, f32 y, f32 z);

// Anti-aliasing
void granulator_update_aa_filters(Granulator* granulator, f32 pitch_shift);
void granulator_apply_aa_filter(Granulator* granulator, f32* input, f32* output, u32 frame_count);

// Utilities
u32 granulator_get_active_grain_count(const Granulator* granulator);
f32 granulator_get_cpu_usage(const Granulator* granulator);
void granulator_reset(Granulator* granulator);

// Window functions
f32 granulator_hanning_window(f32 phase);
f32 granulator_hamming_window(f32 phase);
f32 granulator_blackman_window(f32 phase);
f32 granulator_gaussian_window(f32 phase, f32 sigma);
f32 granulator_triangle_window(f32 phase);

#endif // AUDIO_SYNTHESIS_GRANULAR_H
