#include <audio/audio_effects.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Internal helper to find a free effect slot in a bus
static i32 find_free_effect_slot(AudioEffectsSystem *system) {
  for (u32 i = 0; i < MAX_EFFECT_BUSES; i++) {
    if (!system->effects[i].active &&
        system->effects[i].type == EFFECT_TYPE_NONE) {
      return (i32)i;
    }
  }
  return -1;
}

static inline f32 dB_to_linear(f32 db) { return powf(10.0f, db / 20.0f); }

static inline f32 linear_to_dB(f32 linear) {
  return 20.0f * log10f(fmaxf(linear, 0.00001f));
}

// ============================================================================
// DSP IMPLEMENTATION
// ============================================================================

static f32 process_echo_sample(f32 input, EchoParams *params, f32 *buffer,
                               u32 buffer_size, u32 *pos) {
  // Read from delay line
  u32 read_pos = *pos;
  // Calculate delay in samples (clamped to buffer size)
  u32 delay_samples = (u32)(params->delay_time * 44100.0f);
  if (delay_samples >= buffer_size)
    delay_samples = buffer_size - 1;
  if (delay_samples == 0)
    delay_samples = 1;

  u32 write_pos = (read_pos + delay_samples) % buffer_size;

  f32 delayed_sample = buffer[read_pos];

  // Write to delay line with feedback
  buffer[write_pos] = input + delayed_sample * params->feedback;

  // Mix wet/dry
  f32 output = input * params->dry_level + delayed_sample * params->wet_level;

  // Advance position
  *pos = (*pos + 1) % buffer_size;

  return output;
}

static f32 allpass_filter(f32 input, f32 *buffer, u32 buffer_size, u32 *pos,
                          f32 feedback) {
  f32 delayed = buffer[*pos];
  f32 output = -input + delayed;
  buffer[*pos] = input + (delayed * feedback);
  *pos = (*pos + 1) % buffer_size;
  return output;
}

static f32 comb_filter(f32 input, f32 *buffer, u32 buffer_size, u32 *pos,
                       f32 feedback, f32 damping) {
  f32 delayed = buffer[*pos];

  // Simple low-pass damping
  // This requires state, simplifying for this implementation to just feedback
  f32 output = delayed;
  buffer[*pos] = input + (delayed * feedback);

  *pos = (*pos + 1) % buffer_size;
  return output;
}

// Basic Schroeder Reverb Implementation
// Note: A full implementation requires multiple separate buffers for
// combs/allpasses. For this MVP, we will simulate a simplified reverb using the
// allocated large buffer by partitioning it.
static f32 process_reverb_sample(f32 input, ReverbParams *params, f32 *buffer,
                                 u32 buffer_size, u32 *pos) {
  // Partition buffer for 4 combs and 2 allpasses
  // Offsets (arbitrary logic for MVP to use single buffer)
  u32 size_c1 = buffer_size / 8;

  // Using a multi-tap delay for "cheap" reverb
  f32 out = 0.0f;

  // Write input to buffer
  buffer[*pos] = input;

  // Read from multiple taps prime numbers apart
  u32 t1 = (*pos + 1111) % buffer_size;
  u32 t2 = (*pos + 3333) % buffer_size;
  u32 t3 = (*pos + 5555) % buffer_size;
  u32 t4 = (*pos + 7777) % buffer_size;

  f32 tap1 = buffer[t1];
  f32 tap2 = buffer[t2];
  f32 tap3 = buffer[t3];
  f32 tap4 = buffer[t4];

  out = (tap1 + tap2 + tap3 + tap4) * 0.25f;

  // Apply feedback to the buffer head (poor man's FDN)
  buffer[*pos] =
      input + (out * params->decay_time * 0.5f); // 0.5 to prevent explosion

  *pos = (*pos + 1) % buffer_size;

  return input * params->dry_level + out * params->wet_level;
}

// ============================================================================
// PUBLIC API
// ============================================================================

void audio_effects_init(AudioEffectsSystem *system, ma_engine *engine) {
  if (!system)
    return;

  memset(system, 0, sizeof(AudioEffectsSystem));
  system->engine = engine;
  system->initialized = true;

  // Initialize presets
  system->reverb_presets[REVERB_PRESET_SMALL_ROOM] =
      (ReverbParams){.room_size = 0.3f,
                     .damping = 0.5f,
                     .wet_level = 0.3f,
                     .dry_level = 0.8f,
                     .decay_time = 0.8f,
                     .pre_delay = 0.01f,
                     .width = 0.5f};
  system->reverb_presets[REVERB_PRESET_MEDIUM_ROOM] =
      (ReverbParams){.room_size = 0.5f,
                     .damping = 0.5f,
                     .wet_level = 0.4f,
                     .dry_level = 0.7f,
                     .decay_time = 1.5f,
                     .pre_delay = 0.02f,
                     .width = 0.7f};
  system->reverb_presets[REVERB_PRESET_LARGE_ROOM] =
      (ReverbParams){.room_size = 0.8f,
                     .damping = 0.3f,
                     .wet_level = 0.5f,
                     .dry_level = 0.6f,
                     .decay_time = 2.5f,
                     .pre_delay = 0.03f,
                     .width = 0.9f};
  system->reverb_presets[REVERB_PRESET_HALL] =
      (ReverbParams){.room_size = 0.9f,
                     .damping = 0.2f,
                     .wet_level = 0.6f,
                     .dry_level = 0.5f,
                     .decay_time = 3.5f,
                     .pre_delay = 0.04f,
                     .width = 1.0f};
  system->reverb_presets[REVERB_PRESET_CAVE] =
      (ReverbParams){.room_size = 1.0f,
                     .damping = 0.1f,
                     .wet_level = 0.7f,
                     .dry_level = 0.4f,
                     .decay_time = 5.0f,
                     .pre_delay = 0.05f,
                     .width = 1.0f};
  system->reverb_presets[REVERB_PRESET_CATHEDRAL] =
      (ReverbParams){.room_size = 1.0f,
                     .damping = 0.1f,
                     .wet_level = 0.8f,
                     .dry_level = 0.3f,
                     .decay_time = 7.0f,
                     .pre_delay = 0.06f,
                     .width = 1.0f};
  system->reverb_presets[REVERB_PRESET_UNDERWATER] =
      (ReverbParams){.room_size = 0.4f,
                     .damping = 0.9f,
                     .wet_level = 0.5f,
                     .dry_level = 0.5f,
                     .decay_time = 1.2f,
                     .pre_delay = 0.01f,
                     .width = 0.4f};
  system->reverb_presets[REVERB_PRESET_CUSTOM] =
      (ReverbParams){.room_size = 0.5f,
                     .damping = 0.5f,
                     .wet_level = 0.5f,
                     .dry_level = 0.5f,
                     .decay_time = 1.5f,
                     .pre_delay = 0.02f,
                     .width = 0.5f};

  // Initialize equalizer
  // audio_effects_eq_init_global(44100.0f); // Default rate, will be updated

  printf("Audio Effects System Initialized\n");
}

void audio_effects_shutdown(AudioEffectsSystem *system) {
  if (!system || !system->initialized)
    return;

  for (u32 i = 0; i < MAX_EFFECT_BUSES; i++) {
    if (system->effects[i].buffer) {
      free(system->effects[i].buffer);
      system->effects[i].buffer = NULL;
    }
  }

  system->initialized = false;
  printf("Audio Effects System Shutdown\n");
}

void audio_effect_destroy(AudioEffectsSystem *system, u32 effect_id) {
  if (!system || effect_id >= MAX_EFFECT_BUSES)
    return;

  AudioEffect *effect = &system->effects[effect_id];
  effect->active = false;
  effect->type = EFFECT_TYPE_NONE;

  if (effect->buffer) {
    free(effect->buffer);
    effect->buffer = NULL;
  }

  system->active_effect_count--;
}

void audio_effect_set_active(AudioEffectsSystem *system, u32 effect_id,
                             bool active) {
  if (!system || effect_id >= MAX_EFFECT_BUSES)
    return;
  system->effects[effect_id].active = active;
}

ReverbParams audio_get_reverb_preset(ReverbPreset preset) {
  // Return default preset values for static access
  // In a real system, we might query the system instance if presets are mutable
  switch (preset) {
  case REVERB_PRESET_SMALL_ROOM:
    return (ReverbParams){0.3f, 0.5f, 0.3f, 0.8f, 0.8f, 0.01f, 0.5f};
  case REVERB_PRESET_MEDIUM_ROOM:
    return (ReverbParams){0.5f, 0.5f, 0.4f, 0.7f, 1.5f, 0.02f, 0.7f};
  case REVERB_PRESET_LARGE_ROOM:
    return (ReverbParams){0.8f, 0.3f, 0.5f, 0.6f, 2.5f, 0.03f, 0.9f};
  case REVERB_PRESET_HALL:
    return (ReverbParams){0.9f, 0.2f, 0.6f, 0.5f, 3.5f, 0.04f, 1.0f};
  case REVERB_PRESET_CAVE:
    return (ReverbParams){1.0f, 0.1f, 0.7f, 0.4f, 5.0f, 0.05f, 1.0f};
  case REVERB_PRESET_CATHEDRAL:
    return (ReverbParams){1.0f, 0.1f, 0.8f, 0.3f, 7.0f, 0.06f, 1.0f};
  case REVERB_PRESET_UNDERWATER:
    return (ReverbParams){0.4f, 0.9f, 0.5f, 0.5f, 1.2f, 0.01f, 0.4f};
  default:
    return (ReverbParams){0.5f, 0.5f, 0.5f, 0.5f, 1.5f, 0.02f, 0.5f};
  }
}

u32 audio_effect_create_reverb(AudioEffectsSystem *system,
                               ReverbPreset preset) {
  if (!system)
    return 0xFFFFFFFF;

  i32 slot = find_free_effect_slot(system);
  if (slot < 0)
    return 0xFFFFFFFF;

  AudioEffect *effect = &system->effects[slot];
  effect->type = EFFECT_TYPE_REVERB;
  effect->active = true;

  // Load params from preset or system presets
  if (preset < MAX_REVERB_PRESETS) {
    effect->params.reverb = system->reverb_presets[preset];
  } else {
    effect->params.reverb = audio_get_reverb_preset(REVERB_PRESET_MEDIUM_ROOM);
  }

  // Allocate buffer for reverb
  u32 sample_rate = 44100;
  u32 buffer_samples = sample_rate * 2;
  effect->buffer = (f32 *)calloc(buffer_samples, sizeof(f32));
  if (effect->buffer)
    memset(effect->buffer, 0, buffer_samples * sizeof(f32));
  effect->buffer_size = buffer_samples;
  effect->buffer_position = 0;

  system->active_effect_count++;
  return (u32)slot;
}

u32 audio_effect_create_echo(AudioEffectsSystem *system, f32 delay,
                             f32 feedback) {
  if (!system)
    return 0xFFFFFFFF;

  i32 slot = find_free_effect_slot(system);
  if (slot < 0)
    return 0xFFFFFFFF;

  AudioEffect *effect = &system->effects[slot];
  effect->type = EFFECT_TYPE_ECHO;
  effect->active = true;
  effect->params.echo = (EchoParams){.delay_time = delay,
                                     .feedback = feedback,
                                     .wet_level = 0.5f,
                                     .dry_level = 0.5f};

  // Allocate buffer for 2 seconds max delay
  u32 sample_rate = 44100;
  u32 buffer_samples = sample_rate * 2;
  effect->buffer = (f32 *)calloc(buffer_samples, sizeof(f32));
  if (effect->buffer)
    memset(effect->buffer, 0, buffer_samples * sizeof(f32));
  effect->buffer_size = buffer_samples;
  effect->buffer_position = 0;

  system->active_effect_count++;
  return (u32)slot;
}

u32 audio_effect_create_lowpass(AudioEffectsSystem *system, f32 cutoff_freq) {
  if (!system)
    return 0xFFFFFFFF;

  i32 slot = find_free_effect_slot(system);
  if (slot < 0)
    return 0xFFFFFFFF;

  AudioEffect *effect = &system->effects[slot];
  effect->type = EFFECT_TYPE_LOWPASS;
  effect->active = true;
  effect->params.filter = (FilterParams){
      .cutoff_freq = cutoff_freq, .resonance = 0.1f, .wet_level = 1.0f};

  effect->buffer = (f32 *)calloc(16, sizeof(f32));
  effect->buffer_size = 16;
  effect->buffer_position = 0;

  system->active_effect_count++;
  return (u32)slot;
}

u32 audio_effect_create_highpass(AudioEffectsSystem *system, f32 cutoff_freq) {
  if (!system)
    return 0xFFFFFFFF;

  i32 slot = find_free_effect_slot(system);
  if (slot < 0)
    return 0xFFFFFFFF;

  AudioEffect *effect = &system->effects[slot];
  effect->type = EFFECT_TYPE_HIGHPASS;
  effect->active = true;
  effect->params.filter = (FilterParams){
      .cutoff_freq = cutoff_freq, .resonance = 0.1f, .wet_level = 1.0f};

  effect->buffer = (f32 *)calloc(16, sizeof(f32));
  effect->buffer_size = 16;
  effect->buffer_position = 0;

  system->active_effect_count++;
  return (u32)slot;
}

void audio_effect_set_reverb_params(AudioEffectsSystem *system, u32 effect_id,
                                    ReverbParams params) {
  if (!system || effect_id >= MAX_EFFECT_BUSES)
    return;
  if (system->effects[effect_id].type == EFFECT_TYPE_REVERB) {
    system->effects[effect_id].params.reverb = params;
  }
}

void audio_effect_set_echo_params(AudioEffectsSystem *system, u32 effect_id,
                                  EchoParams params) {
  if (!system || effect_id >= MAX_EFFECT_BUSES)
    return;
  if (system->effects[effect_id].type == EFFECT_TYPE_ECHO) {
    system->effects[effect_id].params.echo = params;
  }
}

void audio_effect_set_filter_params(AudioEffectsSystem *system, u32 effect_id,
                                    FilterParams params) {
  if (!system || effect_id >= MAX_EFFECT_BUSES)
    return;
  if (system->effects[effect_id].type == EFFECT_TYPE_LOWPASS ||
      system->effects[effect_id].type == EFFECT_TYPE_HIGHPASS) {
    system->effects[effect_id].params.filter = params;
  }
}

void audio_effects_update(AudioEffectsSystem *system) {
  if (!system || !system->initialized)
    return;

  // Update active effects
  for (u32 i = 0; i < MAX_EFFECT_BUSES; i++) {
    if (system->effects[i].active) {
      // Future: Update modulators, LFOs, or smooth parameter changes here
    }
  }
}

void audio_effect_apply_to_sound(AudioEffectsSystem *system, u32 effect_id,
                                 ma_sound *sound) {}

// Internal helper to estimate room size (stub for raycast)
static f32 estimate_room_size(Vec3 position) {
  // In a real implementation: Cast rays in 6 directions
  // For now, return a default "Medium" size
  return 10.0f; // meters
}

void audio_effects_update_dynamic_reverb(AudioEffectsSystem *system,
                                         Vec3 listener_position,
                                         f32 delta_time) {
  if (!system || !system->initialized)
    return;

  f32 room_size = estimate_room_size(listener_position);
  ReverbPreset target_preset = REVERB_PRESET_MEDIUM_ROOM;

  if (room_size < 5.0f)
    target_preset = REVERB_PRESET_SMALL_ROOM;
  else if (room_size < 15.0f)
    target_preset = REVERB_PRESET_MEDIUM_ROOM;
  else if (room_size < 30.0f)
    target_preset = REVERB_PRESET_LARGE_ROOM;
  else
    target_preset = REVERB_PRESET_CATHEDRAL;

  // Find active reverb and update params (interpolation would be better but
  // snap for now)
  for (u32 i = 0; i < MAX_EFFECT_BUSES; i++) {
    if (system->effects[i].active &&
        system->effects[i].type == EFFECT_TYPE_REVERB) {
      // audio_effect_set_reverb_params(system, i,
      // system->reverb_presets[target_preset]); Commented out to avoid snap
      // artifacts without interpolation
    }
  }
}

// EQ Implementation
void audio_effects_eq_init(Equalizer *eq, f32 sample_rate) {
  if (!eq)
    return;
  memset(eq, 0, sizeof(Equalizer));
  eq->sample_rate = sample_rate;
  eq->band_count = 10;

  // Standard ISO octave bands
  f32 freqs[] = {31.25f,  62.5f,   125.0f,  250.0f,  500.0f,
                 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};

  for (int i = 0; i < 10; i++) {
    eq->bands[i].frequency = freqs[i];
    eq->bands[i].gain = 0.0f;
    eq->bands[i].q = 1.414f; // Sqrt(2) for octave bandwidth
    eq->bands[i].type = EQ_FILTER_TYPE_PEAK;
    eq->bands[i].enabled = true;

    if (i == 0)
      eq->bands[i].type = EQ_FILTER_TYPE_LOW_SHELF;
    if (i == 9)
      eq->bands[i].type = EQ_FILTER_TYPE_HIGH_SHELF;

    // Recalculate coefficients (stub)
    // audio_effects_eq_set_band_gain(eq, i, 0.0f);
  }

  eq->enabled = true;
  eq->master_gain = 0.0f;
}

// Biquad calculation helper (Cookbook formulae)
static void update_biquad(Equalizer *eq, u32 band) {
  if (!eq || band >= eq->band_count)
    return;

  EQBand *b = &eq->bands[band];
  f32 Fs = eq->sample_rate;
  f32 f0 = b->frequency;
  f32 Q = b->q;
  // f32 gain = b->gain; // Not used for LPF/HPF usually, but maybe for boost?
  // Standard LPF doesn't use gain.

  f32 w0 = 2.0f * M_PI * f0 / Fs;
  f32 alpha = sinf(w0) / (2.0f * Q);
  f32 cos_w0 = cosf(w0);
  f32 sqrt_A = sqrtf(A);

  f32 a0 = 1.0f + alpha; // Normalize by a0 later
  f32 b0, b1, b2, a1, a2;

  // Clear coefficients
  b0 = b1 = b2 = a1 = a2 = 0.0f;

  switch (b->type) {
  case EQ_FILTER_TYPE_LOW_PASS:
    b0 = (1.0f - cos_w0) / 2.0f;
    b1 = 1.0f - cos_w0;
    b2 = (1.0f - cos_w0) / 2.0f;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha;
    break;

  case EQ_FILTER_TYPE_HIGH_PASS:
    b0 = (1.0f + cos_w0) / 2.0f;
    b1 = -(1.0f + cos_w0);
    b2 = (1.0f + cos_w0) / 2.0f;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha;
    break;

  case EQ_FILTER_TYPE_PEAK:
    b0 = 1.0f + alpha * A;
    b1 = -2.0f * cos_w0;
    b2 = 1.0f - alpha * A;
    a0 = 1.0f + alpha / A;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha / A;
    break;

  case EQ_FILTER_TYPE_LOW_SHELF:
    b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * sqrt_A * alpha);
    b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * sqrt_A * alpha);
    a0 = (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * sqrt_A * alpha;
    a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    a2 = (A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * sqrt_A * alpha;
    break;

  case EQ_FILTER_TYPE_HIGH_SHELF:
    b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * sqrt_A * alpha);
    b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * sqrt_A * alpha);
    a0 = (A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * sqrt_A * alpha;
    a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    a2 = (A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * sqrt_A * alpha;
    break;

  case EQ_FILTER_TYPE_NOTCH:
    b0 = 1.0f;
    b1 = -2.0f * cos_w0;
    b2 = 1.0f;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha;
    break;

  // Stub for others
  default:
    b0 = 1.0f;
    a0 = 1.0f; // Identity
    break;
  }

  // Normalize
  eq->filters[band].a0 = 1.0f; // Since we divide everything else by a0
  eq->filters[band].a1 = a1 / a0;
  eq->filters[band].a2 = a2 / a0;
  eq->filters[band].b1 =
      b1 / a0; // Note: Standard Form 1 usually labels input 'b' and output 'a'
  eq->filters[band].b2 = b2 / a0;
  // wait, struct has a0, a1, a2, b1, b2.
  // Standard biquad: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] -
  // a2*y[n-2] Note: a0 is usually 1 after normalization. Let's assume struct
  // stores normalized coefficients. We need b0 in the struct! Currently struct
  // has: f32 a0, a1, a2, b1, b2; Wait, looking at header... struct { f32 a0,
  // a1, a2, b1, b2; ... } Usually coeff are: b0, b1, b2, a1, a2. I likely
  // mapped a0 -> b0 in previous step? Or just a0 was typo for b0? Let's modify
  // the struct access to use b0 if possible, or re-purpose a0 as b0. Actually,
  // standard is: a0 is divisor. Let's assume the struct meant: b0, b1, b2, a1,
  // a2. Accessing members:
  eq->filters[band].a0 = b0 / a0; // Reuse a0 as b0 (feedforward 0)
  eq->filters[band].a1 = b1 / a0; // Reuse a1 as b1 (feedforward 1)
  eq->filters[band].a2 = b2 / a0; // Reuse a2 as b2 (feedforward 2)
  eq->filters[band].b1 = a1 / a0; // Reuse b1 as a1 (feedback 1)
  eq->filters[band].b2 = a2 / a0; // Reuse b2 as a2 (feedback 2)
}

void audio_effects_eq_process(Equalizer *eq, f32 *input_buffer,
                              f32 *output_buffer, u32 frame_count) {
  if (!eq || !eq->enabled) {
    memcpy(output_buffer, input_buffer, frame_count * sizeof(f32));
    return;
  }
  // Iterate samples
  for (u32 i = 0; i < frame_count; i++) {
    f32 x = input_buffer[i];
    f32 y = x;
    // Series biquads
    for (u32 b = 0; b < eq->band_count; b++) {
      // Apply biquad (direct form 1)
      // y = (b0/a0)*x + (b1/a0)*x1 + (b2/a0)*x2 - (a1/a0)*y1 - (a2/a0)*y2
      // Using stub coefficients for now
    }
    output_buffer[i] = y;
  }
}

void audio_effects_eq_set_band_gain(Equalizer *eq, u32 band_index,
                                    f32 gain_db) {
  if (band_index < 10 && eq) {
    eq->bands[band_index].gain = gain_db;
    update_biquad(eq, band_index);
  }
}

// Global instance (stub)
static Equalizer g_global_eq;

void audio_effects_eq_init_global(f32 sample_rate) {
  audio_effects_eq_init(&g_global_eq, sample_rate);
}

Equalizer *audio_effects_eq_get_global(void) { return &g_global_eq; }
