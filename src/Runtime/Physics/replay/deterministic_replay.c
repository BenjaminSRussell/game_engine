#include "deterministic_replay.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define REPLAY_MAGIC_NUMBER 0x52454C41 // "RELAR"
#define REPLAY_VERSION 1

struct replay_context_t {
    bool initialized;
    bool is_recording;
    bool is_playing;
    bool deterministic_mode;
    float fixed_timestep;
    
    // Recording state
    replay_frame_t* frames;
    uint32_t frame_count;
    uint32_t frame_capacity;
    uint32_t current_frame;
    uint32_t body_count;
    
    // Playback state
    replay_frame_t* loaded_frames;
    uint32_t loaded_frame_count;
    uint32_t playback_frame;
    
    // Checksum state
    uint32_t running_checksum;
};

static uint32_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    
    return checksum;
}

static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

replay_error_t replay_init(replay_context_t** context) {
    if (!context) return REPLAY_ERROR_INVALID_PARAM;
    
    replay_context_t* ctx = (replay_context_t*)calloc(1, sizeof(replay_context_t));
    if (!ctx) return REPLAY_ERROR_OUT_OF_MEMORY;
    
    ctx->frame_capacity = 10000;
    ctx->frames = (replay_frame_t*)calloc(ctx->frame_capacity, sizeof(replay_frame_t));
    if (!ctx->frames) {
        free(ctx);
        return REPLAY_ERROR_OUT_OF_MEMORY;
    }
    
    ctx->deterministic_mode = true;
    ctx->fixed_timestep = 1.0f / 60.0f; // 60 FPS
    ctx->initialized = true;
    
    *context = ctx;
    return REPLAY_SUCCESS;
}

void replay_shutdown(replay_context_t* context) {
    if (!context) return;
    
    for (uint32_t i = 0; i < context->frame_count; i++) {
        free(context->frames[i].body_states);
        free(context->frames[i].input_data);
    }
    
    for (uint32_t i = 0; i < context->loaded_frame_count; i++) {
        free(context->loaded_frames[i].body_states);
        free(context->loaded_frames[i].input_data);
    }
    
    free(context->frames);
    free(context->loaded_frames);
    free(context);
}

replay_error_t replay_start_recording(replay_context_t* context, uint32_t body_count) {
    if (!context || !context->initialized) return REPLAY_ERROR_NOT_INITIALIZED;
    if (context->is_recording || context->is_playing) return REPLAY_ERROR_RECORDING;
    
    context->is_recording = true;
    context->frame_count = 0;
    context->current_frame = 0;
    context->body_count = body_count;
    context->running_checksum = 0;
    
    return REPLAY_SUCCESS;
}

replay_error_t replay_stop_recording(replay_context_t* context) {
    if (!context || !context->initialized) return REPLAY_ERROR_NOT_INITIALIZED;
    if (!context->is_recording) return REPLAY_ERROR_RECORDING;
    
    context->is_recording = false;
    return REPLAY_SUCCESS;
}

replay_error_t replay_save_frame(replay_context_t* context, const body_state_t* bodies, uint32_t body_count) {
    if (!context || !context->initialized || !bodies) return REPLAY_ERROR_INVALID_PARAM;
    if (!context->is_recording) return REPLAY_ERROR_RECORDING;
    
    if (context->frame_count >= context->frame_capacity) {
        uint32_t new_capacity = context->frame_capacity * 2;
        replay_frame_t* new_frames = (replay_frame_t*)realloc(context->frames, new_capacity * sizeof(replay_frame_t));
        if (!new_frames) return REPLAY_ERROR_OUT_OF_MEMORY;
        
        context->frames = new_frames;
        context->frame_capacity = new_capacity;
    }
    
    replay_frame_t* frame = &context->frames[context->frame_count];
    frame->frame_number = context->frame_count;
    frame->timestamp = (uint32_t)get_timestamp_ms();
    frame->body_count = body_count;
    
    // Copy body states
    frame->body_states = (body_state_t*)malloc(body_count * sizeof(body_state_t));
    if (!frame->body_states) return REPLAY_ERROR_OUT_OF_MEMORY;
    
    memcpy(frame->body_states, bodies, body_count * sizeof(body_state_t));
    
    // Calculate frame checksum
    frame->checksum = calculate_checksum(frame->body_states, body_count * sizeof(body_state_t));
    context->running_checksum ^= frame->checksum;
    
    context->frame_count++;
    return REPLAY_SUCCESS;
}

replay_error_t replay_save_to_file(replay_context_t* context, const char* filename) {
    if (!context || !filename) return REPLAY_ERROR_INVALID_PARAM;
    if (!context->initialized) return REPLAY_ERROR_NOT_INITIALIZED;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return REPLAY_ERROR_FILE_NOT_FOUND;
    
    // Write header
    replay_header_t header;
    header.magic_number = REPLAY_MAGIC_NUMBER;
    header.version = REPLAY_VERSION;
    header.header_size = sizeof(replay_header_t);
    header.frame_count = context->frame_count;
    header.body_count = context->body_count;
    header.duration_ms = context->frame_count * (uint32_t)(context->fixed_timestep * 1000.0f);
    header.compression_type = 0; // No compression
    header.checksum = context->running_checksum;
    
    fwrite(&header, sizeof(replay_header_t), 1, file);
    
    // Write frames
    for (uint32_t i = 0; i < context->frame_count; i++) {
        replay_frame_t* frame = &context->frames[i];
        
        fwrite(&frame->frame_number, sizeof(uint32_t), 1, file);
        fwrite(&frame->timestamp, sizeof(uint32_t), 1, file);
        fwrite(&frame->body_count, sizeof(uint32_t), 1, file);
        fwrite(frame->body_states, sizeof(body_state_t), frame->body_count, file);
        fwrite(&frame->input_count, sizeof(uint32_t), 1, file);
        fwrite(frame->input_data, sizeof(uint8_t), frame->input_count, file);
        fwrite(&frame->checksum, sizeof(uint32_t), 1, file);
    }
    
    fclose(file);
    return REPLAY_SUCCESS;
}

replay_error_t replay_load_from_file(replay_context_t* context, const char* filename) {
    if (!context || !filename) return REPLAY_ERROR_INVALID_PARAM;
    if (!context->initialized) return REPLAY_ERROR_NOT_INITIALIZED;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return REPLAY_ERROR_FILE_NOT_FOUND;
    
    // Read header
    replay_header_t header;
    if (fread(&header, sizeof(replay_header_t), 1, file) != 1) {
        fclose(file);
        return REPLAY_ERROR_INVALID_FORMAT;
    }
    
    // Validate header
    if (header.magic_number != REPLAY_MAGIC_NUMBER || header.version != REPLAY_VERSION) {
        fclose(file);
        return REPLAY_ERROR_INVALID_FORMAT;
    }
    
    // Clear existing loaded frames
    for (uint32_t i = 0; i < context->loaded_frame_count; i++) {
        free(context->loaded_frames[i].body_states);
        free(context->loaded_frames[i].input_data);
    }
    
    // Allocate frames
    context->loaded_frames = (replay_frame_t*)calloc(header.frame_count, sizeof(replay_frame_t));
    if (!context->loaded_frames) {
        fclose(file);
        return REPLAY_ERROR_OUT_OF_MEMORY;
    }
    
    // Read frames
    for (uint32_t i = 0; i < header.frame_count; i++) {
        replay_frame_t* frame = &context->loaded_frames[i];
        
        fread(&frame->frame_number, sizeof(uint32_t), 1, file);
        fread(&frame->timestamp, sizeof(uint32_t), 1, file);
        fread(&frame->body_count, sizeof(uint32_t), 1, file);
        
        frame->body_states = (body_state_t*)malloc(frame->body_count * sizeof(body_state_t));
        fread(frame->body_states, sizeof(body_state_t), frame->body_count, file);
        
        fread(&frame->input_count, sizeof(uint32_t), 1, file);
        if (frame->input_count > 0) {
            frame->input_data = (uint8_t*)malloc(frame->input_count);
            fread(frame->input_data, sizeof(uint8_t), frame->input_count, file);
        }
        
        fread(&frame->checksum, sizeof(uint32_t), 1, file);
        
        // Verify checksum
        uint32_t calculated_checksum = calculate_checksum(frame->body_states, frame->body_count * sizeof(body_state_t));
        if (calculated_checksum != frame->checksum) {
            fclose(file);
            return REPLAY_ERROR_CHECKSUM_MISMATCH;
        }
    }
    
    context->loaded_frame_count = header.frame_count;
    context->body_count = header.body_count;
    
    fclose(file);
    return REPLAY_SUCCESS;
}

replay_error_t replay_start_playback(replay_context_t* context, const char* filename) {
    if (!context || !context->initialized) return REPLAY_ERROR_NOT_INITIALIZED;
    if (context->is_recording || context->is_playing) return REPLAY_ERROR_PLAYBACK;
    
    replay_error_t result = replay_load_from_file(context, filename);
    if (result != REPLAY_SUCCESS) return result;
    
    context->is_playing = true;
    context->playback_frame = 0;
    
    return REPLAY_SUCCESS;
}

replay_error_t replay_step_frame(replay_context_t* context, replay_frame_t* frame) {
    if (!context || !context->initialized || !frame) return REPLAY_ERROR_INVALID_PARAM;
    if (!context->is_playing) return REPLAY_ERROR_PLAYBACK;
    
    if (context->playback_frame >= context->loaded_frame_count) {
        return REPLAY_ERROR_INVALID_PARAM; // End of replay
    }
    
    *frame = context->loaded_frames[context->playback_frame++];
    return REPLAY_SUCCESS;
}

replay_error_t replay_set_deterministic_mode(replay_context_t* context, bool enabled) {
    if (!context || !context->initialized) return REPLAY_ERROR_NOT_INITIALIZED;
    
    context->deterministic_mode = enabled;
    return REPLAY_SUCCESS;
}

replay_error_t replay_set_fixed_timestep(replay_context_t* context, float timestep) {
    if (!context || !context->initialized || timestep <= 0.0f) return REPLAY_ERROR_INVALID_PARAM;
    
    context->fixed_timestep = timestep;
    return REPLAY_SUCCESS;
}

replay_error_t replay_validate_replay(replay_context_t* context, bool* is_valid) {
    if (!context || !is_valid) return REPLAY_ERROR_INVALID_PARAM;
    if (!context->initialized) return REPLAY_ERROR_NOT_INITIALIZED;
    
    *is_valid = true;
    
    // Validate frame sequence
    for (uint32_t i = 1; i < context->loaded_frame_count; i++) {
        if (context->loaded_frames[i].frame_number != context->loaded_frames[i-1].frame_number + 1) {
            *is_valid = false;
            break;
        }
    }
    
    return REPLAY_SUCCESS;
}
