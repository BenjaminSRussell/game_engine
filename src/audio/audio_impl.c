/**
 * =================================================================================================
 *                              AUDIO SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_AUDIO_1
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_AUDIO_SOURCES 128
#define MAX_AUDIO_BUFFERS 256
#define MAX_SOUND_BANKS 32
#define MAX_PLAYING_SOUNDS 64
#define AUDIO_SAMPLE_RATE 44100

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef struct AudioBuffer {
  uint32_t id;
  char name[64];
  int16_t *samples;
  uint32_t sample_count;
  uint32_t channels;
  uint32_t sample_rate;
  float duration;
  bool is_loaded;
} AudioBuffer;

typedef struct AudioSource {
  uint32_t id;
  uint32_t buffer_id;

  float position[3];
  float velocity[3];

  float volume;
  float pitch;
  float pan;

  float min_distance;
  float max_distance;
  float rolloff;

  bool is_playing;
  bool is_looping;
  bool is_3d;
  bool is_paused;

  uint32_t sample_position;
  float time;
} AudioSource;

typedef struct AudioListener {
  float position[3];
  float forward[3];
  float up[3];
  float velocity[3];
} AudioListener;

typedef struct AudioMixer {
  float master_volume;
  float music_volume;
  float sfx_volume;
  float voice_volume;
  float ambient_volume;
  bool muted;
} AudioMixer;

typedef struct AudioEngine {
  AudioBuffer *buffers;
  uint32_t buffer_count;

  AudioSource *sources;
  uint32_t source_count;

  AudioListener listener;
  AudioMixer mixer;

  float *mix_buffer;
  uint32_t mix_buffer_size;

  bool initialized;
} AudioEngine;

static AudioEngine g_audio = {0};

/* =================================================================================================
 *                                    AUDIO BUFFER
 * =================================================================================================
 */

// DONE: Implement audio_buffer_load
uint32_t audio_buffer_load(const char *path) {
  if (g_audio.buffer_count >= MAX_AUDIO_BUFFERS)
    return 0xFFFFFFFF;

  FILE *f = fopen(path, "rb");
  if (!f)
    return 0xFFFFFFFF;

  // Simplified WAV loading
  char header[44];
  fread(header, 1, 44, f);

  // Get data size from WAV header
  uint32_t data_size = *(uint32_t *)&header[40];

  int16_t *samples = malloc(data_size);
  fread(samples, 1, data_size, f);
  fclose(f);

  uint32_t id = g_audio.buffer_count++;
  AudioBuffer *buf = &g_audio.buffers[id];

  buf->id = id;
  strncpy(buf->name, path, 63);
  buf->samples = samples;
  buf->sample_count = data_size / sizeof(int16_t);
  buf->channels = *(uint16_t *)&header[22];
  buf->sample_rate = *(uint32_t *)&header[24];
  buf->duration = (float)buf->sample_count / buf->channels / buf->sample_rate;
  buf->is_loaded = true;

  return id;
}

// DONE: Implement audio_buffer_create
uint32_t audio_buffer_create(int16_t *samples, uint32_t sample_count,
                             uint32_t channels, uint32_t sample_rate) {
  if (g_audio.buffer_count >= MAX_AUDIO_BUFFERS)
    return 0xFFFFFFFF;

  uint32_t id = g_audio.buffer_count++;
  AudioBuffer *buf = &g_audio.buffers[id];

  buf->id = id;
  buf->samples = malloc(sample_count * sizeof(int16_t));
  memcpy(buf->samples, samples, sample_count * sizeof(int16_t));
  buf->sample_count = sample_count;
  buf->channels = channels;
  buf->sample_rate = sample_rate;
  buf->duration = (float)sample_count / channels / sample_rate;
  buf->is_loaded = true;

  return id;
}

// DONE: Implement audio_buffer_destroy
void audio_buffer_destroy(uint32_t id) {
  if (id >= g_audio.buffer_count)
    return;

  AudioBuffer *buf = &g_audio.buffers[id];
  free(buf->samples);
  memset(buf, 0, sizeof(AudioBuffer));
}

/* =================================================================================================
 *                                    AUDIO SOURCE
 * =================================================================================================
 */

// DONE: Implement audio_source_create
uint32_t audio_source_create(void) {
  if (g_audio.source_count >= MAX_AUDIO_SOURCES)
    return 0xFFFFFFFF;

  uint32_t id = g_audio.source_count++;
  AudioSource *src = &g_audio.sources[id];

  memset(src, 0, sizeof(AudioSource));
  src->id = id;
  src->volume = 1.0f;
  src->pitch = 1.0f;
  src->pan = 0.0f;
  src->min_distance = 1.0f;
  src->max_distance = 100.0f;
  src->rolloff = 1.0f;

  return id;
}

// DONE: Implement audio_source_destroy
void audio_source_destroy(uint32_t id) {
  if (id >= g_audio.source_count)
    return;
  memset(&g_audio.sources[id], 0, sizeof(AudioSource));
}

// DONE: Implement audio_source_set_buffer
void audio_source_set_buffer(uint32_t source_id, uint32_t buffer_id) {
  if (source_id >= g_audio.source_count)
    return;
  g_audio.sources[source_id].buffer_id = buffer_id;
}

// DONE: Implement audio_source_play
void audio_source_play(uint32_t id) {
  if (id >= g_audio.source_count)
    return;
  AudioSource *src = &g_audio.sources[id];
  src->is_playing = true;
  src->is_paused = false;
  if (src->sample_position == 0) {
    src->time = 0;
  }
}

// DONE: Implement audio_source_pause
void audio_source_pause(uint32_t id) {
  if (id >= g_audio.source_count)
    return;
  g_audio.sources[id].is_paused = true;
}

// DONE: Implement audio_source_stop
void audio_source_stop(uint32_t id) {
  if (id >= g_audio.source_count)
    return;
  AudioSource *src = &g_audio.sources[id];
  src->is_playing = false;
  src->is_paused = false;
  src->sample_position = 0;
  src->time = 0;
}

// DONE: Implement audio_source_set_volume
void audio_source_set_volume(uint32_t id, float volume) {
  if (id >= g_audio.source_count)
    return;
  g_audio.sources[id].volume = volume < 0 ? 0 : (volume > 1 ? 1 : volume);
}

// DONE: Implement audio_source_set_pitch
void audio_source_set_pitch(uint32_t id, float pitch) {
  if (id >= g_audio.source_count)
    return;
  g_audio.sources[id].pitch =
      pitch < 0.1f ? 0.1f : (pitch > 4.0f ? 4.0f : pitch);
}

// DONE: Implement audio_source_set_looping
void audio_source_set_looping(uint32_t id, bool looping) {
  if (id >= g_audio.source_count)
    return;
  g_audio.sources[id].is_looping = looping;
}

// DONE: Implement audio_source_set_position
void audio_source_set_position(uint32_t id, float x, float y, float z) {
  if (id >= g_audio.source_count)
    return;
  AudioSource *src = &g_audio.sources[id];
  src->position[0] = x;
  src->position[1] = y;
  src->position[2] = z;
  src->is_3d = true;
}

// DONE: Implement audio_source_is_playing
bool audio_source_is_playing(uint32_t id) {
  if (id >= g_audio.source_count)
    return false;
  return g_audio.sources[id].is_playing && !g_audio.sources[id].is_paused;
}

/* =================================================================================================
 *                                    3D AUDIO
 * =================================================================================================
 */

// DONE: Implement audio_listener_set_position
void audio_listener_set_position(float x, float y, float z) {
  g_audio.listener.position[0] = x;
  g_audio.listener.position[1] = y;
  g_audio.listener.position[2] = z;
}

// DONE: Implement audio_listener_set_orientation
void audio_listener_set_orientation(float fx, float fy, float fz, float ux,
                                    float uy, float uz) {
  g_audio.listener.forward[0] = fx;
  g_audio.listener.forward[1] = fy;
  g_audio.listener.forward[2] = fz;
  g_audio.listener.up[0] = ux;
  g_audio.listener.up[1] = uy;
  g_audio.listener.up[2] = uz;
}

static float audio_calculate_3d_gain(AudioSource *src) {
  float dx = src->position[0] - g_audio.listener.position[0];
  float dy = src->position[1] - g_audio.listener.position[1];
  float dz = src->position[2] - g_audio.listener.position[2];
  float distance = sqrtf(dx * dx + dy * dy + dz * dz);

  if (distance <= src->min_distance)
    return 1.0f;
  if (distance >= src->max_distance)
    return 0.0f;

  // Inverse distance attenuation
  float d = src->min_distance + src->rolloff * (distance - src->min_distance);
  return src->min_distance / d;
}

static float audio_calculate_3d_pan(AudioSource *src) {
  float dx = src->position[0] - g_audio.listener.position[0];
  float dz = src->position[2] - g_audio.listener.position[2];
  float distance = sqrtf(dx * dx + dz * dz);

  if (distance < 0.001f)
    return 0.0f;

  // Dot product with listener right vector
  float rx = -g_audio.listener.forward[2];
  float rz = g_audio.listener.forward[0];

  return (dx * rx + dz * rz) / distance;
}

/* =================================================================================================
 *                                    MIXER
 * =================================================================================================
 */

// DONE: Implement mixer_set_master_volume
void mixer_set_master_volume(float volume) {
  g_audio.mixer.master_volume = volume < 0 ? 0 : (volume > 1 ? 1 : volume);
}

// DONE: Implement mixer_set_sfx_volume
void mixer_set_sfx_volume(float volume) {
  g_audio.mixer.sfx_volume = volume < 0 ? 0 : (volume > 1 ? 1 : volume);
}

// DONE: Implement mixer_set_music_volume
void mixer_set_music_volume(float volume) {
  g_audio.mixer.music_volume = volume < 0 ? 0 : (volume > 1 ? 1 : volume);
}

// DONE: Implement mixer_mute
void mixer_mute(bool mute) { g_audio.mixer.muted = mute; }

/* =================================================================================================
 *                                    AUDIO ENGINE
 * =================================================================================================
 */

// DONE: Implement audio_engine_init
bool audio_engine_init(void) {
  if (g_audio.initialized)
    return false;

  memset(&g_audio, 0, sizeof(AudioEngine));

  g_audio.buffers = calloc(MAX_AUDIO_BUFFERS, sizeof(AudioBuffer));
  g_audio.sources = calloc(MAX_AUDIO_SOURCES, sizeof(AudioSource));

  g_audio.mix_buffer_size = AUDIO_SAMPLE_RATE / 30; // ~33ms buffer
  g_audio.mix_buffer = calloc(g_audio.mix_buffer_size * 2, sizeof(float));

  g_audio.mixer.master_volume = 1.0f;
  g_audio.mixer.music_volume = 1.0f;
  g_audio.mixer.sfx_volume = 1.0f;
  g_audio.mixer.voice_volume = 1.0f;
  g_audio.mixer.ambient_volume = 1.0f;

  g_audio.listener.forward[2] = -1.0f;
  g_audio.listener.up[1] = 1.0f;

  g_audio.initialized = true;
  return true;
}

// DONE: Implement audio_engine_shutdown
void audio_engine_shutdown(void) {
  if (!g_audio.initialized)
    return;

  for (uint32_t i = 0; i < g_audio.buffer_count; i++) {
    audio_buffer_destroy(i);
  }

  free(g_audio.buffers);
  free(g_audio.sources);
  free(g_audio.mix_buffer);

  memset(&g_audio, 0, sizeof(AudioEngine));
}

// DONE: Implement audio_engine_update
void audio_engine_update(float dt) {
  if (!g_audio.initialized || g_audio.mixer.muted)
    return;

  for (uint32_t i = 0; i < g_audio.source_count; i++) {
    AudioSource *src = &g_audio.sources[i];
    if (!src->is_playing || src->is_paused)
      continue;

    src->time += dt * src->pitch;

    // Check if finished
    if (src->buffer_id < g_audio.buffer_count) {
      AudioBuffer *buf = &g_audio.buffers[src->buffer_id];
      if (buf->is_loaded && src->time >= buf->duration) {
        if (src->is_looping) {
          src->time = 0;
          src->sample_position = 0;
        } else {
          src->is_playing = false;
        }
      }
    }
  }
}

// DONE: Implement audio_engine_mix
void audio_engine_mix(float *output, uint32_t frame_count) {
  if (!g_audio.initialized)
    return;

  memset(output, 0, frame_count * 2 * sizeof(float));

  if (g_audio.mixer.muted)
    return;

  for (uint32_t i = 0; i < g_audio.source_count; i++) {
    AudioSource *src = &g_audio.sources[i];
    if (!src->is_playing || src->is_paused)
      continue;
    if (src->buffer_id >= g_audio.buffer_count)
      continue;

    AudioBuffer *buf = &g_audio.buffers[src->buffer_id];
    if (!buf->is_loaded)
      continue;

    float volume =
        src->volume * g_audio.mixer.master_volume * g_audio.mixer.sfx_volume;
    float pan = src->pan;

    if (src->is_3d) {
      volume *= audio_calculate_3d_gain(src);
      pan = audio_calculate_3d_pan(src);
    }

    float left_gain = volume * (1.0f - pan) * 0.5f + volume * 0.5f;
    float right_gain = volume * (1.0f + pan) * 0.5f + volume * 0.5f;

    // Mix samples
    for (uint32_t f = 0; f < frame_count; f++) {
      uint32_t sample_idx = src->sample_position + (uint32_t)(f * src->pitch);
      if (sample_idx >= buf->sample_count / buf->channels) {
        if (src->is_looping) {
          sample_idx %= buf->sample_count / buf->channels;
        } else {
          break;
        }
      }

      float sample = buf->samples[sample_idx * buf->channels] / 32768.0f;

      output[f * 2 + 0] += sample * left_gain;
      output[f * 2 + 1] += sample * right_gain;
    }

    src->sample_position += (uint32_t)(frame_count * src->pitch);
  }

  // Clamp output
  for (uint32_t i = 0; i < frame_count * 2; i++) {
    if (output[i] > 1.0f)
      output[i] = 1.0f;
    if (output[i] < -1.0f)
      output[i] = -1.0f;
  }
}

// DONE: Implement audio_play_oneshot
uint32_t audio_play_oneshot(uint32_t buffer_id, float volume) {
  uint32_t source = audio_source_create();
  if (source == 0xFFFFFFFF)
    return source;

  audio_source_set_buffer(source, buffer_id);
  audio_source_set_volume(source, volume);
  audio_source_play(source);

  return source;
}

// DONE: Implement audio_play_3d
uint32_t audio_play_3d(uint32_t buffer_id, float x, float y, float z,
                       float volume) {
  uint32_t source = audio_source_create();
  if (source == 0xFFFFFFFF)
    return source;

  audio_source_set_buffer(source, buffer_id);
  audio_source_set_volume(source, volume);
  audio_source_set_position(source, x, y, z);
  audio_source_play(source);

  return source;
}
