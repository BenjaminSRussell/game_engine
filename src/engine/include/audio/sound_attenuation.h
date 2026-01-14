// Audio/Sound Attenuation & Occlusion
#ifndef SOUND_ATTENUATION_H
#define SOUND_ATTENUATION_H

#include <common.h>
#include "math/vec3.h"

// Distance attenuation shapes
typedef enum {
  ATTENUATION_LINEAR,
  ATTENUATION_LOGARITHMIC,
  ATTENUATION_INVERSE,
  ATTENUATION_NATURAL_SOUND
} AttenuationDistanceModel;

typedef enum {
  SHAPE_SPHERE,
  SHAPE_BOX,
  SHAPE_CAPSULE,
  SHAPE_CONE
} AttenuationShape;

typedef struct {
  bool enable_volume_attenuation;
  AttenuationDistanceModel distance_algorithm;
  f32 inner_radius;     // Is at full volume
  f32 falloff_distance; // Volume fades to 0
  f32 db_attenuation_at_max;

  bool enable_spatialization; // 3D positioning
  bool enable_air_absorption; // Low-pass filter over distance

  // Focus (Cone)
  f32 inner_cone_angle;
  f32 outer_cone_angle;
  f32 outer_volume_level;

  // Occlusion (Raycast)
  bool enable_occlusion;
  f32 occlusion_interpolation_time; // Smoothness
  bool use_complex_collision;

  // Reverb
  bool enable_reverb_send;
  f32 manual_reverb_send;
  // ... custom reverb submix curves

} SoundAttenuationSettings;

#ifdef __cplusplus
extern "C" {
#endif

// Calculate current volume multiplier based on listener position
f32 audio_calculate_attenuation(const SoundAttenuationSettings *settings,
                                Vec3 source_pos, Vec3 listener_pos);

// Calculate low-pass filter frequency for air absorption
f32 audio_calculate_air_absorption(const SoundAttenuationSettings *settings,
                                   f32 distance);

#ifdef __cplusplus
}
#endif

#endif
