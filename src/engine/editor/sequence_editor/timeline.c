#include "editor/sequence_editor/timeline.h"
#include "editor/sequence_editor/camera_sequencer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                          TIMELINE / SEQUENCER IMPLEMENTATION
 * =================================================================================================
 *
 * PURPOSE: Non-linear editor for cutscenes and events.
 * =================================================================================================
 */

// Timeline structure
typedef struct {
  double duration;
  double current_time;
  bool is_playing;
  float playback_speed;

  // Camera sequencer integration
  struct CameraSequencer *camera_seq;

  // Track data would go here (in a full implementation)
  // For now we focus on camera sequencing
} Timeline;

static Timeline *g_timeline = NULL;

// Initialize timeline
bool timeline_init(double duration) {
  if (g_timeline) {
    printf("[Timeline] Already initialized\n");
    return false;
  }

  g_timeline = calloc(1, sizeof(Timeline));
  if (!g_timeline) {
    printf("[Timeline] Failed to allocate timeline\n");
    return false;
  }

  g_timeline->duration = duration;
  g_timeline->current_time = 0.0;
  g_timeline->is_playing = false;
  g_timeline->playback_speed = 1.0f;

  // Create camera sequencer
  g_timeline->camera_seq = camera_sequencer_create();

  printf("[Timeline] Initialized with duration: %.2f\n", duration);
  return true;
}

// Shutdown timeline
void timeline_shutdown(void) {
  if (!g_timeline)
    return;

  if (g_timeline->camera_seq) {
    camera_sequencer_destroy(g_timeline->camera_seq);
  }

  free(g_timeline);
  g_timeline = NULL;

  printf("[Timeline] Shutdown\n");
}

// Playback control
void timeline_play(void) {
  if (!g_timeline)
    return;
  g_timeline->is_playing = true;

  if (g_timeline->camera_seq) {
    camera_sequencer_play(g_timeline->camera_seq);
  }

  printf("[Timeline] Playing\n");
}

void timeline_pause(void) {
  if (!g_timeline)
    return;
  g_timeline->is_playing = false;

  if (g_timeline->camera_seq) {
    camera_sequencer_pause(g_timeline->camera_seq);
  }

  printf("[Timeline] Paused\n");
}

void timeline_stop(void) {
  if (!g_timeline)
    return;
  g_timeline->is_playing = false;
  g_timeline->current_time = 0.0;

  if (g_timeline->camera_seq) {
    camera_sequencer_stop(g_timeline->camera_seq);
  }

  printf("[Timeline] Stopped\n");
}

void timeline_seek(double time) {
  if (!g_timeline)
    return;

  g_timeline->current_time = time;
  if (g_timeline->current_time < 0.0) {
    g_timeline->current_time = 0.0;
  }
  if (g_timeline->current_time > g_timeline->duration) {
    g_timeline->current_time = g_timeline->duration;
  }

  if (g_timeline->camera_seq) {
    camera_sequencer_seek(g_timeline->camera_seq, g_timeline->current_time);
  }

  printf("[Timeline] Seeked to: %.2f\n", g_timeline->current_time);
}

// Update timeline
void timeline_update(double delta_time) {
  if (!g_timeline || !g_timeline->is_playing)
    return;

  g_timeline->current_time += delta_time * g_timeline->playback_speed;

  // Clamp to duration
  if (g_timeline->current_time > g_timeline->duration) {
    g_timeline->current_time = g_timeline->duration;
    timeline_stop();
  }

  // Update camera sequencer
  if (g_timeline->camera_seq) {
    camera_sequencer_update(g_timeline->camera_seq, delta_time);
  }
}

// Get current time
double timeline_get_current_time(void) {
  return g_timeline ? g_timeline->current_time : 0.0;
}

// Get duration
double timeline_get_duration(void) {
  return g_timeline ? g_timeline->duration : 0.0;
}

// Get camera sequencer
CameraSequencer *timeline_get_camera_sequencer(void) {
  return g_timeline ? g_timeline->camera_seq : NULL;
}

// Set playback speed
void timeline_set_playback_speed(float speed) {
  if (!g_timeline)
    return;
  g_timeline->playback_speed = speed;

  if (g_timeline->camera_seq) {
    camera_sequencer_set_playback_speed(g_timeline->camera_seq, speed);
  }

  printf("[Timeline] Playback speed: %.2f\n", speed);
}
