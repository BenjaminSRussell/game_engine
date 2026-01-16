#include "ecs/components/audio_components.h"
#include "ecs/ecs.h"
#include "engine/include/core/logger.h"
#include <string.h>

u32 g_audio_source_component_id = 0;
u32 g_audio_listener_component_id = 0;

void register_audio_components(void *world) {
  if (!world) {
    LOG_ERROR("[Audio] Cannot register components: null world");
    return;
  }

  // Register Source
  ComponentInfo source_info = {.name = "AudioSource",
                               .size = sizeof(AudioSourceComponent),
                               .alignment = alignof(AudioSourceComponent)};
  g_audio_source_component_id = ecs_register_component(world, &source_info);

  // Register Listener
  ComponentInfo listener_info = {.name = "AudioListener",
                                 .size = sizeof(AudioListenerComponent),
                                 .alignment = alignof(AudioListenerComponent)};
  g_audio_listener_component_id = ecs_register_component(world, &listener_info);

  LOG_INFO("[Audio] Registered components:");
  LOG_INFO("  AudioSource: ID %u", g_audio_source_component_id);
  LOG_INFO("  AudioListener: ID %u", g_audio_listener_component_id);
}

AudioSourceComponent audio_source_create_2d(u32 sound_id, f32 volume,
                                            bool loop) {
  AudioSourceComponent source = {0};
  source.sound_id = sound_id;
  source.volume = volume;
  source.pitch = 1.0f;
  source.is_3d = false;
  source.loop = loop;
  source.play_on_awake = true;
  source.is_playing = false;
  return source;
}

AudioSourceComponent audio_source_create_3d(u32 sound_id, f32 volume,
                                            f32 min_dist, f32 max_dist) {
  AudioSourceComponent source = {0};
  source.sound_id = sound_id;
  source.volume = volume;
  source.pitch = 1.0f;
  source.is_3d = true;
  source.loop = false;
  source.min_distance = min_dist;
  source.max_distance = max_dist;
  source.rolloff_factor = 1.0f;
  source.play_on_awake = true;
  source.is_playing = false;
  return source;
}

AudioListenerComponent audio_listener_create(void) {
  AudioListenerComponent listener = {0};
  listener.master_volume = 1.0f;
  listener.enabled = true;
  return listener;
}
