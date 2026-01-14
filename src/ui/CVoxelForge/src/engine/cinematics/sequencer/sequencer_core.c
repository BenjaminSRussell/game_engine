// sequencer_core.c - Sequencer Core Implementation
#include "engine/cinematics/sequencer/sequencer_core.h"
#include <include/core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_SEQUENCES 16
#define MAX_TRACKS_PER_SEQUENCE 32
#define MAX_KEYFRAMES_PER_TRACK 256
#define SEQUENCER_DEFAULT_FPS 30.0f
#define SEQUENCER_TIME_SCALE 1.0f

typedef struct {
  float time;
  void* data;
  InterpolationType interpolation;
  bool is_valid;
} Keyframe;

typedef struct {
  TrackType type;
  char name[64];
  Keyframe keyframes[MAX_KEYFRAMES_PER_TRACK];
  u32 keyframe_count;
  bool is_enabled;
  bool is_muted;
} Track;

typedef struct {
  char name[128];
  float duration;
  float current_time;
  float playback_speed;
  bool is_playing;
  bool is_looping;
  u32 track_count;
  Track tracks[MAX_TRACKS_PER_SEQUENCE];
  u32 current_frame;
} Sequence;

typedef struct {
  Sequence sequences[MAX_SEQUENCES];
  u32 sequence_count;
  u32 active_sequence_index;
  float global_time;
  float playback_speed;
  bool is_playing;
  bool is_paused;
  SequencerPlayMode play_mode;
  float last_update_time;
} SequencerSystem;

static SequencerSystem g_sequencer = {0};

// Helper functions
static float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

static float cubic_interpolate(float p0, float p1, float p2, float p3, float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  return p0 * (1.0f - 3.0f * t2 + 2.0f * t3) +
         p1 * (3.0f * t2 - 4.0f * t3 + 1.0f) +
         p2 * (3.0f * t2 - 2.0f * t3) +
         p3 * t3;
}

static void* interpolate_keyframe(const Keyframe* from, const Keyframe* to, InterpolationType type, float t) {
  if (!from || !to || !from->data || !to->data) return NULL;
  
  void* result = malloc(sizeof(float));
  if (!result) return NULL;
  
  switch (type) {
    case INTERP_LINEAR:
      *((float*)result) = lerp(*((float*)from->data), *((float*)to->data), t);
      break;
    case INTERP_CONSTANT:
      *((float*)result) = *((float*)from->data);
      break;
    case INTERP_CUBIC:
      *((float*)result) = cubic_interpolate(
        *((float*)from->data), *((float*)from->data), *((float*)to->data), *((float*)to->data), t);
      break;
    case INTERP_BEZIER:
      *((float*)result) = lerp(*((float*)from->data), *((float*)to->data), t);
      break;
    case INTERP_SPRING:
      *((float*)result) = lerp(*((float*)from->data), *((float*)to->data), t);
      break;
    default:
      *((float*)result) = *((float*)from->data);
      break;
  }
  
  return result;
}

// Sequencer System API
SequencerSystem* sequencer_create(void) {
  memset(&g_sequencer, 0, sizeof(SequencerSystem));
  
  g_sequencer.playback_speed = 1.0f;
  g_sequencer.global_time = 0.0f;
  g_sequencer.is_playing = false;
  g_sequencer.is_paused = false;
  g_sequencer.play_mode = SEQUENCER_PLAY_NORMAL;
  g_sequencer.last_update_time = 0.0f;
  
  LOG_INFO("Sequencer system created");
  return &g_sequencer;
}

void sequencer_destroy(SequencerSystem *system) {
  if (!system) return;
  
  // Free all sequence data
  for (u32 i = 0; i < system->sequence_count; i++) {
    Sequence *seq = &system->sequences[i];
    
    for (u32 j = 0; j < seq->track_count; j++) {
      Track *track = &seq->tracks[j];
      
      for (u32 k = 0; k < track->keyframe_count; k++) {
        if (track->keyframes[k].data) {
          free(track->keyframes[k].data);
          track->keyframes[k].data = NULL;
        }
      }
    }
  }
  
  memset(&g_sequencer, 0, sizeof(SequencerSystem));
  LOG_INFO("Sequencer system destroyed");
}

u32 sequencer_create_sequence(SequencerSystem *system, const char *name, float duration) {
  if (!system || !name || system->sequence_count >= MAX_SEQUENCES) {
    return 0;
  }
  
  Sequence *seq = &system->sequences[system->sequence_count++];
  memset(seq, 0, sizeof(Sequence));
  
  strncpy(seq->name, name, sizeof(seq->name) - 1);
  seq->duration = duration;
  seq->current_time = 0.0f;
  seq->playback_speed = 1.0f;
  seq->is_playing = false;
  seq->is_looping = true;
  
  LOG_DEBUG("Created sequence '%s' with duration %.2f seconds", name, duration);
  return system->sequence_count;
}

void sequencer_remove_sequence(SequencerSystem *system, u32 sequence_index) {
  if (!system || sequence_index >= system->sequence_count) return;
  
  // Free sequence data
  Sequence *seq = &system->sequences[sequence_index];
  
  for (u32 i = 0; i < seq->track_count; i++) {
    Track *track = &seq->tracks[i];
    
    for (u32 j = 0; j < track->keyframe_count; j++) {
      if (track->keyframes[j].data) {
        free(track->keyframes[j].data);
        track->keyframes[j].data = NULL;
      }
    }
  }
  
  // Remove sequence from array
  for (u32 i = sequence_index; i < system->sequence_count - 1; i++) {
    system->sequences[i] = system->sequences[i + 1];
  }
  
  system->sequence_count--;
  
  LOG_DEBUG("Removed sequence at index %u", sequence_index);
}

u32 sequencer_add_track(SequencerSystem *system, u32 sequence_index, TrackType type, const char *name) {
  if (!system || sequence_index >= system->sequence_count) return 0;
  
  Sequence *seq = &system->sequences[sequence_index];
  if (seq->track_count >= MAX_TRACKS_PER_SEQUENCE) return 0;
  
  Track *track = &seq->tracks[seq->track_count++];
  memset(track, 0, sizeof(Track));
  
  track->type = type;
  strncpy(track->name, name, sizeof(track->name) - 1);
  track->is_enabled = true;
  track->is_muted = false;
  
  LOG_DEBUG("Added track '%s' (type: %d) to sequence %u", name, type, sequence_index);
  return seq->track_count;
}

u32 sequencer_add_keyframe(SequencerSystem *system, u32 sequence_index, u32 track_index, 
                             float time, void *data, InterpolationType interpolation) {
  if (!system || sequence_index >= system->sequence_count || 
      track_index >= system->sequences[sequence_index].track_count) {
    return 0;
  }
  
  Track *track = &system->sequences[sequence_index].tracks[track_index];
  if (track->keyframe_count >= MAX_KEYFRAMES_PER_TRACK) return 0;
  
  Keyframe *keyframe = &track->keyframes[track->keyframe_count++];
  memset(keyframe, 0, sizeof(Keyframe));
  
  keyframe->time = time;
  keyframe->data = data ? malloc(sizeof(float)) : NULL;
  if (data && keyframe->data) {
    memcpy(keyframe->data, data, sizeof(float));
  }
  keyframe->interpolation = interpolation;
  keyframe->is_valid = true;
  
  LOG_DEBUG("Added keyframe at time %.2f to track %u in sequence %u", time, track_index, sequence_index);
  return track->keyframe_count;
}

void sequencer_set_active_sequence(SequencerSystem *system, u32 sequence_index) {
  if (!system || sequence_index >= system->sequence_count) return;
  
  system->active_sequence_index = sequence_index;
  system->global_time = 0.0f;
  
  Sequence *seq = &system->sequences[sequence_index];
  seq->current_time = 0.0f;
  
  LOG_INFO("Set active sequence to '%s' (%.2f seconds)", seq->name, seq->duration);
}

void sequencer_play(SequencerSystem *system, SequencerPlayMode mode) {
  if (!system) return;
  
  system->is_playing = true;
  system->is_paused = false;
  system->play_mode = mode;
  system->last_update_time = 0.0f;
  
  if (mode == SEQUENCER_PLAY_FROM_START) {
    system->global_time = 0.0f;
  }
  
  LOG_INFO("Started sequencer playback (mode: %d)", mode);
}

void sequencer_pause(SequencerSystem *system) {
  if (!system) return;
  
  system->is_paused = true;
  LOG_INFO("Paused sequencer playback");
}

void sequencer_stop(SequencerSystem *system) {
  if (!system) return;
  
  system->is_playing = false;
  system->is_paused = false;
  system->global_time = 0.0f;
  
  LOG_INFO("Stopped sequencer playback");
}

void sequencer_seek(SequencerSystem *system, float time) {
  if (!system) return;
  
  system->global_time = fmaxf(0.0f, fminf(time, sequencer_get_duration(system)));
  LOG_DEBUG("Seeked sequencer to time %.2f", system->global_time);
}

void sequencer_update(SequencerSystem *system, float delta_time) {
  if (!system || !system->is_playing || system->is_paused) return;
  
  system->global_time += delta_time * system->playback_speed;
  
  // Check if sequence has ended
  Sequence *seq = &system->sequences[system->active_sequence_index];
  if (system->global_time >= seq->duration) {
    if (seq->is_looping) {
      system->global_time = fmodf(system->global_time, seq->duration);
    } else {
      sequencer_stop(system);
      return;
    }
  }
  
  // Update sequence time
  seq->current_time = system->global_time;
  
  // Update all tracks
  for (u32 i = 0; i < seq->track_count; i++) {
    Track *track = &seq->tracks[i];
    
    if (!track->is_enabled || track->is_muted) continue;
    
    sequencer_update_track(track, seq->current_time);
  }
  
  system->last_update_time = system->global_time;
}

void sequencer_update_track(Track *track, float current_time) {
  if (!track || track->keyframe_count == 0) return;
  
  // Find surrounding keyframes
  Keyframe *prev_keyframe = NULL;
  Keyframe *next_keyframe = NULL;
  
  for (u32 i = 0; i < track->keyframe_count; i++) {
    Keyframe *keyframe = &track->keyframes[i];
    
    if (keyframe->time <= current_time) {
      prev_keyframe = keyframe;
    }
    
    if (keyframe->time >= current_time && !next_keyframe) {
      next_keyframe = keyframe;
      break;
    }
  }
  
  if (!prev_keyframe) {
    // Before first keyframe
    prev_keyframe = &track->keyframes[0];
  }
  
  if (!next_keyframe) {
    // At or after last keyframe
    next_keyframe = prev_keyframe;
  }
  
  // Calculate interpolation factor
  float t = 0.0f;
  if (next_keyframe->time != prev_keyframe->time) {
    t = (current_time - prev_keyframe->time) / (next_keyframe->time - prev_keyframe->time);
  }
  
  // Interpolate between keyframes
  if (prev_keyframe->data && next_keyframe->data) {
    void* interpolated_data = interpolate_keyframe(prev_keyframe, next_keyframe, 
                                                   prev_keyframe->interpolation, t);
    
    if (interpolated_data) {
      // Apply interpolated data to the appropriate target
      sequencer_apply_keyframe_data(track, interpolated_data);
      free(interpolated_data);
    }
  }
}

void sequencer_apply_keyframe_data(Track *track, void *data) {
  if (!track || !data) return;
  
  // This would integrate with the actual engine systems
  // For now, we'll just log the update
  LOG_DEBUG("Applied keyframe data to track '%s'", track->name);
}

float sequencer_get_current_time(SequencerSystem *system) {
  if (!system || system->active_sequence_index >= system->sequence_count) {
    return 0.0f;
  }
  return system->sequences[system->active_sequence_index].current_time;
}

float sequencer_get_duration(SequencerSystem *system) {
  if (!system || system->active_sequence_index >= system->sequence_count) {
    return 0.0f;
  }
  return system->sequences[system->active_sequence].duration;
}

float sequencer_get_progress(SequencerSystem *system) {
  float duration = sequencer_get_duration(system);
  return duration > 0.0f ? sequencer_get_current_time(system) / duration : 0.0f;
}

bool sequencer_is_playing(SequencerSystem *system) {
  return system ? system->is_playing && !system->is_paused : false;
}

bool sequencer_is_paused(SequencerSystem *system) {
  return system ? system->is_paused : false;
}

void sequencer_set_playback_speed(SequencerSystem *system, float speed) {
  if (!system) return;
  
  system->playback_speed = fmaxf(0.1f, speed);
  LOG_DEBUG("Set sequencer playback speed to %.2fx", system->playback_speed);
}

void sequencer_set_looping(SequencerSystem *system, u32 sequence_index, bool looping) {
  if (!system || sequence_index >= system->sequence_count) return;
  
  system->sequences[sequence_index].is_looping = looping;
  LOG_DEBUG("Set sequence %u looping to %s", sequence_index, looping ? "enabled" : "disabled");
}

void sequencer_set_track_muted(SequencerSystem *system, u32 sequence_index, u32 track_index, bool muted) {
  if (!system || sequence_index >= system->sequence_count || 
      track_index >= system->sequences[sequence_index].track_count) {
    return;
  }
  
  system->sequences[sequence_index].tracks[track_index].is_muted = muted;
  LOG_DEBUG("Set track %u in sequence %u muted to %s", track_index, sequence_index, muted ? "muted" : "unmuted");
}

void sequencer_set_track_enabled(SequencerSystem *system, u32 sequence_index, u32 track_index, bool enabled) {
  if (!system || sequence_index >= system->sequence_count || 
      track_index >= system->sequences[sequence_index].track_count) {
    return;
  }
  
  system->sequences[sequence_index].tracks[track_index].is_enabled = enabled;
  LOG_DEBUG("Set track %u in sequence %u enabled to %s", track_index, sequence_index, enabled ? "enabled" : "disabled");
}

// Utility functions
u32 sequencer_get_sequence_count(const SequencerSystem *system) {
  return system ? system->sequence_count : 0;
}

const char* sequencer_get_sequence_name(const SequencerSystem *system, u32 sequence_index) {
  if (!system || sequence_index >= system->sequence_count) return NULL;
  return system->sequences[sequence_index].name;
}

u32 sequencer_get_track_count(const SequencerSystem *system, u32 sequence_index) {
  if (!system || sequence_index >= system->sequence_count) return 0;
  return system->sequences[sequence_index].track_count;
}

const char* sequencer_get_track_name(const SequencerSystem *system, u32 sequence_index, u32 track_index) {
  if (!system || sequence_index >= system->sequence_count || 
      track_index >= system->sequences[sequence_index].track_count) {
    return NULL;
  }
  return system->sequences[sequence_index].tracks[track_index].name;
}

TrackType sequencer_get_track_type(const SequencerSystem *system, u32 sequence_index, u32 track_index) {
  if (!system || sequence_index >= system->sequence_count || 
      track_index >= system->sequences[sequence_index].track_count) {
    return TRACK_TYPE_COUNT;
  }
  return system->sequences[sequence_index].tracks[track_index].type;
}

u32 sequencer_get_keyframe_count(const SequencerSystem *system, u32 sequence_index, u32 track_index) {
  if (!system || sequence_index >= system->sequence_count || 
      track_index >= system->sequences[sequence_index].track_count) {
    return 0;
  }
  return system->sequences[sequence_index].tracks[track_index].keyframe_count;
}

float sequencer_get_keyframe_time(const SequencerSystem *system, u32 sequence_index, u32 track_index, u32 keyframe_index) {
  if (!system || sequence_index >= system->sequence_count || 
      track_index >= system->sequences[sequence_index].track_count ||
      keyframe_index >= system->sequences[sequence_index].tracks[track_index].keyframe_count) {
    return 0.0f;
  }
  return system->sequences[sequence_index].tracks[track_index].keyframes[keyframe_index].time;
}

void sequencer_get_statistics(const SequencerSystem *system, u32 *out_sequences, u32 *out_tracks, 
                              u32 *out_keyframes, float *out_total_duration) {
  if (!system || !out_sequences || !out_tracks || !out_keyframes || !out_total_duration) return;
  
  *out_sequences = system->sequence_count;
  *out_tracks = 0;
  *out_keyframes = 0;
  *out_total_duration = 0.0f;
  
  for (u32 i = 0; i < system->sequence_count; i++) {
    const Sequence *seq = &system->sequences[i];
    *out_tracks += seq->track_count;
    *out_keyframes += seq->track_count * MAX_KEYFRAMES_PER_TRACK;
    *out_total_duration += seq->duration;
  }
}

void sequencer_debug_print_status(const SequencerSystem *system) {
  if (!system) return;
  
  LOG_INFO("=== Sequencer Status ===");
  LOG_INFO("Sequences: %u", system->sequence_count);
  LOG_INFO("Active sequence: %u", system->active_sequence_index);
  LOG_INFO("Global time: %.2f", system->global_time);
  LOG_INFO("Playback speed: %.2fx", system->playback_speed);
  LOG_INFO("Playing: %s", system->is_playing ? "yes" : "no");
  LOG_INFO("Paused: %s", system->is_paused ? "yes" : "no");
  
  if (system->active_sequence_index < system->sequence_count) {
    const Sequence *seq = &system->sequences[system->active_sequence_index];
    LOG_INFO("Active sequence: '%s'", seq->name);
    LOG_INFO("Duration: %.2f seconds", seq->duration);
    LOG_INFO("Current time: %.2f seconds", seq->current_time);
    LOG_INFO("Progress: %.1f%%", sequencer_get_progress(system) * 100.0f);
    LOG_INFO("Tracks: %u", seq->track_count);
    LOG_INFO("Looping: %s", seq->is_looping ? "yes" : "no");
    
    for (u32 i = 0; i < seq->track_count; i++) {
      const Track *track = &seq->tracks[i];
      LOG_DEBUG("  Track %u: %s (%s, %s, %u keyframes)", 
               i, track->name,
               track->is_enabled ? "enabled" : "disabled",
               track->is_muted ? "muted" : "unmuted",
               track->keyframe_count);
    }
  }
  
  LOG_INFO("=====================");
}
