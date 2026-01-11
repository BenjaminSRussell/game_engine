// include/audio/audio_effects.h
//
// Purpose: Audio effects processing system for applying real-time DSP effects
// to audio sources (reverb, echo, low-pass filters, high-pass filters).
//
// Key Features:
// - Reverb with configurable parameters (room size, damping, wet/dry mix)
// - Echo/delay effects with feedback
// - Low-pass and high-pass filters for environmental effects
// - Per-source effect application
// - Global effect buses
//
// Public APIs:
// - audio_effects_init: Initialize effects system
// - audio_effects_apply_reverb: Apply reverb to audio source
// - audio_effects_apply_filter: Apply frequency filter
// - audio_effects_update: Process audio effects each frame
//
// Invariants:
// - Effects system must be initialized before use
// - Effect parameters should be normalized (0.0-1.0)
// - Effects applied per-source or globally via buses
//
#ifndef AUDIO_EFFECTS_H
#define AUDIO_EFFECTS_H

#include "include/vendor/miniaudio.h"
#include <common.h>
#include <math/vec3.h>

#define MAX_EFFECT_BUSES 4
#define MAX_REVERB_PRESETS 8

typedef enum {
  EFFECT_TYPE_NONE,
  EFFECT_TYPE_REVERB,
  EFFECT_TYPE_ECHO,
  EFFECT_TYPE_LOWPASS,
  EFFECT_TYPE_HIGHPASS,
  EFFECT_TYPE_DISTORTION,
  EFFECT_TYPE_CHORUS
} EffectType;

typedef enum {
  REVERB_PRESET_SMALL_ROOM,
  REVERB_PRESET_MEDIUM_ROOM,
  REVERB_PRESET_LARGE_ROOM,
  REVERB_PRESET_HALL,
  REVERB_PRESET_CAVE,
  REVERB_PRESET_CATHEDRAL,
  REVERB_PRESET_UNDERWATER,
  REVERB_PRESET_CUSTOM
} ReverbPreset;

typedef struct {
  f32 room_size;  // 0.0 to 1.0
  f32 damping;    // 0.0 to 1.0 (high freq absorption)
  f32 wet_level;  // 0.0 to 1.0 (reverb volume)
  f32 dry_level;  // 0.0 to 1.0 (direct sound volume)
  f32 decay_time; // Seconds (0.1 to 10.0)
  f32 pre_delay;  // Seconds (0.0 to 0.1)
  f32 width;      // 0.0 to 1.0 (stereo width)
} ReverbParams;

typedef struct {
  f32 delay_time; // Seconds (0.01 to 2.0)
  f32 feedback;   // 0.0 to 0.95 (avoid infinite feedback)
  f32 wet_level;  // 0.0 to 1.0
  f32 dry_level;  // 0.0 to 1.0
} EchoParams;

typedef struct {
  f32 cutoff_freq; // Hertz (20 to 20000)
  f32 resonance;   // 0.0 to 1.0 (Q factor)
  f32 wet_level;   // 0.0 to 1.0
} FilterParams;

typedef struct {
  EffectType type;
  bool active;

  union {
    ReverbParams reverb;
    EchoParams echo;
    FilterParams filter;
  } params;

  // Processed audio buffer
  f32 *buffer;
  u32 buffer_size;
  u32 buffer_position;
} AudioEffect;

typedef struct {
  AudioEffect effects[MAX_EFFECT_BUSES];
  u32 active_effect_count;

  // Pre-configured reverb presets
  ReverbParams reverb_presets[MAX_REVERB_PRESETS];

  ma_engine *engine;
  bool initialized;
} AudioEffectsSystem;

// Lifecycle
void audio_effects_init(AudioEffectsSystem *system, ma_engine *engine);
void audio_effects_shutdown(AudioEffectsSystem *system);

// Effect creation
u32 audio_effect_create_reverb(AudioEffectsSystem *system, ReverbPreset preset);

// Reset all effects (clear all active effects)
void audio_effects_reset(AudioEffectsSystem *system);

u32 audio_effect_create_echo(AudioEffectsSystem *system, f32 delay,
                             f32 feedback);
u32 audio_effect_create_lowpass(AudioEffectsSystem *system, f32 cutoff_freq);
u32 audio_effect_create_highpass(AudioEffectsSystem *system, f32 cutoff_freq);

// Effect control
void audio_effect_set_active(AudioEffectsSystem *system, u32 effect_id,
                             bool active);
void audio_effect_set_reverb_params(AudioEffectsSystem *system, u32 effect_id,
                                    ReverbParams params);
void audio_effect_set_echo_params(AudioEffectsSystem *system, u32 effect_id,
                                  EchoParams params);
void audio_effect_set_filter_params(AudioEffectsSystem *system, u32 effect_id,
                                    FilterParams params);
void audio_effect_destroy(AudioEffectsSystem *system, u32 effect_id);

// Apply effects to sounds
void audio_effect_apply_to_sound(AudioEffectsSystem *system, u32 effect_id,
                                 ma_sound *sound);
void audio_effect_remove_from_sound(AudioEffectsSystem *system,
                                    ma_sound *sound);

// Update (call each frame)
void audio_effects_update(AudioEffectsSystem *system);

// Helper: Get reverb preset parameters
ReverbParams audio_get_reverb_preset(ReverbPreset preset);
// Reverb params setter (global/bus)
void audio_effects_set_reverb_params(AudioEffectsSystem *system, u32 bus_index,
                                     const ReverbParams *params);

// Dynamic reverb parameter adjustment
void audio_effects_update_dynamic_reverb(AudioEffectsSystem *system,
                                         Vec3 listener_position,
                                         f32 delta_time);
ReverbParams audio_effects_get_dynamic_reverb(Vec3 position);
void audio_effects_force_room_reanalysis(AudioEffectsSystem *system,
                                         Vec3 listener_position);
bool audio_effects_is_cave_environment(Vec3 position);
bool audio_effects_is_large_hall(Vec3 position);

// Environment detection
typedef enum {
  ENVIRONMENT_OUTDOOR_OPEN,
  ENVIRONMENT_OUTDOOR_SEMI,
  ENVIRONMENT_SMALL_ROOM,
  ENVIRONMENT_MEDIUM_ROOM,
  ENVIRONMENT_LARGE_ROOM,
  ENVIRONMENT_CAVERN,
  ENVIRONMENT_HALL
} EnvironmentType;

EnvironmentType audio_effects_get_environment_type(Vec3 position);

// Multi-band EQ (equalizer) system
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
} EQBand;

typedef struct {
  EQBand bands[10];
  u32 band_count;
  f32 sample_rate;

  // Biquad filter coefficients for each band (implementation detail)
  struct {
    f32 a0, a1, a2, b1, b2;
    f32 x1, x2, y1, y2; // Filter history
  } filters[10];

  bool enabled;
  f32 master_gain; // Master gain in dB
} Equalizer;

// EQ initialization and management
void audio_effects_eq_init(Equalizer *eq, f32 sample_rate);
void audio_effects_eq_init_global(f32 sample_rate);
void audio_effects_eq_init_bus(u32 bus_index, f32 sample_rate);
Equalizer *audio_effects_eq_get_global(void);
Equalizer *audio_effects_eq_get_bus(u32 bus_index);

// EQ processing
void audio_effects_eq_process(Equalizer *eq, f32 *input_buffer,
                              f32 *output_buffer, u32 frame_count);

// EQ band manipulation
void audio_effects_eq_set_band_gain(Equalizer *eq, u32 band_index, f32 gain_db);
void audio_effects_eq_set_band_frequency(Equalizer *eq, u32 band_index,
                                         f32 frequency);
void audio_effects_eq_set_band_q(Equalizer *eq, u32 band_index, f32 q);
void audio_effects_eq_set_band_type(Equalizer *eq, u32 band_index,
                                    EQFilterType type);
void audio_effects_eq_set_band_enabled(Equalizer *eq, u32 band_index,
                                       bool enabled);
void audio_effects_eq_set_master_gain(Equalizer *eq, f32 gain_db);

// EQ presets
void audio_effects_eq_load_preset(Equalizer *eq, EQPreset preset);

// EQ band information
const char *audio_effects_eq_get_band_name(u32 band_index);
f32 audio_effects_eq_get_band_frequency(Equalizer *eq, u32 band_index);
f32 audio_effects_eq_get_band_gain(Equalizer *eq, u32 band_index);
bool audio_effects_eq_is_band_enabled(Equalizer *eq, u32 band_index);

#endif // AUDIO_EFFECTS_H