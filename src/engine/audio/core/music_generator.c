/**
 * @file music_generator.c
 * @brief Procedural Music System.
 *
 * Generates layered music tracks based on gameplay intensity.
 * Uses a stem-based approach (Drum, Bass, Pad, Lead).
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <audio/core/music_generator.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef enum IntensityLevel {
  INTENSITY_CALM = 0,
  INTENSITY_EXPLORE,
  INTENSITY_COMBAT,
  INTENSITY_BOSS
} IntensityLevel;

typedef struct MusicLayer {
  char name[32];
  float volume;
  float target_volume;
  // ... pointer to audio clip data ...
  bool is_playing;
  float fade_speed; // Units/sec
} MusicLayer;

typedef struct MusicSystem {
  IntensityLevel current_intensity;
  float transition_timer;
  MusicLayer stems[4]; // 0=Base, 1=Percussion, 2=Melody, 3=HighEnergy
  float bpm;
  float beat_timer;
} MusicSystem;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

void music_update_transition(MusicSystem *sys, float dt) {
  // Fade volumes towards target
  for (int i = 0; i < 4; i++) {
    MusicLayer *layer = &sys->stems[i];
    if (layer->volume < layer->target_volume) {
      layer->volume += layer->fade_speed * dt;
      if (layer->volume > layer->target_volume)
        layer->volume = layer->target_volume;
    } else if (layer->volume > layer->target_volume) {
      layer->volume -= layer->fade_speed * dt;
      if (layer->volume < layer->target_volume)
        layer->volume = layer->target_volume;
    }
  }
}

void music_set_intensity(MusicSystem *sys, IntensityLevel level) {
  sys->current_intensity = level;

  // Define mixing rules based on intensity
  switch (level) {
  case INTENSITY_CALM:
    sys->stems[0].target_volume = 1.0f; // Base Pad
    sys->stems[1].target_volume = 0.0f; // Drums
    sys->stems[2].target_volume = 0.2f; // Soft Melody
    sys->stems[3].target_volume = 0.0f; // Action
    break;

  case INTENSITY_EXPLORE:
    sys->stems[0].target_volume = 1.0f;
    sys->stems[1].target_volume = 0.5f; // Light Drums
    sys->stems[2].target_volume = 0.8f; // Melody
    sys->stems[3].target_volume = 0.0f;
    break;

  case INTENSITY_COMBAT:
    sys->stems[0].target_volume = 0.8f;
    sys->stems[1].target_volume = 1.0f; // Heavy Drums
    sys->stems[2].target_volume = 0.5f;
    sys->stems[3].target_volume = 1.0f; // Action Lead
    break;

  case INTENSITY_BOSS:
    // All faders max
    for (int i = 0; i < 4; i++)
      sys->stems[i].target_volume = 1.0f;
    break;
  }
}

/**
 * @brief Syncs clip triggering to musical beats.
 */
void music_process_metronome(MusicSystem *sys, float dt) {
  float sec_per_beat = 60.0f / sys->bpm;
  sys->beat_timer += dt;

  if (sys->beat_timer >= sec_per_beat) {
    sys->beat_timer -= sec_per_beat;
    // On Beat Event
    // ... trigger quantinzed phrases ...
  }
}
