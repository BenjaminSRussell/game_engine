/**
 * @file audio_api.h
 * @brief Audio subsystem API
 */

#ifndef VOXELFORGE_AUDIO_API_H
#define VOXELFORGE_AUDIO_API_H

#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct AudioEngine AudioEngine;
typedef struct AudioSource AudioSource;
typedef struct AudioClip AudioClip;
typedef struct AudioListener AudioListener;
typedef struct AudioGroup AudioGroup;

// ============================================================================
// Audio Clip
// ============================================================================

typedef enum AudioFormat {
  AUDIO_FORMAT_MONO_8 = 0,
  AUDIO_FORMAT_MONO_16,
  AUDIO_FORMAT_STEREO_8,
  AUDIO_FORMAT_STEREO_16,
  AUDIO_FORMAT_MONO_FLOAT,
  AUDIO_FORMAT_STEREO_FLOAT,
} AudioFormat;

typedef struct AudioClipDesc {
  const void *data;
  usize data_size;
  AudioFormat format;
  u32 sample_rate;
  b8 streaming;
  const char *debug_name;
} AudioClipDesc;

VF_API AudioClip *audio_clip_create(const AudioClipDesc *desc);
VF_API AudioClip *audio_clip_load(const char *path);
VF_API void audio_clip_destroy(AudioClip *clip);

// ============================================================================
// Audio Source
// ============================================================================

typedef struct AudioSourceDesc {
  Vec3 position;
  f32 volume; // 0.0 - 1.0
  f32 pitch;  // 0.5 - 2.0
  f32 min_distance;
  f32 max_distance;
  b8 loop;
  b8 spatial; // 3D positioned
  AudioGroup *group;
} AudioSourceDesc;

VF_API AudioSource *audio_source_create(const AudioSourceDesc *desc);
VF_API void audio_source_destroy(AudioSource *source);
VF_API void audio_source_play(AudioSource *source, AudioClip *clip);
VF_API void audio_source_pause(AudioSource *source);
VF_API void audio_source_stop(AudioSource *source);
VF_API b8 audio_source_is_playing(AudioSource *source);
VF_API void audio_source_set_position(AudioSource *source, Vec3 position);
VF_API void audio_source_set_velocity(AudioSource *source, Vec3 velocity);
VF_API void audio_source_set_volume(AudioSource *source, f32 volume);
VF_API void audio_source_set_pitch(AudioSource *source, f32 pitch);
VF_API void audio_source_set_loop(AudioSource *source, b8 loop);

// ============================================================================
// Audio Listener
// ============================================================================

VF_API void audio_listener_set_position(Vec3 position);
VF_API void audio_listener_set_orientation(Vec3 forward, Vec3 up);
VF_API void audio_listener_set_velocity(Vec3 velocity);

// ============================================================================
// Audio Groups (Mixing)
// ============================================================================

VF_API AudioGroup *audio_group_create(const char *name);
VF_API void audio_group_destroy(AudioGroup *group);
VF_API void audio_group_set_volume(AudioGroup *group, f32 volume);
VF_API void audio_group_set_muted(AudioGroup *group, b8 muted);
VF_API void audio_group_set_parent(AudioGroup *group, AudioGroup *parent);

// ============================================================================
// Engine
// ============================================================================

VF_API VF_Result audio_init(void);
VF_API void audio_shutdown(void);
VF_API void audio_update(f32 delta_time);
VF_API void audio_set_master_volume(f32 volume);
VF_API f32 audio_get_master_volume(void);
VF_API void audio_pause_all(void);
VF_API void audio_resume_all(void);

// One-shot playback (convenience)
VF_API void audio_play_oneshot(AudioClip *clip, Vec3 position, f32 volume);
VF_API void audio_play_2d(AudioClip *clip, f32 volume);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_AUDIO_API_H
