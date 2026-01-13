#ifndef CAMERA_SEQUENCER_H
#define CAMERA_SEQUENCER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Camera Sequencer - Cinematic camera animation and sequencing system
 * Supports multi-camera shots, cuts, transitions, and camera effects
 */

typedef struct Camera Camera;

// Camera Shot - Represents a single camera shot in the sequence
typedef struct {
  uint32_t shot_id;
  uint32_t camera_id;
  double start_time;
  double duration;

  // Transition settings
  enum {
    TRANSITION_CUT,
    TRANSITION_DISSOLVE,
    TRANSITION_FADE_BLACK,
    TRANSITION_FADE_WHITE,
    TRANSITION_WIPE,
    TRANSITION_PUSH
  } transition_type;
  double transition_duration;

  // Camera effects
  struct {
    bool enable_dof;
    float focal_distance;
    float aperture;
    float focal_length;

    bool enable_shake;
    float shake_intensity;
    float shake_frequency;

    bool enable_motion_blur;
    float motion_blur_amount;
  } effects;

  // Focus tracking
  bool use_focus_target;
  uint32_t focus_target_id; // Entity ID to track

  // Composition guides
  bool show_rule_of_thirds;
  bool show_safe_area;
  bool show_center_cross;
} CameraShot;

// Camera Track - Contains multiple shots
typedef struct {
  uint32_t track_id;
  char name[64];
  CameraShot *shots;
  uint32_t shot_count;
  uint32_t shot_capacity;
  bool is_active;
} CameraTrack;

// Camera Sequencer - Main sequencer system
typedef struct CameraSequencer {
  CameraTrack *tracks;
  uint32_t track_count;
  uint32_t track_capacity;

  double current_time;
  CameraShot *active_shot;
  Camera *active_camera;

  // Playback state
  bool is_playing;
  float playback_speed;

  // Preview settings
  uint32_t preview_width;
  uint32_t preview_height;
} CameraSequencer;

// Initialization
CameraSequencer *camera_sequencer_create(void);
void camera_sequencer_destroy(CameraSequencer *seq);

// Track management
uint32_t camera_sequencer_add_track(CameraSequencer *seq, const char *name);
void camera_sequencer_remove_track(CameraSequencer *seq, uint32_t track_id);
CameraTrack *camera_sequencer_get_track(CameraSequencer *seq,
                                        uint32_t track_id);

// Shot management
uint32_t camera_track_add_shot(CameraTrack *track, uint32_t camera_id,
                               double start_time, double duration);
void camera_track_remove_shot(CameraTrack *track, uint32_t shot_id);
CameraShot *camera_track_get_shot(CameraTrack *track, uint32_t shot_id);

// Sequencer control
void camera_sequencer_play(CameraSequencer *seq);
void camera_sequencer_pause(CameraSequencer *seq);
void camera_sequencer_stop(CameraSequencer *seq);
void camera_sequencer_seek(CameraSequencer *seq, double time);

// Update and evaluation
void camera_sequencer_update(CameraSequencer *seq, double delta_time);
CameraShot *camera_sequencer_get_active_shot(CameraSequencer *seq);
Camera *camera_sequencer_get_active_camera(CameraSequencer *seq);

// Effects
void camera_shot_set_dof(CameraShot *shot, float focal_distance,
                         float aperture);
void camera_shot_set_shake(CameraShot *shot, float intensity, float frequency);
void camera_shot_set_focus_target(CameraShot *shot, uint32_t target_id);

// Rendering helpers
void camera_sequencer_render_composition_guides(CameraSequencer *seq);
void camera_sequencer_get_viewport_rect(CameraSequencer *seq, int *x, int *y,
                                        int *w, int *h);

#endif // CAMERA_SEQUENCER_H
