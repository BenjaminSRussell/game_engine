#ifndef AUDIO_DSP_EQUALIZER_H
#define AUDIO_DSP_EQUALIZER_H

#include "include/common.h"
#include "engine/include/math/math.h"

#define MAX_EQ_BANDS 10
#define MAX_EQ_PRESETS 8

typedef enum {
    EQ_FILTER_TYPE_PEAK,
    EQ_FILTER_TYPE_LOW_SHELF,
    EQ_FILTER_TYPE_HIGH_SHELF,
    EQ_FILTER_TYPE_LOW_PASS,
    EQ_FILTER_TYPE_HIGH_PASS,
    EQ_FILTER_TYPE_BAND_PASS,
    EQ_FILTER_TYPE_NOTCH
} EQFilterType;

typedef enum {
    EQ_PRESET_FLAT,
    EQ_PRESET_BASS_BOOST,
    EQ_PRESET_TREBLE_BOOST,
    EQ_PRESET_VOCAL,
    EQ_PRESET_ROCK,
    EQ_PRESET_JAZZ,
    EQ_PRESET_CLASSICAL,
    EQ_PRESET_ELECTRONIC,
    EQ_PRESET_COUNT
} EQPreset;

typedef struct {
    f32 frequency;
    f32 gain;      // in dB, -20 to +20
    f32 q;         // quality factor, 0.1 to 10.0
    f32 bandwidth; // in octaves
    EQFilterType type;
    bool enabled;
    
    // Biquad filter coefficients
    f32 a0, a1, a2, b1, b2;
    f32 x1, x2, y1, y2; // Filter history
} EQBand;

typedef struct {
    EQBand bands[MAX_EQ_BANDS];
    u32 band_count;
    f32 sample_rate;
    f32 master_gain; // Master gain in dB
    bool enabled;
    
    // Frequency analyzer data
    f32 fft_buffer[1024];
    f32 magnitude_spectrum[512];
    u32 fft_size;
    bool analyzer_enabled;
} Equalizer;

// Equalizer lifecycle
Equalizer* eq_create(f32 sample_rate);
void eq_destroy(Equalizer* eq);

// Band management
void eq_add_band(Equalizer* eq, f32 frequency, f32 gain_db, f32 q, EQFilterType type);
void eq_remove_band(Equalizer* eq, u32 band_index);
void eq_set_band_gain(Equalizer* eq, u32 band_index, f32 gain_db);
void eq_set_band_frequency(Equalizer* eq, u32 band_index, f32 frequency);
void eq_set_band_q(Equalizer* eq, u32 band_index, f32 q);
void eq_set_band_type(Equalizer* eq, u32 band_index, EQFilterType type);
void eq_set_band_enabled(Equalizer* eq, u32 band_index, bool enabled);

// Processing
void eq_process(Equalizer* eq, f32* input_buffer, f32* output_buffer, u32 frame_count);
void eq_process_stereo(Equalizer* eq, f32* left_buffer, f32* right_buffer, u32 frame_count);

// Filter implementations
void eq_parametric_band(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate);
void eq_low_shelf_filter(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate);
void eq_high_shelf_filter(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate);
void eq_bell_filter(EQBand* band, f32 frequency, f32 gain_db, f32 q, f32 sample_rate);
void eq_low_pass_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate);
void eq_high_pass_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate);
void eq_band_pass_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate);
void eq_notch_filter(EQBand* band, f32 frequency, f32 q, f32 sample_rate);

// Multi-band processing
void eq_process_multi_band(Equalizer* eq, f32* buffer, u32 frame_count);

// SIMD optimization
void eq_process_simd(Equalizer* eq, f32* input_buffer, f32* output_buffer, u32 frame_count);

// Frequency analyzer
void eq_enable_analyzer(Equalizer* eq, bool enabled);
void eq_update_analyzer(Equalizer* eq, f32* buffer, u32 frame_count);
const f32* eq_get_frequency_spectrum(Equalizer* eq);

// Presets
void eq_load_preset(Equalizer* eq, EQPreset preset);
void eq_save_preset(Equalizer* eq, EQPreset preset);

// Utilities
const char* eq_get_filter_type_name(EQFilterType type);
const char* eq_get_preset_name(EQPreset preset);
void eq_reset(Equalizer* eq);

#endif // AUDIO_DSP_EQUALIZER_H
