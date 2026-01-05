#pragma once

typedef struct {
    float ceiling_db;
    float release_ms;
    
    float *lookahead_buffer;
    int lookahead_size;
    int lookahead_pos;
    
    float gain_reduction;
    float envelope;
} Limiter;

void limiter_init(Limiter *limiter, int sample_rate);
void limiter_destroy(Limiter *limiter);

void limiter_set_ceiling(Limiter *limiter, float ceiling_db);
void limiter_set_release(Limiter *limiter, float release_ms);

float limiter_process(Limiter *limiter, float input);
void limiter_process_stereo(Limiter *limiter, float *left, float *right);
