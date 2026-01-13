/*
 * ui_animation_sequencer.h
 * UI Animation Sequencing System
 * Timeline-based animation sequencing and control
 *
 * Part of UI Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_ANIMATION_SEQUENCER_H
#define UI_ANIMATION_SEQUENCER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================
 */

/* Animation track types */
typedef enum ui_animation_track_type {
    UI_TRACK_PROPERTY,
    UI_TRACK_EVENT,
    UI_TRACK_AUDIO,
    UI_TRACK_EFFECT,
    UI_TRACK_TRANSITION
} ui_animation_track_type_t;

/* Timeline playback states */
typedef enum ui_timeline_state {
    UI_TIMELINE_STOPPED,
    UI_TIMELINE_PLAYING,
    UI_TIMELINE_PAUSED,
    UI_TIMELINE_SEEKING
} ui_timeline_state_t;

/* Animation clip handle */
typedef struct ui_animation_clip_handle {
    uint32_t id;
} ui_animation_clip_handle_t;

/* Timeline handle */
typedef struct ui_timeline_handle {
    uint32_t id;
} ui_timeline_handle_t;

/* Track handle */
typedef struct ui_animation_track_handle {
    uint32_t id;
} ui_animation_track_handle_t;

/* Animation clip descriptor */
typedef struct ui_animation_clip_desc {
    /* Clip properties */
    const char *name;
    float duration;
    float start_time;
    float end_time;
    bool loop;
    
    /* Playback speed */
    float playback_speed;
    
    /* Blending */
    float blend_in_time;
    float blend_out_time;
    
    /* User data */
    void *user_data;
} ui_animation_clip_desc_t;

/* Animation track descriptor */
typedef struct ui_animation_track_desc {
    /* Track properties */
    ui_animation_track_type_t type;
    const char *name;
    bool enabled;
    bool muted;
    
    /* Target element */
    uint32_t element_id;
    const char *property_name;
    
    /* Track timing */
    float start_time;
    float duration;
    
    /* Animation data */
    ui_animation_clip_handle_t clip;
    
    /* Track-specific data */
    union {
        struct {
            /* Property track data */
            ui_animation_curve_handle_t curve;
        } property;
        
        struct {
            /* Event track data */
            void (*event_callback)(uint32_t element_id, float time, void *user_data);
            void *user_data;
        } event;
        
        struct {
            /* Audio track data */
            const char *audio_file;
            float volume;
            bool loop;
        } audio;
    } track_data;
    
    /* User data */
    void *user_data;
} ui_animation_track_desc_t;

/* Timeline descriptor */
typedef struct ui_timeline_desc {
    /* Timeline properties */
    const char *name;
    float duration;
    float frame_rate;
    
    /* Playback settings */
    bool loop;
    float start_time;
    float end_time;
    
    /* Initial tracks */
    uint32_t track_count;
    const ui_animation_track_desc_t *tracks;
    
    /* User data */
    void *user_data;
} ui_timeline_desc_t;

/* Animation clip state */
typedef struct ui_animation_clip_state {
    uint32_t id;
    char name[64];
    
    /* Timing */
    float duration;
    float start_time;
    float end_time;
    bool loop;
    
    /* Playback */
    float playback_speed;
    float blend_in_time;
    float blend_out_time;
    
    /* State */
    bool active;
    float current_time;
    float weight;
    
    /* User data */
    void *user_data;
    
    /* Next in linked list */
    struct ui_animation_clip_state *next;
} ui_animation_clip_state_t;

/* Animation track state */
typedef struct ui_animation_track_state {
    uint32_t id;
    ui_animation_track_type_t type;
    char name[64];
    
    /* State */
    bool enabled;
    bool muted;
    bool active;
    
    /* Target */
    uint32_t element_id;
    char property_name[64];
    
    /* Timing */
    float start_time;
    float duration;
    float current_time;
    
    /* Animation data */
    ui_animation_clip_handle_t clip;
    
    /* Track-specific state */
    union {
        struct {
            /* Property track state */
            ui_animation_curve_handle_t curve;
            float current_value;
        } property;
        
        struct {
            /* Event track state */
            void (*event_callback)(uint32_t element_id, float time, void *user_data);
            void *user_data;
            bool last_event_triggered;
        } event;
        
        struct {
            /* Audio track state */
            char audio_file[256];
            float volume;
            bool loop;
            bool playing;
        } audio;
    } track_state;
    
    /* User data */
    void *user_data;
    
    /* Next in linked list */
    struct ui_animation_track_state *next;
} ui_animation_track_state_t;

/* Timeline state */
typedef struct ui_timeline_state {
    uint32_t id;
    char name[64];
    
    /* Timing */
    float duration;
    float frame_rate;
    float current_time;
    float start_time;
    float end_time;
    
    /* Playback */
    ui_timeline_state_t state;
    bool loop;
    float playback_speed;
    
    /* Tracks */
    ui_animation_track_state_t *tracks;
    uint32_t track_count;
    
    /* User data */
    void *user_data;
    
    /* Next in linked list */
    struct ui_timeline_state *next;
} ui_timeline_state_t;

/* Timeline evaluation result */
typedef struct ui_timeline_evaluation_result {
    float time;
    ui_timeline_state_t state;
    bool valid;
} ui_timeline_evaluation_result_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/* Initialization */
int ui_animation_sequencer_init(void);
void ui_animation_sequencer_shutdown(void);

/* Timeline Management */
int ui_timeline_create(
    ui_timeline_handle_t *out_handle,
    const ui_timeline_desc_t *desc
);
void ui_timeline_destroy(ui_timeline_handle_t handle);

/* Timeline Playback Control */
void ui_timeline_play(ui_timeline_handle_t handle);
void ui_timeline_pause(ui_timeline_handle_t handle);
void ui_timeline_stop(ui_timeline_handle_t handle);
void ui_timeline_seek(ui_timeline_handle_t handle, float time);
void ui_timeline_restart(ui_timeline_handle_t handle);

/* Timeline Properties */
int ui_timeline_set_loop(ui_timeline_handle_t handle, bool loop);
int ui_timeline_set_playback_speed(ui_timeline_handle_t handle, float speed);
int ui_timeline_set_time_range(ui_timeline_handle_t handle, float start, float end);
float ui_timeline_get_duration(ui_timeline_handle_t handle);
float ui_timeline_get_current_time(ui_timeline_handle_t handle);
ui_timeline_state_t ui_timeline_get_state(ui_timeline_handle_t handle);

/* Track Management */
int ui_timeline_add_track(
    ui_timeline_handle_t timeline,
    ui_animation_track_handle_t *out_handle,
    const ui_animation_track_desc_t *desc
);
void ui_timeline_remove_track(ui_timeline_handle_t timeline, ui_animation_track_handle_t track);

/* Track Properties */
int ui_track_set_enabled(ui_animation_track_handle_t track, bool enabled);
int ui_track_set_muted(ui_animation_track_handle_t track, bool muted);
int ui_track_set_timing(ui_animation_track_handle_t track, float start, float duration);
bool ui_track_is_enabled(ui_animation_track_handle_t track);
bool ui_track_is_muted(ui_animation_track_handle_t track);

/* Clip Management */
int ui_clip_create(
    ui_animation_clip_handle_t *out_handle,
    const ui_animation_clip_desc_t *desc
);
void ui_clip_destroy(ui_animation_clip_handle_t clip);

/* Clip Properties */
int ui_clip_set_loop(ui_animation_clip_handle_t clip, bool loop);
int ui_clip_set_playback_speed(ui_animation_clip_handle_t clip, float speed);
int ui_clip_set_blending(ui_animation_clip_handle_t clip, float blend_in, float blend_out);
float ui_clip_get_duration(ui_animation_clip_handle_t clip);

/* System Update */
void ui_animation_sequencer_update(float delta_time);

/* Timeline Evaluation */
ui_timeline_evaluation_result_t ui_timeline_evaluate(ui_timeline_handle_t handle, float time);

/* Batch Operations */
void ui_timeline_play_all(void);
void ui_timeline_pause_all(void);
void ui_timeline_stop_all(void);
void ui_timeline_seek_all(float time);

/* Timeline Export/Import */
int ui_timeline_export(ui_timeline_handle_t handle, const char *filename);
int ui_timeline_import(ui_timeline_handle_t *out_handle, const char *filename);

/* Statistics */
uint32_t ui_animation_sequencer_get_timeline_count(void);
uint32_t ui_animation_sequencer_get_track_count(ui_timeline_handle_t timeline);
uint32_t ui_animation_sequencer_get_active_count(void);

/* ============================================================================
 * TIMELINE UTILITIES
 * ============================================================================
 */

/* Time utilities */
float ui_timeline_frames_to_time(ui_timeline_handle_t timeline, uint32_t frames);
uint32_t ui_timeline_time_to_frames(ui_timeline_handle_t timeline, float time);
float ui_timeline_snap_to_frame(ui_timeline_handle_t timeline, float time);

/* Timeline markers */
typedef struct ui_timeline_marker {
    float time;
    const char *label;
    uint32_t color;
} ui_timeline_marker_t;

int ui_timeline_add_marker(ui_timeline_handle_t timeline, const ui_timeline_marker_t *marker);
void ui_timeline_remove_marker(ui_timeline_handle_t timeline, float time);
uint32_t ui_timeline_get_marker_count(ui_timeline_handle_t timeline);
ui_timeline_marker_t ui_timeline_get_marker(ui_timeline_handle_t timeline, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* UI_ANIMATION_SEQUENCER_H */
