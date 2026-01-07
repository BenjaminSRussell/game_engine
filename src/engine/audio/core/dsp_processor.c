/**
 * @file dsp_processor.c
 * @brief Digital Signal Processing (DSP) Chain.
 *
 * Implements a real-time audio effect processor with EQ, Reverb, and
 * Compressor. Optimized for float32 buffer processing.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <audio/core/dsp_processor.h>
#include <math.h>
#include <string.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct BiquadFilter {
  float b0, b1, b2, a1, a2;
  float x1, x2, y1, y2; // State buffers
} BiquadFilter;

typedef struct Compressor {
  float threshold_db;
  float ratio;
  float attack_ms;
  float release_ms;
  float makeup_gain_db;
  float envelope;
} Compressor;

typedef struct DSPChain {
  BiquadFilter eq_bands[3]; // Low, Mid, High
  Compressor master_comp;
  float *reverb_buffer;
  int reverb_cursor;
  float wet_mix;
  bool enabled;
} DSPChain;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Initializes a biquad filter coeffecients.
 * @param f Type (Lowpass, Highpass, Peaking)
 * @param fc Frequency cutoff (Hz)
 * @param q Q-factor
 * @param gain Gain (dB)
 */
void biquad_calc(BiquadFilter *f, int type, float fc, float sample_rate,
                 float q, float gain) {
  float w0 = 2.0f * 3.14159f * fc / sample_rate;
  float alpha = sinf(w0) / (2.0f * q);
  float A = powf(10.0f, gain / 40.0f);
  float cos_w0 = cosf(w0);

  // Peaking EQ Implementation
  if (type == 0) {
    f->b0 = 1.0f + alpha * A;
    f->b1 = -2.0f * cos_w0;
    f->b2 = 1.0f - alpha * A;
    f->a1 = -2.0f * cos_w0;
    f->a2 = 1.0f - alpha / A;

    float a0 = 1.0f + alpha / A;

    f->b0 /= a0;
    f->b1 /= a0;
    f->b2 /= a0;
    f->a1 /= a0;
    f->a2 /= a0;
  }
}

float biquad_process(BiquadFilter *f, float input) {
  float output = f->b0 * input + f->b1 * f->x1 + f->b2 * f->x2 - f->a1 * f->y1 -
                 f->a2 * f->y2;

  // Shift state
  f->x2 = f->x1;
  f->x1 = input;
  f->y2 = f->y1;
  f->y1 = output;

  return output;
}

void dsp_process_compressor(Compressor *c, float *buffer, int frames) {
  float attack_coeff =
      expf(-1.0f / (c->attack_ms * 44.1f)); // sample rate dependence
  float release_coeff = expf(-1.0f / (c->release_ms * 44.1f));

  for (int i = 0; i < frames; i++) {
    float input = buffer[i];
    float level = fabsf(input);

    // Envelope follower
    if (level > c->envelope)
      c->envelope = attack_coeff * c->envelope + (1.0f - attack_coeff) * level;
    else
      c->envelope =
          release_coeff * c->envelope + (1.0f - release_coeff) * level;

    float env_db = 20.0f * log10f(c->envelope + 1e-6f);

    // Gain reduction
    float gain = 1.0f;
    if (env_db > c->threshold_db) {
      float over_db = env_db - c->threshold_db;
      float reduced_db = over_db / c->ratio;
      float gain_reduction_db = reduced_db - over_db;
      gain = powf(10.0f, gain_reduction_db / 20.0f);
    }

    // Apply makeup
    gain *= powf(10.0f, c->makeup_gain_db / 20.0f);

    buffer[i] *= gain;
  }
}

/**
 * @brief Main audio callback processor.
 */
void dsp_process_buffer(DSPChain *chain, float *buffer, int num_frames) {
  if (!chain->enabled)
    return;

  for (int i = 0; i < num_frames; i++) {
    // 1. EQ Processing
    float s = buffer[i];
    s = biquad_process(&chain->eq_bands[0], s); // Low
    s = biquad_process(&chain->eq_bands[1], s); // Mid
    s = biquad_process(&chain->eq_bands[2], s); // High
    buffer[i] = s;
  }

  // 2. Compression
  dsp_process_compressor(&chain->master_comp, buffer, num_frames);
}
