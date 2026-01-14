#include "audio/advanced/dolby_atmos.h"
#include "audio/audio_system.h"
#include "core/engine.h"
#include <stdio.h>

/**
 * =================================================================================================
 *                                   DOLBY ATMOS INTEGRATION
 * =================================================================================================
 * 
 * Integration layer for Dolby Atmos with the main audio system
 */

// -------------------------------------------------------------------------------------------------
// INTEGRATION STATE
// -------------------------------------------------------------------------------------------------

static AtmosRenderer g_atmos_renderer = {0};
static bool g_atmos_enabled = false;
static float g_output_buffer[ATMOS_BUFFER_SIZE * ATMOS_MAX_BED_CHANNELS] = {0};

// -------------------------------------------------------------------------------------------------
// INITIALIZATION AND SHUTDOWN
// -------------------------------------------------------------------------------------------------

bool Audio_InitializeAtmos() {
    printf("Initializing Dolby Atmos integration...\n");
    
    // Initialize Dolby Atmos SDK
    if (!Atmos_InitializeSDK(&g_atmos_renderer)) {
        printf("Failed to initialize Dolby Atmos SDK\n");
        return false;
    }
    
    // Detect and configure speaker layout
    if (!Atmos_DetectSpeakerLayout(&g_atmos_renderer.speaker_config)) {
        printf("Failed to configure speaker layout\n");
        return false;
    }
    
    // Configure speaker distances and delays
    Atmos_ConfigureSpeakerDistances(&g_atmos_renderer.speaker_config);
    
    // Load HRTF database for binaural rendering
    Atmos_LoadHRTFDatabase(NULL);
    
    // Check if headphones are detected (simplified)
    bool headphones_detected = true; // In real implementation, would detect audio device
    if (headphones_detected) {
        printf("Headphones detected, enabling binaural rendering\n");
        Atmos_EnableBinauralMode(&g_atmos_renderer.speaker_config);
        Atmos_SimulateRoomForHeadphones(&g_atmos_renderer);
        Atmos_EnhanceExternalization(&g_atmos_renderer);
    }
    
    g_atmos_enabled = true;
    printf("Dolby Atmos integration initialized successfully\n");
    return true;
}

void Audio_ShutdownAtmos() {
    if (g_atmos_enabled) {
        Atmos_Shutdown(&g_atmos_renderer);
        g_atmos_enabled = false;
        printf("Dolby Atmos integration shutdown\n");
    }
}

// -------------------------------------------------------------------------------------------------
// AUDIO PROCESSING
// -------------------------------------------------------------------------------------------------

void Audio_ProcessAtmosFrame() {
    if (!g_atmos_enabled || !Atmos_IsInitialized(&g_atmos_renderer)) {
        return;
    }
    
    // Update listener position from game state
    // This would typically get the player/camera position
    Vec3 listener_pos = {0.0f, 1.7f, 0.0f}; // Default ear height
    Quat listener_orient = {0.0f, 0.0f, 0.0f, 1.0f};
    
    Atmos_UpdateListener(&g_atmos_renderer, listener_pos, listener_orient);
    
    // Check if binaural rendering is enabled
    if (g_atmos_renderer.speaker_config.headphone_mode) {
        // Render binaural audio for headphones
        static float left_buffer[ATMOS_BUFFER_SIZE];
        static float right_buffer[ATMOS_BUFFER_SIZE];
        
        if (Atmos_RenderBinauralFrame(&g_atmos_renderer, left_buffer, right_buffer, ATMOS_BUFFER_SIZE)) {
            // Interleave stereo output
            float stereo_buffer[ATMOS_BUFFER_SIZE * 2];
            for (size_t i = 0; i < ATMOS_BUFFER_SIZE; i++) {
                stereo_buffer[i * 2] = left_buffer[i];
                stereo_buffer[i * 2 + 1] = right_buffer[i];
            }
            
            // Send stereo audio to output device
            Audio_SendToOutput(stereo_buffer, ATMOS_BUFFER_SIZE * 2);
        }
    } else {
        // Render multi-channel audio for speakers
        if (Atmos_RenderFrame(&g_atmos_renderer, g_output_buffer, ATMOS_BUFFER_SIZE)) {
            // Send rendered audio to output device
            Audio_SendToOutput(g_output_buffer, ATMOS_BUFFER_SIZE * g_atmos_renderer.speaker_config.channel_count);
        }
    }
}

// -------------------------------------------------------------------------------------------------
// OBJECT MANAGEMENT
// -------------------------------------------------------------------------------------------------

uint32_t Audio_CreateAtmosObject(AtmosObjectType type) {
    if (!g_atmos_enabled) {
        return 0;
    }
    
    return Atmos_CreateAudioObject(&g_atmos_renderer, type);
}

bool Audio_AssignAudioToAtmosObject(uint32_t object_id, void* audio_data, size_t size) {
    if (!g_atmos_enabled) {
        return false;
    }
    
    return Atmos_AssignAudioToObject(&g_atmos_renderer, object_id, audio_data, size);
}

bool Audio_SetAtmosObjectPosition(uint32_t object_id, float x, float y, float z) {
    if (!g_atmos_enabled) {
        return false;
    }
    
    Vec3 position = {x, y, z};
    return Atmos_SetObjectPosition(&g_atmos_renderer, object_id, position);
}

bool Audio_SetAtmosObjectVelocity(uint32_t object_id, float vx, float vy, float vz) {
    if (!g_atmos_enabled || object_id == 0 || object_id > ATMOS_MAX_OBJECTS) {
        return false;
    }
    
    uint32_t index = object_id - 1;
    if (!g_atmos_renderer.objects[index].active) {
        return false;
    }
    
    g_atmos_renderer.objects[index].velocity = (Vec3){vx, vy, vz};
    return true;
}

bool Audio_SetAtmosObjectGain(uint32_t object_id, float gain) {
    if (!g_atmos_enabled) {
        return false;
    }
    
    return Atmos_SetObjectGain(&g_atmos_renderer, object_id, gain);
}

bool Audio_RemoveAtmosObject(uint32_t object_id) {
    if (!g_atmos_enabled || object_id == 0 || object_id > ATMOS_MAX_OBJECTS) {
        return false;
    }
    
    uint32_t index = object_id - 1;
    if (!g_atmos_renderer.objects[index].active) {
        return false;
    }
    
    // Deactivate object
    g_atmos_renderer.objects[index].active = false;
    g_atmos_renderer.objects[index].audio_data = NULL;
    g_atmos_renderer.objects[index].data_size = 0;
    g_atmos_renderer.active_object_count--;
    
    printf("Removed Atmos object %u\n", object_id);
    return true;
}

// -------------------------------------------------------------------------------------------------
// GAME INTEGRATION HELPERS
// -------------------------------------------------------------------------------------------------

void Audio_UpdateAtmosFromGameWorld() {
    if (!g_atmos_enabled) {
        return;
    }
    
    // This function would be called each frame to update Atmos objects
    // based on game world state (moving entities, environmental sounds, etc.)
    
    // Example: Update positions for all moving objects
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!g_atmos_renderer.objects[i].active) {
            continue;
        }
        
        // Skip static objects
        if (g_atmos_renderer.objects[i].type == ATMOS_OBJECT_STATIC) {
            continue;
        }
        
        // For dynamic objects, update position based on game entity
        // This would interface with the game's entity system
        // Audio_UpdateAtmosObjectFromEntity(i);
    }
}

void Audio_SetAtmosMasterVolume(float volume) {
    if (!g_atmos_enabled) {
        return;
    }
    
    Atmos_SetMasterGain(&g_atmos_renderer, volume);
}

bool Audio_IsAtmosEnabled() {
    return g_atmos_enabled;
}

AtmosRenderer* Audio_GetAtmosRenderer() {
    return &g_atmos_renderer;
}

// -------------------------------------------------------------------------------------------------
// EXPORT AND DEBUGGING
// -------------------------------------------------------------------------------------------------

bool Audio_ExportAtmosRecording(const char* filename) {
    if (!g_atmos_enabled) {
        return false;
    }
    
    // Generate metadata and export to ADM BWF format
    AtmosMetadata metadata = {0};
    if (!Atmos_GenerateMetadata(&g_atmos_renderer, &metadata)) {
        return false;
    }
    
    return Atmos_ExportADM_BWF(&g_atmos_renderer, filename);
}

void Audio_DebugAtmosObjects() {
    if (!g_atmos_enabled) {
        printf("Dolby Atmos is not enabled\n");
        return;
    }
    
    printf("=== Dolby Atmos Debug Info ===\n");
    printf("Active objects: %u / %d\n", g_atmos_renderer.active_object_count, ATMOS_MAX_OBJECTS);
    printf("Speaker layout: %d (%u channels)\n", g_atmos_renderer.speaker_config.layout, g_atmos_renderer.speaker_config.channel_count);
    printf("Master gain: %.2f\n", g_atmos_renderer.master_gain);
    printf("Listener position: (%.2f, %.2f, %.2f)\n", 
           g_atmos_renderer.listener_position.x,
           g_atmos_renderer.listener_position.y,
           g_atmos_renderer.listener_position.z);
    
    printf("\nActive Objects:\n");
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!g_atmos_renderer.objects[i].active) {
            continue;
        }
        
        AtmosAudioObject* obj = &g_atmos_renderer.objects[i];
        printf("  Object %u: Type=%d, Pos=(%.2f,%.2f,%.2f), Gain=%.2f, Priority=%u\n",
               obj->id, obj->type, obj->position.x, obj->position.y, obj->position.z,
               obj->gain, obj->priority);
    }
    printf("==============================\n");
}
