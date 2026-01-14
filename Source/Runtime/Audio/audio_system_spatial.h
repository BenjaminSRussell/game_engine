/**
 * =================================================================================================
 *                          AUDIO SYSTEM SPATIAL INTEGRATION
 * =================================================================================================
 * 
 * Integration point for spatial audio system with main audio system
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "audio/audio_system.h"
#include "audio/spatial/spatial_audio_bridge.h"

// Add spatial audio context to AudioSystem
typedef struct {
    AudioSystem base_system;
    SpatialAudioContext* spatial_context;
    bool spatial_enabled;
} AudioSystemWithSpatial;

// Initialize audio system with spatial audio support
void audio_system_with_spatial_init(AudioSystemWithSpatial* sys, u32 max_channels);
void audio_system_with_spatial_free(AudioSystemWithSpatial* sys);

// Toggle spatial audio processing
void audio_enable_spatial(AudioSystemWithSpatial* sys, bool enable);
bool audio_is_spatial_enabled(AudioSystemWithSpatial* sys);

// Access spatial audio context for advanced control
SpatialAudioContext* audio_get_spatial_context(AudioSystemWithSpatial* sys);
