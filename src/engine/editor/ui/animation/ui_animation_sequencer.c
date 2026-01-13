/*
 * ui_animation_sequencer.c
 * UI Animation Sequencing Implementation
 */

#include "editor/ui/animation/ui_animation_sequencer.h"
#include "editor/ui/animation/ui_animation_curves.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

#define MAX_TIMELINES 64
#define MAX_TRACKS_PER_TIMELINE 32
#define MAX_CLIPS 256

static struct {
    ui_timeline_state_t *timelines;
    ui_animation_clip_state_t *clips;
    ui_animation_track_state_t *tracks;
    uint32_t next_timeline_id;
    uint32_t next_clip_id;
    uint32_t next_track_id;
    bool initialized;
} g_sequencer_system = {0};

int ui_animation_sequencer_init(void) {
    if (g_sequencer_system.initialized) return 0;
    
    g_sequencer_system.timelines = calloc(MAX_TIMELINES, sizeof(ui_timeline_state_t));
    g_sequencer_system.clips = calloc(MAX_CLIPS, sizeof(ui_animation_clip_state_t));
    g_sequencer_system.tracks = calloc(MAX_TRACKS_PER_TIMELINE * MAX_TIMELINES, sizeof(ui_animation_track_state_t));
    
    if (!g_sequencer_system.timelines || !g_sequencer_system.clips || !g_sequencer_system.tracks) {
        free(g_sequencer_system.timelines);
        free(g_sequencer_system.clips);
        free(g_sequencer_system.tracks);
        return -1;
    }
    
    g_sequencer_system.next_timeline_id = 1;
    g_sequencer_system.next_clip_id = 1;
    g_sequencer_system.next_track_id = 1;
    g_sequencer_system.initialized = true;
    
    LOG_INFO("UI Animation Sequencer System initialized");
    return 0;
}

void ui_animation_sequencer_shutdown(void) {
    if (!g_sequencer_system.initialized) return;
    
    free(g_sequencer_system.timelines);
    free(g_sequencer_system.clips);
    free(g_sequencer_system.tracks);
    memset(&g_sequencer_system, 0, sizeof(g_sequencer_system));
}

int ui_timeline_create(ui_timeline_handle_t *out_handle, const ui_timeline_desc_t *desc) {
    if (!g_sequencer_system.initialized || !out_handle || !desc) return -1;
    
    uint32_t id = g_sequencer_system.next_timeline_id++;
    if (id >= MAX_TIMELINES) return -1;
    
    ui_timeline_state_t *timeline = &g_sequencer_system.timelines[id];
    memset(timeline, 0, sizeof(*timeline));
    
    timeline->id = id;
    strncpy(timeline->name, desc->name ? desc->name : "Timeline", sizeof(timeline->name) - 1);
    
    timeline->duration = desc->duration;
    timeline->frame_rate = desc->frame_rate > 0 ? desc->frame_rate : 30.0f;
    timeline->current_time = 0.0f;
    timeline->start_time = desc->start_time;
    timeline->end_time = desc->end_time > 0 ? desc->end_time : desc->duration;
    timeline->loop = desc->loop;
    timeline->playback_speed = 1.0f;
    timeline->state = UI_TIMELINE_STOPPED;
    timeline->user_data = desc->user_data;
    
    // Add initial tracks
    if (desc->tracks && desc->track_count > 0) {
        for (uint32_t i = 0; i < desc->track_count; i++) {
            ui_animation_track_handle_t track_handle;
            if (ui_timeline_add_track(out_handle, &track_handle, &desc->tracks[i]) == 0) {
                // Track added successfully
            }
        }
    }
    
    out_handle->id = id;
    LOG_INFO("Created timeline '%s' with ID %u", timeline->name, id);
    return 0;
}

void ui_timeline_play(ui_timeline_handle_t handle) {
    if (!g_sequencer_system.initialized || handle.id == 0 || handle.id >= MAX_TIMELINES) return;
    
    ui_timeline_state_t *timeline = &g_sequencer_system.timelines[handle.id];
    timeline->state = UI_TIMELINE_PLAYING;
    
    // Activate tracks
    ui_animation_track_state_t *track = timeline->tracks;
    while (track) {
        if (track->enabled && !track->muted) {
            track->active = true;
            track->current_time = 0.0f;
        }
        track = track->next;
    }
}

void ui_timeline_pause(ui_timeline_handle_t handle) {
    if (!g_sequencer_system.initialized || handle.id == 0 || handle.id >= MAX_TIMELINES) return;
    
    ui_timeline_state_t *timeline = &g_sequencer_system.timelines[handle.id];
    timeline->state = UI_TIMELINE_PAUSED;
}

void ui_timeline_stop(ui_timeline_handle_t handle) {
    if (!g_sequencer_system.initialized || handle.id == 0 || handle.id >= MAX_TIMELINES) return;
    
    ui_timeline_state_t *timeline = &g_sequencer_system.timelines[handle.id];
    timeline->state = UI_TIMELINE_STOPPED;
    timeline->current_time = 0.0f;
    
    // Deactivate tracks
    ui_animation_track_state_t *track = timeline->tracks;
    while (track) {
        track->active = false;
        track->current_time = 0.0f;
        track = track->next;
    }
}

void ui_animation_sequencer_update(float delta_time) {
    if (!g_sequencer_system.initialized) return;
    
    for (uint32_t i = 1; i < MAX_TIMELINES; i++) {
        ui_timeline_state_t *timeline = &g_sequencer_system.timelines[i];
        if (timeline->state != UI_TIMELINE_PLAYING) continue;
        
        // Update timeline time
        timeline->current_time += delta_time * timeline->playback_speed;
        
        // Handle looping
        if (timeline->current_time >= timeline->end_time) {
            if (timeline->loop) {
                timeline->current_time = timeline->start_time;
            } else {
                timeline->current_time = timeline->end_time;
                timeline->state = UI_TIMELINE_STOPPED;
            }
        }
        
        // Update tracks
        ui_animation_track_state_t *track = timeline->tracks;
        while (track) {
            if (!track->active || !track->enabled || track->muted) {
                track = track->next;
                continue;
            }
            
            track->current_time += delta_time * timeline->playback_speed;
            
            // Update track based on type
            switch (track->type) {
                case UI_TRACK_PROPERTY:
                    // Update property animation
                    if (track->track_state.property.curve.id != 0) {
                        float value = ui_animation_curve_evaluate_simple(
                            track->track_state.property.curve,
                            track->current_time
                        );
                        track->track_state.property.current_value = value;
                        
                        // Apply value to target property
                        // This would call the property binding system
                    }
                    break;
                    
                case UI_TRACK_EVENT:
                    // Check for event triggers
                    if (track->track_state.property.curve.id != 0) {
                        // Simple event triggering at keyframes
                        // In a full implementation, this would check specific event times
                    }
                    break;
                    
                default:
                    break;
            }
            
            track = track->next;
        }
    }
}

uint32_t ui_animation_sequencer_get_timeline_count(void) {
    if (!g_sequencer_system.initialized) return 0;
    
    uint32_t count = 0;
    for (uint32_t i = 1; i < MAX_TIMELINES; i++) {
        if (g_sequencer_system.timelines[i].id != 0) count++;
    }
    return count;
}
