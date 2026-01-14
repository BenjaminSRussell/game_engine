#ifndef MINIAUDIO_STUB_H
#define MINIAUDIO_STUB_H

#include <core/types.h>

// Types
typedef struct ma_engine_config {
  int channels;
  int sampleRate;
} ma_engine_config;

typedef struct ma_engine {
  int dummy;
} ma_engine;

typedef struct ma_sound {
  int dummy;
} ma_sound;

typedef int ma_result;
#define MA_SUCCESS 0
#define MA_ERROR -1

#define MA_SOUND_FLAG_DECODE 0x1
#define MA_SOUND_FLAG_ASYNC 0x2
#define MA_SOUND_FLAG_STREAM 0x4

// Functions
static inline ma_engine_config ma_engine_config_init() {
  ma_engine_config c = {0, 0};
  return c;
}

static inline ma_result ma_engine_init(const ma_engine_config *config,
                                       ma_engine *pEngine) {
  (void)config;
  (void)pEngine;
  return MA_SUCCESS;
}

static inline void ma_engine_uninit(ma_engine *pEngine) { (void)pEngine; }

static inline void ma_engine_set_volume(ma_engine *pEngine, float volume) {
  (void)pEngine;
  (void)volume;
}

static inline ma_result
ma_sound_init_from_file(ma_engine *pEngine, const char *pFilePath, u32 flags,
                        void *pGroup, void *pParent, ma_sound *pSound) {
  (void)pEngine;
  (void)pFilePath;
  (void)flags;
  (void)pGroup;
  (void)pParent;
  (void)pSound;
  return MA_SUCCESS;
}

static inline void ma_sound_uninit(ma_sound *pSound) { (void)pSound; }

static inline void ma_sound_start(ma_sound *pSound) { (void)pSound; }

static inline void ma_sound_stop(ma_sound *pSound) { (void)pSound; }

static inline void ma_sound_set_volume(ma_sound *pSound, float volume) {
  (void)pSound;
  (void)volume;
}

static inline void ma_sound_set_pitch(ma_sound *pSound, float pitch) {
  (void)pSound;
  (void)pitch;
}

static inline void ma_sound_set_looping(ma_sound *pSound, bool isLooping) {
  (void)pSound;
  (void)isLooping;
}

static inline void ma_sound_set_position(ma_sound *pSound, float x, float y,
                                         float z) {
  (void)pSound;
  (void)x;
  (void)y;
  (void)z;
}

static inline bool ma_sound_is_playing(const ma_sound *pSound) {
  (void)pSound;
  return false;
}

static inline void ma_engine_listener_set_position(ma_engine *pEngine,
                                                   u32 listenerIndex, float x,
                                                   float y, float z) {
  (void)pEngine;
  (void)listenerIndex;
  (void)x;
  (void)y;
  (void)z;
}

static inline void ma_engine_listener_set_direction(ma_engine *pEngine,
                                                    u32 listenerIndex, float x,
                                                    float y, float z) {
  (void)pEngine;
  (void)listenerIndex;
  (void)x;
  (void)y;
  (void)z;
}

static inline void ma_engine_listener_set_world_up(ma_engine *pEngine,
                                                   u32 listenerIndex, float x,
                                                   float y, float z) {
  (void)pEngine;
  (void)listenerIndex;
  (void)x;
  (void)y;
  (void)z;
}

static inline void ma_engine_listener_set_velocity(ma_engine *pEngine,
                                                   u32 listenerIndex, float x,
                                                   float y, float z) {
  (void)pEngine;
  (void)listenerIndex;
  (void)x;
  (void)y;
  (void)z;
}

#endif // MINIAUDIO_STUB_H
