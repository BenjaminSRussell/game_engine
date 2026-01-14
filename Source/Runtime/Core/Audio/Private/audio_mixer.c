#include "../Public/unified_audio.h"
#include "audio_types.h"
#include "miniaudio_stub.h"

void audio_set_master_volume(AudioSystem *system, f32 volume) {
  if (!system)
    return;
  system->master_volume = volume;
  ma_engine_set_volume((ma_engine *)system->engine, volume);
}

void audio_set_category_volume(AudioSystem *system, AudioCategory category,
                               f32 volume) {
  if (!system || category >= AUDIO_CATEGORY_COUNT)
    return;
  system->category_volumes[category] = volume;

  // Update active sources
  for (u32 i = 0; i < system->max_sources; i++) {
    if (system->sources[i].active && system->sources[i].category == category) {
      f32 final = system->sources[i].volume * system->master_volume * volume;
      ma_sound_set_volume((ma_sound *)system->sources[i].backend_handle, final);
    }
  }
}

void audio_set_listener(AudioSystem *system, Vec3 position, Vec3 forward,
                        Vec3 up, Vec3 velocity) {
  if (!system)
    return;

  system->listener_pos = position;
  system->listener_forward = forward;
  system->listener_up = up;

  ma_engine_listener_set_position((ma_engine *)system->engine, 0, position.x,
                                  position.y, position.z);
  ma_engine_listener_set_direction((ma_engine *)system->engine, 0, forward.x,
                                   forward.y, forward.z);
  ma_engine_listener_set_world_up((ma_engine *)system->engine, 0, up.x, up.y,
                                  up.z);
  ma_engine_listener_set_velocity((ma_engine *)system->engine, 0, velocity.x,
                                  velocity.y, velocity.z);
}
