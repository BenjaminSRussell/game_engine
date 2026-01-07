#include "gameplay/replay_system.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <include/math/math.h>
#include <stdio.h>

/**
 * =================================================================================================
 *                                   REPLAY & HIGHLIGHTS SYSTEM - COMPLETE
 * =================================================================================================
 */

// Helper function to get current timestamp in milliseconds
static uint32_t get_current_time_ms(void) {
    return (uint32_t)(time(NULL) * 1000);
}

// Helper function to get high-resolution time
static uint64_t get_high_res_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// Helper function to calculate checksum
static uint32_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 1) | (checksum >> 31);
        checksum += bytes[i];
    }
    
    return checksum;
}

// =================================================================================================
// CORE FUNCTIONS
// =================================================================================================

bool replay_system_init(ReplaySystem* replay) {
    if (!replay) {
        LOG_ERROR("Invalid replay system pointer");
        return false;
    }
    
    memset(replay, 0, sizeof(ReplaySystem));
    
    // Initialize metadata
    strcpy(replay->metadata.version, "1.0");
    replay->metadata.creation_time = get_current_time_ms();
    replay->metadata.is_public = false;
    replay->metadata.is_compressed = false;
    replay->metadata.is_encrypted = false;
    
    // Initialize recording context
    replay->recording.is_recording = false;
    replay->recording.recording_fps = 30.0f;
    replay->recording.auto_markers_enabled = true;
    replay->recording.snapshot_interval_ms = 5000;  // 5 seconds
    replay->recording.rolling_buffer_size = MAX_REPLAY_FRAMES;
    replay->recording.use_compression = false;
    replay->recording.quality_level = 3;
    
    // Initialize playback context
    replay->playback.state = PLAYBACK_STATE_STOPPED;
    replay->playback.current_frame = 0;
    replay->playback.target_frame = 0;
    replay->playback.playback_speed = 1.0f;
    replay->playback.is_looping = false;
    replay->playback.auto_camera_enabled = true;
    replay->playback.show_markers = true;
    replay->playback.show_hud = true;
    replay->playback.is_reversed = false;
    
    LOG_INFO("Replay system initialized successfully");
    return true;
}

void replay_system_shutdown(ReplaySystem* replay) {
    if (!replay) return;
    
    // Stop recording if active
    if (replay->recording.is_recording) {
        replay_stop_recording(replay);
    }
    
    // Stop playback if active
    if (replay->playback.state != PLAYBACK_STATE_STOPPED) {
        replay_stop_playback(replay);
    }
    
    // Clear all data
    memset(replay, 0, sizeof(ReplaySystem));
    
    LOG_INFO("Replay system shutdown");
}

ReplaySystem* replay_system_create(void) {
    ReplaySystem* replay = malloc(sizeof(ReplaySystem));
    if (!replay) {
        LOG_ERROR("Failed to allocate replay system");
        return NULL;
    }
    
    if (!replay_system_init(replay)) {
        free(replay);
        return NULL;
    }
    
    return replay;
}

void replay_system_destroy(ReplaySystem* replay) {
    if (!replay) return;
    
    replay_system_shutdown(replay);
    free(replay);
}

// =================================================================================================
// RECORDING FUNCTIONS
// =================================================================================================

bool replay_start_recording(ReplaySystem* replay, const char* name, float fps) {
    if (!replay || !name) {
        LOG_ERROR("Invalid parameters for start recording");
        return false;
    }
    
    if (replay->recording.is_recording) {
        LOG_ERROR("Recording is already active");
        return false;
    }
    
    // Clear previous data
    memset(&replay->frames, 0, sizeof(replay->frames));
    replay->frame_count = 0;
    
    memset(&replay->snapshots, 0, sizeof(replay->snapshots));
    replay->snapshot_count = 0;
    
    memset(&replay->markers, 0, sizeof(replay->markers));
    replay->marker_count = 0;
    
    // Set up recording
    strncpy(replay->metadata.name, name, sizeof(replay->metadata.name) - 1);
    replay->recording.is_recording = true;
    replay->recording.start_time_ms = get_current_time_ms();
    replay->recording.current_frame = 0;
    replay->recording.recording_fps = fps;
    replay->recording.last_snapshot_time = replay->recording.start_time_ms;
    
    // Create initial snapshot
    replay_create_snapshot(replay);
    
    LOG_INFO("Started replay recording: %s at %.1f FPS", name, fps);
    return true;
}

bool replay_stop_recording(ReplaySystem* replay) {
    if (!replay || !replay->recording.is_recording) {
        LOG_ERROR("No active recording to stop");
        return false;
    }
    
    // Calculate final statistics
    replay->metadata.duration_ms = get_current_time_ms() - replay->recording.start_time_ms;
    replay->statistics.total_frames = replay->frame_count;
    replay->statistics.total_duration_ms = replay->metadata.duration_ms;
    replay->statistics.entity_count = replay->entity_registry_count;
    replay->statistics.average_fps = replay->frame_count > 0 ? 
                                     (float)replay->frame_count / (replay->metadata.duration_ms / 1000.0f) : 0.0f;
    
    // Calculate file size estimate
    replay->metadata.file_size = replay->frame_count * sizeof(ReplayFrame);
    replay->statistics.file_size_bytes = replay->metadata.file_size;
    
    // Calculate checksum
    replay->metadata.checksum = calculate_checksum(replay->frames, replay->frame_count * sizeof(ReplayFrame));
    
    replay->recording.is_recording = false;
    
    LOG_INFO("Stopped replay recording: %u frames, %.2f seconds", 
             replay->frame_count, replay->metadata.duration_ms / 1000.0f);
    return true;
}

bool replay_add_frame(ReplaySystem* replay, const ReplayFrame* frame) {
    if (!replay || !frame || !replay->recording.is_recording) {
        LOG_ERROR("Invalid parameters or not recording");
        return false;
    }
    
    if (replay->frame_count >= MAX_REPLAY_FRAMES) {
        LOG_ERROR("Maximum replay frames reached");
        return false;
    }
    
    // Validate frame
    if (!replay_validate_frame(frame)) {
        LOG_ERROR("Invalid frame data");
        return false;
    }
    
    // Copy frame
    replay->frames[replay->frame_count] = *frame;
    replay->frames[replay->frame_count].frame_number = replay->recording.current_frame;
    replay->frames[replay->frame_count].timestamp_ms = get_current_time_ms() - replay->recording.start_time_ms;
    
    replay->frame_count++;
    replay->recording.current_frame++;
    
    // Create snapshot if needed
    uint32_t current_time = get_current_time_ms() - replay->recording.start_time_ms;
    if (current_time - replay->recording.last_snapshot_time >= replay->recording.snapshot_interval_ms) {
        replay_create_snapshot(replay);
        replay->recording.last_snapshot_time = current_time;
    }
    
    return true;
}

// =================================================================================================
// PLAYBACK FUNCTIONS
// =================================================================================================

bool replay_start_playback(ReplaySystem* replay) {
    if (!replay) {
        LOG_ERROR("Invalid replay system");
        return false;
    }
    
    if (replay->frame_count == 0) {
        LOG_ERROR("No frames to play");
        return false;
    }
    
    if (replay->playback.state != PLAYBACK_STATE_STOPPED) {
        LOG_ERROR("Playback is already active");
        return false;
    }
    
    replay->playback.state = PLAYBACK_STATE_PLAYING;
    replay->playback.current_frame = 0;
    replay->playback.target_frame = replay->frame_count - 1;
    
    LOG_INFO("Started replay playback: %u frames", replay->frame_count);
    return true;
}

bool replay_stop_playback(ReplaySystem* replay) {
    if (!replay) {
        LOG_ERROR("Invalid replay system");
        return false;
    }
    
    replay->playback.state = PLAYBACK_STATE_STOPPED;
    replay->playback.current_frame = 0;
    
    LOG_INFO("Stopped replay playback");
    return true;
}

// =================================================================================================
// UTILITY FUNCTIONS
// =================================================================================================

uint32_t replay_get_frame_count(const ReplaySystem* replay) {
    return replay ? replay->frame_count : 0;
}

uint32_t replay_get_duration_ms(const ReplaySystem* replay) {
    return replay ? replay->metadata.duration_ms : 0;
}

float replay_get_fps(const ReplaySystem* replay) {
    return replay ? replay->statistics.average_fps : 0.0f;
}

bool replay_is_recording(const ReplaySystem* replay) {
    return replay ? replay->recording.is_recording : false;
}

bool replay_is_playing(const ReplaySystem* replay) {
    return replay ? replay->playback.state == PLAYBACK_STATE_PLAYING : false;
}

// =================================================================================================
// VALIDATION FUNCTIONS
// =================================================================================================

bool replay_validate(const ReplaySystem* replay) {
    if (!replay) return false;
    
    if (replay->metadata.name[0] == '\0') return false;
    if (replay->frame_count == 0) return false;
    
    return true;
}

bool replay_validate_frame(const ReplayFrame* frame) {
    if (!frame) return false;
    
    if (frame->type >= FRAME_TYPE_VISUAL) return false;
    
    return true;
}
