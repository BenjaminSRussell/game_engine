#include "../Public/unified_audio.h"
#include "audio_types.h"
#include <string.h>

// We need the miniaudio type definitions
#include "miniaudio_stub.h"

static u32 find_free_slot(AudioSystem *system) {
  for (u32 i = 0; i < system->max_sources; i++) {
    if (!system->sources[i].active)
      return i;
  }
  // Simple stealing: Find oldest or finished? For now, just return failure
  return 0xFFFFFFFF;
}

u32 audio_play_sound_2d(AudioSystem *system, const char *filepath, f32 volume,
                        bool loop, AudioCategory category) {
  if (!system || !filepath)
    return 0xFFFFFFFF;

  u32 id = find_free_slot(system);
  if (id == 0xFFFFFFFF)
    return 0xFFFFFFFF;

  ma_sound *sound = UNIFIED_ALLOC(sizeof(ma_sound));
  // Flags for 2D/Stream?
  ma_result result = ma_sound_init_from_file(
      (ma_engine *)system->engine, filepath,
      MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, sound);

  if (result != MA_SUCCESS) {
    LOG_ERROR(LOG_CAT_AUDIO, "Failed to load sound: %s", filepath);
    UNIFIED_FREE(sound);
    return 0xFFFFFFFF;
  }

  AudioSourceInternal *src = &system->sources[id];
  src->id = id; // use index as ID for simplicity
  src->active = true;
  src->looping = loop;
  src->category = category;
  src->volume = volume;
  src->backend_handle = sound;

  // Apply initial settings
  f32 final_vol =
      volume * system->master_volume * system->category_volumes[category];
  ma_sound_set_volume(sound, final_vol);
  ma_sound_set_looping(sound, loop);
  ma_sound_start(sound);

  return id;
}

u32 audio_play_sound_3d(AudioSystem *system, const char *filepath,
                        Vec3 position, f32 volume, bool loop,
                        AudioCategory category) {
  u32 id = audio_play_sound_2d(system, filepath, volume, loop, category);
  if (id != 0xFFFFFFFF) {
    audio_set_source_position(system, id, position);
  }
  return id;
}

void audio_stop_sound(AudioSystem *system, u32 id) {
  if (!system || id >= system->max_sources)
    return;
  if (system->sources[id].active) {
    ma_sound *snd = (ma_sound *)system->sources[id].backend_handle;
    if (snd) {
      ma_sound_stop(snd);
      ma_sound_uninit(snd);
      UNIFIED_FREE(snd);
    }
    system->sources[id].active = false;
    system->sources[id].backend_handle = NULL;
  }
}

void audio_set_source_position(AudioSystem *system, u32 id, Vec3 position) {
  if (!system || id >= system->max_sources || !system->sources[id].active)
    return;

  AudioSourceInternal *src = &system->sources[id];
  src->position = position;
  ma_sound *snd = (ma_sound *)src->backend_handle;
  if (snd) {
    ma_sound_set_position(snd, position.x, position.y, position.z);
  }
}
