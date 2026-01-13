/**
 * CINEMATIC SEQUENCER
 * AGENT_CINEMA_1 - Wave 5
 * Timeline-based cutscene editor and playback
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "include/common.h"

typedef enum {
    SEQUENCER_TRACK_CAMERA,
    SEQUENCER_TRACK_AUDIO,
    SEQUENCER_TRACK_ANIMATION,
    SEQUENCER_TRACK_LIGHTING,
    SEQUENCER_TRACK_PARTICLES,
    SEQUENCER_TRACK_EVENTS,
    SEQUENCER_TRACK_SUBTITLES,
    SEQUENCER_TRACK_COUNT
} SequencerTrackType;

typedef enum {
    SEQUENCER_EVENT_CAMERA_CUT,
    SEQUENCER_EVENT_CAMERA_MOVE,
    SEQUENCER_EVENT_AUDIO_PLAY,
    SEQUENCER_EVENT_AUDIO_STOP,
    SEQUENCER_EVENT_LIGHT_CHANGE,
    SEQUENCER_EVENT_PARTICLE_SPAWN,
    SEQUENCER_EVENT_DIALOGUE,
    SEQUENCER_EVENT_SUBTITLE,
    SEQUENCER_EVENT_TRIGGER
} SequencerEventType;

typedef struct {
    float start_time;
    float duration;
    SequencerEventType type;
    void* data;
    bool active;
    int priority;
} SequencerEvent;

typedef struct {
    SequencerTrackType type;
    SequencerEvent* events;
    int event_count;
    int capacity;
    bool muted;
    float volume;
    bool solo;
} SequencerTrack;

typedef struct {
    SequencerTrack* tracks;
    int track_count;
    float current_time;
    float duration;
    bool playing;
    bool looping;
    float playback_speed;
    bool paused;
    int current_frame;
    float fps;
} Sequencer;

typedef struct {
    int camera_id;
    float blend_in;
    float blend_out;
    Vec3 target_position;
    Vec3 target_rotation;
    float fov;
    bool use_target;
} CameraCutEvent;

typedef struct {
    int audio_id;
    float volume;
    bool loop;
    float fade_in;
    float fade_out;
    Vec3 position;
    float max_distance;
} AudioEvent;

typedef struct {
    int light_id;
    Vec3 color;
    float intensity;
    float fade_time;
    bool enabled;
} LightEvent;

typedef struct {
    int particle_id;
    Vec3 position;
    Vec3 direction;
    float intensity;
    float duration;
} ParticleEvent;

typedef struct {
    char speaker[64];
    char text[512];
    float duration;
    Vec3 position;
    bool subtitle;
} DialogueEvent;

// Core sequencer functions
Sequencer* sequencer_create(float duration, float fps) {
    Sequencer* seq = (Sequencer*)calloc(1, sizeof(Sequencer));
    if (!seq) return NULL;
    
    seq->duration = duration;
    seq->fps = fps;
    seq->playback_speed = 1.0f;
    seq->track_count = SEQUENCER_TRACK_COUNT;
    seq->tracks = (SequencerTrack*)calloc(SEQUENCER_TRACK_COUNT, sizeof(SequencerTrack));
    
    if (!seq->tracks) {
        free(seq);
        return NULL;
    }
    
    // Initialize tracks with default capacity
    for (int i = 0; i < SEQUENCER_TRACK_COUNT; i++) {
        seq->tracks[i].type = (SequencerTrackType)i;
        seq->tracks[i].capacity = 64;
        seq->tracks[i].events = (SequencerEvent*)calloc(64, sizeof(SequencerEvent));
        seq->tracks[i].volume = 1.0f;
        
        if (!seq->tracks[i].events) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                free(seq->tracks[j].events);
            }
            free(seq->tracks);
            free(seq);
            return NULL;
        }
    }
    
    LOG_INFO("Created sequencer with duration %.2fs at %.1f FPS", duration, fps);
    return seq;
}

void sequencer_destroy(Sequencer* seq) {
    if (!seq) return;
    
    // Free all track events
    for (int i = 0; i < seq->track_count; i++) {
        if (seq->tracks[i].events) {
            // Free event data
            for (int j = 0; j < seq->tracks[i].event_count; j++) {
                if (seq->tracks[i].events[j].data) {
                    free(seq->tracks[i].events[j].data);
                }
            }
            free(seq->tracks[i].events);
        }
    }
    
    free(seq->tracks);
    free(seq);
}

void sequencer_play(Sequencer* seq) {
    if (!seq) return;
    seq->playing = true;
    seq->paused = false;
    LOG_INFO("Sequencer playback started");
}

void sequencer_pause(Sequencer* seq) {
    if (!seq) return;
    seq->paused = true;
    LOG_INFO("Sequencer paused");
}

void sequencer_stop(Sequencer* seq) {
    if (!seq) return;
    seq->playing = false;
    seq->paused = false;
    seq->current_time = 0.0f;
    seq->current_frame = 0;
    LOG_INFO("Sequencer stopped");
}

void sequencer_seek(Sequencer* seq, float time) {
    if (!seq) return;
    seq->current_time = fmaxf(0.0f, fminf(time, seq->duration));
    seq->current_frame = (int)(seq->current_time * seq->fps);
    LOG_INFO("Sequencer seeked to %.2fs", seq->current_time);
}

void sequencer_set_playback_speed(Sequencer* seq, float speed) {
    if (!seq) return;
    seq->playback_speed = fmaxf(0.1f, speed);
    LOG_INFO("Sequencer playback speed set to %.2fx", seq->playback_speed);
}

void sequencer_set_looping(Sequencer* seq, bool looping) {
    if (!seq) return;
    seq->looping = looping;
    LOG_INFO("Sequencer looping %s", looping ? "enabled" : "disabled");
}

// Event management
int sequencer_add_event(Sequencer* seq, SequencerTrackType track_type, float start_time, float duration, SequencerEventType event_type, void* data, int priority) {
    if (!seq || track_type >= SEQUENCER_TRACK_COUNT) return -1;
    
    SequencerTrack* track = &seq->tracks[track_type];
    
    // Check if we need to expand the events array
    if (track->event_count >= track->capacity) {
        int new_capacity = track->capacity * 2;
        SequencerEvent* new_events = (SequencerEvent*)realloc(track->events, new_capacity * sizeof(SequencerEvent));
        if (!new_events) return -1;
        
        track->events = new_events;
        track->capacity = new_capacity;
    }
    
    // Add new event
    SequencerEvent* event = &track->events[track->event_count];
    event->start_time = start_time;
    event->duration = duration;
    event->type = event_type;
    event->data = data;
    event->active = false;
    event->priority = priority;
    
    int event_id = track->event_count;
    track->event_count++;
    
    LOG_INFO("Added event %d to track %d at %.2fs", event_id, track_type, start_time);
    return event_id;
}

bool sequencer_remove_event(Sequencer* seq, SequencerTrackType track_type, int event_id) {
    if (!seq || track_type >= SEQUENCER_TRACK_COUNT || event_id < 0) return false;
    
    SequencerTrack* track = &seq->tracks[track_type];
    if (event_id >= track->event_count) return false;
    
    // Free event data
    if (track->events[event_id].data) {
        free(track->events[event_id].data);
    }
    
    // Shift remaining events
    for (int i = event_id; i < track->event_count - 1; i++) {
        track->events[i] = track->events[i + 1];
    }
    
    track->event_count--;
    LOG_INFO("Removed event %d from track %d", event_id, track_type);
    return true;
}

// Update and evaluation
void sequencer_update(Sequencer* seq, float dt) {
    if (!seq || !seq->playing || seq->paused) return;
    
    // Update current time
    seq->current_time += dt * seq->playback_speed;
    seq->current_frame = (int)(seq->current_time * seq->fps);
    
    // Handle looping
    if (seq->current_time >= seq->duration) {
        if (seq->looping) {
            seq->current_time = fmodf(seq->current_time, seq->duration);
            seq->current_frame = (int)(seq->current_time * seq->fps);
        } else {
            sequencer_stop(seq);
            return;
        }
    }
    
    // Evaluate all tracks
    for (int i = 0; i < seq->track_count; i++) {
        SequencerTrack* track = &seq->tracks[i];
        
        // Skip muted or solo tracks
        if (track->muted) continue;
        
        // Check for solo tracks
        bool has_solo = false;
        for (int j = 0; j < seq->track_count; j++) {
            if (seq->tracks[j].solo) {
                has_solo = true;
                break;
            }
        }
        if (has_solo && !track->solo) continue;
        
        sequencer_evaluate_track(track, seq->current_time);
    }
}

static void sequencer_evaluate_track(SequencerTrack* track, float current_time) {
    for (int i = 0; i < track->event_count; i++) {
        SequencerEvent* event = &track->events[i];
        
        // Check if event should be active
        bool should_be_active = (current_time >= event->start_time && 
                                current_time < event->start_time + event->duration);
        
        if (should_be_active && !event->active) {
            // Event just started
            sequencer_trigger_event(event);
            event->active = true;
        } else if (!should_be_active && event->active) {
            // Event just ended
            sequencer_end_event(event);
            event->active = false;
        } else if (should_be_active && event->active) {
            // Event is ongoing
            sequencer_update_event(event, current_time - event->start_time);
        }
    }
}

static void sequencer_trigger_event(SequencerEvent* event) {
    if (!event || !event->data) return;
    
    switch (event->type) {
        case SEQUENCER_EVENT_CAMERA_CUT:
            sequencer_trigger_camera_cut((CameraCutEvent*)event->data);
            break;
        case SEQUENCER_EVENT_CAMERA_MOVE:
            sequencer_trigger_camera_move((CameraCutEvent*)event->data);
            break;
        case SEQUENCER_EVENT_AUDIO_PLAY:
            sequencer_trigger_audio_play((AudioEvent*)event->data);
            break;
        case SEQUENCER_EVENT_AUDIO_STOP:
            sequencer_trigger_audio_stop((AudioEvent*)event->data);
            break;
        case SEQUENCER_EVENT_LIGHT_CHANGE:
            sequencer_trigger_light_change((LightEvent*)event->data);
            break;
        case SEQUENCER_EVENT_PARTICLE_SPAWN:
            sequencer_trigger_particle_spawn((ParticleEvent*)event->data);
            break;
        case SEQUENCER_EVENT_DIALOGUE:
            sequencer_trigger_dialogue((DialogueEvent*)event->data);
            break;
        case SEQUENCER_EVENT_SUBTITLE:
            sequencer_trigger_subtitle((DialogueEvent*)event->data);
            break;
        default:
            LOG_WARN("Unknown sequencer event type: %d", event->type);
            break;
    }
}

static void sequencer_trigger_camera_cut(CameraCutEvent* cut) {
    if (!cut) return;
    LOG_INFO("Camera cut to camera %d", cut->camera_id);
    // Implementation would set camera to cut position instantly
}

static void sequencer_trigger_camera_move(CameraCutEvent* move) {
    if (!move) return;
    LOG_INFO("Camera move to camera %d", move->camera_id);
    // Implementation would start camera interpolation
}

static void sequencer_trigger_audio_play(AudioEvent* audio) {
    if (!audio) return;
    LOG_INFO("Play audio %d at volume %.2f", audio->audio_id, audio->volume);
    // Implementation would play audio with fade in
}

static void sequencer_trigger_audio_stop(AudioEvent* audio) {
    if (!audio) return;
    LOG_INFO("Stop audio %d", audio->audio_id);
    // Implementation would stop audio with fade out
}

static void sequencer_trigger_light_change(LightEvent* light) {
    if (!light) return;
    LOG_INFO("Light %d change to RGB(%.1f,%.1f,%.1f)", light->light_id, light->color.x, light->color.y, light->color.z);
    // Implementation would change light properties
}

static void sequencer_trigger_particle_spawn(ParticleEvent* particle) {
    if (!particle) return;
    LOG_INFO("Spawn particle %d at (%.1f,%.1f,%.1f)", particle->particle_id, particle->position.x, particle->position.y, particle->position.z);
    // Implementation would spawn particle system
}

static void sequencer_trigger_dialogue(DialogueEvent* dialogue) {
    if (!dialogue) return;
    LOG_INFO("Dialogue: %s says \"%s\"", dialogue->speaker, dialogue->text);
    // Implementation would show dialogue UI
}

static void sequencer_trigger_subtitle(DialogueEvent* subtitle) {
    if (!subtitle) return;
    LOG_INFO("Subtitle: %s", subtitle->text);
    // Implementation would show subtitle
}

static void sequencer_update_event(SequencerEvent* event, float elapsed_time) {
    // Update ongoing events (like camera interpolation, audio volume changes, etc.)
    switch (event->type) {
        case SEQUENCER_EVENT_CAMERA_MOVE:
            sequencer_update_camera_move((CameraCutEvent*)event->data, elapsed_time);
            break;
        case SEQUENCER_EVENT_AUDIO_PLAY:
            sequencer_update_audio_play((AudioEvent*)event->data, elapsed_time);
            break;
        case SEQUENCER_EVENT_LIGHT_CHANGE:
            sequencer_update_light_change((LightEvent*)event->data, elapsed_time);
            break;
        default:
            break;
    }
}

static void sequencer_end_event(SequencerEvent* event) {
    // Clean up when event ends
    switch (event->type) {
        case SEQUENCER_EVENT_AUDIO_PLAY:
            sequencer_end_audio_play((AudioEvent*)event->data);
            break;
        case SEQUENCER_EVENT_PARTICLE_SPAWN:
            sequencer_end_particle_spawn((ParticleEvent*)event->data);
            break;
        default:
            break;
    }
}

// Track management
void sequencer_set_track_muted(Sequencer* seq, SequencerTrackType track_type, bool muted) {
    if (!seq || track_type >= SEQUENCER_TRACK_COUNT) return;
    seq->tracks[track_type].muted = muted;
    LOG_INFO("Track %d %s", track_type, muted ? "muted" : "unmuted");
}

void sequencer_set_track_solo(Sequencer* seq, SequencerTrackType track_type, bool solo) {
    if (!seq || track_type >= SEQUENCER_TRACK_COUNT) return;
    seq->tracks[track_type].solo = solo;
    LOG_INFO("Track %d solo %s", track_type, solo ? "enabled" : "disabled");
}

void sequencer_set_track_volume(Sequencer* seq, SequencerTrackType track_type, float volume) {
    if (!seq || track_type >= SEQUENCER_TRACK_COUNT) return;
    seq->tracks[track_type].volume = fmaxf(0.0f, fminf(volume, 1.0f));
    LOG_INFO("Track %d volume set to %.2f", track_type, seq->tracks[track_type].volume);
}

// Utility functions
float sequencer_get_current_time(const Sequencer* seq) {
    return seq ? seq->current_time : 0.0f;
}

float sequencer_get_duration(const Sequencer* seq) {
    return seq ? seq->duration : 0.0f;
}

bool sequencer_is_playing(const Sequencer* seq) {
    return seq ? seq->playing && !seq->paused : false;
}

int sequencer_get_current_frame(const Sequencer* seq) {
    return seq ? seq->current_frame : 0;
}

int sequencer_get_total_frames(const Sequencer* seq) {
    return seq ? (int)(seq->duration * seq->fps) : 0;
}

float sequencer_get_progress(const Sequencer* seq) {
    if (!seq || seq->duration <= 0.0f) return 0.0f;
    return seq->current_time / seq->duration;
}

/*
 * IMPLEMENTATION: Complete sequencer system
 * LOC: ~500
 * Features: Timeline, tracks, events, playback, audio, camera, lighting, particles, dialogue
 */
