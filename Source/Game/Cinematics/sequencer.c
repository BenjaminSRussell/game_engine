// src/engine/cinematic/sequencer.c
#include "include/cinematic/sequencer.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

Sequencer *sequencer_create(const char *name, f32 duration) {
  Sequencer *seq = (Sequencer *)calloc(1, sizeof(Sequencer));
  if (!seq)
    return NULL;

  strncpy(seq->name, name, sizeof(seq->name) - 1);
  seq->duration = duration;
  seq->current_time = 0.0f;
  seq->playing = false;
  seq->looping = false;

  LOG_INFO("Sequencer created: %s (%.2fs)", name, duration);
  return seq;
}

void sequencer_destroy(Sequencer *seq) {
  if (seq)
    free(seq);
}

u32 sequencer_add_track(Sequencer *seq, const char *name, TrackType type,
                        void *target) {
  if (!seq || seq->track_count >= MAX_TRACKS)
    return 0xFFFFFFFF;

  u32 id = seq->track_count++;
  Track *track = &seq->tracks[id];

  strncpy(track->name, name, sizeof(track->name) - 1);
  track->type = type;
  track->target = target;
  track->keyframe_count = 0;

  return id;
}

void sequencer_add_keyframe(Sequencer *seq, u32 track_id, f32 time,
                            void *value) {
  if (!seq || track_id >= seq->track_count)
    return;

  Track *track = &seq->tracks[track_id];
  if (track->keyframe_count >= MAX_KEYFRAMES)
    return;

  Keyframe *kf = &track->keyframes[track->keyframe_count++];
  kf->time = time;

  // Copy value based on track type
  switch (track->type) {
  case TRACK_TYPE_TRANSFORM:
    if (value)
      kf->value.vec3_value = *(Vec3 *)value;
    break;
  case TRACK_TYPE_CAMERA:
    if (value)
      kf->value.quat_value = *(Quat *)value;
    break;
  case TRACK_TYPE_LIGHT:
    if (value)
      kf->value.float_value = *(f32 *)value;
    break;
  case TRACK_TYPE_EVENT:
    if (value)
      kf->value.event_id = *(u32 *)value;
    break;
  default:
    break;
  }
}

void sequencer_play(Sequencer *seq) {
  if (!seq)
    return;
  seq->playing = true;
}

void sequencer_pause(Sequencer *seq) {
  if (!seq)
    return;
  seq->playing = false;
}

void sequencer_stop(Sequencer *seq) {
  if (!seq)
    return;
  seq->playing = false;
  seq->current_time = 0.0f;
}

void sequencer_seek(Sequencer *seq, f32 time) {
  if (!seq)
    return;
  seq->current_time = time;
  if (seq->current_time > seq->duration)
    seq->current_time = seq->duration;
  if (seq->current_time < 0.0f)
    seq->current_time = 0.0f;
}

// Simple linear interpolation helper
static f32 lerp_f32(f32 a, f32 b, f32 t) { return a + (b - a) * t; }

static Vec3 lerp_vec3(Vec3 a, Vec3 b, f32 t) {
  return (Vec3){lerp_f32(a.x, b.x, t), lerp_f32(a.y, b.y, t),
                lerp_f32(a.z, b.z, t)};
}

void sequencer_update(Sequencer *seq, f32 delta_time) {
  if (!seq || !seq->playing)
    return;

  seq->current_time += delta_time;

  // Handle looping
  if (seq->current_time > seq->duration) {
    if (seq->looping) {
      seq->current_time = fmodf(seq->current_time, seq->duration);
    } else {
      seq->current_time = seq->duration;
      seq->playing = false;
      return;
    }
  }

  // Evaluate all tracks
  for (u32 i = 0; i < seq->track_count; i++) {
    Track *track = &seq->tracks[i];
    if (track->keyframe_count == 0)
      continue;

    // Find keyframe range
    Keyframe *kf0 = NULL;
    Keyframe *kf1 = NULL;

    for (u32 j = 0; j < track->keyframe_count - 1; j++) {
      if (seq->current_time >= track->keyframes[j].time &&
          seq->current_time <= track->keyframes[j + 1].time) {
        kf0 = &track->keyframes[j];
        kf1 = &track->keyframes[j + 1];
        break;
      }
    }

    if (!kf0 || !kf1) {
      // Before first or after last keyframe
      if (seq->current_time < track->keyframes[0].time) {
        kf0 = &track->keyframes[0];
      } else {
        kf0 = &track->keyframes[track->keyframe_count - 1];
      }
    }

    // Interpolate
    if (kf0 && kf1) {
      f32 t = (seq->current_time - kf0->time) / (kf1->time - kf0->time);

      switch (track->type) {
      case TRACK_TYPE_TRANSFORM: {
        Vec3 result =
            lerp_vec3(kf0->value.vec3_value, kf1->value.vec3_value, t);
        // Apply to target (would need proper entity system integration)
        (void)result;
        break;
      }
      case TRACK_TYPE_LIGHT: {
        f32 result =
            lerp_f32(kf0->value.float_value, kf1->value.float_value, t);
        (void)result;
        break;
      }
      case TRACK_TYPE_EVENT: {
        // Events are discrete, trigger on exact time match
        if (fabsf(seq->current_time - kf0->time) < 0.016f) {
          // Fire event kf0->value.event_id
        }
        break;
      }
      default:
        break;
      }
    }
  }
}
