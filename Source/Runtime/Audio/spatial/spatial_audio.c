/**
 * =================================================================================================
 *                          SPATIAL AUDIO SYSTEM
 *                          Phase 6: Audio Excellence
 * =================================================================================================
 *
 * PURPOSE: 3D spatial audio with HRTF, occlusion, and environmental reverb
 * =================================================================================================
 */

#include <audio/audio_engine_types.h>
#include <common.h>
#include <math.h>
#include "engine/include/math/math_all.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Vector helpers (keeping static inline implementations here or moving to
// header if needed) For now, removing the typedefs and defines that are moved.

static inline AudioVec3 audio_vec3_sub(AudioVec3 a, AudioVec3 b) {
  AudioVec3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
  return r;
}

static inline float audio_vec3_dot(AudioVec3 a, AudioVec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float audio_vec3_length(AudioVec3 v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline AudioVec3 audio_vec3_normalize(AudioVec3 v) {
  float len = audio_vec3_length(v);
  if (len > 0.0001f) {
    AudioVec3 r = {v.x / len, v.y / len, v.z / len};
    return r;
  }
  return v;
}

static inline AudioVec3 audio_vec3_cross(AudioVec3 a, AudioVec3 b) {
  AudioVec3 r = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x};
  return r;
}

static inline float audio_vec3_distance(AudioVec3 a, AudioVec3 b) {
  return audio_vec3_length(audio_vec3_sub(a, b));
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

AudioSpatialState *audio_create(void) {
  AudioSpatialState *sys =
      (AudioSpatialState *)calloc(1, sizeof(AudioSpatialState));
  if (!sys)
    return NULL;

  sys->master_volume = 1.0f;
  sys->listener.forward = (AudioVec3){0, 0, 1};
  sys->listener.up = (AudioVec3){0, 1, 0};
  sys->initialized = true;

  // Initialize simple HRTF (head shadow approximation)
  for (int i = 0; i < 180; i++) {
    float angle_rad = ((float)i - 90.0f) * 3.14159f / 180.0f;
    // Simple cardioid pattern
    sys->hrtf_left[i] =
        0.5f * (1.0f + cosf(angle_rad + 1.5f)); // Left ear peaks at -90
    sys->hrtf_right[i] =
        0.5f * (1.0f + cosf(angle_rad - 1.5f)); // Right ear peaks at +90
  }

  return sys;
}

void audio_destroy(AudioSpatialState *sys) {
  if (sys)
    free(sys);
}

// -----------------------------------------------------------------------------
// Source Management
// -----------------------------------------------------------------------------

uint32_t audio_play_sound(AudioSpatialState *sys, uint32_t buffer_id,
                          AudioVec3 pos, float vol, bool loop) {
  if (!sys)
    return UINT32_MAX;

  // Find free source
  for (uint32_t i = 0; i < AUDIO_MAX_SOURCES; i++) {
    if (!sys->sources[i].active) {
      AudioSourceState *s = &sys->sources[i];
      memset(s, 0, sizeof(AudioSourceState));
      s->id = i;
      s->active = true;
      s->playing = true;
      s->buffer_id = buffer_id;
      s->position = pos;
      s->volume = vol;
      s->pitch = 1.0f;
      s->looping = loop;
      s->min_distance = 1.0f;
      s->max_distance = 100.0f;
      s->roll_off = 1.0f;
      s->cone_outer_angle = 360.0f;
      s->direction = (AudioVec3){0, 0, 1};
      return i;
    }
  }
  return UINT32_MAX;
}

void audio_stop_source(AudioSpatialState *sys, uint32_t source_id) {
  if (!sys || source_id >= AUDIO_MAX_SOURCES)
    return;
  sys->sources[source_id].active = false;
  sys->sources[source_id].playing = false;
}

// -----------------------------------------------------------------------------
// Listener Update
// -----------------------------------------------------------------------------

void audio_set_listener(AudioSpatialState *sys, AudioVec3 pos,
                        AudioVec3 forward, AudioVec3 up) {
  if (!sys)
    return;
  sys->listener.position = pos;
  sys->listener.forward = audio_vec3_normalize(forward);
  sys->listener.up = audio_vec3_normalize(up);
}

// -----------------------------------------------------------------------------
// DSP & Spatialization
// -----------------------------------------------------------------------------

static void calculate_spatial_params(AudioSpatialState *sys,
                                     AudioSourceState *src, float *out_gain_l,
                                     float *out_gain_r) {
  if (src->is_2d) {
    *out_gain_l = src->volume * sys->master_volume;
    *out_gain_r = src->volume * sys->master_volume;
    return;
  }

  // Distance attenuation
  AudioVec3 to_source = audio_vec3_sub(src->position, sys->listener.position);
  float dist = audio_vec3_length(to_source);

  float attenuation = 1.0f;
  if (dist > src->min_distance) {
    switch (src->distance_model) {
    case DISTANCE_MODEL_LINEAR:
      if (src->max_distance > src->min_distance) {
        attenuation = 1.0f - (dist - src->min_distance) /
                                 (src->max_distance - src->min_distance);
      } else {
        attenuation = 0.0f;
      }
      break;
    case DISTANCE_MODEL_EXPONENTIAL:
      attenuation = powf(dist / src->min_distance, -src->roll_off);
      break;
    case DISTANCE_MODEL_INVERSE:
    default:
      attenuation =
          src->min_distance /
          (src->min_distance + src->roll_off * (dist - src->min_distance));
      break;
    }
  }
  if (dist > src->max_distance)
    attenuation = 0.0f;

  // Directionality
  AudioVec3 dir = audio_vec3_normalize(to_source);
  AudioVec3 right = audio_vec3_cross(sys->listener.forward, sys->listener.up);

  // Project source direction onto listener's local coordinates
  float dot_fwd = audio_vec3_dot(dir, sys->listener.forward);
  float dot_right = audio_vec3_dot(dir, right);

  // HRTF Approximation (Panning)
  // Calculate azimuth angle (-90 to +90 degrees relative to forward)
  float azimuth = atan2f(dot_right, dot_fwd) * 180.0f / 3.14159f;

  // Map azimuth to lookup table index
  int idx = (int)(azimuth + 90.0f);
  if (idx < 0)
    idx = 0;
  if (idx > 179)
    idx = 179;

  float hrtf_l = sys->hrtf_left[idx];
  float hrtf_r = sys->hrtf_right[idx];

  // Cone attenuation (if directional source)
  float cone_gain = 1.0f;
  if (src->cone_outer_angle < 360.0f) {
    float angle_to_listener =
        acosf(audio_vec3_dot(src->direction,
                             (AudioVec3){-dir.x, -dir.y, -dir.z})) *
        180.0f / 3.14159f;

    if (angle_to_listener > src->cone_outer_angle / 2.0f) {
      cone_gain = src->cone_outer_gain;
    } else if (angle_to_listener > src->cone_inner_angle / 2.0f) {
      float t = (angle_to_listener - src->cone_inner_angle / 2.0f) /
                (src->cone_outer_angle / 2.0f - src->cone_inner_angle / 2.0f);
      cone_gain = 1.0f + t * (src->cone_outer_gain - 1.0f);
    }
  }

  float final_gain = src->volume * sys->master_volume * attenuation *
                     cone_gain * (1.0f - src->occlusion);

  *out_gain_l = final_gain * hrtf_l;
  *out_gain_r = final_gain * hrtf_r;

  // Calculate low-pass based on occlusion and distance (air absorption)
  src->low_pass_gain = 1.0f - (src->occlusion * 0.7f);
  if (dist > 20.0f) {
    src->low_pass_gain *= (1.0f - fminf((dist - 20.0f) * 0.01f, 0.5f));
  }
}

// -----------------------------------------------------------------------------
// Audio Processing Loop
// -----------------------------------------------------------------------------

void audio_mix(AudioSpatialState *sys, float *out_buffer,
               uint32_t sample_count) {
  if (!sys)
    return;

  // Clear output buffer (Stereo interleaved)
  memset(out_buffer, 0, sample_count * 2 * sizeof(float));

  for (uint32_t i = 0; i < AUDIO_MAX_SOURCES; i++) {
    AudioSourceState *src = &sys->sources[i];
    if (!src->active || !src->playing)
      continue;

    float gain_l, gain_r;
    calculate_spatial_params(sys, src, &gain_l, &gain_r);

    // Calculate reverb mix based on zones
    float max_reverb = 0.0f;
    for (uint32_t z = 0; z < sys->zone_count; z++) {
      float dist = audio_vec3_distance(src->position, sys->zones[z].position);
      if (dist < sys->zones[z].radius) {
        float mix = 1.0f - (dist / sys->zones[z].radius);
        if (mix > max_reverb)
          max_reverb = mix;
      }
    }
    src->reverb_mix = max_reverb;

    // Mixing process
    // In a real system, we'd fetch from a buffer pool.
    // For now, we assume buffer_id points to valid PCM data.
    // float* samples = audio_get_buffer_data(src->buffer_id);
    // if (!samples) continue;

    for (uint32_t j = 0; j < sample_count; j++) {
      // Stub: fetching sample (in a real implementation this would be from
      // src->buffer_id)
      float sample = 0.0f;

      // Apply Low-Pass Filter (Simple RC approximation)
      float alpha = src->low_pass_gain;
      src->filter_state_l =
          src->filter_state_l + alpha * (sample - src->filter_state_l);
      src->filter_state_r =
          src->filter_state_r + alpha * (sample - src->filter_state_r);

      float final_l = src->filter_state_l * gain_l;
      float final_r = src->filter_state_r * gain_r;

      out_buffer[j * 2] += final_l;
      out_buffer[j * 2 + 1] += final_r;

      src->cursor++;
      // Handle looping
      // if (src->cursor >= buffer_len) {
      //    if (src->looping) src->cursor = 0;
      //    else src->playing = false; break;
      // }
    }
  }
}

// -----------------------------------------------------------------------------
// Occlusion & Zones
// -----------------------------------------------------------------------------

void audio_set_occlusion(AudioSpatialState *sys, uint32_t source_id,
                         float occlusion) {
  if (!sys || source_id >= AUDIO_MAX_SOURCES)
    return;
  sys->sources[source_id].occlusion = occlusion;
}

uint32_t audio_add_reverb_zone(AudioSpatialState *sys, AudioVec3 pos,
                               float radius) {
  if (!sys || sys->zone_count >= AUDIO_MAX_ZONES)
    return UINT32_MAX;

  ReverbZoneState *z = &sys->zones[sys->zone_count];
  z->position = pos;
  z->radius = radius;
  z->decay_time = 1.5f;
  z->density = 1.0f;

  return sys->zone_count++;
}
