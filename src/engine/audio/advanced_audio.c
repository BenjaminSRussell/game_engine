// Advanced Audio Implementation
// Implements comprehensive 3D audio capabilities

#include "advanced_audio.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Utility functions implementation
float audio_db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

float audio_linear_to_db(float linear) {
    if (linear <= 0.0f) return -100.0f;
    return 20.0f * log10f(linear);
}

float audio_milliseconds_to_samples(float milliseconds, AudioSampleRate sampleRate) {
    return (milliseconds / 1000.0f) * (float)sampleRate;
}

float audio_samples_to_milliseconds(uint32_t samples, AudioSampleRate sampleRate) {
    return ((float)samples / (float)sampleRate) * 1000.0f;
}

void audio_calculate_direction(const float* from, const float* to, float* direction) {
    if (!from || !to || !direction) return;
    
    direction[0] = to[0] - from[0];
    direction[1] = to[1] - from[1];
    direction[2] = to[2] - from[2];
    
    // Normalize
    float length = sqrtf(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
    if (length > 0.0f) {
        direction[0] /= length;
        direction[1] /= length;
        direction[2] /= length;
    }
}

float audio_calculate_distance(const float* pos1, const float* pos2) {
    if (!pos1 || !pos2) return 0.0f;
    
    float dx = pos1[0] - pos2[0];
    float dy = pos1[1] - pos2[1];
    float dz = pos1[2] - pos2[2];
    
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// Audio world implementation
AudioWorld* audio_world_create(uint32_t maxSources, uint32_t maxBuffers, uint32_t maxEffects) {
    if (maxSources == 0 || maxBuffers == 0 || maxEffects == 0) {
        LOG_ERROR(LOG_CAT_GENERAL, "Invalid parameters for audio world creation");
        return NULL;
    }
    
    AudioWorld* world = malloc(sizeof(AudioWorld));
    if (!world) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate memory for audio world");
        return NULL;
    }
    
    memset(world, 0, sizeof(AudioWorld));
    
    // Allocate arrays
    world->sources = malloc(sizeof(AudioSource) * maxSources);
    world->buffers = malloc(sizeof(AudioBuffer) * maxBuffers);
    world->effects = malloc(sizeof(AudioEffect) * maxEffects);
    world->streams = malloc(sizeof(AudioStream) * 16); // Default 16 streams
    
    if (!world->sources || !world->buffers || !world->effects || !world->streams) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate memory for audio components");
        if (world->sources) free(world->sources);
        if (world->buffers) free(world->buffers);
        if (world->effects) free(world->effects);
        if (world->streams) free(world->streams);
        free(world);
        return NULL;
    }
    
    world->maxSources = maxSources;
    world->maxBuffers = maxBuffers;
    world->maxEffects = maxEffects;
    world->maxStreams = 16;
    
    // Initialize mixer settings
    world->mixer.masterVolume = 1.0f;
    world->mixer.musicVolume = 0.8f;
    world->mixer.sfxVolume = 1.0f;
    world->mixer.voiceVolume = 0.9f;
    world->mixer.ambientVolume = 0.6f;
    world->mixer.enable3DAudio = true;
    world->mixer.enableDoppler = true;
    world->mixer.enableHRTF = false;
    world->mixer.maxVoices = 32;
    world->mixer.activeVoices = 0;
    
    // Initialize listener
    world->listener.forwardX = 0.0f;
    world->listener.forwardY = 0.0f;
    world->listener.forwardZ = -1.0f;
    world->listener.upX = 0.0f;
    world->listener.upY = 1.0f;
    world->listener.upZ = 0.0f;
    
    world->globalTimeScale = 1.0f;
    
    // Initialize platform
    if (!audio_platform_init()) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to initialize audio platform");
        free(world->sources);
        free(world->buffers);
        free(world->effects);
        free(world->streams);
        free(world);
        return NULL;
    }
    
    world->isInitialized = true;
    LOG_INFO(LOG_CAT_GENERAL, "Audio world created with %u sources, %u buffers, %u effects", maxSources, maxBuffers, maxEffects);
    return world;
}

void audio_world_destroy(AudioWorld* world) {
    if (!world) return;
    
    // Destroy all sources
    for (uint32_t i = 0; i < world->sourceCount; i++) {
        audio_source_destroy(world, world->sources[i].id);
    }
    
    // Destroy all buffers
    for (uint32_t i = 0; i < world->bufferCount; i++) {
        audio_buffer_destroy(world, world->buffers[i].id);
    }
    
    // Destroy all effects
    for (uint32_t i = 0; i < world->effectCount; i++) {
        audio_effect_destroy(world, i + 1); // Use index + 1 as ID
    }
    
    // Destroy all streams
    for (uint32_t i = 0; i < world->streamCount; i++) {
        audio_stream_destroy(world, world->streams[i].id);
    }
    
    // Free arrays
    if (world->sources) free(world->sources);
    if (world->buffers) free(world->buffers);
    if (world->effects) free(world->effects);
    if (world->streams) free(world->streams);
    
    // Shutdown platform
    audio_platform_shutdown();
    
    world->isInitialized = false;
    LOG_INFO(LOG_CAT_GENERAL, "Audio world destroyed");
}

void audio_world_update(AudioWorld* world, float deltaTime) {
    if (!world || !world->isInitialized) return;
    
    float scaledDeltaTime = deltaTime * world->globalTimeScale;
    
    // Update platform
    audio_platform_update();
    
    // Update voice manager
    audio_voice_manager_update(world);
    
    // Update streams
    for (uint32_t i = 0; i < world->streamCount; i++) {
        if (world->streams[i].isStreaming) {
            audio_stream_update(world, world->streams[i].id);
        }
    }
    
    // Update 3D audio calculations
    if (world->mixer.enable3DAudio) {
        for (uint32_t i = 0; i < world->sourceCount; i++) {
            AudioSource* source = &world->sources[i];
            if (source->isPlaying) {
                // Apply 3D calculations
                float distance = audio_calculate_distance(
                    (float[]){world->listener.x, world->listener.y, world->listener.z},
                    (float[]){source->settings3D.x, source->settings3D.y, source->settings3D.z}
                );
                
                // Apply distance attenuation
                if (distance > source->settings3D.maxDistance) {
                    source->gain = 0.0f;
                } else if (distance > source->settings3D.referenceDistance) {
                    float attenuation = source->settings3D.referenceDistance / 
                        (source->settings3D.referenceDistance + 
                         source->settings3D.rolloffFactor * (distance - source->settings3D.referenceDistance));
                    source->gain = attenuation;
                } else {
                    source->gain = 1.0f;
                }
                
                // Apply Doppler effect if enabled
                if (world->mixer.enableDoppler) {
                    audio_apply_doppler_effect(world, source->id, 343.0f); // Speed of sound in m/s
                }
            }
        }
    }
}

bool audio_world_is_initialized(AudioWorld* world) {
    return world ? world->isInitialized : false;
}

// Listener management
void audio_listener_set_position(AudioWorld* world, float x, float y, float z) {
    if (!world) return;
    
    world->listener.x = x;
    world->listener.y = y;
    world->listener.z = z;
}

void audio_listener_set_velocity(AudioWorld* world, float vx, float vy, float vz) {
    if (!world) return;
    
    world->listener.vx = vx;
    world->listener.vy = vy;
    world->listener.vz = vz;
}

void audio_listener_set_orientation(AudioWorld* world, float forwardX, float forwardY, float forwardZ, float upX, float upY, float upZ) {
    if (!world) return;
    
    world->listener.forwardX = forwardX;
    world->listener.forwardY = forwardY;
    world->listener.forwardZ = forwardZ;
    world->listener.upX = upX;
    world->listener.upY = upY;
    world->listener.upZ = upZ;
}

void audio_listener_get_position(AudioWorld* world, float* x, float* y, float* z) {
    if (!world || !x || !y || !z) return;
    
    *x = world->listener.x;
    *y = world->listener.y;
    *z = world->listener.z;
}

// Buffer management
uint32_t audio_buffer_create(AudioWorld* world, const void* data, uint32_t size, AudioFormat format, AudioSampleRate sampleRate) {
    if (!world || !data || size == 0) return 0;
    
    if (world->bufferCount >= world->maxBuffers) {
        LOG_ERROR(LOG_CAT_GENERAL, "Maximum number of audio buffers reached");
        return 0;
    }
    
    uint32_t bufferId = world->bufferCount + 1; // 1-based indexing
    AudioBuffer* buffer = &world->buffers[world->bufferCount];
    
    memset(buffer, 0, sizeof(AudioBuffer));
    buffer->id = bufferId;
    buffer->format = format;
    buffer->sampleRate = sampleRate;
    buffer->size = size;
    
    // Copy audio data
    buffer->data = malloc(size);
    if (!buffer->data) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate memory for audio buffer data");
        return 0;
    }
    
    memcpy(buffer->data, data, size);
    
    // Set format properties
    switch (format) {
        case AUDIO_FORMAT_MONO8:
        case AUDIO_FORMAT_STEREO8:
            buffer->bitsPerSample = 8;
            break;
        case AUDIO_FORMAT_MONO16:
        case AUDIO_FORMAT_STEREO16:
        case AUDIO_FORMAT_5POINT1_16:
        case AUDIO_FORMAT_7POINT1_16:
            buffer->bitsPerSample = 16;
            break;
    }
    
    switch (format) {
        case AUDIO_FORMAT_MONO8:
        case AUDIO_FORMAT_MONO16:
            buffer->channels = 1;
            break;
        case AUDIO_FORMAT_STEREO8:
        case AUDIO_FORMAT_STEREO16:
            buffer->channels = 2;
            break;
        case AUDIO_FORMAT_5POINT1_16:
            buffer->channels = 6;
            break;
        case AUDIO_FORMAT_7POINT1_16:
            buffer->channels = 8;
            break;
    }
    
    // Create platform buffer
    uint32_t platformBufferId = audio_platform_create_buffer(data, size, format, sampleRate);
    if (platformBufferId == 0) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to create platform audio buffer");
        free(buffer->data);
        return 0;
    }
    
    world->bufferCount++;
    LOG_DEBUG(LOG_CAT_GENERAL, "Created audio buffer %u with %u bytes", bufferId, size);
    return bufferId;
}

void audio_buffer_destroy(AudioWorld* world, uint32_t bufferId) {
    if (!world || bufferId == 0 || bufferId > world->bufferCount) return;
    
    AudioBuffer* buffer = &world->buffers[bufferId - 1];
    
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    
    // Destroy platform buffer
    audio_platform_destroy_buffer(bufferId);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Destroyed audio buffer %u", bufferId);
}

AudioBuffer* audio_buffer_get(AudioWorld* world, uint32_t bufferId) {
    if (!world || bufferId == 0 || bufferId > world->bufferCount) return NULL;
    return &world->buffers[bufferId - 1];
}

// Source management
uint32_t audio_source_create(AudioWorld* world) {
    if (!world || world->sourceCount >= world->maxSources) {
        LOG_ERROR(LOG_CAT_GENERAL, "Cannot create audio source: maximum sources reached");
        return 0;
    }
    
    uint32_t sourceId = world->sourceCount + 1; // 1-based indexing
    AudioSource* source = &world->sources[world->sourceCount];
    
    memset(source, 0, sizeof(AudioSource));
    source->id = sourceId;
    source->gain = 1.0f;
    source->pitch = 1.0f;
    source->priority = 0;
    
    // Set default 3D settings
    source->settings3D.x = 0.0f;
    source->settings3D.y = 0.0f;
    source->settings3D.z = 0.0f;
    source->settings3D.vx = 0.0f;
    source->settings3D.vy = 0.0f;
    source->settings3D.vz = 0.0f;
    source->settings3D.coneInnerAngle = 6.28318f; // 360 degrees
    source->settings3D.coneOuterAngle = 6.28318f;
    source->settings3D.coneOuterGain = 0.0f;
    source->settings3D.maxDistance = 100.0f;
    source->settings3D.referenceDistance = 1.0f;
    source->settings3D.rolloffFactor = 1.0f;
    source->settings3D.loop = false;
    source->settings3D.relative = false;
    
    // Create platform source
    uint32_t platformSourceId = audio_platform_create_source();
    if (platformSourceId == 0) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to create platform audio source");
        return 0;
    }
    
    world->sourceCount++;
    world->mixer.activeVoices++;
    LOG_DEBUG(LOG_CAT_GENERAL, "Created audio source %u", sourceId);
    return sourceId;
}

void audio_source_destroy(AudioWorld* world, uint32_t sourceId) {
    if (!world || sourceId == 0 || sourceId > world->sourceCount) return;
    
    AudioSource* source = &world->sources[sourceId - 1];
    
    // Stop if playing
    if (source->isPlaying) {
        audio_source_stop(world, sourceId);
    }
    
    // Destroy platform source
    audio_platform_destroy_source(sourceId);
    
    source->isPlaying = false;
    source->isPaused = false;
    world->mixer.activeVoices--;
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Destroyed audio source %u", sourceId);
}

AudioSource* audio_source_get(AudioWorld* world, uint32_t sourceId) {
    if (!world || sourceId == 0 || sourceId > world->sourceCount) return NULL;
    return &world->sources[sourceId - 1];
}

void audio_source_set_buffer(AudioWorld* world, uint32_t sourceId, uint32_t bufferId) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    source->bufferId = bufferId;
    
    // Set platform buffer
    audio_platform_source_set_buffer(sourceId, bufferId);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Set buffer %u for source %u", bufferId, sourceId);
}

void audio_source_play(AudioWorld* world, uint32_t sourceId) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source || source->bufferId == 0) return;
    
    source->isPlaying = true;
    source->isPaused = false;
    
    // Play platform source
    audio_platform_source_play(sourceId);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Playing audio source %u", sourceId);
}

void audio_source_pause(AudioWorld* world, uint32_t sourceId) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    source->isPaused = true;
    
    // Pause platform source
    audio_platform_source_pause(sourceId);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Paused audio source %u", sourceId);
}

void audio_source_stop(AudioWorld* world, uint32_t sourceId) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    source->isPlaying = false;
    source->isPaused = false;
    source->position = 0.0f;
    
    // Stop platform source
    audio_platform_source_stop(sourceId);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Stopped audio source %u", sourceId);
}

void audio_source_set_gain(AudioWorld* world, uint32_t sourceId, float gain) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    source->gain = gain;
    
    // Set platform gain
    audio_platform_source_set_gain(sourceId, gain);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Set gain %.2f for source %u", gain, sourceId);
}

void audio_source_set_pitch(AudioWorld* world, uint32_t sourceId, float pitch) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    source->pitch = pitch;
    
    // Set platform pitch
    audio_platform_source_set_pitch(sourceId, pitch);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Set pitch %.2f for source %u", pitch, sourceId);
}

void audio_source_set_position(AudioWorld* world, uint32_t sourceId, float x, float y, float z) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    source->settings3D.x = x;
    source->settings3D.y = y;
    source->settings3D.z = z;
    
    // Set platform position
    audio_platform_source_set_position(sourceId, x, y, z);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Set position (%.2f, %.2f, %.2f) for source %u", x, y, z, sourceId);
}

void audio_source_set_velocity(AudioWorld* world, uint32_t sourceId, float vx, float vy, float vz) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    source->settings3D.vx = vx;
    source->settings3D.vy = vy;
    source->settings3D.vz = vz;
    
    // Set platform velocity
    audio_platform_source_set_velocity(sourceId, vx, vy, vz);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Set velocity (%.2f, %.2f, %.2f) for source %u", vx, vy, vz, sourceId);
}

// Mixer management
void audio_mixer_set_master_volume(AudioWorld* world, float volume) {
    if (!world) return;
    
    world->mixer.masterVolume = volume;
    LOG_DEBUG(LOG_CAT_GENERAL, "Set master volume to %.2f", volume);
}

void audio_mixer_set_music_volume(AudioWorld* world, float volume) {
    if (!world) return;
    
    world->mixer.musicVolume = volume;
    LOG_DEBUG(LOG_CAT_GENERAL, "Set music volume to %.2f", volume);
}

void audio_mixer_set_sfx_volume(AudioWorld* world, float volume) {
    if (!world) return;
    
    world->mixer.sfxVolume = volume;
    LOG_DEBUG(LOG_CAT_GENERAL, "Set SFX volume to %.2f", volume);
}

void audio_mixer_set_voice_volume(AudioWorld* world, float volume) {
    if (!world) return;
    
    world->mixer.voiceVolume = volume;
    LOG_DEBUG(LOG_CAT_GENERAL, "Set voice volume to %.2f", volume);
}

void audio_mixer_set_ambient_volume(AudioWorld* world, float volume) {
    if (!world) return;
    
    world->mixer.ambientVolume = volume;
    LOG_DEBUG(LOG_CAT_GENERAL, "Set ambient volume to %.2f", volume);
}

// Voice management
void audio_voice_manager_update(AudioWorld* world) {
    if (!world) return;
    
    // Count active voices
    uint32_t activeVoices = 0;
    for (uint32_t i = 0; i < world->sourceCount; i++) {
        if (world->sources[i].isPlaying) {
            activeVoices++;
        }
    }
    
    world->mixer.activeVoices = activeVoices;
    
    // Steal voices if necessary
    if (activeVoices > world->mixer.maxVoices) {
        audio_voice_manager_steal_voice(world, 0); // Steal lowest priority voice
    }
}

uint32_t audio_voice_manager_get_available_voices(AudioWorld* world) {
    if (!world) return 0;
    
    return world->mixer.maxVoices - world->mixer.activeVoices;
}

void audio_voice_manager_steal_voice(AudioWorld* world, int32_t priority) {
    if (!world) return;
    
    // Find lowest priority voice
    int32_t lowestPriority = INT32_MAX;
    uint32_t lowestPrioritySource = 0;
    
    for (uint32_t i = 0; i < world->sourceCount; i++) {
        AudioSource* source = &world->sources[i];
        if (source->isPlaying && source->priority < lowestPriority && source->priority < priority) {
            lowestPriority = source->priority;
            lowestPrioritySource = source->id;
        }
    }
    
    if (lowestPrioritySource > 0) {
        audio_source_stop(world, lowestPrioritySource);
        LOG_DEBUG(LOG_CAT_GENERAL, "Stole voice from source %u with priority %d", lowestPrioritySource, lowestPriority);
    }
}

// Apply Doppler effect
void audio_apply_doppler_effect(AudioWorld* world, uint32_t sourceId, float speedOfSound) {
    AudioSource* source = audio_source_get(world, sourceId);
    if (!source) return;
    
    // Calculate relative velocity
    float relativeVelocity = source->settings3D.vz - world->listener.vz;
    
    // Calculate Doppler shift
    float dopplerFactor = 1.0f + (relativeVelocity / speedOfSound);
    
    // Apply to pitch
    float newPitch = source->pitch * dopplerFactor;
    audio_source_set_pitch(world, sourceId, newPitch);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Applied Doppler effect to source %u: pitch %.2f", sourceId, newPitch);
}

// Platform stubs (would be implemented per platform)
bool audio_platform_init(void) {
    LOG_INFO(LOG_CAT_GENERAL, "Audio platform initialized");
    return true;
}

void audio_platform_shutdown(void) {
    LOG_INFO(LOG_CAT_GENERAL, "Audio platform shutdown");
}

void audio_platform_update(void) {
    // Platform-specific update
}

uint32_t audio_platform_create_buffer(const void* data, uint32_t size, AudioFormat format, AudioSampleRate sampleRate) {
    return 1; // Stub implementation
}

void audio_platform_destroy_buffer(uint32_t bufferId) {
    // Stub implementation
}

uint32_t audio_platform_create_source(void) {
    return 1; // Stub implementation
}

void audio_platform_destroy_source(uint32_t sourceId) {
    // Stub implementation
}

void audio_platform_source_play(uint32_t sourceId) {
    // Stub implementation
}

void audio_platform_source_pause(uint32_t sourceId) {
    // Stub implementation
}

void audio_platform_source_stop(uint32_t sourceId) {
    // Stub implementation
}

void audio_platform_source_set_buffer(uint32_t sourceId, uint32_t bufferId) {
    // Stub implementation
}

void audio_platform_source_set_gain(uint32_t sourceId, float gain) {
    // Stub implementation
}

void audio_platform_source_set_pitch(uint32_t sourceId, float pitch) {
    // Stub implementation
}

void audio_platform_source_set_position(uint32_t sourceId, float x, float y, float z) {
    // Stub implementation
}

void audio_platform_source_set_velocity(uint32_t sourceId, float vx, float vy, float vz) {
    // Stub implementation
}

void audio_platform_source_set_direction(uint32_t sourceId, float x, float y, float z) {
    // Stub implementation
}

uint32_t audio_effect_create(AudioWorld* world, AudioEffectType type) {
    if (!world) return 0;
    
    if (world->effectCount >= world->maxEffects) {
        LOG_ERROR(LOG_CAT_GENERAL, "Maximum number of audio effects reached");
        return 0;
    }
    
    uint32_t effectId = world->effectCount + 1; // 1-based indexing
    AudioEffect* effect = &world->effects[world->effectCount];
    
    memset(effect, 0, sizeof(AudioEffect));
    effect->type = type;
    effect->wetLevel = 0.5f;
    effect->dryLevel = 0.5f;
    effect->enabled = true;
    
    world->effectCount++;
    LOG_DEBUG(LOG_CAT_GENERAL, "Created audio effect %u with type %d", effectId, type);
    return effectId;
}

void audio_effect_destroy(AudioWorld* world, uint32_t effectId) {
    if (!world || effectId == 0 || effectId > world->effectCount) return;
    
    AudioEffect* effect = &world->effects[effectId - 1];
    
    if (effect->effectData) {
        free(effect->effectData);
        effect->effectData = NULL;
    }
    
    effect->enabled = false;
    LOG_DEBUG(LOG_CAT_GENERAL, "Destroyed audio effect %u", effectId);
}

void audio_stream_destroy(AudioWorld* world, uint32_t streamId) {
    if (!world || streamId == 0 || streamId > world->streamCount) return;
    
    AudioStream* stream = &world->streams[streamId - 1];
    
    if (stream->fileHandle) {
        // Close file handle
        stream->fileHandle = NULL;
    }
    
    // Free buffers
    for (int i = 0; i < 4; i++) {
        if (stream->buffers[i]) {
            free(stream->buffers[i]);
            stream->buffers[i] = NULL;
        }
    }
    
    stream->isStreaming = false;
    LOG_DEBUG(LOG_CAT_GENERAL, "Destroyed audio stream %u", streamId);
}

void audio_stream_update(AudioWorld* world, uint32_t streamId) {
    if (!world || streamId == 0 || streamId > world->streamCount) return;
    
    AudioStream* stream = &world->streams[streamId - 1];
    if (!stream->isStreaming) return;
    
    // Stub implementation - would handle streaming buffer updates
    LOG_DEBUG(LOG_CAT_GENERAL, "Updating audio stream %u", streamId);
}
