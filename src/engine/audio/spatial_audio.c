/**
 * =================================================================================================
 *                          SPATIAL AUDIO SYSTEM
 *                          Phase 6: Audio Excellence
 * =================================================================================================
 *
 * PURPOSE: 3D spatial audio with HRTF, occlusion, and environmental reverb
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_MAX_SOURCES 64
#define AUDIO_MAX_LISTENERS 1
#define AUDIO_MAX_ZONES 16
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 1024

// Vector math helper
typedef struct {
  float x, y, z;
} AudioVec3;

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

// Audio Source
typedef struct {
  uint32_t id;
  bool active;
  bool playing;
  bool looping;

  AudioVec3 position;
  AudioVec3 velocity;
  float volume;
  float pitch;

  // Spatial properties
  bool is_2d;
  float min_distance;
  float max_distance;
  float roll_off;
  float cone_inner_angle;
  float cone_outer_angle;
  float cone_outer_gain;
  AudioVec3 direction;

  // DSP
  float low_pass_gain;
  float reverb_mix;
  float occlusion;

  // Playback state
  uint32_t buffer_id;
  uint32_t cursor;
} AudioSource;

// Listener
typedef struct {
  AudioVec3 position;
  AudioVec3 velocity;
  AudioVec3 forward;
  AudioVec3 up;
} AudioListener;

// Reverb Zone
typedef struct {
  AudioVec3 position;
  float radius;
  float decay_time;
  float reflections_delay;
  float reflections_gain;
  float late_reverb_delay;
  float late_reverb_gain;
  float density;
  float diffusion;
} ReverbZone;

// Audio System
typedef struct {
  AudioSource sources[AUDIO_MAX_SOURCES];
  AudioListener listener;
  ReverbZone zones[AUDIO_MAX_ZONES];
  uint32_t zone_count;

  float master_volume;
  bool initialized;

  // HRTF Data (simplified)
  float hrtf_left[180]; // Azimuth -90 to +90
  float hrtf_right[180];
} AudioSystem;

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

AudioSystem *audio_create(void) {
  AudioSystem *sys = (AudioSystem *)calloc(1, sizeof(AudioSystem));
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

void audio_destroy(AudioSystem *sys) {
  if (sys)
    free(sys);
}

// -----------------------------------------------------------------------------
// Source Management
// -----------------------------------------------------------------------------

uint32_t audio_play_sound(AudioSystem *sys, uint32_t buffer_id, AudioVec3 pos,
                          float vol, bool loop) {
  if (!sys)
    return UINT32_MAX;

  // Find free source
  for (uint32_t i = 0; i < AUDIO_MAX_SOURCES; i++) {
    if (!sys->sources[i].active) {
      AudioSource *s = &sys->sources[i];
      memset(s, 0, sizeof(AudioSource));
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

void audio_stop_source(AudioSystem *sys, uint32_t source_id) {
  if (!sys || source_id >= AUDIO_MAX_SOURCES)
    return;
  sys->sources[source_id].active = false;
  sys->sources[source_id].playing = false;
}

// -----------------------------------------------------------------------------
// Listener Update
// -----------------------------------------------------------------------------

void audio_set_listener(AudioSystem *sys, AudioVec3 pos, AudioVec3 forward,
                        AudioVec3 up) {
  if (!sys)
    return;
  sys->listener.position = pos;
  sys->listener.forward = audio_vec3_normalize(forward);
  sys->listener.up = audio_vec3_normalize(up);
}

// -----------------------------------------------------------------------------
// DSP & Spatialization
// -----------------------------------------------------------------------------

static void calculate_spatial_params(AudioSystem *sys, AudioSource *src,
                                     float *out_gain_l, float *out_gain_r) {
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
    // Inverse distance model
    attenuation =
        src->min_distance /
        (src->min_distance + src->roll_off * (dist - src->min_distance));
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

void audio_mix(AudioSystem *sys, float *out_buffer, uint32_t sample_count) {
  if (!sys)
    return;

  memset(out_buffer, 0, sample_count * 2 * sizeof(float)); // Stereo clear

  for (uint32_t i = 0; i < AUDIO_MAX_SOURCES; i++) {
    AudioSource *src = &sys->sources[i];
    if (!src->active || !src->playing)
      continue;

    float gain_l, gain_r;
    calculate_spatial_params(sys, src, &gain_l, &gain_r);

    // Check active reverb zones
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

    // Mixing logic would go here (fetching samples, applying gains, filters)
    // Simplified stub:
    // for (j < sample_count) {
    //   float sample = buffer[cursor++];
    //   sample = low_pass(sample, src->low_pass_gain);
    //   out_buffer[j*2] += sample * gain_l;
    //   out_buffer[j*2+1] += sample * gain_r;
    // }
  }
}

// -----------------------------------------------------------------------------
// Occlusion & Zones
// -----------------------------------------------------------------------------

void audio_set_occlusion(AudioSystem *sys, uint32_t source_id,
                         float occlusion) {
  if (!sys || source_id >= AUDIO_MAX_SOURCES)
    return;
  sys->sources[source_id].occlusion = occlusion;
}

uint32_t audio_add_reverb_zone(AudioSystem *sys, AudioVec3 pos, float radius) {
  if (!sys || sys->zone_count >= AUDIO_MAX_ZONES)
    return UINT32_MAX;

  ReverbZone *z = &sys->zones[sys->zone_count];
  z->position = pos;
  z->radius = radius;
  z->decay_time = 1.5f;
  z->density = 1.0f;

  return sys->zone_count++;
}
