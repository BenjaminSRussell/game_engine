/**
 * @file sound_synthesizer.c
 * @brief Real-time Audio Synthesizer.
 *
 * Implements oscillators (Sine, Saw, Square) and ADSR envelopes
 * for generating SFX or musical notes on the fly.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <audio/core/sound_synthesizer.h>
#include <include/math/math_all.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef enum Waveform {
  WAVE_SINE,
  WAVE_SQUARE,
  WAVE_SAW,
  WAVE_TRIANGLE,
  WAVE_NOISE
} Waveform;

typedef struct ADSREnvelope {
  float attack_time;
  float decay_time;
  float sustain_level;
  float release_time;

  float current_time;
  bool note_on;
  bool finished;
} ADSREnvelope;

typedef struct Oscillator {
  Waveform type;
  float frequency;
  float phase;
  float sample_rate;
} Oscillator;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

float osc_next_sample(Oscillator *osc) {
  float sample = 0.0f;
  float t = osc->phase;

  switch (osc->type) {
  case WAVE_SINE:
    sample = sinf(t * 2.0f * PI);
    break;
  case WAVE_SQUARE:
    sample = (t < 0.5f) ? 1.0f : -1.0f;
    break;
  case WAVE_SAW:
    sample = 2.0f * t - 1.0f;
    break;
  case WAVE_TRIANGLE:
    sample = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
    break;
  case WAVE_NOISE:
    sample = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    break;
  }

  // Increment Phase
  float phase_delta = osc->frequency / osc->sample_rate;
  osc->phase += phase_delta;
  if (osc->phase >= 1.0f)
    osc->phase -= 1.0f;

  return sample;
}

float adsr_process(ADSREnvelope *env, float dt) {
  if (env->finished)
    return 0.0f;

  env->current_time += dt;
  float level = 0.0f;

  if (env->note_on) {
    // Attack Phase
    if (env->current_time < env->attack_time) {
      level = env->current_time / env->attack_time;
    }
    // Decay Phase
    else if (env->current_time < (env->attack_time + env->decay_time)) {
      float decay_pos = env->current_time - env->attack_time;
      float progress = decay_pos / env->decay_time;
      level = 1.0f + (env->sustain_level - 1.0f) * progress;
    }
    // Sustain Phase
    else {
      level = env->sustain_level;
    }
  } else {
    // Release Phase
    // ... (requires tracking release start time) ...
    // Simplified Logic:
    level = 0.0f; // instant cut for now
    env->finished = true;
  }

  return level;
}
