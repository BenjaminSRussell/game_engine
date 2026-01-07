#pragma once

typedef enum {
    PITCH_SHIFT_GRANULAR,
    PITCH_SHIFT_PHASE_VOCODER
} PitchShiftMode;

typedef struct {
    PitchShiftMode mode;
    float pitch_ratio; // 1.0 = no shift, 2.0 = octave up, 0.5 = octave down
    
    // Granular synthesis
    float *grain_buffer;
    int grain_size;
    int grain_overlap;
    int grain_pos;
    
    // Phase vocoder (FFT-based)
    float *fft_buffer;
    int fft_size;
    float *phase_acc;
    
    // Formant preservation
    bool preserve_formants;
    
    // Latency
    int latency_samples;
} PitchShifter;

void pitch_shifter_init(PitchShifter *shifter, int sample_rate);
void pitch_shifter_destroy(PitchShifter *shifter);

void pitch_shifter_set_pitch(PitchShifter *shifter, float semitones);
void pitch_shifter_set_mode(PitchShifter *shifter, PitchShiftMode mode);

float pitch_shifter_process(PitchShifter *shifter, float input);
