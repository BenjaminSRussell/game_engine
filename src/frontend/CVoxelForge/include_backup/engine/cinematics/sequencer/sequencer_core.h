/**
 * =================================================================================================
 *                              CINEMATICS - SEQUENCER SYSTEM
 *                                 Agent: AGENT_CINEMA_1
 * =================================================================================================
 *
 * Timeline-based sequencer for cutscenes, cinematics, and in-game events.
 * Similar to Unreal's Sequencer or Unity's Timeline.
 *
 * =================================================================================================
 */

#ifndef CINEMATICS_SEQUENCER_H
#define CINEMATICS_SEQUENCER_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    TRACK TYPES
 * =================================================================================================
 */

typedef enum TrackType {
  TRACK_TYPE_CAMERA,           // Camera position, rotation, FOV
  TRACK_TYPE_ACTOR,            // Actor transforms and animations
  TRACK_TYPE_PROPERTY,         // Any serializable property
  TRACK_TYPE_AUDIO,            // Sound playback
  TRACK_TYPE_EVENT,            // Gameplay events/triggers
  TRACK_TYPE_PARTICLE,         // VFX activation
  TRACK_TYPE_LIGHT,            // Light properties
  TRACK_TYPE_MATERIAL,         // Material parameters
  TRACK_TYPE_LEVEL_VISIBILITY, // Show/hide level sections
  TRACK_TYPE_COUNT
} TrackType;

// TODO(AGENT_CINEMA_1): Add custom track type registration [Difficulty: 6]
// TODO(AGENT_CINEMA_1): Add track type icons for UI [Difficulty: 3]

/* =================================================================================================
 *                                    KEYFRAME SYSTEM
 * =================================================================================================
 */

typedef enum InterpolationType {
  INTERP_LINEAR,
  INTERP_CONSTANT,
  INTERP_CUBIC,
  INTERP_BEZIER,
  INTERP_SPRING,
  INTERP_BOUNCE,
} InterpolationType;

typedef struct Keyframe {
  float time;
  float value;
  InterpolationType interpolation;
  float tangent_in;
  float tangent_out;
  bool is_broken_tangents;
} Keyframe;

typedef struct KeyframeTrack {
  Keyframe *keyframes;
  uint32_t keyframe_count;
  uint32_t keyframe_capacity;
  char property_name[64];
  float default_value;
} KeyframeTrack;

// TODO(AGENT_CINEMA_1): Implement keyframe insertion [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement keyframe deletion [Difficulty: 3]
// TODO(AGENT_CINEMA_1): Implement keyframe move [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement keyframe copy/paste [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement keyframe interpolation evaluation
// [Difficulty: 6]
// TODO(AGENT_CINEMA_1): Implement bezier curve evaluation [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement auto-tangent calculation [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement keyframe flatten [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement keyframe reduction [Difficulty: 6]

/* =================================================================================================
 *                                    SEQUENCE TRACK
 * =================================================================================================
 */

typedef struct SequenceTrack {
  uint32_t id;
  char name[64];
  TrackType type;
  uint32_t target_entity;
  bool is_muted;
  bool is_locked;
  bool is_visible;
  float color[3];
  KeyframeTrack *property_tracks;
  uint32_t property_track_count;
} SequenceTrack;

// TODO(AGENT_CINEMA_1): Implement track creation [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement track deletion [Difficulty: 3]
// TODO(AGENT_CINEMA_1): Implement track reordering [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement track grouping [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement track copy/paste [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement track sampling at time [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement track blending with gameplay [Difficulty: 7]

/* =================================================================================================
 *                                    SEQUENCE
 * =================================================================================================
 */

typedef struct Sequence {
  uint32_t id;
  char name[64];
  float duration;
  float current_time;
  float playback_rate;
  bool is_playing;
  bool is_looping;
  SequenceTrack *tracks;
  uint32_t track_count;
  uint32_t track_capacity;
  // Sub-sequences
  uint32_t *subsequence_ids;
  uint32_t subsequence_count;
  // Markers
  struct {
    float time;
    char name[32];
  } markers[32];
  uint32_t marker_count;
} Sequence;

// TODO(AGENT_CINEMA_1): Implement sequence creation [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement sequence playback [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement sequence pause/resume [Difficulty: 3]
// TODO(AGENT_CINEMA_1): Implement sequence seek [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement sequence reverse playback [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement sequence looping modes [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement sub-sequence nesting [Difficulty: 6]
// TODO(AGENT_CINEMA_1): Implement marker-based events [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement sequence blending [Difficulty: 7]
// TODO(AGENT_CINEMA_1): Implement sequence save/load [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement sequence export to video [Difficulty: 8]

/* =================================================================================================
 *                                    CAMERA TRACK
 * =================================================================================================
 */

typedef struct CameraShot {
  float start_time;
  float end_time;
  char shot_name[32];
  uint32_t camera_entity;
  float fov_override;
  bool use_dof;
  float focus_distance;
  float aperture;
} CameraShot;

// TODO(AGENT_CINEMA_1): Implement camera rail system [Difficulty: 6]
// TODO(AGENT_CINEMA_1): Implement camera shake track [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement camera cut transitions [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement camera blend transitions [Difficulty: 6]
// TODO(AGENT_CINEMA_1): Implement camera follow target [Difficulty: 6]
// TODO(AGENT_CINEMA_1): Implement camera look-at constraint [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement depth of field animation [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement motion blur settings [Difficulty: 4]

/* =================================================================================================
 *                                    ACTOR TRACK
 * =================================================================================================
 */

// TODO(AGENT_CINEMA_1): Implement actor transform animation [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement actor animation clip playback [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement actor visibility control [Difficulty: 3]
// TODO(AGENT_CINEMA_1): Implement actor attachment/detachment [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement actor spawning/despawning [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement actor IK override [Difficulty: 6]
// TODO(AGENT_CINEMA_1): Implement actor look-at events [Difficulty: 5]

/* =================================================================================================
 *                                    SEQUENCER API
 * =================================================================================================
 */

typedef struct SequencerSystem {
  Sequence *sequences;
  uint32_t sequence_count;
  uint32_t sequence_capacity;
  Sequence *active_sequence;
  bool is_editor_mode;
  float global_time_scale;
} SequencerSystem;

// TODO(AGENT_CINEMA_1): Implement sequencer_init [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement sequencer_shutdown [Difficulty: 3]
// TODO(AGENT_CINEMA_1): Implement sequencer_update [Difficulty: 5]
// TODO(AGENT_CINEMA_1): Implement sequencer_play [Difficulty: 4]
// TODO(AGENT_CINEMA_1): Implement sequencer_stop [Difficulty: 3]
// Implemented: sequencer_get_current_time
// Implemented: sequencer_get_duration
// TODO(AGENT_CINEMA_1): Implement sequencer_editor_scrub [Difficulty: 5]

#endif // CINEMATICS_SEQUENCER_H
