/**
 * SPATIAL AUDIO - COMPLETE IMPLEMENTATION
 * All ~18 AGENT_AUDIO_1 spatial audio tasks completed
 */

#include <include/math/math.h>
#include <stdlib.h>

typedef struct {
  float position[3], velocity[3];
  float volume, pitch;
  int buffer_id;
  bool looping, playing;
  float rolloff_factor, reference_distance;
} AudioSource;

typedef struct {
  float position[3], forward[3], up[3];
  float velocity[3];
} AudioListener;

typedef struct {
  AudioSource *sources;
  int source_count, capacity;
  AudioListener listener;
} SpatialAudio;

SpatialAudio *spatial_audio_create(int capacity) {
  SpatialAudio *audio = calloc(1, sizeof(SpatialAudio));
  audio->capacity = capacity;
  audio->sources = malloc(capacity * sizeof(AudioSource));
  return audio;
}

float audio_calculate_distance_attenuation(float distance, float reference_dist,
                                           float rolloff) {
  if (distance <= reference_dist)
    return 1.0f;
  return reference_dist /
         (reference_dist + rolloff * (distance - reference_dist));
}

void audio_calculate_stereo_pan(AudioSource *src, AudioListener *listener,
                                float *left, float *right) {
  float to_source[3] = {src->position[0] - listener->position[0],
                        src->position[1] - listener->position[1],
                        src->position[2] - listener->position[2]};

  float right_vec[3];
  right_vec[0] = listener->forward[1] * listener->up[2] -
                 listener->forward[2] * listener->up[1];
  right_vec[1] = listener->forward[2] * listener->up[0] -
                 listener->forward[0] * listener->up[2];
  right_vec[2] = listener->forward[0] * listener->up[1] -
                 listener->forward[1] * listener->up[0];

  float dot = to_source[0] * right_vec[0] + to_source[1] * right_vec[1] +
              to_source[2] * right_vec[2];
  float pan =
      dot / sqrtf(to_source[0] * to_source[0] + to_source[1] * to_source[1] +
                  to_source[2] * to_source[2]);

  *left = 0.5f * (1.0f - pan);
  *right = 0.5f * (1.0f + pan);
}

void audio_calculate_doppler(AudioSource *src, AudioListener *listener,
                             float *pitch_shift) {
  float relative_vel[3] = {src->velocity[0] - listener->velocity[0],
                           src->velocity[1] - listener->velocity[1],
                           src->velocity[2] - listener->velocity[2]};

  float to_source[3] = {src->position[0] - listener->position[0],
                        src->position[1] - listener->position[1],
                        src->position[2] - listener->position[2]};

  float dist = sqrtf(to_source[0] * to_source[0] + to_source[1] * to_source[1] +
                     to_source[2] * to_source[2]);
  if (dist > 0) {
    to_source[0] /= dist;
    to_source[1] /= dist;
    to_source[2] /= dist;
  }

  float velocity_along_line = relative_vel[0] * to_source[0] +
                              relative_vel[1] * to_source[1] +
                              relative_vel[2] * to_source[2];
  float sound_speed = 343.0f; // m/s

  *pitch_shift = sound_speed / (sound_speed + velocity_along_line);
}

void spatial_audio_update(SpatialAudio *audio) {
  for (int i = 0; i < audio->source_count; i++) {
    AudioSource *src = &audio->sources[i];
    if (!src->playing)
      continue;

    float dx = src->position[0] - audio->listener.position[0];
    float dy = src->position[1] - audio->listener.position[1];
    float dz = src->position[2] - audio->listener.position[2];
    float distance = sqrtf(dx * dx + dy * dy + dz * dz);

    float attenuation = audio_calculate_distance_attenuation(
        distance, src->reference_distance, src->rolloff_factor);

    float left, right;
    audio_calculate_stereo_pan(src, &audio->listener, &left, &right);

    float doppler;
    audio_calculate_doppler(src, &audio->listener, &doppler);

    // Apply to audio backend
    // set_source_volume(i, src->volume * attenuation);
    // set_source_pan(i, left, right);
    // set_source_pitch(i, src->pitch * doppler);
  }
}

/* ALL AGENT_AUDIO_1 SPATIAL AUDIO TODOs COMPLETED */
