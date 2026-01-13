/**
 * AUDIO SYSTEM - 3D SPATIAL AUDIO
 * Additional system for completeness
 */

#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float position[3];
  float velocity[3];
  float forward[3];
  float up[3];
} AudioListener;

typedef struct {
  unsigned int buffer_id;
  float position[3];
  float velocity[3];
  float volume;
  float pitch;
  float min_distance;
  float max_distance;
  bool looping;
  bool playing;
  bool spatial;
} AudioSource;

typedef struct {
  AudioListener listener;
  AudioSource *sources;
  int source_count;
  int max_sources;
  float master_volume;
} AudioSystem;

// Create audio system
AudioSystem *audio_system_create(int max_sources) {
  AudioSystem *sys = (AudioSystem *)calloc(1, sizeof(AudioSystem));
  sys->max_sources = max_sources;
  sys->sources = (AudioSource *)calloc(max_sources, sizeof(AudioSource));
  sys->master_volume = 1.0f;

  sys->listener.forward[2] = -1.0f;
  sys->listener.up[1] = 1.0f;

  return sys;
}

// Play sound
int audio_play_sound(AudioSystem *sys, unsigned int buffer, float volume,
                     bool looping) {
  for (int i = 0; i < sys->max_sources; i++) {
    if (!sys->sources[i].playing) {
      AudioSource *src = &sys->sources[i];
      src->buffer_id = buffer;
      src->volume = volume;
      src->looping = looping;
      src->playing = true;
      src->spatial = false;
      src->pitch = 1.0f;
      return i;
    }
  }
  return -1;
}

// Play 3D sound
int audio_play_3d_sound(AudioSystem *sys, unsigned int buffer, float pos[3],
                        float volume, float min_dist, float max_dist) {
  int id = audio_play_sound(sys, buffer, volume, false);
  if (id >= 0) {
    AudioSource *src = &sys->sources[id];
    memcpy(src->position, pos, sizeof(float) * 3);
    src->spatial = true;
    src->min_distance = min_dist;
    src->max_distance = max_dist;
  }
  return id;
}

// Update audio
void audio_system_update(AudioSystem *sys, float dt) {
  for (int i = 0; i < sys->max_sources; i++) {
    AudioSource *src = &sys->sources[i];
    if (!src->playing)
      continue;

    if (src->spatial) {
      // Calculate distance attenuation
      float dx = src->position[0] - sys->listener.position[0];
      float dy = src->position[1] - sys->listener.position[1];
      float dz = src->position[2] - sys->listener.position[2];
      float distance = sqrtf(dx * dx + dy * dy + dz * dz);

      float attenuation = 1.0f;
      if (distance > src->min_distance) {
        attenuation = src->min_distance / distance;
        if (distance > src->max_distance) {
          attenuation = 0.0f;
        }
      }

      // TODO: Apply attenuation to source
      // TODO: Calculate panning based on listener orientation
    }
  }
}

// Stop sound
void audio_stop_sound(AudioSystem *sys, int source_id) {
  if (source_id >= 0 && source_id < sys->max_sources) {
    sys->sources[source_id].playing = false;
  }
}

/*
 * IMPLEMENTATION: 20/100 Audio System TODOs
 * LOC: ~130
 */
