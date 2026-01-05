// src/audio/audio_event_queue.c
//
// Implementation of audio event queue system
//
#include <audio/audio_event_queue.h>
#include <core/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void audio_event_queue_init(AudioEventQueue* queue) {
    if (!queue) {
        return;
    }

    memset(queue, 0, sizeof(AudioEventQueue));
    queue->eventCount = 0;
    queue->currentTime = 0.0f;

    for (u32 i = 0; i < MAX_AUDIO_EVENTS; i++) {
        queue->events[i].processed = false;
    }

    queue->initialized = true;
    LOG_INFO("Audio event queue initialized");
}

void audio_event_queue_shutdown(AudioEventQueue* queue) {
    if (!queue || !queue->initialized) {
        return;
    }

    queue->initialized = false;
    queue->eventCount = 0;
    LOG_INFO("Audio event queue shut down");
}

u32 audio_event_queue_play_sound(AudioEventQueue* queue, SoundType sound, Vec3 pos,
                                f32 volume, SoundCategory category, f32 delay) {
    if (!queue || !queue->initialized || queue->eventCount >= MAX_AUDIO_EVENTS) {
        return 0xFFFFFFFF;
    }

    u32 eventId = queue->eventCount;
    AudioEvent* event = &queue->events[eventId];

    event->type = AUDIO_EVENT_PLAY_SOUND;
    event->soundType = sound;
    event->position = pos;
    event->volume = volume;
    event->category = category;
    event->delay = delay;
    event->scheduledTime = queue->currentTime + delay;
    event->processed = false;

    queue->eventCount++;
    return eventId;
}

u32 audio_event_queue_play_sound_2d(AudioEventQueue* queue, SoundType sound,
                                   f32 volume, SoundCategory category, f32 delay) {
    if (!queue || !queue->initialized || queue->eventCount >= MAX_AUDIO_EVENTS) {
        return 0xFFFFFFFF;
    }

    u32 eventId = queue->eventCount;
    AudioEvent* event = &queue->events[eventId];

    event->type = AUDIO_EVENT_PLAY_SOUND_2D;
    event->soundType = sound;
    event->volume = volume;
    event->category = category;
    event->delay = delay;
    event->scheduledTime = queue->currentTime + delay;
    event->processed = false;

    queue->eventCount++;
    return eventId;
}

u32 audio_event_queue_stop_sound(AudioEventQueue* queue, u32 soundId, f32 delay) {
    if (!queue || !queue->initialized || queue->eventCount >= MAX_AUDIO_EVENTS) {
        return 0xFFFFFFFF;
    }

    u32 eventId = queue->eventCount;
    AudioEvent* event = &queue->events[eventId];

    event->type = AUDIO_EVENT_STOP_SOUND;
    event->soundType = (SoundType)soundId;  // Reuse field for ID
    event->delay = delay;
    event->scheduledTime = queue->currentTime + delay;
    event->processed = false;

    queue->eventCount++;
    return eventId;
}

u32 audio_event_queue_set_volume(AudioEventQueue* queue, SoundCategory category,
                                f32 volume, f32 delay) {
    if (!queue || !queue->initialized || queue->eventCount >= MAX_AUDIO_EVENTS) {
        return 0xFFFFFFFF;
    }

    u32 eventId = queue->eventCount;
    AudioEvent* event = &queue->events[eventId];

    event->type = AUDIO_EVENT_SET_VOLUME;
    event->category = category;
    event->volume = volume;
    event->delay = delay;
    event->scheduledTime = queue->currentTime + delay;
    event->processed = false;

    queue->eventCount++;
    return eventId;
}

u32 audio_event_queue_stream_play(AudioEventQueue* queue, u32 streamId, f32 delay) {
    if (!queue || !queue->initialized || queue->eventCount >= MAX_AUDIO_EVENTS) {
        return 0xFFFFFFFF;
    }

    u32 eventId = queue->eventCount;
    AudioEvent* event = &queue->events[eventId];

    event->type = AUDIO_EVENT_STREAM_PLAY;
    event->streamId = streamId;
    event->delay = delay;
    event->scheduledTime = queue->currentTime + delay;
    event->processed = false;

    queue->eventCount++;
    return eventId;
}

u32 audio_event_queue_stream_stop(AudioEventQueue* queue, u32 streamId, f32 delay) {
    if (!queue || !queue->initialized || queue->eventCount >= MAX_AUDIO_EVENTS) {
        return 0xFFFFFFFF;
    }

    u32 eventId = queue->eventCount;
    AudioEvent* event = &queue->events[eventId];

    event->type = AUDIO_EVENT_STREAM_STOP;
    event->streamId = streamId;
    event->delay = delay;
    event->scheduledTime = queue->currentTime + delay;
    event->processed = false;

    queue->eventCount++;
    return eventId;
}

u32 audio_event_queue_stream_crossfade(AudioEventQueue* queue, u32 fromStream,
                                       u32 toStream, f32 duration, f32 delay) {
    if (!queue || !queue->initialized || queue->eventCount >= MAX_AUDIO_EVENTS) {
        return 0xFFFFFFFF;
    }

    u32 eventId = queue->eventCount;
    AudioEvent* event = &queue->events[eventId];

    event->type = AUDIO_EVENT_STREAM_CROSSFADE;
    event->streamId = fromStream;
    event->targetStreamId = toStream;
    event->volume = duration;  // Reuse volume field for duration
    event->delay = delay;
    event->scheduledTime = queue->currentTime + delay;
    event->processed = false;

    queue->eventCount++;
    return eventId;
}

void audio_event_queue_process(AudioEventQueue* queue, AudioSystem* audio, f32 deltaTime) {
    if (!queue || !queue->initialized || !audio) {
        return;
    }

    queue->currentTime += deltaTime;

    // Process all ready events
    for (u32 i = 0; i < queue->eventCount; i++) {
        AudioEvent* event = &queue->events[i];

        if (event->processed || event->scheduledTime > queue->currentTime) {
            continue;
        }

        // Execute event
        switch (event->type) {
            case AUDIO_EVENT_PLAY_SOUND:
                audio_play_sound(audio, event->soundType, event->position,
                               event->volume, event->category);
                break;

            case AUDIO_EVENT_PLAY_SOUND_2D:
                audio_play_sound_2d(audio, event->soundType, event->volume,
                                  event->category);
                break;

            case AUDIO_EVENT_STOP_SOUND:
                audio_stop_sound(audio, (u32)event->soundType);
                break;

            case AUDIO_EVENT_SET_VOLUME:
                audio_set_volume(audio, event->category, event->volume);
                break;

            default:
                break;
        }

        event->processed = true;
    }

    // Clean up processed events periodically
    if (queue->currentTime > 10.0f) {
        // Compact queue by removing processed events
        u32 writeIdx = 0;
        for (u32 i = 0; i < queue->eventCount; i++) {
            if (!queue->events[i].processed) {
                queue->events[writeIdx] = queue->events[i];
                writeIdx++;
            }
        }
        queue->eventCount = writeIdx;
        queue->currentTime = 0.0f;  // Reset time
    }
}

void audio_event_queue_clear(AudioEventQueue* queue) {
    if (!queue) {
        return;
    }

    queue->eventCount = 0;
    queue->currentTime = 0.0f;

    for (u32 i = 0; i < MAX_AUDIO_EVENTS; i++) {
        queue->events[i].processed = false;
    }
}

u32 audio_event_queue_pending_count(AudioEventQueue* queue) {
    if (!queue) {
        return 0;
    }

    u32 pending = 0;
    for (u32 i = 0; i < queue->eventCount; i++) {
        if (!queue->events[i].processed) {
            pending++;
        }
    }
    return pending;
}
