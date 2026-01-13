// include/audio/audio_streaming.h
//
// Purpose: Audio streaming system for efficiently playing long-form audio content
// (music, ambient loops) without loading entire files into memory.
//
// Key Features:
// - Background streaming from disk for music tracks
// - Circular buffer management for continuous playback
// - Crossfade support for smooth track transitions
// - Multiple concurrent streams (music + ambient loops)
// - Seek support for track position control
//
// Public APIs:
// - audio_stream_create: Create a new audio stream from file
// - audio_stream_play/pause/stop: Playback control
// - audio_stream_set_volume: Volume control
// - audio_stream_update: Per-frame update for buffer refills
// - audio_stream_crossfade: Smooth transition between streams
//
// Invariants:
// - Streams must be created before playback
// - Buffer sizes should be power of 2 for efficiency
// - Crossfade duration must be less than stream duration
//
#ifndef AUDIO_STREAMING_H
#define AUDIO_STREAMING_H

#include "engine/include/common.h"
#include "include/vendor/miniaudio.h"
#include "include/audio/audio_system.h"

#define MAX_AUDIO_STREAMS 8
#define STREAM_BUFFER_SIZE 8192
#define STREAM_PREFETCH_BUFFERS 3    // Triple buffering for prefetching
#define STREAM_PREFETCH_THRESHOLD 0.7f // Prefetch when 70% of current buffer is consumed

// Enhanced streaming buffer management
typedef struct {
    u8* data;
    u32 size;
    u32 capacity;
    u32 read_pos;
    u32 write_pos;
    u32 bytes_available;
    bool full;
    bool empty;
} StreamBuffer;

// Prefetch management
typedef struct {
    StreamBuffer buffers[STREAM_PREFETCH_BUFFERS];
    u32 current_buffer_index;
    u32 write_buffer_index;
    bool prefetch_active;
    u32 prefetch_size;
    f32 prefetch_threshold;
} StreamPrefetch;

// Buffer management statistics
typedef struct {
    u64 buffer_underruns;
    u64 buffer_overruns;
    u64 prefetch_hits;
    u64 prefetch_misses;
    f32 average_buffer_usage;
    f32 peak_buffer_usage;
    u64 total_bytes_streamed;
} StreamBufferStats;

typedef enum {
    STREAM_STATE_STOPPED,
    STREAM_STATE_PLAYING,
    STREAM_STATE_PAUSED,
    STREAM_STATE_CROSSFADING_OUT,
    STREAM_STATE_CROSSFADING_IN
} StreamState;

typedef struct {
    ma_decoder decoder;
    ma_data_source_base base;

    bool active;
    StreamState state;
    char filepath[256];

    // Playback state
    f32 volume;
    f32 target_volume;
    f32 crossfade_time;
    f32 crossfade_duration;
    bool looping;

    // Category and position
    SoundCategory category;
    bool is_3d;
    Vec3 position;

    // Enhanced streaming buffer management
    StreamPrefetch prefetch;
    
    // Legacy single buffer (for compatibility)
    u8 buffer[STREAM_BUFFER_SIZE];
    u32 buffer_read_pos;
    u32 buffer_write_pos;

    // Statistics
    u64 frames_played;
    u64 total_frames;
    f32 duration_seconds;
    StreamBufferStats buffer_stats;
    
    // Prefetch configuration
    bool use_prefetch;
    u32 prefetch_buffer_size;
    f32 prefetch_sensitivity;
} AudioStream;

typedef struct {
    AudioStream streams[MAX_AUDIO_STREAMS];
    u32 active_stream_count;
    ma_engine* engine;
    bool initialized;
} AudioStreamingSystem;

// Lifecycle
void audio_streaming_init(AudioStreamingSystem* system, ma_engine* engine);
void audio_streaming_shutdown(AudioStreamingSystem* system);

// Stream management
u32 audio_stream_create(AudioStreamingSystem* system, const char* filepath,
                        SoundCategory category, bool looping);
void audio_stream_destroy(AudioStreamingSystem* system, u32 stream_id);

// Playback control
void audio_stream_play(AudioStreamingSystem* system, u32 stream_id);
void audio_stream_pause(AudioStreamingSystem* system, u32 stream_id);
void audio_stream_stop(AudioStreamingSystem* system, u32 stream_id);
void audio_stream_set_volume(AudioStreamingSystem* system, u32 stream_id, f32 volume);
void audio_stream_set_position(AudioStreamingSystem* system, u32 stream_id, Vec3 position);

// Advanced features
void audio_stream_crossfade(AudioStreamingSystem* system, u32 from_stream,
                           u32 to_stream, f32 duration);
void audio_stream_seek(AudioStreamingSystem* system, u32 stream_id, f32 time_seconds);
f32 audio_stream_get_position(AudioStreamingSystem* system, u32 stream_id);

// Enhanced buffer management
void audio_stream_enable_prefetch(AudioStreamingSystem* system, u32 stream_id, bool enable);
void audio_stream_set_prefetch_sensitivity(AudioStreamingSystem* system, u32 stream_id, f32 sensitivity);
void audio_stream_get_buffer_stats(AudioStreamingSystem* system, u32 stream_id, StreamBufferStats* stats);
bool audio_stream_is_buffer_healthy(AudioStreamingSystem* system, u32 stream_id);
void audio_stream_optimize_buffer_usage(AudioStreamingSystem* system, u32 stream_id);

// Update (call each frame)
void audio_streaming_update(AudioStreamingSystem* system, f32 delta_time);

#endif // AUDIO_STREAMING_H