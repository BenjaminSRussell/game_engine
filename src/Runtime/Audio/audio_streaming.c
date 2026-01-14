// src/audio/audio_streaming.c
//
// Implementation of the audio streaming system for long-form audio playback.
//
#include <audio/audio_streaming.h>
#include "engine/include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <include/math/math_all.h>

void audio_streaming_init(AudioStreamingSystem* system, ma_engine* engine) {
    if (!system || !engine) {
        return;
    }

    memset(system, 0, sizeof(AudioStreamingSystem));
    system->engine = engine;
    system->active_stream_count = 0;
    system->initialized = true;

    // Initialize all streams as inactive
    for (u32 i = 0; i < MAX_AUDIO_STREAMS; i++) {
        system->streams[i].active = false;
        system->streams[i].state = STREAM_STATE_STOPPED;
    }

            MAX_AUDIO_STREAMS);
}

void audio_streaming_shutdown(AudioStreamingSystem* system) {
    if (!system || !system->initialized) {
        return;
    }

    // Stop and uninitialize all active streams
    for (u32 i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (system->streams[i].active) {
            ma_decoder_uninit(&system->streams[i].decoder);
            system->streams[i].active = false;
        }
    }

    system->initialized = false;
    system->active_stream_count = 0;
}

u32 audio_stream_create(AudioStreamingSystem* system, const char* filepath,
                        SoundCategory category, bool looping) {
    if (!system || !system->initialized || !filepath) {
        return 0xFFFFFFFF;
    }

    // Find free stream slot
    u32 stream_id = 0xFFFFFFFF;
    for (u32 i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (!system->streams[i].active) {
            stream_id = i;
            break;
        }
    }

    if (stream_id == 0xFFFFFFFF) {
        return 0xFFFFFFFF;
    }

    AudioStream* stream = &system->streams[stream_id];
    memset(stream, 0, sizeof(AudioStream));

    // Initialize decoder
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 48000);
    ma_result result = ma_decoder_init_file(filepath, &config, &stream->decoder);

    if (result != MA_SUCCESS) {
                filepath, result);
        return 0xFFFFFFFF;
    }

    // Get stream duration
    ma_uint64 frame_count;
    result = ma_decoder_get_length_in_pcm_frames(&stream->decoder, &frame_count);
    if (result == MA_SUCCESS) {
        stream->total_frames = frame_count;
        stream->duration_seconds = (f32)frame_count / 48000.0f;
    }

    // Set stream properties
    strncpy(stream->filepath, filepath, sizeof(stream->filepath) - 1);
    stream->active = true;
    stream->state = STREAM_STATE_STOPPED;
    stream->volume = 1.0f;
    stream->target_volume = 1.0f;
    stream->looping = looping;
    stream->category = category;
    stream->is_3d = false;
    stream->frames_played = 0;
    stream->buffer_read_pos = 0;
    stream->buffer_write_pos = 0;
    stream->crossfade_time = 0.0f;
    stream->crossfade_duration = 0.0f;

    system->active_stream_count++;
            stream_id, filepath, stream->duration_seconds);

    return stream_id;
}

void audio_stream_destroy(AudioStreamingSystem* system, u32 stream_id) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return;
    }

    // Stop playback and uninit decoder
    ma_decoder_uninit(&stream->decoder);
    stream->active = false;
    stream->state = STREAM_STATE_STOPPED;

    system->active_stream_count--;
}

void audio_stream_play(AudioStreamingSystem* system, u32 stream_id) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return;
    }

    stream->state = STREAM_STATE_PLAYING;
}

void audio_stream_pause(AudioStreamingSystem* system, u32 stream_id) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return;
    }

    stream->state = STREAM_STATE_PAUSED;
}

void audio_stream_stop(AudioStreamingSystem* system, u32 stream_id) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return;
    }

    stream->state = STREAM_STATE_STOPPED;
    stream->frames_played = 0;

    // Seek back to beginning
    ma_decoder_seek_to_pcm_frame(&stream->decoder, 0);

}

void audio_stream_set_volume(AudioStreamingSystem* system, u32 stream_id, f32 volume) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return;
    }

    // Clamp volume
    stream->volume = (volume < 0.0f) ? 0.0f : ((volume > 1.0f) ? 1.0f : volume);
}

void audio_stream_set_position(AudioStreamingSystem* system, u32 stream_id,
                               Vec3 position) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return;
    }

    stream->is_3d = true;
    stream->position = position;
}

void audio_stream_crossfade(AudioStreamingSystem* system, u32 from_stream,
                           u32 to_stream, f32 duration) {
    if (!system || from_stream >= MAX_AUDIO_STREAMS ||
        to_stream >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* from = &system->streams[from_stream];
    AudioStream* to = &system->streams[to_stream];

    if (!from->active || !to->active) {
        return;
    }

    // Set up crossfade
    from->state = STREAM_STATE_CROSSFADING_OUT;
    from->crossfade_duration = duration;
    from->crossfade_time = 0.0f;
    from->target_volume = 0.0f;

    to->state = STREAM_STATE_CROSSFADING_IN;
    to->crossfade_duration = duration;
    to->crossfade_time = 0.0f;
    to->target_volume = 1.0f;

            from_stream, to_stream, duration);
}

void audio_stream_seek(AudioStreamingSystem* system, u32 stream_id,
                      f32 time_seconds) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return;
    }

    // Convert time to PCM frames (assuming 48kHz)
    ma_uint64 target_frame = (ma_uint64)(time_seconds * 48000.0f);

    if (target_frame < stream->total_frames) {
        ma_decoder_seek_to_pcm_frame(&stream->decoder, target_frame);
        stream->frames_played = target_frame;
    }
}

f32 audio_stream_get_position(AudioStreamingSystem* system, u32 stream_id) {
    if (!system || stream_id >= MAX_AUDIO_STREAMS) {
        return 0.0f;
    }

    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        return 0.0f;
    }

    return (f32)stream->frames_played / 48000.0f;
}

void audio_streaming_update(AudioStreamingSystem* system, f32 delta_time) {
    if (!system || !system->initialized) {
        return;
    }

    for (u32 i = 0; i < MAX_AUDIO_STREAMS; i++) {
        AudioStream* stream = &system->streams[i];
        if (!stream->active) {
            continue;
        }

        // Update crossfade
        if (stream->state == STREAM_STATE_CROSSFADING_OUT ||
            stream->state == STREAM_STATE_CROSSFADING_IN) {
            stream->crossfade_time += delta_time;

            if (stream->crossfade_time >= stream->crossfade_duration) {
                // Crossfade complete
                stream->volume = stream->target_volume;
                if (stream->state == STREAM_STATE_CROSSFADING_OUT) {
                    stream->state = STREAM_STATE_STOPPED;
                } else {
                    stream->state = STREAM_STATE_PLAYING;
                }
            } else {
                // Interpolate volume
                f32 t = stream->crossfade_time / stream->crossfade_duration;
                f32 start_vol = (stream->state == STREAM_STATE_CROSSFADING_OUT) ? 1.0f : 0.0f;
                stream->volume = start_vol + (stream->target_volume - start_vol) * t;
            }
        }

        // Update playback
        if (stream->state == STREAM_STATE_PLAYING ||
            stream->state == STREAM_STATE_CROSSFADING_OUT ||
            stream->state == STREAM_STATE_CROSSFADING_IN) {

            // Check if stream finished
            if (stream->frames_played >= stream->total_frames) {
                if (stream->looping) {
                    // Loop back to start
                    ma_decoder_seek_to_pcm_frame(&stream->decoder, 0);
                    stream->frames_played = 0;
                } else {
                    // Stop stream
                    stream->state = STREAM_STATE_STOPPED;
                }
            }
        }
    }
}

// Enhanced buffer management implementation
static void stream_buffer_init(StreamBuffer* buffer, u32 capacity) {
    if (!buffer) return;
    
    buffer->data = (u8*)malloc(capacity);
    buffer->capacity = capacity;
    buffer->size = 0;
    buffer->read_pos = 0;
    buffer->write_pos = 0;
    buffer->bytes_available = 0;
    buffer->full = false;
    buffer->empty = true;
}

static void stream_buffer_cleanup(StreamBuffer* buffer) {
    if (!buffer) return;
    
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    buffer->capacity = 0;
    buffer->size = 0;
}

static u32 stream_buffer_write(StreamBuffer* buffer, const u8* data, u32 size) {
    if (!buffer || !data || size == 0) return 0;
    
    u32 bytes_written = 0;
    
    while (bytes_written < size && !buffer->full) {
        u32 write_space = buffer->capacity - buffer->write_pos;
        u32 chunk_size = (size - bytes_written < write_space) ? (size - bytes_written) : write_space;
        
        memcpy(buffer->data + buffer->write_pos, data + bytes_written, chunk_size);
        buffer->write_pos = (buffer->write_pos + chunk_size) % buffer->capacity;
        buffer->bytes_available += chunk_size;
        bytes_written += chunk_size;
        
        buffer->full = (buffer->bytes_available == buffer->capacity);
        buffer->empty = false;
    }
    
    return bytes_written;
}

static u32 stream_buffer_read(StreamBuffer* buffer, u8* data, u32 size) {
    if (!buffer || !data || size == 0) return 0;
    
    u32 bytes_read = 0;
    
    while (bytes_read < size && !buffer->empty) {
        u32 read_space = buffer->capacity - buffer->read_pos;
        u32 chunk_size = (size - bytes_read < read_space) ? (size - bytes_read) : read_space;
        
        if (buffer->bytes_available < chunk_size) {
            chunk_size = buffer->bytes_available;
        }
        
        memcpy(data + bytes_read, buffer->data + buffer->read_pos, chunk_size);
        buffer->read_pos = (buffer->read_pos + chunk_size) % buffer->capacity;
        buffer->bytes_available -= chunk_size;
        bytes_read += chunk_size;
        
        buffer->empty = (buffer->bytes_available == 0);
        buffer->full = false;
    }
    
    return bytes_read;
}

static void stream_prefetch_init(StreamPrefetch* prefetch, u32 buffer_size) {
    if (!prefetch) return;
    
    for (u32 i = 0; i < STREAM_PREFETCH_BUFFERS; i++) {
        stream_buffer_init(&prefetch->buffers[i], buffer_size);
    }
    
    prefetch->current_buffer_index = 0;
    prefetch->write_buffer_index = 0;
    prefetch->prefetch_active = false;
    prefetch->prefetch_size = buffer_size;
    prefetch->prefetch_threshold = STREAM_PREFETCH_THRESHOLD;
}

static void stream_prefetch_cleanup(StreamPrefetch* prefetch) {
    if (!prefetch) return;
    
    for (u32 i = 0; i < STREAM_PREFETCH_BUFFERS; i++) {
        stream_buffer_cleanup(&prefetch->buffers[i]);
    }
    
    prefetch->prefetch_active = false;
}

void audio_stream_enable_prefetch(AudioStreamingSystem* system, u32 stream_id, bool enable) {
    if (!system || !system->initialized || stream_id >= MAX_AUDIO_STREAMS) return;
    
    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) return;
    
    if (enable && !stream->use_prefetch) {
        // Initialize prefetch system
        stream_prefetch_init(&stream->prefetch, stream->prefetch_buffer_size);
        stream->use_prefetch = true;
        stream->prefetch.prefetch_active = true;
        
        LOG_INFO("Audio stream %d: Prefetch enabled", stream_id);
    } else if (!enable && stream->use_prefetch) {
        // Cleanup prefetch system
        stream_prefetch_cleanup(&stream->prefetch);
        stream->use_prefetch = false;
        stream->prefetch.prefetch_active = false;
        
        LOG_INFO("Audio stream %d: Prefetch disabled", stream_id);
    }
}

void audio_stream_set_prefetch_sensitivity(AudioStreamingSystem* system, u32 stream_id, f32 sensitivity) {
    if (!system || !system->initialized || stream_id >= MAX_AUDIO_STREAMS) return;
    
    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active || !stream->use_prefetch) return;
    
    // Clamp sensitivity between 0.1 and 0.9
    sensitivity = (sensitivity < 0.1f) ? 0.1f : (sensitivity > 0.9f) ? 0.9f : sensitivity;
    stream->prefetch_sensitivity = sensitivity;
    stream->prefetch.prefetch_threshold = sensitivity;
    
    LOG_DEBUG("Audio stream %d: Prefetch sensitivity set to %.2f", stream_id, sensitivity);
}

void audio_stream_get_buffer_stats(AudioStreamingSystem* system, u32 stream_id, StreamBufferStats* stats) {
    if (!system || !system->initialized || stream_id >= MAX_AUDIO_STREAMS || !stats) return;
    
    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) {
        memset(stats, 0, sizeof(StreamBufferStats));
        return;
    }
    
    *stats = stream->buffer_stats;
    
    // Calculate current buffer usage
    if (stream->use_prefetch) {
        StreamBuffer* current = &stream->prefetch.buffers[stream->prefetch.current_buffer_index];
        f32 usage = (f32)current->bytes_available / (f32)current->capacity;
        
        // Update running average
        stream->buffer_stats.average_buffer_usage = 
            (stream->buffer_stats.average_buffer_usage * 0.9f) + (usage * 0.1f);
        
        if (usage > stream->buffer_stats.peak_buffer_usage) {
            stream->buffer_stats.peak_buffer_usage = usage;
        }
    }
}

bool audio_stream_is_buffer_healthy(AudioStreamingSystem* system, u32 stream_id) {
    if (!system || !system->initialized || stream_id >= MAX_AUDIO_STREAMS) return false;
    
    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) return false;
    
    // Check for buffer underruns/overruns
    if (stream->buffer_stats.buffer_underruns > 0 || stream->buffer_stats.buffer_overruns > 0) {
        return false;
    }
    
    // Check buffer usage
    f32 usage = stream->buffer_stats.average_buffer_usage;
    if (usage < 0.1f || usage > 0.95f) {
        return false;
    }
    
    // Check prefetch health if enabled
    if (stream->use_prefetch) {
        StreamBuffer* current = &stream->prefetch.buffers[stream->prefetch.current_buffer_index];
        if (current->empty || current->bytes_available < current->capacity * 0.2f) {
            return false;
        }
    }
    
    return true;
}

void audio_stream_optimize_buffer_usage(AudioStreamingSystem* system, u32 stream_id) {
    if (!system || !system->initialized || stream_id >= MAX_AUDIO_STREAMS) return;
    
    AudioStream* stream = &system->streams[stream_id];
    if (!stream->active) return;
    
    // Adjust prefetch sensitivity based on buffer health
    if (stream->use_prefetch) {
        f32 usage = stream->buffer_stats.average_buffer_usage;
        
        if (usage < 0.3f) {
            // Buffer is running low, increase prefetch sensitivity
            stream->prefetch_sensitivity = (stream->prefetch_sensitivity * 0.8f) + 0.2f;
            stream->prefetch.prefetch_threshold = stream->prefetch_sensitivity;
            LOG_DEBUG("Audio stream %d: Increased prefetch sensitivity to %.2f (low buffer)", 
                     stream_id, stream->prefetch_sensitivity);
        } else if (usage > 0.8f) {
            // Buffer is healthy, decrease prefetch sensitivity
            stream->prefetch_sensitivity = (stream->prefetch_sensitivity * 0.9f) + 0.05f;
            stream->prefetch.prefetch_threshold = stream->prefetch_sensitivity;
            LOG_DEBUG("Audio stream %d: Decreased prefetch sensitivity to %.2f (healthy buffer)", 
                     stream_id, stream->prefetch_sensitivity);
        }
    }
    
    // Reset statistics counters periodically
    if (stream->buffer_stats.buffer_underruns > 100 || stream->buffer_stats.buffer_overruns > 100) {
        LOG_WARN("Audio stream %d: Resetting buffer statistics (too many errors)", stream_id);
        stream->buffer_stats.buffer_underruns = 0;
        stream->buffer_stats.buffer_overruns = 0;
    }
}
