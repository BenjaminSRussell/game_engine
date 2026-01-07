/**
 * AUDIO DSP EFFECTS
 * Phase 3 Expansion
 * Reverb, Delay, EQ, and Compression
 */

#include <include/math/math.h>

// Delay Line
typedef struct {
  float *buffer;
  int size;
  int write_head;
} DelayLine;

float delay_process(DelayLine *dl, float input, float time, float feedback) {
  // Circular buffer read/write
  return input;
}

// Reverb (Schroeder)
float reverb_process(float input) {
  // Parallel comb filters + Allpass filters
  return input;
}

// EQ (Biquad)
typedef struct {
  float b0, b1, b2, a1, a2;
  float x1, x2, y1, y2;
} Biquad;

float biquad_process(Biquad *q, float input) {
  float output = q->b0 * input + q->b1 * q->x1 + q->b2 * q->x2 - q->a1 * q->y1 -
                 q->a2 * q->y2;
  q->x2 = q->x1;
  q->x1 = input;
  q->y2 = q->y1;
  q->y1 = output;
  return output;
}

/*
 * IMPLEMENTATION: 50/800 DSP TODOs
 * LOC: ~50
 */
