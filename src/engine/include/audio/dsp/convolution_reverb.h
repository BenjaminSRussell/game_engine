#ifndef AUDIO_DSP_CONVOLUTION_REVERB_H
#define AUDIO_DSP_CONVOLUTION_REVERB_H

#include "include/common.h"
#include <fftw3.h>

#define MAX_IR_LENGTH 1048576  // 1M samples (~21 seconds at 48kHz)
#define CONVOLUTION_PARTITION_SIZE 512
#define MAX_CONVOLUTION_PARTITIONS 2048

typedef struct {
    f32* samples;
    u32 length;
    u32 sample_rate;
    char name[64];
    bool loaded;
} ImpulseResponse;

typedef struct {
    f32* time_domain;
    f32* freq_domain;
    fftwf_plan fft_plan;
    fftwf_plan ifft_plan;
    u32 size;
} ConvolutionPartition;

typedef struct {
    ConvolutionPartition* partitions;
    u32 partition_count;
    u32 partition_size;
    u32 ir_length;
    
    // FFT buffers
    f32* fft_buffer;
    f32* ifft_buffer;
    f32* overlap_buffer;
    
    // Input history for partitioned convolution
    f32* input_history;
    u32 history_write_pos;
    
    // Processing state
    bool zero_latency_mode;
    f32 wet_level;
    f32 dry_level;
    f32 pre_delay_samples;
    
    // EQ for IR shaping
    struct {
        f32 low_gain;
        f32 mid_gain;
        f32 high_gain;
        f32 low_freq;
        f32 high_freq;
    } ir_eq;
    
    bool initialized;
} ConvolutionReverb;

// Convolution reverb lifecycle
ConvolutionReverb* convolution_reverb_create(u32 sample_rate, u32 buffer_size);
void convolution_reverb_destroy(ConvolutionReverb* reverb);

// Impulse response management
bool convolution_reverb_load_ir(ConvolutionReverb* reverb, const char* filepath);
bool convolution_reverb_load_ir_from_memory(ConvolutionReverb* reverb, const f32* samples, u32 length, u32 sample_rate);
void convolution_reverb_set_ir(ConvolutionReverb* reverb, const ImpulseResponse* ir);
void convolution_reverb_clear_ir(ConvolutionReverb* reverb);

// Processing
void convolution_reverb_process(ConvolutionReverb* reverb, f32* input, f32* output, u32 frame_count);
void convolution_reverb_process_stereo(ConvolutionReverb* reverb, f32* left_in, f32* right_in, 
                                      f32* left_out, f32* right_out, u32 frame_count);

// FFT-based convolution
void convolution_reverb_fft_process(ConvolutionReverb* reverb, f32* input, f32* output, u32 frame_count);

// Partitioned convolution for long IRs
void convolution_reverb_partitioned_process(ConvolutionReverb* reverb, f32* input, f32* output, u32 frame_count);
void convolution_reverb_create_partitions(ConvolutionReverb* reverb, const f32* ir, u32 ir_length);

// Zero-latency mode
void convolution_reverb_set_zero_latency_mode(ConvolutionReverb* reverb, bool enabled);

// Wet/dry mix and pre-delay
void convolution_reverb_set_wet_level(ConvolutionReverb* reverb, f32 wet_level);
void convolution_reverb_set_dry_level(ConvolutionReverb* reverb, f32 dry_level);
void convolution_reverb_set_pre_delay(ConvolutionReverb* reverb, f32 pre_delay_ms);

// IR shaping EQ
void convolution_reverb_set_ir_eq(ConvolutionReverb* reverb, f32 low_gain, f32 mid_gain, f32 high_gain);
void convolution_reverb_set_ir_eq_freqs(ConvolutionReverb* reverb, f32 low_freq, f32 high_freq);

// Dynamic IR switching
void convolution_reverb_switch_ir(ConvolutionReverb* reverb, const ImpulseResponse* new_ir, u32 fade_samples);

// SIMD optimization
void convolution_reverb_process_simd(ConvolutionReverb* reverb, f32* input, f32* output, u32 frame_count);

// GPU acceleration (placeholder for future implementation)
void convolution_reverb_enable_gpu_acceleration(ConvolutionReverb* reverb, bool enabled);

// Preset library
typedef struct {
    ImpulseResponse irs[16];
    u32 ir_count;
    char library_name[64];
} ReverbPresetLibrary;

ReverbPresetLibrary* convolution_reverb_load_preset_library(const char* library_path);
void convolution_reverb_destroy_preset_library(ReverbPresetLibrary* library);
void convolution_reverb_load_preset(ConvolutionReverb* reverb, ReverbPresetLibrary* library, u32 preset_index);

// Utilities
u32 convolution_reverb_get_latency_samples(const ConvolutionReverb* reverb);
f32 convolution_reverb_get_cpu_usage(const ConvolutionReverb* reverb);
void convolution_reverb_reset(ConvolutionReverb* reverb);

#endif // AUDIO_DSP_CONVOLUTION_REVERB_H
