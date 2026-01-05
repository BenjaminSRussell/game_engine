#pragma once

typedef struct {
    // Level detection
    float rms_buffer[256];
    int rms_index;
    float current_rms;
    
    // Parameters
    float threshold_db;
    float ratio;
    float knee_db;
    float attack_ms;
    float release_ms;
    float makeup_gain_db;
    
    // State
    float gain_reduction;
    float envelope;
    
    // Lookahead
    float *lookahead_buffer;
    int lookahead_size;
    int lookahead_pos;
    
    // Sidechain
    bool use_sidechain;
    float sidechain_mix;
} Compressor;

void compressor_init(Compressor *comp, int sample_rate);
void compressor_destroy(Compressor *comp);

void compressor_set_threshold(Compressor *comp, float threshold_db);
void compressor_set_ratio(Compressor *comp, float ratio);
void compressor_set_attack(Compressor *comp, float attack_ms);
void compressor_set_release(Compressor *comp, float release_ms);

float compressor_process(Compressor *comp, float input, float sidechain_input);
void compressor_process_stereo(Compressor *comp, float *left, float *right);
