#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "include/common.h"
#include "math/quat.h"
#include "math/vec3.h"

#define MAX_TRACKS 32
#define MAX_KEYFRAMES 256

typedef enum {
  TRACK_TYPE_TRANSFORM,
  TRACK_TYPE_CAMERA,
  TRACK_TYPE_LIGHT,
  TRACK_TYPE_EVENT,
  TRACK_TYPE_AUDIO
} TrackType;

typedef struct {
  f32 time;
  union {
    Vec3 vec3_value;
    Quat quat_value;
    f32 float_value;
    u32 event_id;
  } value;
} Keyframe;

typedef struct {
  char name[64];
  TrackType type;
  Keyframe keyframes[MAX_KEYFRAMES];
  u32 keyframe_count;
  void *target; // Entity/Camera/Light reference
} Track;

typedef struct {
  char name[128];
  Track tracks[MAX_TRACKS];
  u32 track_count;

  f32 current_time;
  f32 duration;
  bool playing;
  bool looping;
} Sequencer;

#ifdef __cplusplus
extern "C" {
#endif

Sequencer *sequencer_create(const char *name, f32 duration);
void sequencer_destroy(Sequencer *seq);

u32 sequencer_add_track(Sequencer *seq, const char *name, TrackType type,
                        void *target);
void sequencer_add_keyframe(Sequencer *seq, u32 track_id, f32 time,
                            void *value);

void sequencer_play(Sequencer *seq);
void sequencer_pause(Sequencer *seq);
void sequencer_stop(Sequencer *seq);
void sequencer_seek(Sequencer *seq, f32 time);

void sequencer_update(Sequencer *seq, f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif
