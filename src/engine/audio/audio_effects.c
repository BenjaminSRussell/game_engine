#include <audio/audio_effects.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  // Allocate buffer for reverb (comb filters etc require substantial memory)
  // For basic Schroeder: ~100ms buffer is usually enough for pre-delay and
  // short delays But for Comb filters we need more. Let's allocate 2 seconds
  // at 44.1kHz for now.
  u32 sample_rate = 44100;
  u32 buffer_samples = sample_rate * 2;
  effect->buffer = (f32 *)calloc(buffer_samples, sizeof(f32));
  effect->buffer_size = buffer_samples;
  effect->buffer_position = 0;

  system->active_effect_count++;
  return (u32)slot;
}
