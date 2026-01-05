/**
 * =================================================================================================
 *                              AUDIO SYSTEM - ADAPTIVE MUSIC & 3D SOUND
 *                                      Agent: AGENT_CINEMA_2
 * =================================================================================================
 *
 * Complete audio system with adaptive music, 3D positional audio, and mixing.
 *
 * =================================================================================================
 */

#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* =================================================================================================
 *                                    AUDIO CLIP
 * =================================================================================================
 */

typedef enum AudioFormat {
  AUDIO_FORMAT_PCM_S16,
  AUDIO_FORMAT_PCM_FLOAT,
  AUDIO_FORMAT_COMPRESSED_VORBIS,
  AUDIO_FORMAT_COMPRESSED_OPUS,
  AUDIO_FORMAT_COMPRESSED_MP3,
} AudioFormat;

typedef struct AudioClip {
  uint32_t id;
  char name[64];
  char path[256];
  AudioFormat format;
  uint32_t sample_rate;
  uint32_t channels;
  uint64_t sample_count;
  float duration;
  void *data;
  size_t data_size;
  bool is_streaming;
  bool is_loaded;
} AudioClip;

AudioClip *audio_clip_load(const char *path);
void audio_clip_load_async(const char *path, void (*callback)(AudioClip *));
void audio_clip_unload(AudioClip *clip);
bool audio_clip_decode_vorbis(AudioClip *clip, const void *data, size_t size);
bool audio_clip_decode_opus(AudioClip *clip, const void *data, size_t size);
void *audio_clip_stream_create(AudioClip *clip);
size_t audio_clip_stream_read(void *stream, void *buffer, size_t size);

/* =================================================================================================
 *                                    AUDIO SOURCE (3D)
 * =================================================================================================
 */

typedef struct AudioSource {
  uint32_t id;
  uint32_t clip_id;

  // Spatial
  float position[3];
  float velocity[3];
  float direction[3];

  // Properties
  float volume;
  float pitch;
  float pan;
  bool is_3d;
  bool loop;
  bool play_on_awake;

  // 3D settings
  float min_distance;
  float max_distance;
  float doppler_level;
  float spread_angle;
  float rolloff_factor; // Logarithmic, linear, custom

  // State
  bool is_playing;
  bool is_paused;
  float playback_position;
  float fade_volume;

  // Priority
  int32_t priority;
  bool virtualized;
} AudioSource;

// Forward declaration needed for calculate functions
typedef struct AudioListener AudioListener;

AudioSource *audio_source_create(void);
void audio_source_destroy(AudioSource *source);
void audio_source_play(AudioSource *source);
void audio_source_pause(AudioSource *source);
void audio_source_stop(AudioSource *source);
void audio_source_set_position(AudioSource *source, float x, float y, float z);
void audio_source_set_clip(AudioSource *source, AudioClip *clip);
void audio_source_fade_in(AudioSource *source, float duration);
void audio_source_fade_out(AudioSource *source, float duration);
void audio_source_update(AudioSource *source, float dt);
float audio_source_calculate_volume(AudioSource *source,
                                    const AudioListener *listener);
float audio_source_calculate_pan(AudioSource *source,
                                 const AudioListener *listener);
float audio_source_calculate_doppler(AudioSource *source,
                                     const AudioListener *listener);
void audio_source_virtualize(AudioSource *source);
void audio_source_devirtualize(AudioSource *source);

/* =================================================================================================
 *                                    AUDIO LISTENER
 * =================================================================================================
 */

struct AudioListener {
  float position[3];
  float velocity[3];
  float forward[3];
  float up[3];
  float volume;
  uint32_t attached_entity;
};

void audio_listener_set_transform(AudioListener *listener,
                                  const float *position, const float *forward,
                                  const float *up);
void audio_listener_attach_to_entity(AudioListener *listener,
                                     uint32_t entity_id);
void audio_listener_update(AudioListener *listener, float dt);

/* =================================================================================================
 *                                    AUDIO MIXER
 * =================================================================================================
 */

typedef struct AudioMixerGroup {
  char name[32];
  float volume;
  float pitch;
  bool muted;
  bool solo;
  uint32_t parent_group;
  uint32_t *child_groups;
  uint32_t child_count;

  // Effects chain
  uint32_t *effect_ids;
  uint32_t effect_count;

  // Output
  float output_level_left;
  float output_level_right;
} AudioMixerGroup;

typedef struct AudioMixer {
  AudioMixerGroup *groups;
  uint32_t group_count;
  uint32_t master_group;

  float master_volume;
  float music_volume;
  float sfx_volume;
  float voice_volume;
  float ambient_volume;
  float ui_volume;
} AudioMixer;

// Typedef for AudioEffectType needed for function prototype
typedef enum AudioEffectType AudioEffectType;

AudioMixer *mixer_create(void);
uint32_t mixer_add_group(AudioMixer *mixer, const char *name);
void mixer_remove_group(AudioMixer *mixer, uint32_t group_id);
void mixer_set_volume(AudioMixer *mixer, uint32_t group_id, float volume);
void mixer_mute(AudioMixer *mixer, uint32_t group_id, bool mute);
void mixer_solo(AudioMixer *mixer, uint32_t group_id, bool solo);
uint32_t mixer_add_effect(AudioMixer *mixer, uint32_t group_id,
                          AudioEffectType type);
void mixer_remove_effect(AudioMixer *mixer, uint32_t group_id,
                         uint32_t effect_id);
void mixer_process(AudioMixer *mixer, void *buffer, size_t frames);
float mixer_get_level(AudioMixer *mixer, uint32_t group_id);
void mixer_snapshot_save(AudioMixer *mixer, const char *name);
void mixer_snapshot_restore(AudioMixer *mixer, const char *name);
void mixer_transition(AudioMixer *mixer, const char *snapshot, float duration);

/* =================================================================================================
 *                                    AUDIO EFFECTS
 * =================================================================================================
 */

enum AudioEffectType {
  EFFECT_REVERB,
  EFFECT_DELAY,
  EFFECT_CHORUS,
  EFFECT_FLANGER,
  EFFECT_DISTORTION,
  EFFECT_EQ,
  EFFECT_COMPRESSOR,
  EFFECT_LIMITER,
  EFFECT_LOWPASS,
  EFFECT_HIGHPASS,
  EFFECT_ECHO,
};

typedef struct AudioEffect {
  uint32_t id;
  AudioEffectType type;
  bool bypassed;
  float mix;
  float params[16];
  uint32_t param_count;
} AudioEffect;

void effect_reverb(void *buffer, size_t frames, const float *params);
void effect_delay(void *buffer, size_t frames, const float *params);
void effect_chorus(void *buffer, size_t frames, const float *params);
void effect_distortion(void *buffer, size_t frames, const float *params);
void effect_eq(void *buffer, size_t frames, const float *params);
void effect_compressor(void *buffer, size_t frames, const float *params);
void effect_lowpass(void *buffer, size_t frames, const float *params);
void effect_highpass(void *buffer, size_t frames, const float *params);

/* =================================================================================================
 *                                    ADAPTIVE MUSIC
 * =================================================================================================
 */

typedef struct MusicLayer {
  uint32_t clip_id;
  char name[32];
  float volume;
  bool is_active;
  float fade_time;
  float current_fade;
} MusicLayer;

typedef struct MusicSegment {
  uint32_t id;
  char name[32];
  uint32_t *layer_ids;
  uint32_t layer_count;
  float duration;
  float bpm;
  uint32_t time_signature_numerator;
  uint32_t time_signature_denominator;
  uint32_t *transition_to;
  uint32_t transition_count;
} MusicSegment;

typedef struct MusicState {
  char name[32];
  MusicSegment *segments;
  uint32_t segment_count;
  uint32_t default_segment;
  float intensity;
  float tension;
} MusicState;

typedef struct AdaptiveMusicSystem {
  MusicState *states;
  uint32_t state_count;
  uint32_t current_state;

  MusicSegment *current_segment;
  float segment_position;
  float beat_position;

  MusicLayer *active_layers;
  uint32_t active_layer_count;

  // Transitions
  uint32_t queued_state;
  uint32_t queued_segment;
  bool transition_on_beat;
  bool transition_on_bar;
  bool transition_on_segment_end;

  float crossfade_time;
  float crossfade_progress;
} AdaptiveMusicSystem;

void music_system_init(AdaptiveMusicSystem *system);
void music_system_shutdown(AdaptiveMusicSystem *system);
void music_system_update(AdaptiveMusicSystem *system, float dt);
void music_load_state(AdaptiveMusicSystem *system, const char *path);
void music_set_state(AdaptiveMusicSystem *system, const char *state_name);
void music_queue_transition(AdaptiveMusicSystem *system,
                            const char *state_name);
void music_transition_on_beat(AdaptiveMusicSystem *system, int beat);
void music_set_layer_active(AdaptiveMusicSystem *system, const char *layer,
                            bool active);
void music_set_intensity(AdaptiveMusicSystem *system, float intensity);
void music_set_tension(AdaptiveMusicSystem *system, float tension);
void music_sync_layers(AdaptiveMusicSystem *system);
void music_crossfade(AdaptiveMusicSystem *system, float duration);
void music_stinger_play(AdaptiveMusicSystem *system, const char *stinger);
double music_get_beat_time(AdaptiveMusicSystem *system);

/* =================================================================================================
 *                                    AUDIO MANAGER
 * =================================================================================================
 */

typedef struct AudioManager {
  AudioClip *clips;
  uint32_t clip_count;
  uint32_t clip_capacity;

  AudioSource *sources;
  uint32_t source_count;
  uint32_t source_capacity;
  uint32_t max_voices;

  AudioListener listener;
  AudioMixer mixer;
  AdaptiveMusicSystem music;

  // Platform backend
  void *backend_context;
  uint32_t sample_rate;
  uint32_t buffer_size;

  // State
  bool is_initialized;
  bool is_muted;
  float global_volume;
} AudioManager;

void audio_manager_init(AudioManager *manager);
void audio_manager_shutdown(AudioManager *manager);
void audio_manager_update(AudioManager *manager, float dt);
void audio_manager_render(AudioManager *manager);
AudioClip *audio_manager_load_clip(AudioManager *manager, const char *path);
void audio_manager_play(AudioManager *manager, AudioClip *clip);
void audio_manager_play_at(AudioManager *manager, AudioClip *clip, float x,
                           float y, float z);
void audio_manager_stop_all(AudioManager *manager);
void audio_manager_pause_all(AudioManager *manager);
void audio_manager_set_listener(AudioManager *manager,
                                const AudioListener *listener);
void audio_manager_voice_allocation(AudioManager *manager);
void audio_manager_priority_sort(AudioManager *manager);
void audio_backend_coreaudio(AudioManager *manager);
void audio_backend_wasapi(AudioManager *manager);
void audio_backend_alsa(AudioManager *manager);

#endif // AUDIO_SYSTEM_H
