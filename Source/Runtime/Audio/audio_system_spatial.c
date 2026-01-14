/**
 * =================================================================================================
 *                          AUDIO SYSTEM SPATIAL INTEGRATION
 * =================================================================================================
 */

#include "audio/audio_system_spatial.h"
#include "audio/audio_system.h"
#include <stdlib.h>
#include <stdio.h>

void audio_system_with_spatial_init(AudioSystemWithSpatial* sys, u32 max_channels) {
    if (!sys) return;
    
    // Initialize base audio system
    audio_system_init(&sys->base_system, max_channels);
    
    // Initialize spatial audio
    sys->spatial_context = spatial_audio_create();
    sys->spatial_enabled = false;
    
}

void audio_system_with_spatial_free(AudioSystemWithSpatial* sys) {
    if (!sys) return;
    
    // Free spatial context
    if (sys->spatial_context) {
        spatial_audio_destroy(sys->spatial_context);
        sys->spatial_context = NULL;
    }
    
    // Free base system
    audio_system_free(&sys->base_system);
    
}

void audio_enable_spatial(AudioSystemWithSpatial* sys, bool enable) {
    if (!sys) return;
    
    sys->spatial_enabled = enable;
}

bool audio_is_spatial_enabled(AudioSystemWithSpatial* sys) {
    return sys ? sys->spatial_enabled : false;
}

SpatialAudioContext* audio_get_spatial_context(AudioSystemWithSpatial* sys) {
    return sys ? sys->spatial_context : NULL;
}
