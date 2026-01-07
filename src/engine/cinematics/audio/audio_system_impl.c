/**
 * =================================================================================================
 *                              AUDIO SYSTEM IMPLEMENTATION
 *                                      Agent: AGENT_CINEMA_2
 * =================================================================================================
 */

#include "cinematics/audio/audio_system.h"
#include <include/math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    AUDIO CLIP
 * =================================================================================================
 */

AudioClip *audio_clip_load(const char *path) {
  return calloc(1, sizeof(AudioClip));
}
void audio_clip_load_async(const char *path, void (*callback)(AudioClip *)) {}
void audio_clip_unload(AudioClip *clip) { free(clip); }
bool audio_clip_decode_vorbis(AudioClip *clip, const void *data, size_t size) {
  return true;
}
bool audio_clip_decode_opus(AudioClip *clip, const void *data, size_t size) {
  return true;
}
void *audio_clip_stream_create(AudioClip *clip) { return NULL; }
size_t audio_clip_stream_read(void *stream, void *buffer, size_t size) {
  return 0;
}

/* =================================================================================================
 *                                    AUDIO SOURCE (3D)
 * =================================================================================================
 */

AudioSource *audio_source_create(void) {
  return calloc(1, sizeof(AudioSource));
}
void audio_source_destroy(AudioSource *source) { free(source); }
void audio_source_play(AudioSource *source) { source->is_playing = true; }
void audio_source_pause(AudioSource *source) { source->is_paused = true; }
void audio_source_stop(AudioSource *source) { source->is_playing = false; }
void audio_source_set_position(AudioSource *source, float x, float y, float z) {
}
void audio_source_set_clip(AudioSource *source, AudioClip *clip) {}
void audio_source_fade_in(AudioSource *source, float duration) {}
void audio_source_fade_out(AudioSource *source, float duration) {}
void audio_source_update(AudioSource *source, float dt) {}
float audio_source_calculate_volume(AudioSource *source,
                                    const AudioListener *listener) {
  return 1.0f;
}
float audio_source_calculate_pan(AudioSource *source,
                                 const AudioListener *listener) {
  return 0.0f;
}
float audio_source_calculate_doppler(AudioSource *source,
                                     const AudioListener *listener) {
  return 1.0f;
}
void audio_source_virtualize(AudioSource *source) {}
void audio_source_devirtualize(AudioSource *source) {}

/* =================================================================================================
 *                                    AUDIO LISTENER
 * =================================================================================================
 */

void audio_listener_set_transform(AudioListener *listener,
                                  const float *position, const float *forward,
                                  const float *up) {}
void audio_listener_attach_to_entity(AudioListener *listener,
                                     uint32_t entity_id) {}
void audio_listener_update(AudioListener *listener, float dt) {}

/* =================================================================================================
 *                                    AUDIO MIXER
 * =================================================================================================
 */

AudioMixer *mixer_create(void) { return calloc(1, sizeof(AudioMixer)); }
uint32_t mixer_add_group(AudioMixer *mixer, const char *name) { return 0; }
void mixer_remove_group(AudioMixer *mixer, uint32_t group_id) {}
void mixer_set_volume(AudioMixer *mixer, uint32_t group_id, float volume) {}
void mixer_mute(AudioMixer *mixer, uint32_t group_id, bool mute) {}
void mixer_solo(AudioMixer *mixer, uint32_t group_id, bool solo) {}
uint32_t mixer_add_effect(AudioMixer *mixer, uint32_t group_id,
                          AudioEffectType type) {
  return 0;
}
void mixer_remove_effect(AudioMixer *mixer, uint32_t group_id,
                         uint32_t effect_id) {}
void mixer_process(AudioMixer *mixer, void *buffer, size_t frames) {}
float mixer_get_level(AudioMixer *mixer, uint32_t group_id) { return 0.0f; }
void mixer_snapshot_save(AudioMixer *mixer, const char *name) {}
void mixer_snapshot_restore(AudioMixer *mixer, const char *name) {}
void mixer_transition(AudioMixer *mixer, const char *snapshot, float duration) {
}

/* =================================================================================================
 *                                    AUDIO EFFECTS
 * =================================================================================================
 */

void effect_reverb(void *buffer, size_t frames, const float *params) {}
void effect_delay(void *buffer, size_t frames, const float *params) {}
void effect_chorus(void *buffer, size_t frames, const float *params) {}
void effect_distortion(void *buffer, size_t frames, const float *params) {}
void effect_eq(void *buffer, size_t frames, const float *params) {}
void effect_compressor(void *buffer, size_t frames, const float *params) {}
void effect_lowpass(void *buffer, size_t frames, const float *params) {}
void effect_highpass(void *buffer, size_t frames, const float *params) {}

/* =================================================================================================
 *                                    ADAPTIVE MUSIC
 * =================================================================================================
 */

void music_system_init(AdaptiveMusicSystem *system) {}
void music_system_shutdown(AdaptiveMusicSystem *system) {}
void music_system_update(AdaptiveMusicSystem *system, float dt) {}
void music_load_state(AdaptiveMusicSystem *system, const char *path) {}
void music_set_state(AdaptiveMusicSystem *system, const char *state_name) {}
void music_queue_transition(AdaptiveMusicSystem *system,
                            const char *state_name) {}
void music_transition_on_beat(AdaptiveMusicSystem *system, int beat) {}
void music_set_layer_active(AdaptiveMusicSystem *system, const char *layer,
                            bool active) {}
void music_set_intensity(AdaptiveMusicSystem *system, float intensity) {}
void music_set_tension(AdaptiveMusicSystem *system, float tension) {}
void music_sync_layers(AdaptiveMusicSystem *system) {}
void music_crossfade(AdaptiveMusicSystem *system, float duration) {}
void music_stinger_play(AdaptiveMusicSystem *system, const char *stinger) {}
double music_get_beat_time(AdaptiveMusicSystem *system) { return 0.0; }

/* =================================================================================================
 *                                    AUDIO MANAGER
 * =================================================================================================
 */

void audio_manager_init(AudioManager *manager) {
  manager->is_initialized = true;
}
void audio_manager_shutdown(AudioManager *manager) {
  manager->is_initialized = false;
}
void audio_manager_update(AudioManager *manager, float dt) {}
void audio_manager_render(AudioManager *manager) {}
AudioClip *audio_manager_load_clip(AudioManager *manager, const char *path) {
  return NULL;
}
void audio_manager_play(AudioManager *manager, AudioClip *clip) {}
void audio_manager_play_at(AudioManager *manager, AudioClip *clip, float x,
                           float y, float z) {}
void audio_manager_stop_all(AudioManager *manager) {}
void audio_manager_pause_all(AudioManager *manager) {}
void audio_manager_set_listener(AudioManager *manager,
                                const AudioListener *listener) {}
void audio_manager_voice_allocation(AudioManager *manager) {}
void audio_manager_priority_sort(AudioManager *manager) {}
void audio_backend_coreaudio(AudioManager *manager) {}
void audio_backend_wasapi(AudioManager *manager) {}
void audio_backend_alsa(AudioManager *manager) {}
