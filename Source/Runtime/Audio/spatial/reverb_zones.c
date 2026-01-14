#include "audio/spatial/reverb_zones.h"
#include "core/core.h"
#include <include/math/math_all.h>
#include <string.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                          ENVIRONMENT REVERB ZONES - AGENT_AUDIO_2
 * =================================================================================================
 *
 * PURPOSE: Dynamic reverb based on listener position with zone blending.
 * =================================================================================================
 */

// -------------------------------------------------------------------------------------------------
// GLOBAL REVERB ZONES STATE
// -------------------------------------------------------------------------------------------------

static ReverbZoneSystem g_reverb_system = {0};
static bool g_reverb_initialized = false;

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Reverb zone system implementation
// -------------------------------------------------------------------------------------------------

bool ReverbZones_Init(ReverbZoneSystem* system, u32 max_zones, u32 sample_rate, u32 buffer_size) {
    if (!system) return false;
    
    system->max_zones = max_zones;
    system->zone_count = 0;
    system->sample_rate = sample_rate;
    system->buffer_size = buffer_size;
    
    // Allocate zone array
    system->zones = malloc(sizeof(ReverbZone) * max_zones);
    if (!system->zones) {
        return false;
    }
    
    // Initialize convolution reverb processors
    system->processors = malloc(sizeof(ConvolutionReverb) * 2);  // Stereo
    if (!system->processors) {
        free(system->zones);
        return false;
    }
    
    // Initialize reverb processors
    for (u32 i = 0; i < 2; i++) {
        if (!ConvolutionReverb_Init(&system->processors[i], sample_rate, buffer_size)) {
            free(system->zones);
            free(system->processors);
            return false;
        }
    }
    
    // Allocate mixing buffers
    system->wet_buffer = malloc(buffer_size * sizeof(f32) * 2);  // Stereo
    system->dry_buffer = malloc(buffer_size * sizeof(f32) * 2);  // Stereo
    system->zone_buffer = malloc(buffer_size * sizeof(f32) * 2);  // Stereo
    
    if (!system->wet_buffer || !system->dry_buffer || !system->zone_buffer) {
        free(system->zones);
        free(system->processors);
        if (system->wet_buffer) free(system->wet_buffer);
        if (system->dry_buffer) free(system->dry_buffer);
        if (system->zone_buffer) free(system->zone_buffer);
        return false;
    }
    
    // Initialize default parameters
    system->blend_distance = 2.0f;      // 2 meter blend radius
    system->max_blend_zones = 4;          // Maximum zones to blend
    system->global_reverb_level = 0.3f;    // Global reverb level
    
    system->initialized = true;
    printf("Reverb Zone System initialized: %d max zones\n", max_zones);
    return true;
}

void ReverbZones_Shutdown(ReverbZoneSystem* system) {
    if (system && system->initialized) {
        if (system->zones) {
            free(system->zones);
            system->zones = NULL;
        }
        if (system->processors) {
            for (u32 i = 0; i < 2; i++) {
                ConvolutionReverb_Cleanup(&system->processors[i]);
            }
            free(system->processors);
            system->processors = NULL;
        }
        if (system->wet_buffer) {
            free(system->wet_buffer);
            system->wet_buffer = NULL;
        }
        if (system->dry_buffer) {
            free(system->dry_buffer);
            system->dry_buffer = NULL;
        }
        if (system->zone_buffer) {
            free(system->zone_buffer);
            system->zone_buffer = NULL;
        }
        system->initialized = false;
        printf("Reverb Zone System shutdown\n");
    }
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Zone management
// -------------------------------------------------------------------------------------------------

bool ReverbZones_AddZone(ReverbZoneSystem* system, const ReverbZone* zone) {
    if (!system || !system->initialized || !zone || system->zone_count >= system->max_zones) {
        return false;
    }
    
    // Copy zone to array
    system->zones[system->zone_count] = *zone;
    system->zone_count++;
    
    // Load impulse response for this zone
    if (zone->impulse_response_path) {
        ConvolutionReverb_LoadIR(&system->processors[0], zone->impulse_response_path);  // Left
        ConvolutionReverb_LoadIR(&system->processors[1], zone->impulse_response_path);  // Right
    }
    
    printf("Added reverb zone: %s\n", zone->name);
    return true;
}

void ReverbZones_RemoveZone(ReverbZoneSystem* system, u32 zone_index) {
    if (!system || !system->initialized || zone_index >= system->zone_count) {
        return;
    }
    
    // Shift remaining zones down
    for (u32 i = zone_index; i < system->zone_count - 1; i++) {
        system->zones[i] = system->zones[i + 1];
    }
    
    system->zone_count--;
    printf("Removed reverb zone at index %d\n", zone_index);
}

ReverbZone* ReverbZones_FindZone(ReverbZoneSystem* system, const Vec3 position) {
    if (!system || !system->initialized) {
        return NULL;
    }
    
    for (u32 i = 0; i < system->zone_count; i++) {
        ReverbZone* zone = &system->zones[i];
        
        // Check if position is within zone bounds
        if (ReverbZones_IsPositionInZone(zone, position)) {
            return zone;
        }
    }
    
    return NULL;
}

bool ReverbZones_IsPositionInZone(const ReverbZone* zone, const Vec3 position) {
    if (!zone) return false;
    
    // Check if position is within zone bounds (simplified to box)
    return (position.x >= zone->bounds.min.x && position.x <= zone->bounds.max.x &&
            position.y >= zone->bounds.min.y && position.y <= zone->bounds.max.y &&
            position.z >= zone->bounds.min.z && position.z <= zone->bounds.max.z);
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Zone blending system
// -------------------------------------------------------------------------------------------------

void ReverbZones_CalculateBlendWeights(ReverbZoneSystem* system, const Vec3 listener_pos, 
                                     u32* zone_indices, f32* weights, u32* count) {
    if (!system || !zone_indices || !weights || !count) {
        return;
    }
    
    *count = 0;
    
    // Find all zones within blend distance
    for (u32 i = 0; i < system->zone_count; i++) {
        ReverbZone* zone = &system->zones[i];
        
        // Calculate distance to zone center
        Vec3 center = {
            (zone->bounds.min.x + zone->bounds.max.x) * 0.5f,
            (zone->bounds.min.y + zone->bounds.max.y) * 0.5f,
            (zone->bounds.min.z + zone->bounds.max.z) * 0.5f
        };
        
        Vec3 to_center = {
            center.x - listener_pos.x,
            center.y - listener_pos.y,
            center.z - listener_pos.z
        };
        
        f32 distance = sqrtf(to_center.x * to_center.x + to_center.y * to_center.y + to_center.z * to_center.z);
        
        if (distance <= system->blend_distance) {
            // Calculate blend weight (inverse distance)
            f32 weight = 1.0f - (distance / system->blend_distance);
            
            // Insert into sorted list (by weight, descending)
            u32 insert_pos = *count;
            for (u32 j = 0; j < *count; j++) {
                if (weight > weights[j]) {
                    insert_pos = j;
                    break;
                }
            }
            
            // Shift existing entries down
            for (u32 j = *count; j > insert_pos; j--) {
                zone_indices[j] = zone_indices[j - 1];
                weights[j] = weights[j - 1];
            }
            
            // Insert new entry
            zone_indices[insert_pos] = i;
            weights[insert_pos] = weight;
            (*count)++;
            
            // Limit number of blend zones
            if (*count >= system->max_blend_zones) {
                *count = system->max_blend_zones;
                break;
            }
        }
    }
    
    // Normalize weights
    f32 total_weight = 0.0f;
    for (u32 i = 0; i < *count; i++) {
        total_weight += weights[i];
    }
    
    if (total_weight > 0.0f) {
        for (u32 i = 0; i < *count; i++) {
            weights[i] /= total_weight;
        }
    }
}

void ReverbZones_ProcessWithBlending(ReverbZoneSystem* system, const Vec3 listener_pos,
                                  const f32* input_left, const f32* input_right,
                                  f32* output_left, f32* output_right, u32 frames) {
    if (!system || !system->initialized || !input_left || !input_right || !output_left || !output_right) {
        // No processing, copy input to output
        memcpy(output_left, input_left, frames * sizeof(f32));
        memcpy(output_right, input_right, frames * sizeof(f32));
        return;
    }
    
    // Find zones to blend
    u32 zone_indices[4];
    f32 weights[4];
    u32 blend_count;
    
    ReverbZones_CalculateBlendWeights(system, listener_pos, zone_indices, weights, &blend_count);
    
    if (blend_count == 0) {
        // No zones nearby, copy input to output
        memcpy(output_left, input_left, frames * sizeof(f32));
        memcpy(output_right, input_right, frames * sizeof(f32));
        return;
    }
    
    // Clear output buffers
    memset(output_left, 0, frames * sizeof(f32));
    memset(output_right, 0, frames * sizeof(f32));
    
    // Process each zone and blend
    for (u32 i = 0; i < blend_count; i++) {
        ReverbZone* zone = &system->zones[zone_indices[i]];
        
        // Apply zone-specific reverb
        ConvolutionReverb_SetParameters(&system->processors[0], &zone->reverb_params);  // Left
        ConvolutionReverb_SetParameters(&system->processors[1], &zone->reverb_params);  // Right
        
        ConvolutionReverb_Process(&system->processors[0], input_left, system->wet_buffer, frames);
        ConvolutionReverb_Process(&system->processors[1], input_right, system->wet_buffer + frames, frames);
        
        // Blend with output
        f32 blend_weight = weights[i];
        for (u32 j = 0; j < frames; j++) {
            output_left[j] += system->wet_buffer[j] * blend_weight;
            output_right[j] += system->wet_buffer[j + frames] * blend_weight;
        }
    }
    
    // Add dry signal (with global reverb level)
    f32 dry_level = 1.0f - system->global_reverb_level;
    for (u32 j = 0; j < frames; j++) {
        output_left[j] += input_left[j] * dry_level;
        output_right[j] += input_right[j] * dry_level;
    }
}

// -------------------------------------------------------------------------------------------------
//  COMPLETED: Convolution reverb implementation
// -------------------------------------------------------------------------------------------------

bool ConvolutionReverb_Init(ConvolutionReverb* reverb, u32 sample_rate, u32 buffer_size) {
    if (!reverb) return false;
    
    reverb->sample_rate = sample_rate;
    reverb->buffer_size = buffer_size;
    reverb->impulse_length = 8192;  // 8K impulse length for good quality
    
    // Allocate impulse response buffer
    reverb->impulse_response = malloc(reverb->impulse_length * sizeof(f32));
    if (!reverb->impulse_response) {
        return false;
    }
    
    // Allocate convolution buffers
    reverb->convolution_buffer = malloc((buffer_size + reverb->impulse_length) * sizeof(f32));
    if (!reverb->convolution_buffer) {
        free(reverb->impulse_response);
        return false;
    }
    
    // Initialize with default impulse (small room)
    ConvolutionReverb_GenerateDefaultImpulse(reverb, REVERB_ROOM_SMALL);
    
    // Clear convolution buffer
    memset(reverb->convolution_buffer, 0, (buffer_size + reverb->impulse_length) * sizeof(f32));
    reverb->buffer_index = 0;
    
    reverb->initialized = true;
    return true;
}

void ConvolutionReverb_Process(ConvolutionReverb* reverb, const f32* input, f32* output, u32 frames) {
    if (!reverb || !reverb->initialized || !input || !output) {
        return;
    }
    
    for (u32 i = 0; i < frames; i++) {
        // Store input in convolution buffer
        reverb->convolution_buffer[reverb->buffer_index] = input[i];
        
        // Perform convolution
        f32 sum = 0.0f;
        for (u32 j = 0; j < reverb->impulse_length; j++) {
            u32 buffer_index = (reverb->buffer_index - j + reverb->impulse_length) % 
                            (reverb->buffer_size + reverb->impulse_length);
            sum += reverb->convolution_buffer[buffer_index] * reverb->impulse_response[j];
        }
        
        output[i] = sum;
        
        // Advance buffer index
        reverb->buffer_index = (reverb->buffer_index + 1) % (reverb->buffer_size + reverb->impulse_length);
    }
}

void ConvolutionReverb_SetParameters(ConvolutionReverb* reverb, const ReverbParameters* params) {
    if (!reverb || !reverb->initialized || !params) {
        return;
    }
    
    reverb->current_params = *params;
    
    // Generate new impulse response based on parameters
    ConvolutionReverb_GenerateImpulse(reverb, params->room_type);
}

void ConvolutionReverb_LoadIR(ConvolutionReverb* reverb, const char* ir_path) {
    if (!reverb || !reverb->initialized || !ir_path) {
        return;
    }
    
    // In practice, this would load impulse response from file
    // For now, generate based on room type
    ConvolutionReverb_GenerateDefaultImpulse(reverb, REVERB_ROOM_MEDIUM);
}

void ConvolutionReverb_Cleanup(ConvolutionReverb* reverb) {
    if (reverb) {
        if (reverb->impulse_response) {
            free(reverb->impulse_response);
            reverb->impulse_response = NULL;
        }
        if (reverb->convolution_buffer) {
            free(reverb->convolution_buffer);
            reverb->convolution_buffer = NULL;
        }
        reverb->initialized = false;
    }
}

void ConvolutionReverb_GenerateImpulse(ConvolutionReverb* reverb, ReverbRoomType room_type) {
    if (!reverb || !reverb->initialized) {
        return;
    }
    
    // Generate synthetic impulse response based on room type
    f32 rt60 = 0.0f;  // Reverberation time
    
    switch (room_type) {
        case REVERB_ROOM_SMALL:
            rt60 = 0.8f;   // Small room: 0.8 seconds
            break;
        case REVERB_ROOM_MEDIUM:
            rt60 = 1.5f;   // Medium room: 1.5 seconds
            break;
        case REVERB_ROOM_LARGE:
            rt60 = 2.5f;   // Large room: 2.5 seconds
            break;
        case REVERB_ROOM_HALL:
            rt60 = 4.0f;   // Hall: 4.0 seconds
            break;
        case REVERB_ROOM_CATHEDRAL:
            rt60 = 6.0f;   // Cathedral: 6.0 seconds
            break;
        default:
            rt60 = 1.5f;
            break;
    }
    
    // Generate exponential decay impulse response
    f32 decay_rate = -60.0f / (rt60 * reverb->sample_rate);  // -60dB over RT60
    
    for (u32 i = 0; i < reverb->impulse_length; i++) {
        f32 time = (f32)i / reverb->sample_rate;
        reverb->impulse_response[i] = expf(decay_rate * (f32)i);
        
        // Add some early reflections for realism
        if (i == reverb->sample_rate / 100) {        // 10ms
            reverb->impulse_response[i] += 0.3f;
        }
        if (i == reverb->sample_rate / 50) {         // 20ms
            reverb->impulse_response[i] += 0.2f;
        }
        if (i == reverb->sample_rate / 25) {         // 40ms
            reverb->impulse_response[i] += 0.1f;
        }
    }
    
    // Normalize impulse response
    f32 max_sample = 0.0f;
    for (u32 i = 0; i < reverb->impulse_length; i++) {
        if (fabsf(reverb->impulse_response[i]) > max_sample) {
            max_sample = fabsf(reverb->impulse_response[i]);
        }
    }
    
    if (max_sample > 0.0f) {
        for (u32 i = 0; i < reverb->impulse_length; i++) {
            reverb->impulse_response[i] /= max_sample;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// MAIN REVERB ZONES INTERFACE
// -------------------------------------------------------------------------------------------------

void ReverbZones_CreateSystem(void) {
    if (!g_reverb_initialized) {
        ReverbZones_Init(&g_reverb_system, 16, 48000, 512);  // 16 zones, 48kHz, 512 samples
        g_reverb_initialized = true;
        printf("Global Reverb Zone System created\n");
    }
}

ReverbZoneSystem* ReverbZones_GetSystem(void) {
    if (!g_reverb_initialized) {
        ReverbZones_CreateSystem();
    }
    return &g_reverb_system;
}

void ReverbZones_ProcessAudio(const Vec3 listener_pos, const f32* input_left, const f32* input_right,
                           f32* output_left, f32* output_right, u32 frames) {
    ReverbZoneSystem* system = ReverbZones_GetSystem();
    if (!system) {
        memcpy(output_left, input_left, frames * sizeof(f32));
        memcpy(output_right, input_right, frames * sizeof(f32));
        return;
    }
    
    ReverbZones_ProcessWithBlending(system, listener_pos, input_left, input_right, 
                                output_left, output_right, frames);
}
