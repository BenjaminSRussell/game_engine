/**
 * =================================================================================================
 *                          PITCH SHIFTER
 * =================================================================================================
 */

#include "audio/dsp/pitch_shifter.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math_all.h>

void pitch_shifter_init(PitchShifter *shifter, int sample_rate) {
    shifter->mode = PITCH_SHIFT_GRANULAR;
    shifter->pitch_ratio = 1.0f;
    
    // Granular synthesis setup
    shifter->grain_size = 2048;
    shifter->grain_overlap = 4;
    shifter->grain_buffer = calloc(shifter->grain_size * shifter->grain_overlap, sizeof(float));
    shifter->grain_pos = 0;
    
    // Phase vocoder setup (FFT-based)
    shifter->fft_size = 4096;
    shifter->fft_buffer = calloc(shifter->fft_size * 2, sizeof(float));
    shifter->phase_acc = calloc(shifter->fft_size / 2, sizeof(float));
    
    shifter->preserve_formants = false;
    shifter->latency_samples = shifter->grain_size / 2;
}

void pitch_shifter_destroy(PitchShifter *shifter) {
    free(shifter->grain_buffer);
    free(shifter->fft_buffer);
    free(shifter->phase_acc);
}

void pitch_shifter_set_pitch(PitchShifter *shifter, float semitones) {
    // Convert semitones to ratio: 2^(semitones/12)
    shifter->pitch_ratio = powf(2.0f, semitones / 12.0f);
}

void pitch_shifter_set_mode(PitchShifter *shifter, PitchShiftMode mode) {
    shifter->mode = mode;
}

static float hann_window(int n, int size) {
    return 0.5f * (1.0f - cosf(2.0f * 3.14159f * n / (size - 1)));
}

static float pitch_shift_granular(PitchShifter *shifter, float input) {
    // Granular synthesis method
    // Store input in grain buffer
    shifter->grain_buffer[shifter->grain_pos] = input;
    
    // Read from multiple overlapping grains with different playback rates
    float output = 0.0f;
    
    for (int g = 0; g < shifter->grain_overlap; g++) {
        int grain_offset = g * shifter->grain_size / shifter->grain_overlap;
        int read_pos = (int)((shifter->grain_pos - grain_offset) * shifter->pitch_ratio);
        read_pos = (read_pos + shifter->grain_size * shifter->grain_overlap) % 
                   (shifter->grain_size * shifter->grain_overlap);
        
        // Apply window
        float window = hann_window(shifter->grain_pos % shifter->grain_size, shifter->grain_size);
        output += shifter->grain_buffer[read_pos] * window / shifter->grain_overlap;
    }
    
    shifter->grain_pos = (shifter->grain_pos + 1) % (shifter->grain_size * shifter->grain_overlap);
    
    return output;
}

static float pitch_shift_phase_vocoder(PitchShifter *shifter, float input) {
    // FFT-based phase vocoder (higher quality but more complex)
    // This is a simplified placeholder
    // Real implementation would use FFT library
    
    // 1. Perform FFT on input
    // 2. Modify phase and frequency bins
    // 3. Perform IFFT
    // 4. Apply overlap-add
    
    return input; // Placeholder
}

float pitch_shifter_process(PitchShifter *shifter, float input) {
    if (shifter->mode == PITCH_SHIFT_GRANULAR) {
        return pitch_shift_granular(shifter, input);
    } else {
        return pitch_shift_phase_vocoder(shifter, input);
    }
}
