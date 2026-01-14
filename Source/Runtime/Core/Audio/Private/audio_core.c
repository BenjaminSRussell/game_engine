#include "../Public/unified_audio.h"
#include "audio_types.h"
#include <stdlib.h>
#include <string.h>

// Miniaudio Backend
// #define MINIAUDIO_IMPLEMENTATION // Stub is header-only
#include "miniaudio_stub.h"
// Adjust path to where miniaudio.h actually resides in the project.
// Based on file list, it wasn't explicitly listed in root of audio, but
// audio_system.c included <audio/audio_system.h>. Wait, looking at
// audio_system.c line 174: #define MINIAUDIO_IMPLEMENTATION It doesn't show
// #include "miniaudio.h", it might be inside audio_system.h or implicitly
// available. I will check the project include paths. Typically it's a single
// header library. For this step I will assume it's available in the include
// path or I will standard include it. Actually, I'll search for miniaudio.h
// first to be safe, but for now I'll write the logic assuming standard include.

// To avoid duplicate symbols if other files use it, we define IMPLEMENTATION
// here. But we must be careful if the legacy system is still compiled.

// ... Logic to be written ...
// Since I cannot verify the exact location of miniaudio.h right now without a
// search, I will assume it is in `src/engine/include/audio/` or similar. I'll
// proceed keeping the include generic and fix if verification fails.
// Correction: I'll try to find it first.

AudioSystem *audio_system_create(AudioConfig config) {
  AudioSystem *system = UNIFIED_ALLOC(sizeof(AudioSystem));
  if (!system)
    return NULL;

  memset(system, 0, sizeof(AudioSystem));
  system->config = config;
  system->max_sources = config.max_channels > 0 ? config.max_channels : 32;
  system->master_volume = 1.0f;

  for (int i = 0; i < AUDIO_CATEGORY_COUNT; ++i)
    system->category_volumes[i] = 1.0f;

  system->sources =
      UNIFIED_ALLOC(sizeof(AudioSourceInternal) * system->max_sources);
  memset(system->sources, 0, sizeof(AudioSourceInternal) * system->max_sources);

  // Miniaudio Init
  ma_engine_config engineConfig = ma_engine_config_init();
  engineConfig.channels = 2;
  engineConfig.sampleRate = config.sample_rate > 0 ? config.sample_rate : 44100;

  ma_engine *engine = UNIFIED_ALLOC(sizeof(ma_engine));
  if (ma_engine_init(&engineConfig, engine) != MA_SUCCESS) {
    LOG_ERROR(LOG_CAT_AUDIO, "Failed to initialize Miniaudio engine");
    UNIFIED_FREE(system->sources);
    UNIFIED_FREE(system);
    UNIFIED_FREE(engine);
    return NULL;
  }

  system->engine = engine;

  LOG_INFO(LOG_CAT_AUDIO, "Audio System Initialized");
  return system;
}

void audio_system_destroy(AudioSystem *system) {
  if (!system)
    return;

  if (system->engine) {
    ma_engine_uninit((ma_engine *)system->engine);
    UNIFIED_FREE(system->engine);
  }

  UNIFIED_FREE(system->sources);
  UNIFIED_FREE(system);
  LOG_INFO(LOG_CAT_AUDIO, "Audio System Destroyed");
}

void audio_system_update(AudioSystem *system, f32 delta_time) {
  (void)delta_time;
  if (!system)
    return;

  // Cleanup stopped sounds
  for (u32 i = 0; i < system->max_sources; i++) {
    if (system->sources[i].active) {
      ma_sound *snd = (ma_sound *)system->sources[i].backend_handle;
      if (snd && !ma_sound_is_playing(snd)) {
        // Check if it finished naturally
        ma_sound_uninit(snd);
        UNIFIED_FREE(snd);
        system->sources[i].active = false;
        system->sources[i].backend_handle = NULL;
      }
    }
  }
}
