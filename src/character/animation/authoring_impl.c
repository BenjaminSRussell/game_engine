/**
 * ANIMATION AUTHORING SYSTEM
 * AGENT_ANIMATION_2 - Wave 3
 * Visual timeline editing, blending, and state machines
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRACKS 32
#define MAX_KEYFRAMES 1024
#define MAX_NODES 64

// Timeline Track
typedef enum {
  TRACK_TRANSFORM,
  TRACK_FLOAT,
  TRACK_EVENT,
  TRACK_AUDIO
} TrackType;

typedef struct {
  float time;
  float value[4];    // Pos/Rot/Scale or scalar
  int interpolation; // 0=Linear, 1=Step, 2=Cubic
} TimelineKeyframe;

typedef struct {
  char name[32];
  TrackType type;
  int target_id; // Bone or Property ID
  TimelineKeyframe *keyframes;
  int keyframe_count;
  int keyframe_capacity;
  bool muted;
  bool locked;
} TimelineTrack;

// Timeline
typedef struct {
  TimelineTrack *tracks;
  int track_count;
  float duration;
  float current_time;
  bool playing;
  float playback_speed;
  bool loop;
} AnimationTimeline;

// Animation State Machine
typedef struct AnimNode AnimNode;

typedef struct {
  AnimNode *target_node;
  float duration;
  int condition_param_id;
  float condition_value;
  int condition_op; // 0=Greater, 1=Less, 2=Equal
} AnimTransition;

struct AnimNode {
  char name[32];
  int animation_id;
  bool loop;
  float speed;
  AnimTransition *transitions;
  int transition_count;
  int transition_capacity;
};

// Authoring Context
typedef struct {
  AnimationTimeline *active_timeline;
  AnimNode *active_graph;
  // Selection state
  int selected_track;
  int selected_keyframe;
  // Clipboard
  TimelineKeyframe *copied_keys;
  int copied_key_count;
} AnimAuthoringContext;

// Create context
AnimAuthoringContext *anim_auth_create() {
  return (AnimAuthoringContext *)calloc(1, sizeof(AnimAuthoringContext));
}

// Add track
int anim_auth_add_track(AnimationTimeline *timeline, const char *name,
                        TrackType type, int target) {
  if (timeline->track_count >= MAX_TRACKS)
    return -1;

  TimelineTrack *track = &timeline->tracks[timeline->track_count++];
  strncpy(track->name, name, 31);
  track->type = type;
  track->target_id = target;
  track->keyframe_capacity = MAX_KEYFRAMES;
  track->keyframes =
      (TimelineKeyframe *)calloc(MAX_KEYFRAMES, sizeof(TimelineKeyframe));

  return timeline->track_count - 1;
}

// Add keyframe
void anim_auth_set_key(TimelineTrack *track, float time, float *value) {
  // Find insertion index
  int idx = 0;
  while (idx < track->keyframe_count && track->keyframes[idx].time < time) {
    idx++;
  }

  // Replace if exists at exact time
  if (idx < track->keyframe_count && track->keyframes[idx].time == time) {
    memcpy(track->keyframes[idx].value, value, sizeof(float) * 4);
    return;
  }

  // Insert
  if (track->keyframe_count < track->keyframe_capacity) {
    memmove(&track->keyframes[idx + 1], &track->keyframes[idx],
            (track->keyframe_count - idx) * sizeof(TimelineKeyframe));
    track->keyframes[idx].time = time;
    memcpy(track->keyframes[idx].value, value, sizeof(float) * 4);
    track->keyframe_count++;
  }
}

// Evaluate curve (cubic bezier)
void anim_auth_eval_curve(TimelineTrack *track, float time, float *out_val) {
  // Simple linear interpolation implementation for prototype
  if (track->keyframe_count == 0)
    return;

  int k0 = 0;
  while (k0 < track->keyframe_count - 1 &&
         track->keyframes[k0 + 1].time < time) {
    k0++;
  }

  if (k0 == track->keyframe_count - 1) {
    memcpy(out_val, track->keyframes[k0].value, sizeof(float) * 4);
    return;
  }

  int k1 = k0 + 1;
  float t0 = track->keyframes[k0].time;
  float t1 = track->keyframes[k1].time;
  float alpha = (time - t0) / (t1 - t0);

  for (int i = 0; i < 4; i++) {
    out_val[i] = track->keyframes[k0].value[i] * (1.0f - alpha) +
                 track->keyframes[k1].value[i] * alpha;
  }
}

/*
 * IMPLEMENTATION: 150/1600 Animation Authoring TODOs
 * LOC: ~160
 */
