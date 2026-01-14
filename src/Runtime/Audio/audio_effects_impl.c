/**
 * =================================================================================================
 *                              AUDIO SYSTEM EFFECTS - COMPLETE IMPLEMENTATION
 *                              Based on patterns from dsp/reverb.c, dsp/delay_line.c,
 *                              dsp/compressor.c, and dsp/equalizer.c
 * =================================================================================================
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define SAMPLE_RATE 48000
#define MAX_DELAY_SAMPLES (SAMPLE_RATE * 2)  // 2 seconds max delay
#define NUM_COMB_FILTERS 8
#define NUM_ALLPASS_FILTERS 4

/* =================================================================================================
 *                                    DSP EFFECTS - REVERB
 * =================================================================================================
 */

// Static state for reverb effect
static struct {
    float *comb_buffers[NUM_COMB_FILTERS];
    uint32_t comb_sizes[NUM_COMB_FILTERS];
    uint32_t comb_positions[NUM_COMB_FILTERS];
    float comb_filter_states[NUM_COMB_FILTERS];
    
    float *allpass_buffers[NUM_ALLPASS_FILTERS];
    uint32_t allpass_sizes[NUM_ALLPASS_FILTERS];
    uint32_t allpass_positions[NUM_ALLPASS_FILTERS];
    
    int initialized;
} s_reverb = {0};

static void reverb_init_if_needed(void) {
    if (s_reverb.initialized) return;
    
    // Prime number delay lengths for comb filters (Freeverb-style)
    const uint32_t comb_delays[NUM_COMB_FILTERS] = {1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116};
    for (int i = 0; i < NUM_COMB_FILTERS; i++) {
        s_reverb.comb_sizes[i] = comb_delays[i];
        s_reverb.comb_buffers[i] = calloc(comb_delays[i], sizeof(float));
        s_reverb.comb_positions[i] = 0;
        s_reverb.comb_filter_states[i] = 0.0f;
    }
    
    // Allpass filter delay lengths
    const uint32_t allpass_delays[NUM_ALLPASS_FILTERS] = {225, 556, 441, 341};
    for (int i = 0; i < NUM_ALLPASS_FILTERS; i++) {
        s_reverb.allpass_sizes[i] = allpass_delays[i];
        s_reverb.allpass_buffers[i] = calloc(allpass_delays[i], sizeof(float));
        s_reverb.allpass_positions[i] = 0;
    }
    
    s_reverb.initialized = 1;
}

// Schroeder reverb with comb and allpass filters
void audio_fx_reverb(float *buffer, uint32_t frames, float room_size, float damping) {
    reverb_init_if_needed();
    
    // Calculate feedback based on room size (0.7 to 0.98)
    float feedback = 0.7f + room_size * 0.28f;
    
    for (uint32_t i = 0; i < frames; i++) {
        float input = buffer[i];
        float comb_sum = 0.0f;
        
        // Process through all comb filters in parallel
        for (int c = 0; c < NUM_COMB_FILTERS; c++) {
            uint32_t read_pos = s_reverb.comb_positions[c];
            float delayed = s_reverb.comb_buffers[c][read_pos];
            
            // One-pole lowpass filter for damping (frequency-dependent decay)
            s_reverb.comb_filter_states[c] = 
                delayed * (1.0f - damping) + s_reverb.comb_filter_states[c] * damping;
            
            // Write to buffer with feedback
            s_reverb.comb_buffers[c][read_pos] = 
                input + s_reverb.comb_filter_states[c] * feedback;
            
            // Advance position
            s_reverb.comb_positions[c] = (read_pos + 1) % s_reverb.comb_sizes[c];
            
            comb_sum += delayed;
        }
        
        // Normalize comb output
        float ap_in = comb_sum / NUM_COMB_FILTERS;
        
        // Chain through allpass filters
        for (int a = 0; a < NUM_ALLPASS_FILTERS; a++) {
            uint32_t read_pos = s_reverb.allpass_positions[a];
            float delayed = s_reverb.allpass_buffers[a][read_pos];
            
            float output = -ap_in + delayed;
            s_reverb.allpass_buffers[a][read_pos] = ap_in + delayed * 0.5f;
            
            s_reverb.allpass_positions[a] = (read_pos + 1) % s_reverb.allpass_sizes[a];
            ap_in = output;
        }
        
        // Mix wet and dry (wet amount based on room_size)
        float wet_amount = room_size * 0.33f;
        buffer[i] = input * (1.0f - wet_amount) + ap_in * wet_amount;
    }
}

/* =================================================================================================
 *                                    DSP EFFECTS - DELAY
 * =================================================================================================
 */

// Static state for delay effect
static struct {
    float *buffer;
    uint32_t buffer_size;
    uint32_t write_pos;
    int initialized;
} s_delay = {0};

static void delay_init_if_needed(void) {
    if (s_delay.initialized) return;
    
    s_delay.buffer_size = MAX_DELAY_SAMPLES;
    s_delay.buffer = calloc(s_delay.buffer_size, sizeof(float));
    s_delay.write_pos = 0;
    s_delay.initialized = 1;
}

// Cubic interpolation for fractional delay
static float delay_interpolate_cubic(float *buffer, uint32_t buffer_size, float read_pos) {
    int pos = (int)read_pos;
    float frac = read_pos - pos;
    
    int p0 = (pos - 1 + buffer_size) % buffer_size;
    int p1 = pos % buffer_size;
    int p2 = (pos + 1) % buffer_size;
    int p3 = (pos + 2) % buffer_size;
    
    float a0 = buffer[p3] - buffer[p2] - buffer[p0] + buffer[p1];
    float a1 = buffer[p0] - buffer[p1] - a0;
    float a2 = buffer[p2] - buffer[p0];
    float a3 = buffer[p1];
    
    return a0 * frac * frac * frac + a1 * frac * frac + a2 * frac + a3;
}

// Delay with feedback
void audio_fx_delay(float *buffer, uint32_t frames, float time_ms, float feedback) {
    delay_init_if_needed();
    
    // Clamp feedback to prevent runaway
    if (feedback > 0.95f) feedback = 0.95f;
    if (feedback < 0.0f) feedback = 0.0f;
    
    // Calculate delay in samples
    float delay_samples = (time_ms / 1000.0f) * SAMPLE_RATE;
    if (delay_samples >= s_delay.buffer_size) {
        delay_samples = s_delay.buffer_size - 1;
    }
    if (delay_samples < 0) delay_samples = 0;
    
    for (uint32_t i = 0; i < frames; i++) {
        float input = buffer[i];
        
        // Calculate fractional read position
        float read_pos = s_delay.write_pos - delay_samples;
        if (read_pos < 0) read_pos += s_delay.buffer_size;
        
        // Read from delay buffer with cubic interpolation
        float delayed = delay_interpolate_cubic(s_delay.buffer, s_delay.buffer_size, read_pos);
        
        // Write to buffer with feedback
        s_delay.buffer[s_delay.write_pos] = input + delayed * feedback;
        
        // Advance write position
        s_delay.write_pos = (s_delay.write_pos + 1) % s_delay.buffer_size;
        
        // Mix wet and dry (50/50)
        buffer[i] = input * 0.5f + delayed * 0.5f;
    }
}

/* =================================================================================================
 *                                    DSP EFFECTS - LOWPASS FILTER
 * =================================================================================================
 */

// Static state for lowpass filter (biquad)
static struct {
    float x1, x2;  // Input history
    float y1, y2;  // Output history
    float a0, a1, a2, b1, b2;  // Coefficients
    float last_cutoff;
    int initialized;
} s_lowpass = {0};

static void lowpass_compute_coefficients(float cutoff_hz) {
    if (!s_lowpass.initialized || s_lowpass.last_cutoff != cutoff_hz) {
        // Butterworth lowpass biquad coefficients
        float omega = 2.0f * 3.14159265f * cutoff_hz / SAMPLE_RATE;
        float sin_omega = sinf(omega);
        float cos_omega = cosf(omega);
        float q = 0.707f;  // Butterworth Q
        float alpha = sin_omega / (2.0f * q);
        
        float a0 = 1.0f + alpha;
        float a0_inv = 1.0f / a0;
        
        // Lowpass coefficients
        s_lowpass.a0 = ((1.0f - cos_omega) / 2.0f) * a0_inv;
        s_lowpass.a1 = (1.0f - cos_omega) * a0_inv;
        s_lowpass.a2 = ((1.0f - cos_omega) / 2.0f) * a0_inv;
        s_lowpass.b1 = (-2.0f * cos_omega) * a0_inv;
        s_lowpass.b2 = (1.0f - alpha) * a0_inv;
        
        s_lowpass.last_cutoff = cutoff_hz;
        s_lowpass.initialized = 1;
    }
}

// Biquad lowpass filter
void audio_fx_lowpass(float *buffer, uint32_t frames, float cutoff_hz) {
    // Clamp cutoff frequency
    if (cutoff_hz < 20.0f) cutoff_hz = 20.0f;
    if (cutoff_hz > 20000.0f) cutoff_hz = 20000.0f;
    
    lowpass_compute_coefficients(cutoff_hz);
    
    for (uint32_t i = 0; i < frames; i++) {
        float input = buffer[i];
        
        // Biquad difference equation:
        // y[n] = a0*x[n] + a1*x[n-1] + a2*x[n-2] - b1*y[n-1] - b2*y[n-2]
        float output = s_lowpass.a0 * input 
                     + s_lowpass.a1 * s_lowpass.x1 
                     + s_lowpass.a2 * s_lowpass.x2 
                     - s_lowpass.b1 * s_lowpass.y1 
                     - s_lowpass.b2 * s_lowpass.y2;
        
        // Update history
        s_lowpass.x2 = s_lowpass.x1;
        s_lowpass.x1 = input;
        s_lowpass.y2 = s_lowpass.y1;
        s_lowpass.y1 = output;
        
        buffer[i] = output;
    }
}

/* =================================================================================================
 *                                    DSP EFFECTS - COMPRESSOR
 * =================================================================================================
 */

// Static state for compressor
static struct {
    float envelope;
    float rms_buffer[256];
    int rms_index;
    int initialized;
} s_compressor = {0};

static float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

static float linear_to_db(float linear) {
    if (linear < 0.0001f) linear = 0.0001f;  // Avoid log(0)
    return 20.0f * log10f(linear);
}

// Dynamic range compressor
void audio_fx_compressor(float *buffer, uint32_t frames, float threshold, float ratio) {
    if (!s_compressor.initialized) {
        memset(s_compressor.rms_buffer, 0, sizeof(s_compressor.rms_buffer));
        s_compressor.envelope = 1.0f;
        s_compressor.rms_index = 0;
        s_compressor.initialized = 1;
    }
    
    // Clamp parameters
    if (ratio < 1.0f) ratio = 1.0f;
    if (ratio > 20.0f) ratio = 20.0f;
    
    // Attack and release coefficients (10ms attack, 100ms release)
    float attack_coef = expf(-1.0f / (0.01f * SAMPLE_RATE));
    float release_coef = expf(-1.0f / (0.1f * SAMPLE_RATE));
    
    for (uint32_t i = 0; i < frames; i++) {
        float input = buffer[i];
        
        // RMS level detection
        s_compressor.rms_buffer[s_compressor.rms_index] = input * input;
        s_compressor.rms_index = (s_compressor.rms_index + 1) % 256;
        
        float rms_sum = 0.0f;
        for (int j = 0; j < 256; j++) {
            rms_sum += s_compressor.rms_buffer[j];
        }
        float rms = sqrtf(rms_sum / 256.0f);
        
        // Convert to dB
        float input_db = linear_to_db(rms);
        
        // Calculate gain reduction
        float target_gain = 1.0f;
        if (input_db > threshold) {
            // Apply compression above threshold
            float overshoot = input_db - threshold;
            float compressed_overshoot = overshoot / ratio;
            float output_db = threshold + compressed_overshoot;
            target_gain = db_to_linear(output_db - input_db);
        }
        
        // Apply attack/release envelope
        if (target_gain < s_compressor.envelope) {
            // Attack (fast)
            s_compressor.envelope = attack_coef * s_compressor.envelope 
                                  + (1.0f - attack_coef) * target_gain;
        } else {
            // Release (slow)
            s_compressor.envelope = release_coef * s_compressor.envelope 
                                  + (1.0f - release_coef) * target_gain;
        }
        
        // Apply gain
        buffer[i] = input * s_compressor.envelope;
    }
}

/* =================================================================================================
 *                                    SYNTHESIS
 * =================================================================================================
 */

// Sine wave synthesis
void audio_synth_sine(float *buffer, uint32_t frames, float frequency, float sample_rate) {
    static float phase = 0;
    float phase_inc = 2.0f * 3.14159265f * frequency / sample_rate;

    for (uint32_t i = 0; i < frames; i++) {
        buffer[i] = sinf(phase);
        phase += phase_inc;
        if (phase > 2.0f * 3.14159265f) {
            phase -= 2.0f * 3.14159265f;
        }
    }
}

// White noise synthesis
void audio_synth_noise(float *buffer, uint32_t frames) {
    for (uint32_t i = 0; i < frames; i++) {
        buffer[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
}

/* =================================================================================================
 *                                    CLEANUP
 * =================================================================================================
 */

// Cleanup function (call on engine shutdown)
void audio_fx_cleanup(void) {
    // Cleanup reverb
    if (s_reverb.initialized) {
        for (int i = 0; i < NUM_COMB_FILTERS; i++) {
            free(s_reverb.comb_buffers[i]);
            s_reverb.comb_buffers[i] = NULL;
        }
        for (int i = 0; i < NUM_ALLPASS_FILTERS; i++) {
            free(s_reverb.allpass_buffers[i]);
            s_reverb.allpass_buffers[i] = NULL;
        }
        s_reverb.initialized = 0;
    }
    
    // Cleanup delay
    if (s_delay.initialized) {
        free(s_delay.buffer);
        s_delay.buffer = NULL;
        s_delay.initialized = 0;
    }
    
    // Reset lowpass state
    memset(&s_lowpass, 0, sizeof(s_lowpass));
    
    // Reset compressor state
    memset(&s_compressor, 0, sizeof(s_compressor));
}
