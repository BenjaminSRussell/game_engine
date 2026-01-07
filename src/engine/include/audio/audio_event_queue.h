// include/audio/audio_event_queue.h
//
// Purpose: Audio event scheduling system for batching and optimizing sound playback
//
#ifndef AUDIO_EVENT_QUEUE_H
#define AUDIO_EVENT_QUEUE_H

#include <common.h>
#include <math/vec3.h>
#include "include/audio/audio_system.h"

#define MAX_AUDIO_EVENTS 256
#define AUDIO_EVENT_QUEUE_SIZE 256

typedef enum {
    AUDIO_EVENT_PLAY_SOUND,
    AUDIO_EVENT_PLAY_SOUND_2D,
    AUDIO_EVENT_STOP_SOUND,
    AUDIO_EVENT_SET_VOLUME,
    AUDIO_EVENT_STREAM_PLAY,
    AUDIO_EVENT_STREAM_STOP,
    AUDIO_EVENT_STREAM_CROSSFADE
} AudioEventType;

typedef struct {
    AudioEventType type;
    f32 scheduledTime;       // When to execute (relative to queue time)
    f32 delay;               // Delay before execution

    // Generic parameters
    SoundType soundType;
    Vec3 position;
    f32 volume;
    SoundCategory category;
    u32 streamId;
    u32 targetStreamId;

    bool processed;
} AudioEvent;

typedef struct {
    AudioEvent events[MAX_AUDIO_EVENTS];
    u32 eventCount;
    f32 currentTime;

    bool initialized;
} AudioEventQueue;

// Lifecycle
void audio_event_queue_init(AudioEventQueue* queue);
void audio_event_queue_shutdown(AudioEventQueue* queue);

// Event queueing
u32 audio_event_queue_play_sound(AudioEventQueue* queue, SoundType sound, Vec3 pos,
                                f32 volume, SoundCategory category, f32 delay);
u32 audio_event_queue_play_sound_2d(AudioEventQueue* queue, SoundType sound,
                                   f32 volume, SoundCategory category, f32 delay);
u32 audio_event_queue_stop_sound(AudioEventQueue* queue, u32 soundId, f32 delay);
u32 audio_event_queue_set_volume(AudioEventQueue* queue, SoundCategory category,
                                f32 volume, f32 delay);

// Stream events
u32 audio_event_queue_stream_play(AudioEventQueue* queue, u32 streamId, f32 delay);
u32 audio_event_queue_stream_stop(AudioEventQueue* queue, u32 streamId, f32 delay);
u32 audio_event_queue_stream_crossfade(AudioEventQueue* queue, u32 fromStream,
                                       u32 toStream, f32 duration, f32 delay);

// Processing
void audio_event_queue_process(AudioEventQueue* queue, AudioSystem* audio, f32 deltaTime);
void audio_event_queue_clear(AudioEventQueue* queue);

// Query
u32 audio_event_queue_pending_count(AudioEventQueue* queue);

#endif // AUDIO_EVENT_QUEUE_H
