#ifndef AUDIO_ENGINE_TYPES_H
#define AUDIO_ENGINE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_MAX_SOURCES 64
#define AUDIO_MAX_LISTENERS 1
#define AUDIO_MAX_ZONES 16
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 1024

typedef enum {
  DISTANCE_MODEL_LINEAR,
  DISTANCE_MODEL_INVERSE,
  DISTANCE_MODEL_EXPONENTIAL
} DistanceModel;

typedef enum {
  OCCLUSION_NONE,
  OCCLUSION_MUFFLED,
  OCCLUSION_OBSTRUCTED
} OcclusionState;

// Vector math helper
typedef struct {
  float x, y, z;
} AudioVec3;

// Audio Source State (Spatial only)
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
  float filter_state_l;
  float filter_state_r;
  float reverb_mix;
  float occlusion;
  DistanceModel distance_model;
  OcclusionState occlusion_state;
  float target_occlusion;
  float occlusion_factor;

  // Playback state
  uint32_t buffer_id;
  uint32_t cursor;
} AudioSourceState;

// Listener State
typedef struct {
  AudioVec3 position;
  AudioVec3 velocity;
  AudioVec3 forward;
  AudioVec3 up;
} AudioListenerState;

// Reverb Zone State
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
} ReverbZoneState;

// Audio Spatial State (The lightweight state shared with other systems)
typedef struct {
  AudioSourceState sources[AUDIO_MAX_SOURCES];
  AudioListenerState listener;
  ReverbZoneState zones[AUDIO_MAX_ZONES];
  uint32_t zone_count;

  float master_volume;
  bool initialized;

  // HRTF Data (simplified)
  float hrtf_left[180]; // Azimuth -90 to +90
  float hrtf_right[180];
} AudioSpatialState;

#endif // AUDIO_ENGINE_TYPES_H
