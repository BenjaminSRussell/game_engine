#ifndef TIMELINE_H
#define TIMELINE_H

#include <stdbool.h>

/**
 * Timeline / Sequencer System
 * Main timeline for non-linear editing of cutscenes and events
 */

// Forward declaration
struct CameraSequencer;

// Initialize timeline
bool timeline_init(double duration);
void timeline_shutdown(void);

// Playback control
void timeline_play(void);
void timeline_pause(void);
void timeline_stop(void);
void timeline_seek(double time);

// Update
void timeline_update(double delta_time);

// Getters
double timeline_get_current_time(void);
double timeline_get_duration(void);
struct CameraSequencer *timeline_get_camera_sequencer(void);

// Settings
void timeline_set_playback_speed(float speed);

#endif // TIMELINE_H
