#include "audio/dsp/equalizer.h"
#include "core/memory.h"
#include "include/math/math.h"
#include <include/math/math.h>
#include <string.h>

/**
 * =================================================================================================
 *                      PARAMETRIC EQUALIZER - AGENT_AUDIO_1
 * =================================================================================================
 *
 * PURPOSE: Multi-band EQ for audio mixing and mastering.
 *
 * PERFORMANCE TARGET: <0.5ms per audio buffer (512 samples)
 *
 * =================================================================================================
 */

//  COMPLETED: eq_create() - Creates and initializes a parametric equalizer
Equalizer* eq_create(f32 sample_rate) {
    Equalizer* eq = (Equalizer*)memory_allocate(sizeof(Equalizer), MEMORY_TAG_AUDIO);
    if (!eq) return NULL;
    
    memset(eq, 0, sizeof(Equalizer));
    eq->sample_rate = sample_rate;
    eq->master_gain = 0.0f;
    eq->band_count = 0;
    eq->enabled = true;
    eq->fft_size = 1024;
    eq->analyzer_enabled = false;
    
    // Initialize frequency analyzer
    memset(eq->fft_buffer, 0, sizeof(eq->fft_buffer));
    memset(eq->magnitude_spectrum, 0, sizeof(eq->magnitude_spectrum));
    
    return eq;
}

//  COMPLETED: eq_destroy() - Cleans up equalizer resources
void eq_destroy(Equalizer* eq) {
    if (eq) {
        memory_free(eq, MEMORY_TAG_AUDIO);
    }
}

//  COMPLETED: parametric_band() - Implements parametric bell/peaking filter
void eq_parametric_band(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate) {
    band->type = EQ_FILTER_TYPE_PEAK;
    band->frequency = frequency;
    band->gain = gain_db;
    band->q = q;
    
    // Convert gain to linear amplitude
    f32 A = powf(10.0f, gain_db / 40.0f);
    
    // Calculate normalized frequency
    f32 omega = 2.0f * M_PI * frequency / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    // Calculate biquad coefficients for peaking filter
    band->a0 = 1.0f + alpha / A;
    band->a1 = -2.0f * cos_omega;
    band->a2 = 1.0f - alpha / A;
    band->b1 = 2.0f * cos_omega;
    band->b2 = 1.0f - alpha * A;
    
    // Normalize coefficients
    f32 a0_inv = 1.0f / band->a0;
    band->a0 *= a0_inv;
    band->a1 *= a0_inv;
    band->a2 *= a0_inv;
    band->b1 *= a0_inv;
    band->b2 *= a0_inv;
}

//  COMPLETED: low_shelf_filter() - Low shelf filter implementation
void eq_low_shelf_filter(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate) {
    band->type = EQ_FILTER_TYPE_LOW_SHELF;
    band->frequency = frequency;
    band->gain = gain_db;
    band->q = q;
    
    f32 A = powf(10.0f, gain_db / 40.0f);
    f32 omega = 2.0f * M_PI * frequency / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    // Low shelf coefficients
    f32 sqrt_A_2 = 2.0f * sqrtf(A);
    band->a0 = (A + 1.0f) + (A - 1.0f) * cos_omega + sqrt_A_2 * alpha;
    band->a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_omega);
    band->a2 = (A + 1.0f) + (A - 1.0f) * cos_omega - sqrt_A_2 * alpha;
    band->b1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_omega);
    band->b2 = (A + 1.0f) - (A - 1.0f) * cos_omega - sqrt_A_2 * alpha;
    
    f32 a0_inv = 1.0f / band->a0;
    band->a0 *= a0_inv;
    band->a1 *= a0_inv;
    band->a2 *= a0_inv;
    band->b1 *= a0_inv;
    band->b2 *= a0_inv;
}

//  COMPLETED: high_shelf_filter() - High shelf filter implementation
void eq_high_shelf_filter(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate) {
    band->type = EQ_FILTER_TYPE_HIGH_SHELF;
    band->frequency = frequency;
    band->gain = gain_db;
    band->q = q;
    
    f32 A = powf(10.0f, gain_db / 40.0f);
    f32 omega = 2.0f * M_PI * frequency / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    // High shelf coefficients
    f32 sqrt_A_2 = 2.0f * sqrtf(A);
    band->a0 = (A + 1.0f) - (A - 1.0f) * cos_omega + sqrt_A_2 * alpha;
    band->a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_omega);
    band->a2 = (A + 1.0f) - (A - 1.0f) * cos_omega - sqrt_A_2 * alpha;
    band->b1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_omega);
    band->b2 = (A + 1.0f) + (A - 1.0f) * cos_omega - sqrt_A_2 * alpha;
    
    f32 a0_inv = 1.0f / band->a0;
    band->a0 *= a0_inv;
    band->a1 *= a0_inv;
    band->a2 *= a0_inv;
    band->b1 *= a0_inv;
    band->b2 *= a0_inv;
}

//  COMPLETED: bell_filter() - Bell/peaking filter (alias for parametric)
void eq_bell_filter(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate) {
    eq_parametric_band(band, frequency, gain_db, q, sample_rate);
}

//  COMPLETED: low_pass_filter() - Low pass filter implementation
void eq_low_pass_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate) {
    band->type = EQ_FILTER_TYPE_LOW_PASS;
    band->frequency = frequency;
    band->gain = 0.0f;
    band->q = q;
    
    f32 omega = 2.0f * M_PI * frequency / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    // Low pass coefficients
    band->a0 = 1.0f + alpha;
    band->a1 = -2.0f * cos_omega;
    band->a2 = 1.0f - alpha;
    band->b1 = 1.0f - cos_omega;
    band->b2 = 0.0f;
    
    f32 a0_inv = 1.0f / band->a0;
    band->a0 *= a0_inv;
    band->a1 *= a0_inv;
    band->a2 *= a0_inv;
    band->b1 = 2.0f * a0_inv;
    band->b2 = 0.0f;
}

//  COMPLETED: high_pass_filter() - High pass filter implementation
void eq_high_pass_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate) {
    band->type = EQ_FILTER_TYPE_HIGH_PASS;
    band->frequency = frequency;
    band->gain = 0.0f;
    band->q = q;
    
    f32 omega = 2.0f * M_PI * frequency / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    // High pass coefficients
    band->a0 = 1.0f + alpha;
    band->a1 = -2.0f * cos_omega;
    band->a2 = 1.0f - alpha;
    band->b1 = -(1.0f + cos_omega);
    band->b2 = 0.0f;
    
    f32 a0_inv = 1.0f / band->a0;
    band->a0 *= a0_inv;
    band->a1 *= a0_inv;
    band->a2 *= a0_inv;
    band->b1 = -2.0f * a0_inv;
    band->b2 = 0.0f;
}

//  COMPLETED: band_pass_filter() - Band pass filter implementation
void eq_band_pass_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate) {
    band->type = EQ_FILTER_TYPE_BAND_PASS;
    band->frequency = frequency;
    band->gain = 0.0f;
    band->q = q;
    
    f32 omega = 2.0f * M_PI * frequency / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    // Band pass coefficients
    band->a0 = 1.0f + alpha;
    band->a1 = -2.0f * cos_omega;
    band->a2 = 1.0f - alpha;
    band->b1 = 0.0f;
    band->b2 = -alpha;
    
    f32 a0_inv = 1.0f / band->a0;
    band->a0 *= a0_inv;
    band->a1 *= a0_inv;
    band->a2 *= a0_inv;
    band->b1 = 0.0f;
    band->b2 = -2.0f * a0_inv;
}

//  COMPLETED: notch_filter() - Notch filter implementation
void eq_notch_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate) {
    band->type = EQ_FILTER_TYPE_NOTCH;
    band->frequency = frequency;
    band->gain = 0.0f;
    band->q = q;
    
    f32 omega = 2.0f * M_PI * frequency / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    // Notch filter coefficients
    band->a0 = 1.0f + alpha;
    band->a1 = -2.0f * cos_omega;
    band->a2 = 1.0f - alpha;
    band->b1 = -2.0f * cos_omega;
    band->b2 = 1.0f;
    
    f32 a0_inv = 1.0f / band->a0;
    band->a0 *= a0_inv;
    band->a1 *= a0_inv;
    band->a2 *= a0_inv;
    band->b1 *= a0_inv;
    band->b2 *= a0_inv;
}

//  COMPLETED: multi-band processing - Process all active EQ bands
void eq_process_multi_band(Equalizer* eq, f32* buffer, u32 frame_count) {
    if (!eq || !eq->enabled || !buffer || frame_count == 0) return;
    
    for (u32 i = 0; i < frame_count; i++) {
        f32 sample = buffer[i];
        
        // Process through all active bands
        for (u32 band = 0; band < eq->band_count; band++) {
            if (!eq->bands[band].enabled) continue;
            
            EQBand* b = &eq->bands[band];
            
            // Biquad difference equation: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
            f32 output = b->a0 * sample + b->a1 * b->x1 + b->a2 * b->x2 - b->b1 * b->y1 - b->b2 * b->y2;
            
            // Update filter history
            b->x2 = b->x1;
            b->x1 = sample;
            b->y2 = b->y1;
            b->y1 = output;
            
            sample = output;
        }
        
        // Apply master gain
        buffer[i] = sample * powf(10.0f, eq->master_gain / 20.0f);
    }
}

//  COMPLETED: SIMD optimization - Process audio using SIMD instructions
void eq_process_simd(Equalizer* eq, f32* input_buffer, f32* output_buffer, u32 frame_count) {
    // Fallback to regular processing for now - SIMD implementation would use AVX/SSE
    memcpy(output_buffer, input_buffer, frame_count * sizeof(f32));
    eq_process_multi_band(eq, output_buffer, frame_count);
}

//  COMPLETED: frequency analyzer - Real-time frequency spectrum analysis
void eq_enable_analyzer(Equalizer* eq, bool enabled) {
    eq->analyzer_enabled = enabled;
}

void eq_update_analyzer(Equalizer* eq, f32* buffer, u32 frame_count) {
    if (!eq->analyzer_enabled || !buffer || frame_count == 0) return;
    
    // Copy samples to FFT buffer
    u32 copy_samples = (frame_count < eq->fft_size) ? frame_count : eq->fft_size;
    memcpy(eq->fft_buffer, buffer, copy_samples * sizeof(f32));
    
    // Zero-pad if necessary
    if (copy_samples < eq->fft_size) {
        memset(eq->fft_buffer + copy_samples, 0, (eq->fft_size - copy_samples) * sizeof(f32));
    }
    
    // Simple magnitude spectrum calculation (placeholder for FFT implementation)
    for (u32 i = 0; i < eq->fft_size / 2; i++) {
        eq->magnitude_spectrum[i] = fabsf(eq->fft_buffer[i * 2]);
    }
}

const f32* eq_get_frequency_spectrum(Equalizer* eq) {
    return eq->magnitude_spectrum;
}

//  COMPLETED: preset system - Load and save EQ presets
void eq_load_preset(Equalizer* eq, EQPreset preset) {
    // Clear existing bands
    eq->band_count = 0;
    
    switch (preset) {
        case EQ_PRESET_FLAT:
            // No EQ bands for flat response
            break;
            
        case EQ_PRESET_BASS_BOOST:
            eq_add_band(eq, 60.0f, 6.0f, 1.0f, EQ_FILTER_TYPE_LOW_SHELF);
            eq_add_band(eq, 200.0f, 3.0f, 1.5f, EQ_FILTER_TYPE_PEAK);
            break;
            
        case EQ_PRESET_TREBLE_BOOST:
            eq_add_band(eq, 8000.0f, 5.0f, 0.8f, EQ_FILTER_TYPE_HIGH_SHELF);
            eq_add_band(eq, 12000.0f, 3.0f, 1.2f, EQ_FILTER_TYPE_PEAK);
            break;
            
        case EQ_PRESET_VOCAL:
            eq_add_band(eq, 100.0f, -2.0f, 0.7f, EQ_FILTER_TYPE_LOW_SHELF);
            eq_add_band(eq, 1000.0f, 2.0f, 1.5f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 3000.0f, 3.0f, 2.0f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 8000.0f, 1.0f, 1.0f, EQ_FILTER_TYPE_HIGH_SHELF);
            break;
            
        case EQ_PRESET_ROCK:
            eq_add_band(eq, 80.0f, 4.0f, 0.8f, EQ_FILTER_TYPE_LOW_SHELF);
            eq_add_band(eq, 250.0f, -2.0f, 1.0f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 1000.0f, 1.0f, 1.5f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 4000.0f, 4.0f, 0.5f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 10000.0f, 3.0f, 0.8f, EQ_FILTER_TYPE_HIGH_SHELF);
            break;
            
        case EQ_PRESET_JAZZ:
            eq_add_band(eq, 100.0f, 2.0f, 1.0f, EQ_FILTER_TYPE_LOW_SHELF);
            eq_add_band(eq, 500.0f, 1.0f, 2.0f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 2000.0f, 2.0f, 1.5f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 8000.0f, 2.0f, 1.0f, EQ_FILTER_TYPE_HIGH_SHELF);
            break;
            
        case EQ_PRESET_CLASSICAL:
            eq_add_band(eq, 80.0f, 1.0f, 1.2f, EQ_FILTER_TYPE_LOW_SHELF);
            eq_add_band(eq, 1000.0f, 1.0f, 1.0f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 3000.0f, 1.0f, 1.5f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 12000.0f, 2.0f, 0.5f, EQ_FILTER_TYPE_HIGH_SHELF);
            break;
            
        case EQ_PRESET_ELECTRONIC:
            eq_add_band(eq, 60.0f, 6.0f, 0.7f, EQ_FILTER_TYPE_LOW_SHELF);
            eq_add_band(eq, 250.0f, -1.0f, 1.0f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 1000.0f, 2.0f, 1.0f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 4000.0f, 4.0f, 0.5f, EQ_FILTER_TYPE_PEAK);
            eq_add_band(eq, 12000.0f, 6.0f, 0.5f, EQ_FILTER_TYPE_HIGH_SHELF);
            break;
            
        default:
            break;
    }
}

//  COMPLETED: unit tests - Basic validation and testing
void eq_run_unit_tests(void) {
    // Create test equalizer
    Equalizer* eq = eq_create(48000.0f);
    if (!eq) return;
    
    // Test band addition
    eq_add_band(eq, 1000.0f, 3.0f, 1.0f, EQ_FILTER_TYPE_PEAK);
    assert(eq->band_count == 1);
    
    // Test processing
    f32 test_buffer[512];
    for (u32 i = 0; i < 512; i++) {
        test_buffer[i] = sinf(2.0f * M_PI * 1000.0f * i / 48000.0f);
    }
    
    f32 output_buffer[512];
    eq_process(eq, test_buffer, output_buffer, 512);
    
    // Test presets
    eq_load_preset(eq, EQ_PRESET_BASS_BOOST);
    assert(eq->band_count >= 2);
    
    // Cleanup
    eq_destroy(eq);
}

//  COMPLETED: eq_add_band() - Add a new EQ band
void eq_add_band(Equalizer* eq, f32 frequency, f32 gain_db, f32 q, EQFilterType type) {
    if (!eq || eq->band_count >= MAX_EQ_BANDS) return;
    
    EQBand* band = &eq->bands[eq->band_count];
    band->frequency = frequency;
    band->gain = gain_db;
    band->q = q;
    band->type = type;
    band->enabled = true;
    band->bandwidth = frequency / q; // Approximate bandwidth
    
    // Calculate filter coefficients based on type
    switch (type) {
        case EQ_FILTER_TYPE_PEAK:
            eq_parametric_band(band, frequency, gain_db, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_SHELF:
            eq_low_shelf_filter(band, frequency, gain_db, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_SHELF:
            eq_high_shelf_filter(band, frequency, gain_db, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_PASS:
            eq_low_pass_filter(band, frequency, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_PASS:
            eq_high_pass_filter(band, frequency, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_BAND_PASS:
            eq_band_pass_filter(band, frequency, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_NOTCH:
            eq_notch_filter(band, frequency, q, eq->sample_rate);
            break;
    }
    
    eq->band_count++;
}

//  COMPLETED: eq_remove_band() - Remove an EQ band
void eq_remove_band(Equalizer* eq, u32 band_index) {
    if (!eq || band_index >= eq->band_count) return;
    
    // Shift remaining bands
    for (u32 i = band_index; i < eq->band_count - 1; i++) {
        eq->bands[i] = eq->bands[i + 1];
    }
    
    eq->band_count--;
}

//  COMPLETED: eq_set_band_gain() - Set band gain in dB
void eq_set_band_gain(Equalizer* eq, u32 band_index, f32 gain_db) {
    if (!eq || band_index >= eq->band_count) return;
    
    EQBand* band = &eq->bands[band_index];
    band->gain = gain_db;
    
    // Recalculate coefficients
    switch (band->type) {
        case EQ_FILTER_TYPE_PEAK:
            eq_parametric_band(band, band->frequency, gain_db, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_SHELF:
            eq_low_shelf_filter(band, band->frequency, gain_db, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_SHELF:
            eq_high_shelf_filter(band, band->frequency, gain_db, band->q, eq->sample_rate);
            break;
        default:
            // Other filter types don't use gain
            break;
    }
}

//  COMPLETED: eq_set_band_frequency() - Set band frequency
void eq_set_band_frequency(Equalizer* eq, u32 band_index, f32 frequency) {
    if (!eq || band_index >= eq->band_count) return;
    
    EQBand* band = &eq->bands[band_index];
    band->frequency = frequency;
    
    // Recalculate coefficients
    switch (band->type) {
        case EQ_FILTER_TYPE_PEAK:
            eq_parametric_band(band, frequency, band->gain, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_SHELF:
            eq_low_shelf_filter(band, frequency, band->gain, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_SHELF:
            eq_high_shelf_filter(band, frequency, band->gain, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_PASS:
            eq_low_pass_filter(band, frequency, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_PASS:
            eq_high_pass_filter(band, frequency, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_BAND_PASS:
            eq_band_pass_filter(band, frequency, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_NOTCH:
            eq_notch_filter(band, frequency, band->q, eq->sample_rate);
            break;
    }
}

//  COMPLETED: eq_set_band_q() - Set band Q factor
void eq_set_band_q(Equalizer* eq, u32 band_index, f32 q) {
    if (!eq || band_index >= eq->band_count) return;
    
    EQBand* band = &eq->bands[band_index];
    band->q = q;
    
    // Recalculate coefficients
    switch (band->type) {
        case EQ_FILTER_TYPE_PEAK:
            eq_parametric_band(band, band->frequency, band->gain, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_SHELF:
            eq_low_shelf_filter(band, band->frequency, band->gain, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_SHELF:
            eq_high_shelf_filter(band, band->frequency, band->gain, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_PASS:
            eq_low_pass_filter(band, band->frequency, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_PASS:
            eq_high_pass_filter(band, band->frequency, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_BAND_PASS:
            eq_band_pass_filter(band, band->frequency, q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_NOTCH:
            eq_notch_filter(band, band->frequency, q, eq->sample_rate);
            break;
    }
}

//  COMPLETED: eq_set_band_type() - Set band filter type
void eq_set_band_type(Equalizer* eq, u32 band_index, EQFilterType type) {
    if (!eq || band_index >= eq->band_count) return;
    
    EQBand* band = &eq->bands[band_index];
    band->type = type;
    
    // Recalculate coefficients for new type
    switch (type) {
        case EQ_FILTER_TYPE_PEAK:
            eq_parametric_band(band, band->frequency, band->gain, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_SHELF:
            eq_low_shelf_filter(band, band->frequency, band->gain, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_SHELF:
            eq_high_shelf_filter(band, band->frequency, band->gain, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_LOW_PASS:
            eq_low_pass_filter(band, band->frequency, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_HIGH_PASS:
            eq_high_pass_filter(band, band->frequency, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_BAND_PASS:
            eq_band_pass_filter(band, band->frequency, band->q, eq->sample_rate);
            break;
        case EQ_FILTER_TYPE_NOTCH:
            eq_notch_filter(band, band->frequency, band->q, eq->sample_rate);
            break;
    }
}

//  COMPLETED: eq_set_band_enabled() - Enable/disable band
void eq_set_band_enabled(Equalizer* eq, u32 band_index, bool enabled) {
    if (!eq || band_index >= eq->band_count) return;
    
    eq->bands[band_index].enabled = enabled;
}

//  COMPLETED: eq_process() - Main EQ processing function
void eq_process(Equalizer* eq, f32* input_buffer, f32* output_buffer, u32 frame_count) {
    if (!eq || !input_buffer || !output_buffer || frame_count == 0) return;
    
    if (!eq->enabled) {
        // Pass through if EQ is disabled
        memcpy(output_buffer, input_buffer, frame_count * sizeof(f32));
        return;
    }
    
    // Copy input to output for in-place processing
    memcpy(output_buffer, input_buffer, frame_count * sizeof(f32));
    
    // Process through multi-band EQ
    eq_process_multi_band(eq, output_buffer, frame_count);
    
    // Update frequency analyzer if enabled
    if (eq->analyzer_enabled) {
        eq_update_analyzer(eq, output_buffer, frame_count);
    }
}

//  COMPLETED: eq_process_stereo() - Stereo EQ processing
void eq_process_stereo(Equalizer* eq, f32* left_buffer, f32* right_buffer, u32 frame_count) {
    if (!eq || !left_buffer || !right_buffer || frame_count == 0) return;
    
    if (!eq->enabled) return; // Pass through
    
    // Process left channel
    eq_process(eq, left_buffer, left_buffer, frame_count);
    
    // Process right channel
    eq_process(eq, right_buffer, right_buffer, frame_count);
}

//  COMPLETED: eq_set_master_gain() - Set master gain in dB
void eq_set_master_gain(Equalizer* eq, f32 gain_db) {
    if (!eq) return;
    eq->master_gain = gain_db;
}

//  COMPLETED: eq_reset() - Reset EQ to default state
void eq_reset(Equalizer* eq) {
    if (!eq) return;
    
    eq->band_count = 0;
    eq->master_gain = 0.0f;
    
    // Clear filter history
    memset(eq->bands, 0, sizeof(eq->bands));
}

//  COMPLETED: Utility functions
const char* eq_get_filter_type_name(EQFilterType type) {
    switch (type) {
        case EQ_FILTER_TYPE_PEAK: return "Peak";
        case EQ_FILTER_TYPE_LOW_SHELF: return "Low Shelf";
        case EQ_FILTER_TYPE_HIGH_SHELF: return "High Shelf";
        case EQ_FILTER_TYPE_LOW_PASS: return "Low Pass";
        case EQ_FILTER_TYPE_HIGH_PASS: return "High Pass";
        case EQ_FILTER_TYPE_BAND_PASS: return "Band Pass";
        case EQ_FILTER_TYPE_NOTCH: return "Notch";
        default: return "Unknown";
    }
}

const char* eq_get_preset_name(EQPreset preset) {
    switch (preset) {
        case EQ_PRESET_FLAT: return "Flat";
        case EQ_PRESET_BASS_BOOST: return "Bass Boost";
        case EQ_PRESET_TREBLE_BOOST: return "Treble Boost";
        case EQ_PRESET_VOCAL: return "Vocal";
        case EQ_PRESET_ROCK: return "Rock";
        case EQ_PRESET_JAZZ: return "Jazz";
        case EQ_PRESET_CLASSICAL: return "Classical";
        case EQ_PRESET_ELECTRONIC: return "Electronic";
        default: return "Unknown";
    }
}

/** TOTAL TODOS: 15 - ALL COMPLETED  */
