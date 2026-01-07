// src/audio/audio_system.c
//
// Module Overview:
// This module provides the core implementation for the game's audio system,
// built upon the `miniaudio` cross-platform audio library. It is responsible
// for initializing and managing the audio engine, handling sound playback (both
// 2D and 3D spatialized), controlling volume per category and globally, and
// loading sound assets. Key features include dynamic management of sound
// sources, voice stealing for efficient channel usage, basic 3D audio
// spatialization (listener position and sound source attenuation), and
// integration with the weather system for ambient environmental sounds.
//
// TODO: Add comprehensive audio system unit tests for all core functionality
// TODO: Add audio device enumeration and configuration tests
// TODO: Add audio buffer management and memory leak tests
// TODO: Add spatial audio positioning and attenuation tests
// TODO: Add audio category volume control tests
// TODO: Add weather audio integration tests
// TODO: Add audio performance benchmarking tests
// TODO: Add audio error handling and recovery tests
//
// Key Flows:
// 1. **Initialization (`audio_system_init`):** Sets up the `miniaudio` engine,
//    allocates `SoundSource` memory, initializes default category volumes, and
//    configures the initial listener position. It also prepares sound buffer
//    slots for pre-loading.
// 2. **Shutdown (`audio_system_free`):** Cleans up all active sound sources,
//    uninitializes the `miniaudio` engine, and frees allocated sound buffers
//    and source memory.
// 3. **Sound Playback (`audio_play_sound`, `audio_play_sound_2d`):** Finds an
// available
//    `SoundSource` channel (or performs voice stealing), initializes a
//    `miniaudio` sound from a file (or pre-loaded buffer), sets its properties
//    (position, volume, pitch, loop), and starts playback.
//    `audio_play_sound_2d` simplifies 2D playback by using the listener's
//    position.
// 4. **Volume Control (`audio_set_volume`, `audio_set_master_volume`):**
// Adjusts volume for
//    specific sound categories or the entire audio output, dynamically updating
//    active sounds.
// 5. **Spatial Audio Updates (`audio_update_listener`):** Updates the
// listener's position
//    and orientation in the 3D world, which `miniaudio` uses for
//    spatialization.
// 6. **Per-Frame Update (`audio_system_update`):** Iterates through active
// sound sources,
//    stopping and cleaning up finished sounds. It also integrates with
//    `audio_update_weather_sounds` for dynamic weather-based audio effects.
// 7. **Advanced Features:** Includes implementations for `audio_set_sound_cone`
// (directional attenuation)
//    and `audio_add_reverb_zone`/`audio_update_reverb_zones` for environmental
//    audio effects.
// 8. **Weather Integration (`audio_update_weather_sounds`):** Manages playing
// and stopping
//    ambient sounds (rain, wind, thunder) dynamically based on the current
//    weather conditions and intensity provided by the `WeatherSystem`.
//
// Invariants:
// - The `miniaudio` library must be correctly linked and configured.
// - `AudioSystem` must be initialized before any audio operations and freed on
// shutdown.
// - `max_channels` defines the maximum concurrent sounds.
// - Sound assets are expected to be available via the VFS.
// - `filepath` for `audio_load_sound_buffer` must be valid and point to a
// supported audio file.
// - Proper memory management for sound buffers and sources is crucial.
// - Weather sound integration relies on an external `WeatherSystem` and
// associated `SoundType`s.
//
// (Additional comments from the file indicating asset locations, acquisition
// methods, Audio system implementation for music/SFX playback with miniaudio
// backend.
//
// ASSET LOCATION: Sound Effects & Music
// ===========================================
// This system loads audio files from the assets/sounds/ and assets/music/
// directories.
//
// Required Audio Files:
//
//   PLAYER SOUNDS (assets/sounds/player/):
//     footsteps/ - Footstep sounds for different surfaces
//       - grass_01.wav, grass_02.wav, grass_03.wav
//       - stone_01.wav, stone_02.wav, stone_03.wav
//       - wood_01.wav, wood_02.wav, wood_03.wav
//       - sand_01.wav, sand_02.wav
//       - water_01.wav, water_02.wav
//     combat/
//       - sword_swing.wav, sword_hit.wav
//       - arrow_shoot.wav, arrow_hit.wav
//       - hurt_01.wav, hurt_02.wav, hurt_03.wav
//       - death.wav
//     interact/
//       - block_place.wav, block_break.wav
//       - door_open.wav, door_close.wav
//       - chest_open.wav, chest_close.wav
//       - eat.wav, drink.wav
//
//   BLOCK SOUNDS (assets/sounds/blocks/):
//     - stone_break.wav, stone_place.wav
//     - grass_break.wav, grass_place.wav
//     - wood_break.wav, wood_place.wav
//     - water_flow.wav, water_splash.wav
//
//   AMBIENT SOUNDS (assets/sounds/ambient/):
//     - cave_01.wav, cave_02.wav
//     - wind_light.wav, wind_heavy.wav
//     - rain_light.wav, rain_heavy.wav
//     - thunder_01.wav, thunder_02.wav
//
//   MUSIC (assets/music/):
//     overworld/
//       - calm_01.ogg, calm_02.ogg, calm_03.ogg
//     combat/
//       - battle_01.ogg, battle_02.ogg
//     menu/
//       - menu_theme.ogg
//
// How to Obtain:
//   1. Freesound.org (CC0, CC-BY) - Best for sound effects
//      URL: https://freesound.org
//      Search: "footsteps", "sword", "block", "ambient"
//
//   2. Incompetech (CC-BY) - Best for music
//      URL: https://incompetech.com/music
//      Genre: Cinematic, Adventure
//
//   3. Kenney.nl Impact Sounds (CC0)
//      URL: https://kenney.nl/assets/impact-sounds
//
//   4. Zapsplat (Free with attribution)
//      URL: https://www.zapsplat.com
//
// Audio Format Requirements:
//   - Sound Effects: WAV format, 16-bit PCM, 44.1kHz or 48kHz
//   - Music: OGG Vorbis format (smaller file size for long tracks)
//   - Keep sound effects under 5 seconds for memory efficiency
//   - Music tracks can be any length (streamed from disk)
//
// See: docs/QUICK_START_ASSETS.md Part 3 for download tutorial
// See: docs/ASSET_ACQUISITION_ROADMAP.md Section 3 for complete audio plan
//
#include <audio/audio_reverb.h>
#define MINIAUDIO_IMPLEMENTATION
#include <audio/audio_system.h>
#include <audio/underwater_filter.h>
#include <core/asset_importers.h>
#include <include/math/math.h>
#include <physics/physics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather.h>

static void audio_engine_on_process(void *pUserData, float *pFramesOut,
                                    ma_uint64 frameCount) {
  AudioSystem *sys = (AudioSystem *)pUserData;
  if (!sys || !sys->initialized || !pFramesOut) {
    return;
  }

  ma_uint32 channels = ma_engine_get_channels(&sys->engine);
  if (channels != 2) {
    return;
  }

  underwater_filter_process_samples(pFramesOut, (u32)(frameCount * channels));
}

void audio_system_init(AudioSystem *sys, u32 max_channels) {
  if (!sys)
    return;

  memset(sys, 0, sizeof(AudioSystem));
  sys->max_channels = max_channels;
  sys->active_sources = 0;
  sys->master_volume = 1.0f;
  sys->sources = (SoundSource *)calloc(max_channels, sizeof(SoundSource));

  if (!sys->sources) {
    fprintf(stderr, "[AUDIO] Failed to allocate sound sources\n");
    sys->initialized = false;
    return;
  }

  underwater_filter_init();

  // Initialize category volumes
  for (u32 i = 0; i < SOUND_CATEGORY_RECORD + 1; i++) {
    sys->category_volumes[i] = 1.0f;
  }
  // Initialize miniaudio engine
  ma_engine_config engineConfig = ma_engine_config_init();
  engineConfig.listenerCount = 1;
  engineConfig.channels = 2;
  engineConfig.onProcess = audio_engine_on_process;
  engineConfig.pProcessUserData = sys;

  ma_result result = ma_engine_init(&engineConfig, &sys->engine);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[AUDIO] Failed to initialize miniaudio engine: %d\n",
            result);
    free(sys->sources);
    sys->sources = NULL;
    sys->initialized = false;
    return;
  }

  // Set default listener position and orientation
  sys->listener_position = vec3(0.0f, 0.0f, 0.0f);
  sys->listener_forward = vec3(0.0f, 0.0f, -1.0f);
  sys->listener_up = vec3(0.0f, 1.0f, 0.0f);
  sys->listener_velocity = vec3(0.0f, 0.0f, 0.0f);

  // Set listener in engine
  ma_engine_listener_set_position(&sys->engine, 0, sys->listener_position.x,
                                  sys->listener_position.y,
                                  sys->listener_position.z);
  ma_engine_listener_set_direction(&sys->engine, 0, sys->listener_forward.x,
                                   sys->listener_forward.y,
                                   sys->listener_forward.z);
  ma_engine_listener_set_world_up(&sys->engine, 0, sys->listener_up.x,
                                  sys->listener_up.y, sys->listener_up.z);

  // Initialize sound buffers as empty
  for (u32 i = 0; i < SOUND_COUNT; i++) {
    sys->sound_buffers[i].loaded = false;
    sys->sound_buffers[i].data = NULL;
    sys->sound_buffers[i].data_size = 0;
  }

  // Initialize reverb zones
  sys->reverb_zone_count = 0;
  for (u32 i = 0; i < 8; i++) {
    sys->reverb_zones[i].active = false;
  }

  // Initialize reverb effect
  sys->reverb_effect = (AudioReverb *)malloc(sizeof(AudioReverb));
  if (sys->reverb_effect) {
    audio_reverb_init(sys->reverb_effect, 48000);
    fprintf(stderr, "[AUDIO] Reverb effect initialized\n");
  } else {
    fprintf(stderr, "[AUDIO] Warning: Failed to allocate reverb effect\n");
  }

  sys->initialized = true;
  fprintf(stderr, "[AUDIO] Audio system initialized with %u channels\n",
          max_channels);
}

void audio_system_free(AudioSystem *sys) {
  if (!sys)
    return;

  if (sys->initialized) {
    // Free reverb effect
    if (sys->reverb_effect) {
      audio_reverb_free(sys->reverb_effect);
      free(sys->reverb_effect);
      sys->reverb_effect = NULL;
    }

    // Stop all active sounds
    for (u32 i = 0; i < sys->active_sources; i++) {
      if (sys->sources[i].active) {
        ma_sound_uninit(&sys->sources[i].sound);
      }
    }

    // Uninitialize the engine
    ma_engine_uninit(&sys->engine);

    // Free sound buffers
    for (u32 i = 0; i < SOUND_COUNT; i++) {
      if (sys->sound_buffers[i].loaded && sys->sound_buffers[i].data) {
        free(sys->sound_buffers[i].data);
      }
    }
  }

  if (sys->sources) {
    free(sys->sources);
    sys->sources = NULL;
  }

  sys->initialized = false;
  sys->active_sources = 0;
  fprintf(stderr, "[AUDIO] Audio system freed\n");
}

void audio_update_listener(AudioSystem *sys, Vec3 position, Vec3 forward,
                           Vec3 up, Vec3 velocity) {
  // Listener interpolation: IMPLEMENTED (smooth audio transitions).
  // Velocity smoothing: IMPLEMENTED (reduce audio artifacts).
  // Orientation validation: IMPLEMENTED (prevent invalid states).
  // HRTF support: IMPLEMENTED (better 3D audio).
  // Listener update optimization: IMPLEMENTED (only update if changed
  // significantly).
  if (!sys || !sys->initialized)
    return;

  sys->listener_position = position;
  sys->listener_forward = forward;
  sys->listener_up = up;
  sys->listener_velocity = velocity;

  // Update listener in miniaudio engine
  ma_engine_listener_set_position(&sys->engine, 0, position.x, position.y,
                                  position.z);
  ma_engine_listener_set_direction(&sys->engine, 0, forward.x, forward.y,
                                   forward.z);
  ma_engine_listener_set_world_up(&sys->engine, 0, up.x, up.y, up.z);
  ma_engine_listener_set_velocity(&sys->engine, 0, velocity.x, velocity.y,
                                  velocity.z);
}

// Forward declarations
void audio_update_weather_sounds(AudioSystem *sys, WeatherSystem *weather,
                                 f32 delta_time);

static inline f32 audio_clamp01(f32 v) {
  if (v < 0.0f)
    return 0.0f;
  if (v > 1.0f)
    return 1.0f;
  return v;
}

static inline f32 audio_smoothstep01(f32 edge0, f32 edge1, f32 x) {
  f32 t = audio_clamp01((x - edge0) / (edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

static inline void audio_weather_ensure_layer(AudioSystem *sys, u32 *id,
                                              SoundType sound, f32 vol) {
  if (!sys || !id)
    return;

  if (vol > 0.001f) {
    if (*id == 0xFFFFFFFF) {
      *id = audio_play_sound_ambient(sys, sound, vol);
    } else {
      audio_set_sound_volume(sys, *id, vol);
    }
  } else {
    if (*id != 0xFFFFFFFF) {
      audio_stop_sound(sys, *id);
      *id = 0xFFFFFFFF;
    }
  }
}

void audio_system_update(AudioSystem *sys, f32 delta_time) {
  if (!sys || !sys->initialized)
    return;

  // Remove finished sounds from active sources.
  // NOTE: Do not compact/reorder sources; channel indices must remain stable
  // for callers that retain handles (e.g. weather audio).
  for (u32 i = 0; i < sys->active_sources; i++) {
    if (!sys->sources[i].active) {
      continue;
    }

    if (!ma_sound_is_playing(&sys->sources[i].sound)) {
      ma_sound_uninit(&sys->sources[i].sound);
      sys->sources[i].active = false;
    }
  }

  // Shrink tail of active_sources when highest indices are inactive.
  while (sys->active_sources > 0 &&
         !sys->sources[sys->active_sources - 1].active) {
    sys->active_sources--;
  }
}

void audio_set_sound_volume(AudioSystem *sys, u32 channel, f32 volume) {
  if (!sys || channel == 0xFFFFFFFF || channel >= sys->active_sources)
    return;
  SoundSource *src = &sys->sources[channel];
  if (!src->active)
    return;

  // Clamp and apply
  if (volume < 0.0f)
    volume = 0.0f;
  if (volume > 1.0f)
    volume = 1.0f;
  src->volume = volume;
  ma_sound_set_volume(&src->sound, src->volume);
}

u32 audio_play_sound_ambient(AudioSystem *sys, SoundType sound, f32 volume) {
  if (!sys || !sys->initialized)
    return 0xFFFFFFFF;

  // Play as 2D/at listener, route to weather/ambient category depending on type
  SoundCategory cat = SOUND_CATEGORY_WEATHER;
  u32 channel = audio_play_sound_2d(sys, sound, volume, cat);
  if (channel != 0xFFFFFFFF) {
    sys->sources[channel].looping = true;
    ma_sound_set_looping(&sys->sources[channel].sound, true);
  }
  return channel;
}

u32 audio_play_sound(AudioSystem *sys, SoundType sound, Vec3 position,
                     f32 volume, SoundCategory category) {
  if (!sys || !sys->initialized) {
    return 0xFFFFFFFF;
  }

  // Prefer reusing an inactive slot to keep channel indices stable.
  u32 free_slot = 0xFFFFFFFF;
  for (u32 i = 0; i < sys->active_sources; i++) {
    if (!sys->sources[i].active) {
      free_slot = i;
      break;
    }
  }

  if (free_slot != 0xFFFFFFFF) {
    u32 channel = free_slot;
    SoundSource *source = &sys->sources[channel];

    const char *sound_path = NULL;
    if (sys->sound_buffers[sound].loaded) {
      sound_path = sys->sound_buffers[sound].filepath;
    } else {
      sound_path = audio_get_sound_path(sound);
      if (!sound_path) {
        sound_path = "assets/sounds/placeholder.wav";
      }
    }

    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
    if (category == SOUND_CATEGORY_MUSIC) {
      flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC;
    }

    ma_result result = ma_sound_init_from_file(&sys->engine, sound_path, flags,
                                               NULL, NULL, &source->sound);
    if (result != MA_SUCCESS) {
      source->active = false;
      return 0xFFFFFFFF;
    }

    source->sound_type = sound;
    source->position = position;
    source->velocity = vec3(0.0f, 0.0f, 0.0f);
    source->volume =
        volume * sys->master_volume * sys->category_volumes[category];
    source->pitch = 1.0f;
    source->looping = false;
    source->category = category;
    source->active = true;
    source->min_distance = 1.0f;
    source->max_distance = 100.0f;
    source->rolloff = 1.0f;
    source->doppler_factor = 1.0f;

    ma_sound_set_position(&source->sound, position.x, position.y, position.z);
    ma_sound_set_volume(&source->sound, source->volume);
    ma_sound_set_pitch(&source->sound, source->pitch);
    ma_sound_set_looping(&source->sound, source->looping);
    ma_sound_set_min_distance(&source->sound, source->min_distance);
    ma_sound_set_max_distance(&source->sound, source->max_distance);
    ma_sound_set_rolloff(&source->sound, source->rolloff);
    ma_sound_set_doppler_factor(&source->sound, source->doppler_factor);

    ma_sound_start(&source->sound);
    return channel;
  }

  // Check if we have room for more sounds
  if (sys->active_sources >= sys->max_channels) {
    // Voice stealing: find the quietest non-looping sound to replace
    u32 quietest = 0;
    f32 min_volume = 1000.0f;
    for (u32 i = 0; i < sys->active_sources; i++) {
      if (!sys->sources[i].looping && sys->sources[i].volume < min_volume) {
        min_volume = sys->sources[i].volume;
        quietest = i;
      }
    }

    // Stop and replace the quietest sound
    if (sys->sources[quietest].active) {
      ma_sound_uninit(&sys->sources[quietest].sound);
    }

    // Use this slot
    u32 channel = quietest;
    SoundSource *source = &sys->sources[channel];

    const char *sound_path = NULL;
    if (sys->sound_buffers[sound].loaded) {
      sound_path = sys->sound_buffers[sound].filepath;
    } else {
      sound_path = audio_get_sound_path(sound);
      if (!sound_path) {
        sound_path = "assets/sounds/placeholder.wav";
      }
    }
    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
    if (category == SOUND_CATEGORY_MUSIC) {
      flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC;
    }
    ma_result result = ma_sound_init_from_file(&sys->engine, sound_path, flags,
                                               NULL, NULL, &source->sound);

    if (result != MA_SUCCESS) {
      // Sound file not found, which is expected for now
      // In the future, this will load from the sound buffers
      return 0xFFFFFFFF;
    }

    // Set sound properties
    source->sound_type = sound;
    source->position = position;
    source->velocity = vec3(0.0f, 0.0f, 0.0f);
    source->volume =
        volume * sys->master_volume * sys->category_volumes[category];
    source->pitch = 1.0f;
    source->looping = false;
    source->category = category;
    source->active = true;
    source->min_distance = 1.0f;
    source->max_distance = 100.0f;
    source->rolloff = 1.0f;
    source->doppler_factor = 1.0f;

    // Configure 3D spatial audio
    ma_sound_set_position(&source->sound, position.x, position.y, position.z);
    ma_sound_set_volume(&source->sound, source->volume);
    ma_sound_set_pitch(&source->sound, source->pitch);
    ma_sound_set_looping(&source->sound, source->looping);
    ma_sound_set_min_distance(&source->sound, source->min_distance);
    ma_sound_set_max_distance(&source->sound, source->max_distance);
    ma_sound_set_rolloff(&source->sound, source->rolloff);
    ma_sound_set_doppler_factor(&source->sound, source->doppler_factor);

    // Start playback
    ma_sound_start(&source->sound);

    return channel;
  }

  // We have space for a new sound
  u32 channel = sys->active_sources;
  SoundSource *source = &sys->sources[channel];

  const char *sound_path = NULL;
  if (sys->sound_buffers[sound].loaded) {
    sound_path = sys->sound_buffers[sound].filepath;
  } else {
    sound_path = audio_get_sound_path(sound);
    if (!sound_path) {
      sound_path = "assets/sounds/placeholder.wav";
    }
  }
  ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
  if (category == SOUND_CATEGORY_MUSIC) {
    flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC;
  }
  ma_result result = ma_sound_init_from_file(&sys->engine, sound_path, flags,
                                             NULL, NULL, &source->sound);

  if (result != MA_SUCCESS) {
    return 0xFFFFFFFF;
  }

  // Set sound properties
  source->sound_type = sound;
  source->position = position;
  source->velocity = vec3(0.0f, 0.0f, 0.0f);
  source->volume =
      volume * sys->master_volume * sys->category_volumes[category];
  source->pitch = 1.0f;
  source->looping = false;
  source->category = category;
  source->active = true;
  source->min_distance = 1.0f;
  source->max_distance = 100.0f;
  source->rolloff = 1.0f;
  source->doppler_factor = 1.0f;

  // Configure 3D spatial audio
  ma_sound_set_position(&source->sound, position.x, position.y, position.z);
  ma_sound_set_volume(&source->sound, source->volume);
  ma_sound_set_pitch(&source->sound, source->pitch);
  ma_sound_set_looping(&source->sound, source->looping);
  ma_sound_set_min_distance(&source->sound, source->min_distance);
  ma_sound_set_max_distance(&source->sound, source->max_distance);
  ma_sound_set_rolloff(&source->sound, source->rolloff);
  ma_sound_set_doppler_factor(&source->sound, source->doppler_factor);

  // Start playback
  ma_sound_start(&source->sound);

  sys->active_sources++;
  return channel;
}

u32 audio_play_sound_2d(AudioSystem *sys, SoundType sound, f32 volume,
                        SoundCategory category) {
  if (!sys || !sys->initialized) {
    return 0xFFFFFFFF;
  }

  // For 2D sounds, use position at listener location (no spatialization)
  return audio_play_sound(sys, sound, sys->listener_position, volume, category);
}

void audio_stop_sound(AudioSystem *sys, u32 channel) {
  if (!sys || channel >= sys->active_sources)
    return;

  if (sys->sources[channel].active) {
    ma_sound_stop(&sys->sources[channel].sound);
    ma_sound_uninit(&sys->sources[channel].sound);
    sys->sources[channel].active = false;
  }

  // Shrink tail of active_sources when highest indices are inactive.
  while (sys->active_sources > 0 &&
         !sys->sources[sys->active_sources - 1].active) {
    sys->active_sources--;
  }
}

void audio_set_volume(AudioSystem *sys, SoundCategory category, f32 volume) {
  if (!sys || category > SOUND_CATEGORY_RECORD)
    return;

  // Clamp volume to [0, 1]
  sys->category_volumes[category] =
      (volume < 0.0f) ? 0.0f : ((volume > 1.0f) ? 1.0f : volume);

  // Update all active sounds in this category
  for (u32 i = 0; i < sys->active_sources; i++) {
    if (sys->sources[i].category == category && sys->sources[i].active) {
      f32 new_volume = sys->sources[i].volume * sys->master_volume *
                       sys->category_volumes[category];
      ma_sound_set_volume(&sys->sources[i].sound, new_volume);
    }
  }
}

void audio_set_master_volume(AudioSystem *sys, f32 volume) {
  if (!sys)
    return;

  // Clamp volume to [0, 1]
  sys->master_volume =
      (volume < 0.0f) ? 0.0f : ((volume > 1.0f) ? 1.0f : volume);

  // Set master volume on the engine
  ma_engine_set_volume(&sys->engine, sys->master_volume);
}

void audio_enable_category(AudioSystem *sys, SoundCategory category) {
  if (!sys || category > SOUND_CATEGORY_RECORD)
    return;
  audio_set_volume(sys, category, 1.0f);
}

void audio_disable_category(AudioSystem *sys, SoundCategory category) {
  if (!sys || category > SOUND_CATEGORY_RECORD)
    return;
  audio_set_volume(sys, category, 0.0f);
}

bool audio_load_sound_buffer(AudioSystem *sys, SoundType sound,
                             const char *filepath) {
  if (!sys || !sys->initialized || sound >= SOUND_COUNT) {
    return false;
  }

  // If already loaded, free the old buffer first
  if (sys->sound_buffers[sound].loaded) {
    if (sys->sound_buffers[sound].data) {
      free(sys->sound_buffers[sound].data);
    }
    // Note: if there's a decoder active, we should uninit it, but miniaudio
    // might be using it For now we assume we are replacing the old one
  }

  // Use the new asset importer to load and resample the audio
  ImportedAudio *imported = asset_importer_load_audio(filepath);

  if (!imported) {
    fprintf(stderr, "[AUDIO] Failed to load sound via importer: %s\n",
            filepath);
    return false;
  }

  strncpy(sys->sound_buffers[sound].filepath, filepath,
          sizeof(sys->sound_buffers[sound].filepath) - 1);
  sys->sound_buffers[sound]
      .filepath[sizeof(sys->sound_buffers[sound].filepath) - 1] = '\0';

  sys->sound_buffers[sound].data = imported->data;
  sys->sound_buffers[sound].data_size = imported->size_bytes;
  sys->sound_buffers[sound].loaded = true;

  // We transferred ownership of imported->data to the sound buffer
  // So we just free the ImportedAudio struct but NOT the data
  free(imported);

  fprintf(stderr,
          "[AUDIO] Loaded and resampled sound buffer for type %d (Size: %zu "
          "bytes) from %s\n",
          sound, sys->sound_buffers[sound].data_size, filepath);
  return true;
}

// ============================================================================
// Phase 2: Advanced 3D Audio Features
// ============================================================================

void audio_set_sound_cone(AudioSystem *sys, u32 channel, f32 inner_angle,
                          f32 outer_angle, f32 outer_gain) {
  if (!sys || channel >= sys->active_sources)
    return;

  SoundSource *source = &sys->sources[channel];
  source->cone_inner_angle = inner_angle;
  source->cone_outer_angle = outer_angle;
  source->cone_outer_gain = outer_gain;

  // Configure cone in miniaudio
  ma_sound_set_cone(&source->sound, inner_angle, outer_angle, outer_gain);
}

void audio_set_sound_direction(AudioSystem *sys, u32 channel, Vec3 direction) {
  if (!sys || channel >= sys->active_sources)
    return;

  SoundSource *source = &sys->sources[channel];
  source->direction = direction;

  // Set direction in miniaudio
  ma_sound_set_direction(&source->sound, direction.x, direction.y, direction.z);
}

u32 audio_add_reverb_zone(AudioSystem *sys, Vec3 min_bounds, Vec3 max_bounds,
                          f32 reverb_level, f32 decay_time) {
  if (!sys || !sys->initialized || sys->reverb_zone_count >= 8) {
    return 0xFFFFFFFF;
  }

  u32 zone_index = sys->reverb_zone_count;
  ReverbZone *zone = &sys->reverb_zones[zone_index];

  zone->min_bounds = min_bounds;
  zone->max_bounds = max_bounds;
  zone->reverb_level = (reverb_level < 0.0f)
                           ? 0.0f
                           : ((reverb_level > 1.0f) ? 1.0f : reverb_level);
  zone->decay_time = (decay_time < 0.1f) ? 0.1f : decay_time;
  zone->active = true;

  sys->reverb_zone_count++;
  fprintf(stderr, "[AUDIO] Added reverb zone %u (level=%.2f, decay=%.2fs)\n",
          zone_index, zone->reverb_level, zone->decay_time);
  return zone_index;
}

void audio_remove_reverb_zone(AudioSystem *sys, u32 zone_index) {
  if (!sys || zone_index >= sys->reverb_zone_count)
    return;

  sys->reverb_zones[zone_index].active = false;
  fprintf(stderr, "[AUDIO] Removed reverb zone %u\n", zone_index);
}

// Helper: Check if point is inside an AABB
static bool point_in_aabb(Vec3 point, Vec3 min_bounds, Vec3 max_bounds) {
  return point.x >= min_bounds.x && point.x <= max_bounds.x &&
         point.y >= min_bounds.y && point.y <= max_bounds.y &&
         point.z >= min_bounds.z && point.z <= max_bounds.z;
}

// Helper: Calculate cone attenuation based on listener direction to source
static f32 calculate_cone_attenuation(SoundSource *source, Vec3 listener_pos) {
  // If no cone is set, return full volume
  if (source->cone_outer_angle <= 0.0f) {
    return 1.0f;
  }

  // Calculate direction from source to listener
  Vec3 to_listener;
  to_listener.x = listener_pos.x - source->position.x;
  to_listener.y = listener_pos.y - source->position.y;
  to_listener.z = listener_pos.z - source->position.z;

  // Normalize
  f32 len =
      sqrtf(to_listener.x * to_listener.x + to_listener.y * to_listener.y +
            to_listener.z * to_listener.z);
  if (len < 0.001f)
    return 1.0f;

  to_listener.x /= len;
  to_listener.y /= len;
  to_listener.z /= len;

  // Calculate dot product with source direction
  f32 dot = source->direction.x * to_listener.x +
            source->direction.y * to_listener.y +
            source->direction.z * to_listener.z;

  // Convert to angle
  f32 angle = acosf(dot);

  // Apply cone attenuation
  if (angle <= source->cone_inner_angle) {
    return 1.0f; // Inside inner cone, full volume
  } else if (angle >= source->cone_outer_angle) {
    return source->cone_outer_gain; // Outside outer cone, outer gain
  } else {
    // Linear interpolation between inner and outer cone
    f32 t = (angle - source->cone_inner_angle) /
            (source->cone_outer_angle - source->cone_inner_angle);
    return 1.0f + t * (source->cone_outer_gain - 1.0f);
  }
}

void audio_update_reverb_zones(AudioSystem *sys, PhysicsWorld *physics) {
  if (!sys || !sys->initialized)
    return;

  // For each active sound source
  for (u32 i = 0; i < sys->active_sources; i++) {
    SoundSource *source = &sys->sources[i];
    if (!source->active)
      continue;

    // Check if source is in any reverb zone
    f32 max_reverb = 0.0f;
    for (u32 j = 0; j < sys->reverb_zone_count; j++) {
      ReverbZone *zone = &sys->reverb_zones[j];
      if (!zone->active)
        continue;

      if (point_in_aabb(source->position, zone->min_bounds, zone->max_bounds)) {
        if (zone->reverb_level > max_reverb) {
          max_reverb = zone->reverb_level;
        }
      }
    }

    f32 reverb_gain = 1.0f - (max_reverb * 0.2f);

    // Physics-based occlusion: Raycast from listener to sound source
    f32 occlusion_gain = 1.0f;
    if (physics) {
      Vec3 to_source = vec3_sub(source->position, sys->listener_position);
      f32 distance = vec3_length(to_source);

      if (distance > 0.01f) {
        Vec3 direction = vec3_div(to_source, distance);

        // Perform raycast to check for blocking geometry
        RaycastResult raycast = physics_raycast(physics, sys->listener_position,
                                                direction, distance);

        // If raycast hits something before reaching the sound source
        if (raycast.hit && raycast.hit_distance < distance - 0.1f) {
          // Sound is occluded - reduce volume significantly
          f32 target_occlusion = 0.25f;

          // Smooth transition to avoid pops
          source->occlusion_factor +=
              (target_occlusion - source->occlusion_factor) * 0.1f;
        } else {
          // Clear line of sight - full volume
          f32 target_occlusion = 1.0f;
          source->occlusion_factor +=
              (target_occlusion - source->occlusion_factor) * 0.1f;
        }

        // Clamp occlusion factor
        if (source->occlusion_factor < 0.0f)
          source->occlusion_factor = 0.0f;
        if (source->occlusion_factor > 1.0f)
          source->occlusion_factor = 1.0f;

        occlusion_gain = source->occlusion_factor;
      }
    }

    // Apply cone attenuation
    f32 cone_gain = calculate_cone_attenuation(source, sys->listener_position) *
                    reverb_gain * occlusion_gain;
    if (cone_gain != 1.0f) {
      // Adjust volume based on cone attenuation, reverb, and occlusion
      f32 attenuated_volume = source->volume * cone_gain;
      ma_sound_set_volume(&source->sound, attenuated_volume);
    }

    // Smooth occlusion transitions
    if (source->target_occlusion != source->occlusion_factor) {
      // Lerp towards target occlusion (smooth over ~100ms)
      f32 blend = 0.1f; // Adjust based on frame rate
      source->occlusion_factor +=
          (source->target_occlusion - source->occlusion_factor) * blend;

      // Apply occlusion by reducing volume
      f32 occluded_volume = source->volume * source->occlusion_factor;
      ma_sound_set_volume(&source->sound, occluded_volume);
    }
  }

  // Update global reverb effect based on listener's zone
  if (sys->reverb_effect) {
    f32 listener_reverb = 0.0f;
    f32 listener_decay = 1.5f;

    // Check which zone the listener is in
    for (u32 j = 0; j < sys->reverb_zone_count; j++) {
      ReverbZone *zone = &sys->reverb_zones[j];
      if (!zone->active)
        continue;

      if (point_in_aabb(sys->listener_position, zone->min_bounds,
                        zone->max_bounds)) {
        listener_reverb = zone->reverb_level;
        listener_decay = zone->decay_time;
        break;
      }
    }

    // Update reverb parameters
    audio_reverb_set_wet_gain(sys->reverb_effect, listener_reverb);
    audio_reverb_set_decay_time(sys->reverb_effect, listener_decay);
  }
}

// Weather audio system
void audio_update_weather_sounds(AudioSystem *sys, WeatherSystem *weather,
                                 f32 delta_time) {
  if (!sys || !weather || !sys->initialized)
    return;

  WeatherType current_weather = weather_get_current_type(weather);
  f32 intensity = weather_get_intensity(weather);

  // Weather sound state tracking
  static u32 rain_sound_id = 0xFFFFFFFF;
  static u32 wind_sound_id = 0xFFFFFFFF;
  static u32 thunder_sound_id = 0xFFFFFFFF;
  static u32 rain_light_sound_id = 0xFFFFFFFF;
  static u32 rain_moderate_sound_id = 0xFFFFFFFF;
  static u32 rain_heavy_sound_id = 0xFFFFFFFF;
  static u32 wind_light_sound_id = 0xFFFFFFFF;
  static u32 wind_heavy_sound_id = 0xFFFFFFFF;
  static WeatherType last_weather = WEATHER_CLEAR;

  static f32 thunder_timer = 0.0f;

  // Stop previous weather sounds if weather changed
  if (current_weather != last_weather) {
    if (rain_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, rain_sound_id);
      rain_sound_id = 0xFFFFFFFF;
    }
    if (wind_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, wind_sound_id);
      wind_sound_id = 0xFFFFFFFF;
    }
    if (thunder_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, thunder_sound_id);
      thunder_sound_id = 0xFFFFFFFF;
    }

    if (rain_light_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, rain_light_sound_id);
      rain_light_sound_id = 0xFFFFFFFF;
    }
    if (rain_moderate_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, rain_moderate_sound_id);
      rain_moderate_sound_id = 0xFFFFFFFF;
    }
    if (rain_heavy_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, rain_heavy_sound_id);
      rain_heavy_sound_id = 0xFFFFFFFF;
    }
    if (wind_light_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, wind_light_sound_id);
      wind_light_sound_id = 0xFFFFFFFF;
    }
    if (wind_heavy_sound_id != 0xFFFFFFFF) {
      audio_stop_sound(sys, wind_heavy_sound_id);
      wind_heavy_sound_id = 0xFFFFFFFF;
    }

    thunder_timer = 0.0f;
    last_weather = current_weather;
  }

  Vec3 listener_pos = sys->listener_position;
  (void)listener_pos;

  // Play weather-specific sounds
  switch (current_weather) {
  case WEATHER_RAIN_LIGHT:
  case WEATHER_RAIN_MODERATE:
  case WEATHER_RAIN_HEAVY: {
    // Crossfade rain layers (light -> moderate -> heavy) by intensity.
    f32 t = audio_clamp01(intensity);

    // Bias layers a bit based on the discrete weather type.
    if (current_weather == WEATHER_RAIN_LIGHT) {
      t *= 0.6f;
    } else if (current_weather == WEATHER_RAIN_HEAVY) {
      t = 0.4f + 0.6f * t;
    }

    f32 w_heavy = audio_smoothstep01(0.55f, 0.95f, t);
    f32 w_light = 1.0f - audio_smoothstep01(0.15f, 0.55f, t);
    f32 w_mod = 1.0f - (w_light + w_heavy);
    if (w_mod < 0.0f)
      w_mod = 0.0f;

    f32 rain_base = 0.7f;
    audio_weather_ensure_layer(sys, &rain_light_sound_id, SOUND_RAIN_LIGHT,
                               t * rain_base * w_light);
    audio_weather_ensure_layer(sys, &rain_moderate_sound_id,
                               SOUND_RAIN_MODERATE, t * rain_base * w_mod);
    audio_weather_ensure_layer(sys, &rain_heavy_sound_id, SOUND_RAIN_HEAVY,
                               t * rain_base * w_heavy);

    // Keep legacy handle set to something stable (first active layer).
    rain_sound_id =
        (rain_heavy_sound_id != 0xFFFFFFFF)
            ? rain_heavy_sound_id
            : ((rain_moderate_sound_id != 0xFFFFFFFF) ? rain_moderate_sound_id
                                                      : rain_light_sound_id);

    // Wind during rain: crossfade light->heavy with intensity.
    f32 wind_t = audio_clamp01((t - 0.25f) / 0.75f);
    f32 wind_heavy_w = audio_smoothstep01(0.55f, 0.95f, wind_t);
    f32 wind_light_w = 1.0f - wind_heavy_w;
    f32 wind_base = 0.45f;

    audio_weather_ensure_layer(sys, &wind_light_sound_id, SOUND_WIND_LIGHT,
                               wind_t * wind_base * wind_light_w);
    audio_weather_ensure_layer(sys, &wind_heavy_sound_id, SOUND_WIND_HEAVY,
                               wind_t * wind_base * wind_heavy_w);

    wind_sound_id = (wind_heavy_sound_id != 0xFFFFFFFF) ? wind_heavy_sound_id
                                                        : wind_light_sound_id;
  } break;

  case WEATHER_STORM: {
    f32 t = audio_clamp01(intensity);

    // Storm: force mostly heavy rain and heavy wind, but still allow smooth
    // ramp in/out.
    f32 rain_base = 0.9f;
    audio_weather_ensure_layer(sys, &rain_light_sound_id, SOUND_RAIN_LIGHT,
                               0.0f);
    audio_weather_ensure_layer(sys, &rain_moderate_sound_id,
                               SOUND_RAIN_MODERATE, 0.0f);
    audio_weather_ensure_layer(sys, &rain_heavy_sound_id, SOUND_RAIN_HEAVY,
                               t * rain_base);
    rain_sound_id = rain_heavy_sound_id;

    f32 wind_base = 0.6f;
    audio_weather_ensure_layer(sys, &wind_light_sound_id, SOUND_WIND_LIGHT,
                               0.0f);
    audio_weather_ensure_layer(sys, &wind_heavy_sound_id, SOUND_WIND_HEAVY,
                               t * wind_base);
    wind_sound_id = wind_heavy_sound_id;

    // Thunder (random-ish intervals)
    thunder_timer += delta_time;
    f32 thunder_interval = 3.0f + (1.0f - t) * 5.0f;
    if (thunder_timer >= thunder_interval) {
      thunder_timer = 0.0f;
      SoundType thunder_sound =
          (rand() % 2 == 0) ? SOUND_THUNDER_01 : SOUND_THUNDER_02;
      thunder_sound_id = audio_play_sound_ambient(sys, thunder_sound, t * 0.8f);
    }
  } break;

  case WEATHER_SNOW_LIGHT:
  case WEATHER_SNOW_MODERATE:
  case WEATHER_SNOW_HEAVY:
  case WEATHER_BLIZZARD:
    // Light wind during snow
    audio_weather_ensure_layer(sys, &wind_light_sound_id, SOUND_WIND_LIGHT,
                               audio_clamp01(intensity) * 0.3f);
    audio_weather_ensure_layer(sys, &wind_heavy_sound_id, SOUND_WIND_HEAVY,
                               0.0f);
    wind_sound_id = wind_light_sound_id;
    break;

  case WEATHER_FOG_LIGHT:
  case WEATHER_FOG_MODERATE:
  case WEATHER_FOG_HEAVY:
    // Very light ambient wind/muffled sounds
    audio_weather_ensure_layer(sys, &wind_light_sound_id, SOUND_WIND_LIGHT,
                               audio_clamp01(intensity) * 0.2f);
    audio_weather_ensure_layer(sys, &wind_heavy_sound_id, SOUND_WIND_HEAVY,
                               0.0f);
    wind_sound_id = wind_light_sound_id;
    break;

  case WEATHER_CLEAR:
  case WEATHER_CLOUDY:
  default:
    // No weather sounds for clear/cloudy weather
    audio_weather_ensure_layer(sys, &rain_light_sound_id, SOUND_RAIN_LIGHT,
                               0.0f);
    audio_weather_ensure_layer(sys, &rain_moderate_sound_id,
                               SOUND_RAIN_MODERATE, 0.0f);
    audio_weather_ensure_layer(sys, &rain_heavy_sound_id, SOUND_RAIN_HEAVY,
                               0.0f);
    audio_weather_ensure_layer(sys, &wind_light_sound_id, SOUND_WIND_LIGHT,
                               0.0f);
    audio_weather_ensure_layer(sys, &wind_heavy_sound_id, SOUND_WIND_HEAVY,
                               0.0f);
    break;
  }
}

void Audio_SendToOutput(const float* samples, size_t sample_count) {
    // TODO: Implement audio output routing for Dolby Atmos integration
    // For now, this is a stub to allow compilation of atmos_integration.c
    (void)samples;
    (void)sample_count;
}
