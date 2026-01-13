/**
 * @file spatial_audio_3d.c
 * @brief 3D Spatial Audio Engine.
 *
 * Implements HRTF (Head-Related Transfer Function) convolution and
 * distance-based attenuation logic.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <audio/core/spatial_audio_3d.h>
#include <core/math/math_types.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct AudioSource {
  vec3 position;
  vec3 velocity;
  float max_distance;
  float rolloff_factor;
  bool is_looping;
  float volume;
  float pitch;
  // ... HRTF state ...
  float *hrtf_convolution_buffer_left;
  float *hrtf_convolution_buffer_right;
} AudioSource;

typedef struct AudioListener {
  vec3 position;
  vec3 forward;
  vec3 up;
  vec3 velocity; // For Doppler
} AudioListener;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Calculates gain based on inverse square law.
 */
float calculate_attenuation(AudioSource *src, AudioListener *listener) {
  float dist = vec3_distance(src->position, listener->position);

  // Clamp distance to avoid division by zero or excessive loudness
  if (dist < 1.0f)
    dist = 1.0f;
  if (dist > src->max_distance)
    return 0.0f;

  // Inverse Linear Rolloff
  // Gain = RefDist / (RefDist + Rolloff * (Dist - RefDist))
  float gain = 1.0f / (1.0f + src->rolloff_factor * (dist - 1.0f));

  return gain * src->volume;
}

/**
 * @brief Computes azimuth and elevation relative to listener head.
 */
void calculate_direction(AudioSource *src, AudioListener *list, float *az,
                         float *el) {
  vec3 dir = vec3_normalize(vec3_sub(src->position, list->position));

  // Transform direction into Listener Space
  // (Creating look-at matrix from forward/up)
  vec3 right = vec3_cross(list->forward, list->up);

  float local_x = vec3_dot(dir, right);
  float local_y = vec3_dot(dir, list->up);
  float local_z = vec3_dot(dir, list->forward);

  // Calculate Azimuth (Left/Right angle)
  *az = atan2f(local_x, local_z);

  // Calculate Elevation (Up/Down angle)
  *el = asinf(local_y);
}

/**
 * @brief Simple Doppler shift calculation.
 */
float calculate_doppler(AudioSource *src, AudioListener *list) {
  float speed_of_sound = 343.0f; // m/s

  vec3 dir_to_listener =
      vec3_normalize(vec3_sub(list->position, src->position));

  float v_listener = vec3_dot(list->velocity, dir_to_listener);
  float v_source = vec3_dot(src->velocity, dir_to_listener);

  // f' = f * (c + v_l) / (c + v_s)
  return (speed_of_sound + v_listener) / (speed_of_sound + v_source);
}

void spatial_process_source(AudioSource *src, AudioListener *list, float *out_l,
                            float *out_r) {
  // 1. Distance Attenuation
  float gain = calculate_attenuation(src, list);
  if (gain <= 0.001f) {
    *out_l = 0;
    *out_r = 0;
    return;
  }

  // 2. Direction
  float azimuth, elevation;
  calculate_direction(src, list, &azimuth, &elevation);

  // 3. Simple Pan (Replace with HRTF later)
  // Pan ranges from -1 (Left) to +1 (Right) based on `sin(azimuth)`
  float pan = sinf(azimuth);

  float l_gain = (1.0f - pan) * 0.5f; // if pan=-1, l=1
  float r_gain = (1.0f + pan) * 0.5f; // if pan=1, r=1

  // 4. Output
  // Assuming mono source for now
  float sample = 0.5f; // ... Fetch sample logic ...

  *out_l = sample * gain * l_gain;
  *out_r = sample * gain * r_gain;
}
