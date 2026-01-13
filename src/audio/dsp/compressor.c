/**
 * =================================================================================================
 *                          DYNAMIC RANGE COMPRESSOR
 * =================================================================================================
 */

#include "audio/dsp/compressor.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

static float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

static float linear_to_db(float linear) {
    return 20.0f * log10f(linear);
}

void compressor_init(Compressor *comp, int sample_rate) {
    memset(comp, 0, sizeof(Compressor));
    
    comp->threshold_db = -20.0f;
    comp->ratio = 4.0f;
    comp->knee_db = 6.0f;
    comp->attack_ms = 10.0f;
    comp->release_ms = 100.0f;
    comp->makeup_gain_db = 0.0f;
    
    comp->gain_reduction = 1.0f;
    comp->envelope = 0.0f;
    
    // Lookahead buffer (5ms)
    comp->lookahead_size = (int)((5.0f / 1000.0f) * sample_rate);
    comp->lookahead_buffer = calloc(comp->lookahead_size, sizeof(float));
    comp->lookahead_pos = 0;
}

void compressor_destroy(Compressor *comp) {
    free(comp->lookahead_buffer);
}

void compressor_set_threshold(Compressor *comp, float threshold_db) {
    comp->threshold_db = threshold_db;
}

void compressor_set_ratio(Compressor *comp, float ratio) {
    comp->ratio = ratio;
}

void compressor_set_attack(Compressor *comp, float attack_ms) {
    comp->attack_ms = attack_ms;
}

void compressor_set_release(Compressor *comp, float release_ms) {
    comp->release_ms = release_ms;
}

static float calculate_rms(Compressor *comp, float input) {
    // Add to RMS buffer
    comp->rms_buffer[comp->rms_index] = input * input;
    comp->rms_index = (comp->rms_index + 1) % 256;
    
    // Calculate RMS
    float sum = 0.0f;
    for (int i = 0; i < 256; i++) {
        sum += comp->rms_buffer[i];
    }
    return sqrtf(sum / 256.0f);
}

static float apply_knee(Compressor *comp, float input_db) {
    float knee_start = comp->threshold_db - comp->knee_db / 2.0f;
    float knee_end = comp->threshold_db + comp->knee_db / 2.0f;
    
    if (input_db < knee_start) {
        return input_db;
    } else if (input_db > knee_end) {
        return comp->threshold_db + (input_db - comp->threshold_db) / comp->ratio;
    } else {
        // Soft knee curve
        float knee_input = input_db - knee_start;
        float knee_delta = knee_input / comp->knee_db;
        float gain_reduction = (1.0f / comp->ratio - 1.0f) * knee_delta * knee_delta;
        return input_db + comp->knee_db * gain_reduction;
    }
}

float compressor_process(Compressor *comp, float input, float sidechain_input) {
    // Use sidechain or input for detection
    float detect_signal = comp->use_sidechain ? sidechain_input : input;
    
    // RMS level detection
    float rms = calculate_rms(comp, detect_signal);
    float input_db = linear_to_db(rms + 0.0001f); // Avoid log(0)
    
    // Apply knee and calculate gain reduction
    float output_db = apply_knee(comp, input_db);
    float target_gain = db_to_linear(output_db - input_db);
    
    // Attack/Release envelope
    const float sample_rate = 48000.0f;
    float attack_coef = expf(-1.0f / (comp->attack_ms * sample_rate / 1000.0f));
    float release_coef = expf(-1.0f / (comp->release_ms * sample_rate / 1000.0f));
    
    if (target_gain < comp->envelope) {
        comp->envelope = attack_coef * comp->envelope + (1.0f - attack_coef) * target_gain;
    } else {
        comp->envelope = release_coef * comp->envelope + (1.0f - release_coef) * target_gain;
    }
    
    // Apply gain reduction and makeup gain
    float makeup = db_to_linear(comp->makeup_gain_db);
    return input * comp->envelope * makeup;
}

void compressor_process_stereo(Compressor *comp, float *left, float *right) {
    // Stereo linking: use max of both channels for detection
    float max_input = fabsf(*left) > fabsf(*right) ? *left : *right;
    
    float compressed = compressor_process(comp, max_input, 0.0f);
    float gain = compressed / (max_input + 0.0001f);
    
    *left *= gain;
    *right *= gain;
}
