// Audio integration implementation
#include <audio/audio_integration.h>
#include <weather/weather_audio.h>
#include <world/water_system.h>
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

// Sound file mappings
static const char *AUDIO_SOURCE_NAMES[AUDIO_SOURCE_COUNT] = {
    "rain",
    "wind",
    "thunder",
    "water_flow",
    "splash",
    "bubble",
    "footstep",
    "creature",
    "ui",
    "ambience"
};

static const char *AUDIO_CHANNEL_NAMES[AUDIO_CHANNEL_COUNT] = {
    "music",
    "ambient",
    "weather",
    "water",
    "creature",
    "ui",
    "voice",
    "3d_world"
};

// Default channel volumes
static const f32 DEFAULT_CHANNEL_VOLUMES[AUDIO_CHANNEL_COUNT] = {
    1.0f,  // Music
    0.7f,  // Ambient
    0.8f,  // Weather
    0.6f,  // Water
    0.7f,  // Creature
    0.9f,  // UI
    1.0f,  // Voice
    0.8f   // 3D World
};

void audio_integration_init(AudioIntegration *integration, AudioSystem *engine) {
    if (!integration) return;

    memset(integration, 0, sizeof(AudioIntegration));
    integration->audio_engine = engine;

    // Initialize channel volumes
    for (u32 i = 0; i < AUDIO_CHANNEL_COUNT; i++) {
        integration->channel_volumes[i] = DEFAULT_CHANNEL_VOLUMES[i];
    }

    // Initialize source handles
    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        integration->active_sources[i] = 0;
        integration->source_properties[i].loop = false;
        integration->source_properties[i].volume = 1.0f;
        integration->source_properties[i].pitch = 1.0f;
        integration->source_properties[i].distance_scale = 1.0f;
    }

    integration->enable_3d_audio = true;
    integration->enable_doppler = false;
    integration->global_volume = 1.0f;
    integration->listener_distance_scale = 1.0f;
    integration->initialized = true;

    LOG_INFO("Audio integration system initialized");
}

void audio_integration_free(AudioIntegration *integration) {
    if (!integration) return;

    // Stop all sounds
    audio_integration_stop_all_sounds(integration, AUDIO_CHANNEL_MUSIC);

    memset(integration, 0, sizeof(AudioIntegration));
}

void audio_integration_connect_weather(AudioIntegration *integration,
                                      WeatherAudioSystem *weather_audio) {
    if (!integration) return;
    integration->weather_audio = weather_audio;
}

void audio_integration_connect_water(AudioIntegration *integration,
                                    WaterSystem *water_system) {
    if (!integration) return;
    integration->water_system = water_system;
}

AudioSourceHandle audio_integration_play_sound(AudioIntegration *integration,
                                              AudioSourceType type,
                                              const char *sound_name,
                                              AudioChannelType channel,
                                              f32 volume) {
    if (!integration || !integration->initialized) return 0;

    // Validate parameters
    if (type >= AUDIO_SOURCE_COUNT || channel >= AUDIO_CHANNEL_COUNT) {
        LOG_WARN("Invalid audio source or channel type");
        return 0;
    }

    // Calculate final volume
    f32 final_volume = volume *
        integration->channel_volumes[channel] *
        integration->global_volume;

    if (final_volume < 0.01f) return 0;

    // Create a handle for this sound
    // In a real implementation, this would interact with the audio engine
    AudioSourceHandle handle = (AudioSourceHandle)(type + 1);

    integration->active_sources[type] = handle;

    LOG_TRACE("Playing sound: %s (type: %s, channel: %s, volume: %.2f)",
             sound_name, AUDIO_SOURCE_NAMES[type],
             AUDIO_CHANNEL_NAMES[channel], final_volume);

    return handle;
}

AudioSourceHandle audio_integration_play_3d_sound(AudioIntegration *integration,
                                                 AudioSourceType type,
                                                 const char *sound_name,
                                                 Vec3 position,
                                                 AudioChannelType channel,
                                                 f32 volume) {
    if (!integration || !integration->initialized) return 0;

    if (!integration->enable_3d_audio) {
        return audio_integration_play_sound(integration, type, sound_name,
                                           channel, volume);
    }

    // Validate parameters
    if (type >= AUDIO_SOURCE_COUNT || channel >= AUDIO_CHANNEL_COUNT) {
        LOG_WARN("Invalid audio source or channel type");
        return 0;
    }

    // Calculate final volume
    f32 final_volume = volume *
        integration->channel_volumes[channel] *
        integration->global_volume;

    if (final_volume < 0.01f) return 0;

    // Create a handle for this 3D sound
    AudioSourceHandle handle = (AudioSourceHandle)(type + 1);

    integration->active_sources[type] = handle;

    // Store 3D properties
    integration->source_properties[type].position = position;
    integration->source_properties[type].volume = volume;

    LOG_TRACE("Playing 3D sound: %s at (%.1f, %.1f, %.1f) with volume %.2f",
             sound_name, position.x, position.y, position.z, final_volume);

    return handle;
}

void audio_integration_stop_sound(AudioIntegration *integration,
                                 AudioSourceHandle handle) {
    if (!integration) return;

    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] == handle) {
            integration->active_sources[i] = 0;
            LOG_TRACE("Stopped audio source %u", i);
            return;
        }
    }
}

void audio_integration_stop_all_sounds(AudioIntegration *integration,
                                      AudioChannelType channel) {
    if (!integration) return;

    if (channel >= AUDIO_CHANNEL_COUNT) return;

    u32 count = 0;
    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] != 0) {
            integration->active_sources[i] = 0;
            count++;
        }
    }

    LOG_TRACE("Stopped %u audio sources on channel %s", count,
             AUDIO_CHANNEL_NAMES[channel]);
}

void audio_integration_set_channel_volume(AudioIntegration *integration,
                                         AudioChannelType channel,
                                         f32 volume) {
    if (!integration || channel >= AUDIO_CHANNEL_COUNT) return;

    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    integration->channel_volumes[channel] = volume;

    LOG_TRACE("Set channel %s volume to %.2f",
             AUDIO_CHANNEL_NAMES[channel], volume);
}

f32 audio_integration_get_channel_volume(AudioIntegration *integration,
                                        AudioChannelType channel) {
    if (!integration || channel >= AUDIO_CHANNEL_COUNT) return 0.0f;
    return integration->channel_volumes[channel];
}

void audio_integration_set_master_volume(AudioIntegration *integration,
                                        f32 volume) {
    if (!integration) return;

    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    integration->global_volume = volume;

    LOG_TRACE("Set master volume to %.2f", volume);
}

void audio_integration_update_source_position(AudioIntegration *integration,
                                             AudioSourceHandle handle,
                                             Vec3 position) {
    if (!integration) return;

    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] == handle) {
            integration->source_properties[i].position = position;
            return;
        }
    }
}

void audio_integration_update_listener_position(AudioIntegration *integration,
                                               Vec3 position, Vec3 forward,
                                               Vec3 up) {
    if (!integration) return;

    // In a real implementation, this would update the audio engine's listener
    // position and orientation for 3D audio calculations

    LOG_TRACE("Updated listener position to (%.1f, %.1f, %.1f)",
             position.x, position.y, position.z);
}

void audio_integration_set_source_properties(AudioIntegration *integration,
                                            AudioSourceHandle handle,
                                            const Audio3DProperties *props) {
    if (!integration || !props) return;

    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] == handle) {
            integration->source_properties[i] = *props;
            return;
        }
    }
}

void audio_integration_set_loop(AudioIntegration *integration,
                               AudioSourceHandle handle, bool loop) {
    if (!integration) return;

    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] == handle) {
            integration->source_properties[i].loop = loop;
            LOG_TRACE("Set loop %s for audio source %u",
                     loop ? "enabled" : "disabled", i);
            return;
        }
    }
}

bool audio_integration_is_playing(AudioIntegration *integration,
                                 AudioSourceHandle handle) {
    if (!integration) return false;

    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] == handle) {
            return true;
        }
    }

    return false;
}

const char *audio_source_type_get_name(AudioSourceType type) {
    if (type >= AUDIO_SOURCE_COUNT) return "Unknown";
    return AUDIO_SOURCE_NAMES[type];
}

const char *audio_channel_type_get_name(AudioChannelType channel) {
    if (channel >= AUDIO_CHANNEL_COUNT) return "Unknown";
    return AUDIO_CHANNEL_NAMES[channel];
}

void audio_integration_update(AudioIntegration *integration, f32 delta_time) {
    if (!integration || !integration->initialized) return;

    // Update 3D audio positions if needed
    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] == 0) continue;

        // Update doppler effect if enabled
        if (integration->enable_doppler) {
            Audio3DProperties *props = &integration->source_properties[i];
            // Doppler calculation would go here
            // pitch = base_pitch * (speed_of_sound + listener_velocity) /
            //                      (speed_of_sound + source_velocity)
        }
    }

    // Update weather audio if connected
    if (integration->weather_audio) {
        // Weather audio is updated separately by weather_audio_update()
    }

    // Update water audio if connected
    if (integration->water_system) {
        // Water audio updates would go here
    }
}

AudioIntegrationStats audio_integration_get_stats(
    const AudioIntegration *integration) {
    AudioIntegrationStats stats = {0};

    if (!integration) return stats;

    stats.active_sources = 0;
    stats.playing_sounds = 0;
    stats.looped_sounds = 0;

    for (u32 i = 0; i < AUDIO_SOURCE_COUNT; i++) {
        if (integration->active_sources[i] != 0) {
            stats.playing_sounds++;

            if (integration->source_properties[i].loop) {
                stats.looped_sounds++;
            }
        }
    }

    stats.active_sources = stats.playing_sounds;

    // CPU and memory usage would be calculated based on actual audio engine
    stats.cpu_usage = (f32)stats.playing_sounds * 0.5f;  // Approximate
    stats.memory_usage = (f32)stats.playing_sounds * 0.1f;  // Approximate in MB

    return stats;
}
