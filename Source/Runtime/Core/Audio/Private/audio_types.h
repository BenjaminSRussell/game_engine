#ifndef AUDIO_TYPES_H
#define AUDIO_TYPES_H

#include "../Public/unified_audio.h"
#include <unified_logger.h>
#include <unified_memory.h>

// Mocking Miniaudio types for independence or we could include miniaudio.h here
// Ideally, we include miniaudio in the .c files to keep this header clean
// But we need the sizing for struct definitions if we want direct embedding.
// For strict encapsulation, we'll use void* for the backend handle or forward
// declare.

// We will assume Miniaudio is used in the .c implementation layer.
// Here we define our unified structures.

typedef struct {
  u32 id;
  bool active;
  bool looping;
  AudioCategory category;

  // Transform
  Vec3 position;
  Vec3 velocity;

  // Properties
  f32 volume;
  f32 pitch;

  // Backend handle (void* to avoid exposing miniaudio.h)
  void *backend_handle;
} AudioSourceInternal;

struct AudioSystem {
  AudioConfig config;

  AudioSourceInternal *sources;
  u32 max_sources;
  u32 active_source_count;

  f32 master_volume;
  f32 category_volumes[AUDIO_CATEGORY_COUNT];

  // Listener
  Vec3 listener_pos;
  Vec3 listener_forward;
  Vec3 listener_up;

  // Backend engine handle
  void *engine;
};

#endif // AUDIO_TYPES_H
