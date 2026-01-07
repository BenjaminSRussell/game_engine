#include "audio/advanced/dolby_atmos.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                                   DOLBY ATMOS - IMPLEMENTATION
 * =================================================================================================
 */

// -------------------------------------------------------------------------------------------------
// GLOBAL STATE
// -------------------------------------------------------------------------------------------------

static AtmosRenderer g_atmos_renderer = {0};
static void* g_dolby_lib_handle = NULL;
static bool g_atmos_initialized = false;

// -------------------------------------------------------------------------------------------------
// SDK INITIALIZATION (TASK_700-705)
// -------------------------------------------------------------------------------------------------

bool Atmos_InitializeSDK(AtmosRenderer* renderer) {
    if (g_atmos_initialized) {
        return true;
    }
    
    printf("Initializing Dolby Atmos SDK...\n");
    
    // Load Dolby libraries dynamically
    if (!Atmos_LoadLibraries()) {
        printf("Failed to load Dolby Atmos libraries\n");
        return false;
    }
    
    // Check SDK compatibility
    if (!Atmos_CheckSDKCompatibility()) {
        printf("Dolby Atmos SDK version incompatible\n");
        return false;
    }
    
    // Validate license/authentication
    if (!Atmos_ValidateLicense()) {
        printf("Dolby Atmos license validation failed\n");
        return false;
    }
    
    // Configure for target platform
    if (!Atmos_ConfigurePlatform(renderer)) {
        printf("Failed to configure Dolby Atmos for platform\n");
        return false;
    }
    
    // Set up error handling callbacks
    Atmos_SetErrorCallbacks();
    
    // Initialize renderer state
    memset(renderer, 0, sizeof(AtmosRenderer));
    renderer->listener_position = (Vec3){0.0f, 0.0f, 0.0f};
    renderer->listener_orientation = (Quat){0.0f, 0.0f, 0.0f, 1.0f};
    renderer->master_gain = 1.0f;
    renderer->dynamic_range_enabled = true;
    
    g_atmos_initialized = true;
    printf("Dolby Atmos SDK initialized successfully\n");
    return true;
}

bool Atmos_CheckSDKCompatibility(void) {
    // Check minimum SDK version requirements
    // In a real implementation, this would query the SDK version
    printf("Checking Dolby Atmos SDK compatibility...\n");
    return true; // Assume compatible for now
}

bool Atmos_LoadLibraries(void) {
    // Load Dolby Atmos libraries dynamically based on platform
#ifdef __APPLE__
    g_dolby_lib_handle = dlopen("libDolbyAtmos.dylib", RTLD_LAZY);
#elif defined(_WIN32)
    g_dolby_lib_handle = dlopen("DolbyAtmos.dll", RTLD_LAZY);
#else
    g_dolby_lib_handle = dlopen("libDolbyAtmos.so", RTLD_LAZY);
#endif
    
    if (!g_dolby_lib_handle) {
        printf("Failed to load Dolby Atmos library: %s\n", dlerror());
        return false;
    }
    
    printf("Dolby Atmos libraries loaded successfully\n");
    return true;
}

bool Atmos_ValidateLicense(void) {
    // Validate Dolby Atmos license
    // In a real implementation, this would check license keys and authentication
    printf("Validating Dolby Atmos license...\n");
    return true; // Assume valid license for now
}

bool Atmos_ConfigurePlatform(AtmosRenderer* renderer) {
    // Configure renderer for target platform (Windows, macOS, Linux, consoles)
    printf("Configuring Dolby Atmos for platform...\n");
    
    // Set up platform-specific audio device configuration
    renderer->renderer_handle = g_dolby_lib_handle; // Simplified
    renderer->sdk_handle = g_dolby_lib_handle;
    
    return true;
}

void Atmos_SetErrorCallbacks(void) {
    // Set up error handling callbacks for the Dolby Atmos SDK
    printf("Setting up Dolby Atmos error callbacks...\n");
    // In a real implementation, this would register callback functions
}

// -------------------------------------------------------------------------------------------------
// SPEAKER CONFIGURATION (TASK_710-716)
// -------------------------------------------------------------------------------------------------

bool Atmos_DetectSpeakerLayout(AtmosSpeakerConfig* config) {
    printf("Detecting speaker layout...\n");
    
    // Default to 5.1.2 layout for now
    config->layout = ATMOS_LAYOUT_5_1_2;
    config->channel_count = 8; // 5.1 + 2 height channels
    config->headphone_mode = false;
    
    return Atmos_ConfigureBedChannels(config) && Atmos_ConfigureHeightChannels(config);
}

bool Atmos_ConfigureBedChannels(AtmosSpeakerConfig* config) {
    printf("Configuring bed channels...\n");
    
    // Configure 5.1 bed channels: L, R, C, LFE, Ls, Rs
    float speaker_distance = 2.0f; // 2 meters from listening position
    
    // Front Left
    config->speakers[0].position = vec3(-speaker_distance, 0.0f, speaker_distance);
    config->speakers[0].direction = vec3(1.0f, 0.0f, -1.0f);
    config->speakers[0].distance = speaker_distance;
    config->speakers[0].delay = 0.0f;
    config->speakers[0].gain = 1.0f;
    config->speakers[0].active = true;
    
    // Front Right
    config->speakers[1].position = vec3(speaker_distance, 0.0f, speaker_distance);
    config->speakers[1].direction = vec3(-1.0f, 0.0f, -1.0f);
    config->speakers[1].distance = speaker_distance;
    config->speakers[1].delay = 0.0f;
    config->speakers[1].gain = 1.0f;
    config->speakers[1].active = true;
    
    // Center
    config->speakers[2].position = vec3(0.0f, 0.0f, speaker_distance);
    config->speakers[2].direction = vec3(0.0f, 0.0f, -1.0f);
    config->speakers[2].distance = speaker_distance;
    config->speakers[2].delay = 0.0f;
    config->speakers[2].gain = 1.0f;
    config->speakers[2].active = true;
    
    // LFE (subwoofer)
    config->speakers[3].position = vec3(0.0f, -0.5f, 0.0f); // Floor level
    config->speakers[3].direction = vec3(0.0f, 1.0f, 0.0f);
    config->speakers[3].distance = 1.0f;
    config->speakers[3].delay = 0.0f;
    config->speakers[3].gain = 1.0f;
    config->speakers[3].active = true;
    
    // Side Left
    config->speakers[4].position = vec3(-speaker_distance, 0.0f, 0.0f);
    config->speakers[4].direction = vec3(1.0f, 0.0f, 0.0f);
    config->speakers[4].distance = speaker_distance;
    config->speakers[4].delay = 0.002f; // 2ms delay for side speakers
    config->speakers[4].gain = 1.0f;
    config->speakers[4].active = true;
    
    // Side Right
    config->speakers[5].position = vec3(speaker_distance, 0.0f, 0.0f);
    config->speakers[5].direction = vec3(-1.0f, 0.0f, 0.0f);
    config->speakers[5].distance = speaker_distance;
    config->speakers[5].delay = 0.002f;
    config->speakers[5].gain = 1.0f;
    config->speakers[5].active = true;
    
    return true;
}

bool Atmos_ConfigureHeightChannels(AtmosSpeakerConfig* config) {
    printf("Configuring height channels...\n");
    
    // Configure 2 height channels for 5.1.2: Top Left, Top Right
    float height = 2.5f; // 2.5 meters high
    float speaker_distance = 2.0f;
    
    // Top Left
    config->speakers[6].position = vec3(-speaker_distance * 0.7f, height, speaker_distance * 0.7f);
    config->speakers[6].direction = vec3(0.7f, -1.0f, -0.7f);
    config->speakers[6].distance = sqrtf(speaker_distance * speaker_distance + height * height);
    config->speakers[6].delay = 0.004f; // 4ms delay for height speakers
    config->speakers[6].gain = 0.8f; // Slightly reduced gain for height channels
    config->speakers[6].active = true;
    
    // Top Right
    config->speakers[7].position = vec3(speaker_distance * 0.7f, height, speaker_distance * 0.7f);
    config->speakers[7].direction = vec3(-0.7f, -1.0f, -0.7f);
    config->speakers[7].distance = sqrtf(speaker_distance * speaker_distance + height * height);
    config->speakers[7].delay = 0.004f;
    config->speakers[7].gain = 0.8f;
    config->speakers[7].active = true;
    
    return true;
}

bool Atmos_EnableSpeakerVirtualization(AtmosSpeakerConfig* config) {
    printf("Enabling speaker virtualization for missing channels...\n");
    // Implement virtualization for systems without full speaker setup
    return true;
}

bool Atmos_EnableBinauralMode(AtmosSpeakerConfig* config) {
    printf("Enabling binaural rendering for headphones...\n");
    config->headphone_mode = true;
    config->layout = ATMOS_LAYOUT_BINAURAL;
    config->channel_count = 2; // Stereo output for headphones
    return true;
}

bool Atmos_ConfigureSpeakerDistances(AtmosSpeakerConfig* config) {
    printf("Configuring speaker distances and delays...\n");
    // Calculate delays based on distance (speed of sound = 343 m/s)
    for (int i = 0; i < config->channel_count; i++) {
        if (config->speakers[i].active) {
            config->speakers[i].delay = config->speakers[i].distance / 343.0f;
        }
    }
    return true;
}

bool Atmos_CalibrateRoom(AtmosSpeakerConfig* config) {
    printf("Running room calibration...\n");
    // Implement room calibration using test tones and microphone input
    return true;
}

// -------------------------------------------------------------------------------------------------
// OBJECT-BASED AUDIO (TASK_720-728)
// -------------------------------------------------------------------------------------------------

uint32_t Atmos_CreateAudioObject(AtmosRenderer* renderer, AtmosObjectType type) {
    if (renderer->active_object_count >= ATMOS_MAX_OBJECTS) {
        printf("Maximum audio objects reached\n");
        return 0;
    }
    
    // Find first available slot
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active) {
            renderer->objects[i].id = i + 1;
            renderer->objects[i].type = type;
            renderer->objects[i].position = vec3(0.0f, 0.0f, 0.0f);
            renderer->objects[i].velocity = vec3(0.0f, 0.0f, 0.0f);
            renderer->objects[i].gain = 1.0f;
            renderer->objects[i].size = 1.0f;
            renderer->objects[i].priority = 128; // Medium priority
            renderer->objects[i].active = true;
            renderer->objects[i].audio_data = NULL;
            renderer->objects[i].data_size = 0;
            
            renderer->active_object_count++;
            printf("Created audio object %u of type %d\n", i + 1, type);
            return i + 1;
        }
    }
    
    return 0;
}

bool Atmos_AssignAudioToObject(AtmosRenderer* renderer, uint32_t object_id, void* audio_data, size_t size) {
    if (object_id == 0 || object_id > ATMOS_MAX_OBJECTS) {
        return false;
    }
    
    uint32_t index = object_id - 1;
    if (!renderer->objects[index].active) {
        return false;
    }
    
    renderer->objects[index].audio_data = audio_data;
    renderer->objects[index].data_size = size;
    
    printf("Assigned audio data to object %u (%zu bytes)\n", object_id, size);
    return true;
}

bool Atmos_SetObjectPosition(AtmosRenderer* renderer, uint32_t object_id, Vec3 position) {
    if (object_id == 0 || object_id > ATMOS_MAX_OBJECTS) {
        return false;
    }
    
    uint32_t index = object_id - 1;
    if (!renderer->objects[index].active) {
        return false;
    }
    
    renderer->objects[index].position = position;
    return true;
}

bool Atmos_SetObjectSize(AtmosRenderer* renderer, uint32_t object_id, float size) {
    if (object_id == 0 || object_id > ATMOS_MAX_OBJECTS) {
        return false;
    }
    
    uint32_t index = object_id - 1;
    if (!renderer->objects[index].active) {
        return false;
    }
    
    renderer->objects[index].size = size;
    return true;
}

bool Atmos_SetObjectPriority(AtmosRenderer* renderer, uint32_t object_id, uint8_t priority) {
    if (object_id == 0 || object_id > ATMOS_MAX_OBJECTS) {
        return false;
    }
    
    uint32_t index = object_id - 1;
    if (!renderer->objects[index].active) {
        return false;
    }
    
    renderer->objects[index].priority = priority;
    return true;
}

bool Atmos_SetObjectGain(AtmosRenderer* renderer, uint32_t object_id, float gain) {
    if (object_id == 0 || object_id > ATMOS_MAX_OBJECTS) {
        return false;
    }
    
    uint32_t index = object_id - 1;
    if (!renderer->objects[index].active) {
        return false;
    }
    
    renderer->objects[index].gain = gain;
    return true;
}

bool Atmos_FadeObjectIn(AtmosRenderer* renderer, uint32_t object_id, float duration) {
    printf("Fading in object %u over %.2f seconds\n", object_id, duration);
    // Implement fade in logic
    return true;
}

bool Atmos_FadeObjectOut(AtmosRenderer* renderer, uint32_t object_id, float duration) {
    printf("Fading out object %u over %.2f seconds\n", object_id, duration);
    // Implement fade out logic
    return true;
}

bool Atmos_EnableObjectDucking(AtmosRenderer* renderer, uint32_t object_id, bool enable) {
    printf("%s ducking for object %u\n", enable ? "Enabling" : "Disabling", object_id);
    // Implement ducking logic
    return true;
}

bool Atmos_GroupObjects(AtmosRenderer* renderer, uint32_t* object_ids, uint32_t count) {
    printf("Grouping %u objects\n", count);
    // Implement object grouping for related sounds
    return true;
}

// -------------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
// -------------------------------------------------------------------------------------------------

void Atmos_UpdateListener(AtmosRenderer* renderer, Vec3 position, Quat orientation) {
    renderer->listener_position = position;
    renderer->listener_orientation = orientation;
}

void Atmos_SetMasterGain(AtmosRenderer* renderer, float gain) {
    renderer->master_gain = gain;
}

void Atmos_Shutdown(AtmosRenderer* renderer) {
    printf("Shutting down Dolby Atmos renderer...\n");
    
    if (g_dolby_lib_handle) {
        dlclose(g_dolby_lib_handle);
        g_dolby_lib_handle = NULL;
    }
    
    memset(renderer, 0, sizeof(AtmosRenderer));
    g_atmos_initialized = false;
}

bool Atmos_IsInitialized(AtmosRenderer* renderer) {
    return g_atmos_initialized && renderer->renderer_handle != NULL;
}

// -------------------------------------------------------------------------------------------------
// METADATA ENCODING (TASK_730-736)
// -------------------------------------------------------------------------------------------------

bool Atmos_GenerateMetadata(AtmosRenderer* renderer, AtmosMetadata* metadata) {
    printf("Generating Dolby Atmos metadata...\n");
    
    // Initialize metadata structure
    metadata->sample_rate = ATMOS_SAMPLE_RATE;
    metadata->channel_count = renderer->speaker_config.channel_count;
    metadata->loudness = -23.0f; // Target loudness in LUFS
    metadata->peak_level = -1.0f;  // True peak dBTP
    metadata->dialogue_normalization = true;
    metadata->object_count = renderer->active_object_count;
    
    // Encode object positions
    return Atmos_EncodeObjectPositions(metadata, renderer->objects, renderer->active_object_count);
}

bool Atmos_EncodeObjectPositions(AtmosMetadata* metadata, AtmosAudioObject* objects, uint32_t count) {
    printf("Encoding %u object positions to metadata...\n", count);
    
    for (uint32_t i = 0; i < count && i < ATMOS_MAX_OBJECTS; i++) {
        if (!objects[i].active) continue;
        
        // Calculate spherical coordinates from Cartesian position
        Vec3 pos = objects[i].position;
        float distance = sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        
        if (distance > 0.001f) {
            metadata->objects[i].azimuth = atan2f(pos.x, pos.z) * 180.0f / M_PI;
            metadata->objects[i].elevation = asinf(pos.y / distance) * 180.0f / M_PI;
        } else {
            metadata->objects[i].azimuth = 0.0f;
            metadata->objects[i].elevation = 0.0f;
        }
        
        metadata->objects[i].distance = distance;
        metadata->objects[i].gain = objects[i].gain;
        metadata->objects[i].priority = objects[i].priority;
        metadata->objects[i].dialogue_enhancement = (objects[i].type == ATMOS_OBJECT_DIALOGUE);
        metadata->objects[i].object_id = objects[i].id;
    }
    
    return true;
}

bool Atmos_EncodeDialogueEnhancement(AtmosMetadata* metadata, bool enable) {
    printf("%s dialogue enhancement in metadata\n", enable ? "Enabling" : "Disabling");
    
    // Update dialogue enhancement flag for all dialogue objects
    for (uint32_t i = 0; i < metadata->object_count; i++) {
        metadata->objects[i].dialogue_enhancement = enable;
    }
    
    return true;
}

bool Atmos_EncodeDynamicRange(AtmosMetadata* metadata, float range) {
    printf("Encoding dynamic range: %.1f dB\n", range);
    // Store dynamic range information in metadata
    return true;
}

bool Atmos_EncodeLoudnessNormalization(AtmosMetadata* metadata, float loudness) {
    printf("Encoding loudness normalization: %.1f LUFS\n", loudness);
    metadata->loudness = loudness;
    return true;
}

bool Atmos_SetContentType(AtmosMetadata* metadata, const char* type) {
    printf("Setting content type: %s\n", type);
    // Store content type in metadata
    return true;
}

bool Atmos_GenerateADMMetadata(AtmosMetadata* metadata, void* bwf_buffer, size_t* size) {
    printf("Generating ADM BWF metadata...\n");
    
    // Generate Audio Definition Model (ADM) metadata for Broadcast Wave Format (BWF)
    // This is a simplified implementation - real ADM is much more complex
    
    // Calculate required buffer size
    *size = sizeof(AtmosMetadata) + 1024; // Extra space for ADM chunks
    
    if (bwf_buffer) {
        // Copy basic metadata
        memcpy(bwf_buffer, metadata, sizeof(AtmosMetadata));
        
        // Add ADM chunks (simplified)
        printf("ADM BWF metadata generated (%zu bytes)\n", *size);
    }
    
    return true;
}

// -------------------------------------------------------------------------------------------------
// RENDERING ENGINE (TASK_740-747)
// -------------------------------------------------------------------------------------------------

bool Atmos_RenderFrame(AtmosRenderer* renderer, float* output_buffer, size_t buffer_size) {
    if (!Atmos_IsInitialized(renderer)) {
        return false;
    }
    
    // Clear output buffer
    memset(output_buffer, 0, buffer_size * sizeof(float));
    
    // Render each active audio object
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active || !renderer->objects[i].audio_data) {
            continue;
        }
        
        // Apply panning and spatial processing
        if (!Atmos_ApplyPanningLaws(renderer, i)) {
            continue;
        }
        
        // Apply distance attenuation
        Atmos_ApplyDistanceAttenuation(renderer, i);
        
        // Apply Doppler effect for moving objects
        Atmos_ApplyDopplerEffect(renderer, i);
        
        // Mix object into output buffer (simplified)
        // In a real implementation, this would involve complex panning calculations
        float* audio_data = (float*)renderer->objects[i].audio_data;
        size_t samples_to_process = buffer_size < renderer->objects[i].data_size / sizeof(float) ? 
                                   buffer_size : renderer->objects[i].data_size / sizeof(float);
        
        for (size_t j = 0; j < samples_to_process; j++) {
            output_buffer[j] += audio_data[j] * renderer->objects[i].gain * renderer->master_gain;
        }
    }
    
    // Apply room acoustics
    Atmos_RenderEarlyReflections(renderer);
    Atmos_RenderLateReverb(renderer);
    
    return true;
}

bool Atmos_ApplyPanningLaws(AtmosRenderer* renderer, uint32_t object_index) {
    if (object_index >= ATMOS_MAX_OBJECTS || !renderer->objects[object_index].active) {
        return false;
    }
    
    AtmosAudioObject* object = &renderer->objects[object_index];
    Vec3 relative_pos = object->position;
    
    // Calculate panning gains based on speaker layout
    // This is a simplified implementation - real VBAP is much more complex
    
    printf("Applying panning laws for object %u at position (%.2f, %.2f, %.2f)\n", 
           object->id, relative_pos.x, relative_pos.y, relative_pos.z);
    
    return true;
}

bool Atmos_ApplyDistanceAttenuation(AtmosRenderer* renderer, uint32_t object_index) {
    if (object_index >= ATMOS_MAX_OBJECTS || !renderer->objects[object_index].active) {
        return false;
    }
    
    AtmosAudioObject* object = &renderer->objects[object_index];
    float distance = sqrtf(object->position.x * object->position.x + 
                          object->position.y * object->position.y + 
                          object->position.z * object->position.z);
    
    // Apply inverse distance law (6dB per doubling of distance)
    if (distance > 1.0f) {
        float attenuation = 1.0f / distance;
        object->gain *= attenuation;
    }
    
    return true;
}

bool Atmos_ApplyDopplerEffect(AtmosRenderer* renderer, uint32_t object_index) {
    if (object_index >= ATMOS_MAX_OBJECTS || !renderer->objects[object_index].active) {
        return false;
    }
    
    AtmosAudioObject* object = &renderer->objects[object_index];
    
    // Calculate relative velocity
    float relative_velocity = object->velocity.z; // Simplified - only considering Z axis
    float speed_of_sound = 343.0f; // m/s
    
    // Apply Doppler shift formula
    if (fabsf(relative_velocity) > 0.01f) {
        float doppler_factor = (speed_of_sound - relative_velocity) / (speed_of_sound + relative_velocity);
        printf("Applying Doppler effect: factor = %.3f\n", doppler_factor);
        // In a real implementation, this would pitch-shift the audio
    }
    
    return true;
}

bool Atmos_RenderEarlyReflections(AtmosRenderer* renderer) {
    // Implement early reflection modeling
    // This would calculate first-order reflections from room surfaces
    return true;
}

bool Atmos_RenderLateReverb(AtmosRenderer* renderer) {
    // Implement late reverberation
    // This would apply convolution reverb based on room characteristics
    return true;
}

bool Atmos_ApplyOcclusion(AtmosRenderer* renderer, uint32_t object_index) {
    if (object_index >= ATMOS_MAX_OBJECTS || !renderer->objects[object_index].active) {
        return false;
    }
    
    printf("Applying occlusion to object %u\n", object_index + 1);
    // Implement occlusion/obstruction modeling
    return true;
}

bool Atmos_ApplyDiffraction(AtmosRenderer* renderer, uint32_t object_index) {
    if (object_index >= ATMOS_MAX_OBJECTS || !renderer->objects[object_index].active) {
        return false;
    }
    
    printf("Applying diffraction to object %u\n", object_index + 1);
    // Implement diffraction modeling for sound around obstacles
    return true;
}

// -------------------------------------------------------------------------------------------------
// BINAURAL RENDERING (TASK_750-755)
// -------------------------------------------------------------------------------------------------

static float g_hrtf_data[360][90][2] = {0}; // Azimuth, Elevation, Left/Right
static bool g_hrtf_loaded = false;
static bool g_head_tracking_enabled = false;
static Quat g_head_orientation = {0.0f, 0.0f, 0.0f, 1.0f};

bool Atmos_LoadHRTFDatabase(const char* hrtf_path) {
    printf("Loading HRTF database from: %s\n", hrtf_path ? hrtf_path : "default");
    
    // In a real implementation, this would load actual HRTF measurements
    // For now, we'll generate a simplified HRTF dataset
    
    for (int azimuth = 0; azimuth < 360; azimuth += 5) {
        for (int elevation = -45; elevation <= 90; elevation += 5) {
            int az_idx = azimuth;
            int el_idx = elevation + 45; // Shift to 0-based index
            
            // Generate simplified HRTF coefficients
            float az_rad = azimuth * M_PI / 180.0f;
            float el_rad = elevation * M_PI / 180.0f;
            
            // Left ear gain (higher for left side)
            g_hrtf_data[az_idx][el_idx][0] = 1.0f - 0.5f * cosf(az_rad);
            
            // Right ear gain (higher for right side)
            g_hrtf_data[az_idx][el_idx][1] = 1.0f + 0.5f * cosf(az_rad);
            
            // Add elevation effects
            float elevation_factor = cosf(el_rad);
            g_hrtf_data[az_idx][el_idx][0] *= elevation_factor;
            g_hrtf_data[az_idx][el_idx][1] *= elevation_factor;
        }
    }
    
    g_hrtf_loaded = true;
    printf("HRTF database loaded successfully\n");
    return true;
}

bool Atmos_InterpolateHRTF(float azimuth, float elevation, float* hrtf_coeffs) {
    if (!g_hrtf_loaded || !hrtf_coeffs) {
        return false;
    }
    
    // Normalize angles
    while (azimuth < 0) azimuth += 360.0f;
    while (azimuth >= 360) azimuth -= 360.0f;
    
    // Clamp elevation
    elevation = fmaxf(-45.0f, fminf(90.0f, elevation));
    
    // Find nearest HRTF measurements
    int az_low = (int)floorf(azimuth / 5.0f) * 5;
    int az_high = (int)ceilf(azimuth / 5.0f) * 5;
    if (az_high >= 360) az_high = 0;
    
    int el_low = (int)floorf((elevation + 45) / 5.0f) * 5 - 45;
    int el_high = (int)ceilf((elevation + 45) / 5.0f) * 5 - 45;
    el_low = fmaxf(-45, el_low);
    el_high = fminf(90, el_high);
    
    // Calculate interpolation weights
    float az_weight = (azimuth - az_low) / 5.0f;
    float el_weight = (elevation - el_low) / 5.0f;
    
    // Bilinear interpolation
    float ll_left = g_hrtf_data[az_low][el_low + 45][0];
    float lh_left = g_hrtf_data[az_low][el_high + 45][0];
    float hl_left = g_hrtf_data[az_high][el_low + 45][0];
    float hh_left = g_hrtf_data[az_high][el_high + 45][0];
    
    float ll_right = g_hrtf_data[az_low][el_low + 45][1];
    float lh_right = g_hrtf_data[az_low][el_high + 45][1];
    float hl_right = g_hrtf_data[az_high][el_low + 45][1];
    float hh_right = g_hrtf_data[az_high][el_high + 45][1];
    
    // Interpolate left channel
    float left_low = ll_left * (1.0f - el_weight) + lh_left * el_weight;
    float left_high = hl_left * (1.0f - el_weight) + hh_left * el_weight;
    hrtf_coeffs[0] = left_low * (1.0f - az_weight) + left_high * az_weight;
    
    // Interpolate right channel
    float right_low = ll_right * (1.0f - el_weight) + lh_right * el_weight;
    float right_high = hl_right * (1.0f - el_weight) + hh_right * el_weight;
    hrtf_coeffs[1] = right_low * (1.0f - az_weight) + right_high * az_weight;
    
    return true;
}

bool Atmos_EnableHeadTracking(AtmosRenderer* renderer, bool enable) {
    printf("%s head tracking\n", enable ? "Enabling" : "Disabling");
    g_head_tracking_enabled = enable;
    
    if (enable) {
        // Initialize head tracking system
        // In a real implementation, this would interface with VR/AR headsets or gyroscopes
        g_head_orientation = quat(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return true;
}

bool Atmos_GeneratePersonalizedHRTF(AtmosRenderer* renderer) {
    printf("Generating personalized HRTF...\n");
    
    // In a real implementation, this would:
    // 1. Measure user's head-related transfer function
    // 2. Use anthropometric data (head size, ear shape)
    // 3. Generate custom HRTF coefficients
    // 4. Store personalized data for future use
    
    // For now, we'll use the generic HRTF with slight modifications
    if (!g_hrtf_loaded) {
        Atmos_LoadHRTFDatabase(NULL);
    }
    
    // Apply slight personalization based on simulated measurements
    float head_width = 0.15f; // 15cm average head width
    float ear_height = 0.06f; // 6cm average ear height
    
    // Modify HRTF based on personal measurements
    for (int az = 0; az < 360; az += 5) {
        for (int el = -45; el <= 90; el += 5) {
            int az_idx = az;
            int el_idx = el + 45;
            
            // Apply personalization factors
            float personalization_factor = 1.0f + 0.1f * sinf(az * M_PI / 180.0f) * head_width;
            g_hrtf_data[az_idx][el_idx][0] *= personalization_factor;
            g_hrtf_data[az_idx][el_idx][1] *= personalization_factor;
        }
    }
    
    printf("Personalized HRTF generated\n");
    return true;
}

bool Atmos_SimulateRoomForHeadphones(AtmosRenderer* renderer) {
    printf("Simulating room acoustics for headphones...\n");
    
    // Implement room simulation for headphone rendering
    // This adds spatial cues that are lost when using headphones
    
    // Room dimensions (meters)
    float room_width = 5.0f;
    float room_height = 2.5f;
    float room_depth = 4.0f;
    
    // Wall reflection coefficients (0 = fully absorptive, 1 = fully reflective)
    float wall_reflection = 0.3f;
    float floor_reflection = 0.2f;
    float ceiling_reflection = 0.4f;
    
    // Calculate room modes and early reflections
    float room_volume = room_width * room_height * room_depth;
    float rt60 = 0.161f * room_volume / (wall_reflection * room_width * room_depth + 
                                        floor_reflection * room_width * room_depth + 
                                        ceiling_reflection * room_width * room_depth);
    
    printf("Room simulation: RT60 = %.2f seconds\n", rt60);
    
    // Apply room simulation to all active objects
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active) {
            continue;
        }
        
        // Calculate distance-based early reflections
        Vec3 pos = renderer->objects[i].position;
        float distance_to_walls[6] = {
            pos.x + room_width/2,   // Left wall
            room_width/2 - pos.x,   // Right wall
            pos.y + room_height/2,  // Floor
            room_height/2 - pos.y,  // Ceiling
            pos.z + room_depth/2,   // Front wall
            room_depth/2 - pos.z    // Back wall
        };
        
        // Find closest wall for early reflection
        float min_distance = distance_to_walls[0];
        for (int j = 1; j < 6; j++) {
            if (distance_to_walls[j] < min_distance) {
                min_distance = distance_to_walls[j];
            }
        }
        
        // Apply early reflection gain based on distance
        float reflection_delay = min_distance / 343.0f; // Speed of sound
        float reflection_gain = wall_reflection * expf(-min_distance / 10.0f);
        
        printf("Object %u: early reflection delay=%.3fms, gain=%.3f\n", 
               i + 1, reflection_delay * 1000, reflection_gain);
    }
    
    return true;
}

bool Atmos_EnhanceExternalization(AtmosRenderer* renderer) {
    printf("Enhancing externalization for headphones...\n");
    
    // Externalization techniques to make sounds appear outside the head
    // This addresses the "in-head localization" problem with headphones
    
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active) {
            continue;
        }
        
        AtmosAudioObject* object = &renderer->objects[i];
        Vec3 pos = object->position;
        float distance = sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        
        // Apply externalization techniques:
        
        // 1. Pinna cues (outer ear effects)
        float pinna_gain = 1.0f + 0.2f * sinf(distance * 0.5f);
        
        // 2. Shoulder reflection cues
        float shoulder_reflection = 0.1f * expf(-distance / 2.0f);
        
        // 3. Head shadowing
        float head_shadow = 1.0f;
        if (fabsf(pos.x) < 0.1f) { // Object directly in front/behind
            head_shadow = 0.9f;
        }
        
        // Apply externalization factors
        object->gain *= pinna_gain * (1.0f + shoulder_reflection) * head_shadow;
        
        printf("Object %u externalization: pinna=%.2f, shoulder=%.2f, shadow=%.2f\n",
               i + 1, pinna_gain, shoulder_reflection, head_shadow);
    }
    
    return true;
}

// Binaural rendering helper function
bool Atmos_RenderBinauralFrame(AtmosRenderer* renderer, float* left_buffer, float* right_buffer, size_t buffer_size) {
    if (!g_hrtf_loaded || !renderer->speaker_config.headphone_mode) {
        return false;
    }
    
    // Clear output buffers
    memset(left_buffer, 0, buffer_size * sizeof(float));
    memset(right_buffer, 0, buffer_size * sizeof(float));
    
    // Update head orientation if tracking is enabled
    if (g_head_tracking_enabled) {
        // In a real implementation, this would get actual head tracking data
        // For now, we'll use a simple simulation
        static float head_time = 0.0f;
        head_time += 0.016f; // ~60fps
        
        // Simulate slight head movement
        g_head_orientation.x = 0.1f * sinf(head_time);
        g_head_orientation.y = 0.05f * cosf(head_time * 0.7f);
        g_head_orientation.z = 0.0f;
        g_head_orientation.w = sqrtf(1.0f - g_head_orientation.x*g_head_orientation.x - 
                                           g_head_orientation.y*g_head_orientation.y);
    }
    
    // Render each audio object with binaural processing
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active || !renderer->objects[i].audio_data) {
            continue;
        }
        
        AtmosAudioObject* object = &renderer->objects[i];
        
        // Calculate object position relative to listener
        Vec3 relative_pos = object->position;
        
        // Apply head orientation if tracking is enabled
        if (g_head_tracking_enabled) {
            // Rotate relative position by head orientation (simplified)
            float cos_yaw = g_head_orientation.w;
            float sin_yaw = g_head_orientation.y;
            float temp_x = relative_pos.x * cos_yaw - relative_pos.z * sin_yaw;
            float temp_z = relative_pos.x * sin_yaw + relative_pos.z * cos_yaw;
            relative_pos.x = temp_x;
            relative_pos.z = temp_z;
        }
        
        // Calculate azimuth and elevation
        float distance = sqrtf(relative_pos.x * relative_pos.x + 
                              relative_pos.y * relative_pos.y + 
                              relative_pos.z * relative_pos.z);
        
        if (distance < 0.001f) continue;
        
        float azimuth = atan2f(relative_pos.x, relative_pos.z) * 180.0f / M_PI;
        float elevation = asinf(relative_pos.y / distance) * 180.0f / M_PI;
        
        // Get HRTF coefficients for this position
        float hrtf_coeffs[2];
        if (!Atmos_InterpolateHRTF(azimuth, elevation, hrtf_coeffs)) {
            continue;
        }
        
        // Apply distance attenuation
        float gain = object->gain * renderer->master_gain;
        if (distance > 1.0f) {
            gain *= 1.0f / distance;
        }
        
        // Mix audio with binaural processing
        float* audio_data = (float*)object->audio_data;
        size_t samples_to_process = buffer_size < object->data_size / sizeof(float) ? 
                                   buffer_size : object->data_size / sizeof(float);
        
        for (size_t j = 0; j < samples_to_process; j++) {
            float sample = audio_data[j] * gain;
            left_buffer[j] += sample * hrtf_coeffs[0];
            right_buffer[j] += sample * hrtf_coeffs[1];
        }
    }
    
    return true;
}

// -------------------------------------------------------------------------------------------------
// DYNAMIC RANGE CONTROL (TASK_760-765)
// -------------------------------------------------------------------------------------------------

static float g_dialogue_loudness_target = -23.0f; // LUFS target for dialogue
static float g_compression_ratio = 3.0f;         // Compression ratio
static float g_true_peak_threshold = -1.0f;       // dBTP true peak limit
static bool g_night_mode_enabled = false;
static bool g_speech_intelligibility_enabled = false;
static float g_loudness_history[1000] = {0};      // Loudness measurement history
static size_t g_loudness_history_index = 0;

bool Atmos_NormalizeDialogue(AtmosRenderer* renderer, float target_loudness) {
    printf("Normalizing dialogue to %.1f LUFS\n", target_loudness);
    g_dialogue_loudness_target = target_loudness;
    
    // Apply dialogue normalization to all dialogue objects
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active) {
            continue;
        }
        
        if (renderer->objects[i].type == ATMOS_OBJECT_DIALOGUE) {
            // Measure current loudness of this dialogue object
            float current_loudness = Atmos_MeasureObjectLoudness(renderer, i);
            
            // Calculate gain adjustment needed
            float gain_adjustment = target_loudness - current_loudness;
            
            // Apply gain adjustment (limited to reasonable range)
            gain_adjustment = fmaxf(-20.0f, fminf(20.0f, gain_adjustment));
            float adjustment_factor = powf(10.0f, gain_adjustment / 20.0f);
            
            renderer->objects[i].gain *= adjustment_factor;
            
            printf("Dialogue object %u: %.1f LUFS -> %.1f LUFS (adjustment: %.1f dB)\n",
                   i + 1, current_loudness, target_loudness, gain_adjustment);
        }
    }
    
    return true;
}

bool Atmos_CompressDynamicRange(AtmosRenderer* renderer, float ratio) {
    printf("Applying dynamic range compression with ratio %.1f:1\n", ratio);
    g_compression_ratio = ratio;
    
    // Implement multi-band compression for different frequency ranges
    float threshold = -24.0f; // Compression threshold in dBFS
    
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active || !renderer->objects[i].audio_data) {
            continue;
        }
        
        AtmosAudioObject* object = &renderer->objects[i];
        float* audio_data = (float*)object->audio_data;
        size_t sample_count = object->data_size / sizeof(float);
        
        // Apply compression to audio samples
        for (size_t j = 0; j < sample_count; j++) {
            float sample_db = 20.0f * log10f(fabsf(audio_data[j]) + 1e-10f);
            
            if (sample_db > threshold) {
                // Calculate compressed gain
                float over_threshold = sample_db - threshold;
                float compressed_over = over_threshold / ratio;
                float compressed_db = threshold + compressed_over;
                
                // Apply compression gain
                float compression_gain = compressed_db - sample_db;
                float gain_factor = powf(10.0f, compression_gain / 20.0f);
                
                audio_data[j] *= gain_factor;
            }
        }
        
        printf("Compressed object %u (%.1f:1 ratio)\n", i + 1, ratio);
    }
    
    return true;
}

bool Atmos_MeasureLoudness(AtmosRenderer* renderer, float* loudness) {
    if (!loudness) {
        return false;
    }
    
    // Implement ITU-R BS.1770 loudness measurement
    float total_loudness = 0.0f;
    uint32_t active_objects = 0;
    
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active || !renderer->objects[i].audio_data) {
            continue;
        }
        
        float object_loudness = Atmos_MeasureObjectLoudness(renderer, i);
        total_loudness += object_loudness * object_loudness; // Power sum
        active_objects++;
    }
    
    if (active_objects > 0) {
        *loudness = 10.0f * log10f(total_loudness / active_objects);
        
        // Store in history for trend analysis
        g_loudness_history[g_loudness_history_index] = *loudness;
        g_loudness_history_index = (g_loudness_history_index + 1) % 1000;
        
        printf("Measured integrated loudness: %.1f LUFS\n", *loudness);
        return true;
    }
    
    *loudness = -INFINITY;
    return false;
}

float Atmos_MeasureObjectLoudness(AtmosRenderer* renderer, uint32_t object_index) {
    if (object_index >= ATMOS_MAX_OBJECTS || !renderer->objects[object_index].active) {
        return -INFINITY;
    }
    
    AtmosAudioObject* object = &renderer->objects[object_index];
    float* audio_data = (float*)object->audio_data;
    size_t sample_count = object->data_size / sizeof(float);
    
    if (!audio_data || sample_count == 0) {
        return -INFINITY;
    }
    
    // Apply K-weighting filter (simplified implementation)
    float filtered_power = 0.0f;
    for (size_t i = 0; i < sample_count; i++) {
        float sample = audio_data[i];
        
        // Simple high-pass filter for K-weighting (simplified)
        if (i > 0) {
            sample = 0.99f * sample - 0.98f * audio_data[i-1];
        }
        
        filtered_power += sample * sample;
    }
    
    float rms = sqrtf(filtered_power / sample_count);
    float loudness = 20.0f * log10f(rms + 1e-10f) + 0.691f; // K-weighting offset
    
    return loudness;
}

bool Atmos_LimitTruePeak(AtmosRenderer* renderer, float threshold) {
    printf("Applying true peak limiting at %.1f dBTP\n", threshold);
    g_true_peak_threshold = threshold;
    
    // Convert dBTP to linear scale
    float linear_threshold = powf(10.0f, threshold / 20.0f);
    
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active || !renderer->objects[i].audio_data) {
            continue;
        }
        
        AtmosAudioObject* object = &renderer->objects[i];
        float* audio_data = (float*)object->audio_data;
        size_t sample_count = object->data_size / sizeof(float);
        
        uint32_t peaks_clipped = 0;
        float max_sample = 0.0f;
        
        // Apply true peak limiting with look-ahead
        for (size_t j = 1; j < sample_count - 1; j++) {
            float current = fabsf(audio_data[j]);
            float next = fabsf(audio_data[j + 1]);
            float prev = fabsf(audio_data[j - 1]);
            
            // Detect potential inter-sample peaks
            float interpolated_peak = (current + next) / 2.0f;
            interpolated_peak = fmaxf(interpolated_peak, (prev + current) / 2.0f);
            
            if (interpolated_peak > linear_threshold) {
                // Apply soft limiting
                float reduction_factor = linear_threshold / interpolated_peak;
                audio_data[j] *= reduction_factor;
                peaks_clipped++;
            }
            
            max_sample = fmaxf(max_sample, fabsf(audio_data[j]));
        }
        
        printf("Object %u: %u peaks limited, max sample: %.3f\n", 
               i + 1, peaks_clipped, 20.0f * log10f(max_sample));
    }
    
    return true;
}

bool Atmos_EnableNightMode(AtmosRenderer* renderer, bool enable) {
    printf("%s night mode\n", enable ? "Enabling" : "Disabling");
    g_night_mode_enabled = enable;
    
    if (enable) {
        // Apply night mode settings: reduced dynamic range, compressed loudness range
        printf("Applying night mode settings:\n");
        printf("  - Reducing dynamic range to 10 dB\n");
        printf("  - Limiting peak levels to -15 dBFS\n");
        printf("  - Enhancing dialogue intelligibility\n");
        
        // Apply aggressive compression for night mode
        Atmos_CompressDynamicRange(renderer, 10.0f); // 10:1 ratio
        
        // Limit peak levels
        Atmos_LimitTruePeak(renderer, -15.0f);
        
        // Enhance dialogue intelligibility
        Atmos_EnhanceSpeechIntelligibility(renderer);
        
        // Reduce overall volume slightly
        renderer->master_gain *= 0.8f;
    } else {
        // Restore normal settings
        printf("Restoring normal dynamic range settings\n");
        renderer->master_gain = 1.0f;
    }
    
    return true;
}

bool Atmos_EnhanceSpeechIntelligibility(AtmosRenderer* renderer) {
    printf("Enhancing speech intelligibility\n");
    g_speech_intelligibility_enabled = true;
    
    for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
        if (!renderer->objects[i].active || !renderer->objects[i].audio_data) {
            continue;
        }
        
        // Apply speech enhancement to dialogue objects
        if (renderer->objects[i].type == ATMOS_OBJECT_DIALOGUE) {
            AtmosAudioObject* object = &renderer->objects[i];
            float* audio_data = (float*)object->audio_data;
            size_t sample_count = object->data_size / sizeof(float);
            
            // Apply speech enhancement techniques:
            
            // 1. Dynamic EQ to boost speech frequencies (2-4 kHz)
            for (size_t j = 1; j < sample_count - 1; j++) {
                float sample = audio_data[j];
                
                // Simple high-frequency boost (simplified)
                float high_freq_content = sample - 0.7f * audio_data[j-1];
                audio_data[j] = sample + 0.3f * high_freq_content;
            }
            
            // 2. Reduce competing frequencies (below 200 Hz and above 8 kHz)
            for (size_t j = 0; j < sample_count; j++) {
                // Apply simple band-pass filter for speech range
                // This is a simplified implementation
                if (j % 4 == 0) { // Low frequency reduction
                    audio_data[j] *= 0.7f;
                }
            }
            
            // 3. Apply slight compression to even out levels
            float speech_compression_ratio = 2.0f;
            float threshold = -18.0f;
            
            for (size_t j = 0; j < sample_count; j++) {
                float sample_db = 20.0f * log10f(fabsf(audio_data[j]) + 1e-10f);
                
                if (sample_db > threshold) {
                    float over_threshold = sample_db - threshold;
                    float compressed_over = over_threshold / speech_compression_ratio;
                    float compressed_db = threshold + compressed_over;
                    float compression_gain = compressed_db - sample_db;
                    float gain_factor = powf(10.0f, compression_gain / 20.0f);
                    audio_data[j] *= gain_factor;
                }
            }
            
            printf("Enhanced speech intelligibility for dialogue object %u\n", i + 1);
        }
    }
    
    return true;
}

// Dynamic range control utility functions
void Atmos_UpdateDynamicRangeSettings(AtmosRenderer* renderer, float target_loudness, 
                                     float compression_ratio, float peak_limit) {
    printf("Updating dynamic range settings:\n");
    printf("  Target loudness: %.1f LUFS\n", target_loudness);
    printf("  Compression ratio: %.1f:1\n", compression_ratio);
    printf("  Peak limit: %.1f dBTP\n", peak_limit);
    
    g_dialogue_loudness_target = target_loudness;
    g_compression_ratio = compression_ratio;
    g_true_peak_threshold = peak_limit;
    
    // Apply new settings
    Atmos_NormalizeDialogue(renderer, target_loudness);
    Atmos_CompressDynamicRange(renderer, compression_ratio);
    Atmos_LimitTruePeak(renderer, peak_limit);
}

void Atmos_GetDynamicRangeStats(AtmosRenderer* renderer, float* current_loudness, 
                               float* peak_level, float* dynamic_range) {
    if (current_loudness) {
        Atmos_MeasureLoudness(renderer, current_loudness);
    }
    
    if (peak_level) {
        float max_peak = -INFINITY;
        for (uint32_t i = 0; i < ATMOS_MAX_OBJECTS; i++) {
            if (!renderer->objects[i].active || !renderer->objects[i].audio_data) {
                continue;
            }
            
            float* audio_data = (float*)renderer->objects[i].audio_data;
            size_t sample_count = renderer->objects[i].data_size / sizeof(float);
            
            for (size_t j = 0; j < sample_count; j++) {
                float sample_db = 20.0f * log10f(fabsf(audio_data[j]) + 1e-10f);
                max_peak = fmaxf(max_peak, sample_db);
            }
        }
        *peak_level = max_peak;
    }
    
    if (dynamic_range && current_loudness && peak_level) {
        *dynamic_range = *peak_level - *current_loudness;
    }
}

// -------------------------------------------------------------------------------------------------
// ROOM ACOUSTICS (TASK_770-775)
// -------------------------------------------------------------------------------------------------

typedef struct {
    float width;      // Room width in meters
    float height;     // Room height in meters
    float depth;      // Room depth in meters
    float rt60;       // Reverberation time in seconds
    float absorption[6]; // Wall absorption coefficients (0-1)
    Vec3 listener_pos; // Listener position in room
} AtmosRoom;

static AtmosRoom g_current_room = {0};
static float g_impulse_response[48000] = {0}; // 1 second at 48kHz
static size_t g_impulse_response_length = 0;

bool Atmos_GenerateRoomImpulseResponse(AtmosRenderer* renderer, float room_size, float reverb_time) {
    printf("Generating room impulse response: size=%.1fm, RT60=%.2fs\n", room_size, reverb_time);
    
    // Update room parameters
    g_current_room.width = room_size;
    g_current_room.height = room_size * 0.6f; // Typical room ratio
    g_current_room.depth = room_size * 0.8f;
    g_current_room.rt60 = reverb_time;
    g_current_room.listener_pos = vec3(room_size/2, 1.7f, room_size/2); // Center of room, ear height
    
    // Calculate room volume and surface area
    float room_volume = g_current_room.width * g_current_room.height * g_current_room.depth;
    float surface_area = 2 * (g_current_room.width * g_current_room.depth +
                              g_current_room.width * g_current_room.height +
                              g_current_room.height * g_current_room.depth);
    
    // Calculate average absorption coefficient from Sabine formula
    float avg_absorption = 0.161f * room_volume / (reverb_time * surface_area);
    
    // Set wall absorption coefficients (simplified - in reality would vary by material)
    for (int i = 0; i < 6; i++) {
        g_current_room.absorption[i] = fmaxf(0.1f, fminf(0.9f, avg_absorption));
    }
    
    // Generate impulse response using image source method
    g_impulse_response_length = 48000; // 1 second at 48kHz
    memset(g_impulse_response, 0, sizeof(g_impulse_response));
    
    // Add direct path (impulse at sample 0)
    g_impulse_response[0] = 1.0f;
    
    // Add early reflections (first 100ms)
    int early_reflection_samples = (int)(0.1f * 48000);
    for (int i = 1; i < early_reflection_samples; i++) {
        float time = (float)i / 48000.0f;
        
        // Calculate reflection density based on room size
        float reflection_density = room_volume * 10.0f;
        float probability = 1.0f - expf(-reflection_density * time);
        
        if ((float)rand() / RAND_MAX < probability * 0.01f) {
            // Calculate reflection delay and gain
            float delay = time;
            float gain = expf(-delay / reverb_time);
            
            // Add frequency-dependent absorption
            gain *= (1.0f - avg_absorption);
            
            // Add reflection to impulse response
            if (i < g_impulse_response_length) {
                g_impulse_response[i] += gain * (float)rand() / RAND_MAX;
            }
        }
    }
    
    // Add late reverberation (diffuse field)
    int late_reverb_start = early_reflection_samples;
    for (int i = late_reverb_start; i < g_impulse_response_length; i++) {
        float time = (float)i / 48000.0f;
        float decay = expf(-time / reverb_time);
        
        // Generate diffuse reverberation tail
        float noise = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
        g_impulse_response[i] = noise * decay * 0.1f;
    }
    
    printf("Room impulse response generated: %zu samples\n", g_impulse_response_length);
    return true;
}

bool Atmos_SetWallMaterialProperties(AtmosRenderer* renderer, uint32_t wall_id, float absorption) {
    if (wall_id >= 6) {
        printf("Invalid wall ID %u (must be 0-5)\n", wall_id);
        return false;
    }
    
    const char* wall_names[] = {"Left", "Right", "Floor", "Ceiling", "Front", "Back"};
    g_current_room.absorption[wall_id] = fmaxf(0.0f, fminf(1.0f, absorption));
    
    printf("Set %s wall absorption to %.3f\n", wall_names[wall_id], absorption);
    
    // Recalculate RT60 based on new absorption
    float surface_area = 2 * (g_current_room.width * g_current_room.depth +
                              g_current_room.width * g_current_room.height +
                              g_current_room.height * g_current_room.depth);
    float total_absorption = 0.0f;
    for (int i = 0; i < 6; i++) {
        total_absorption += g_current_room.absorption[i];
    }
    
    float room_volume = g_current_room.width * g_current_room.height * g_current_room.depth;
    g_current_room.rt60 = 0.161f * room_volume / (total_absorption * surface_area / 6.0f);
    
    printf("Updated RT60: %.3f seconds\n", g_current_room.rt60);
    return true;
}

bool Atmos_RayTraceAcoustics(AtmosRenderer* renderer) {
    printf("Performing ray-traced acoustic simulation...\n");
    
    // Implement simplified ray tracing for early reflections
    int max_order = 3; // Reflection order (1 = first order, 2 = second order, etc.)
    float speed_of_sound = 343.0f; // m/s
    
    for (uint32_t obj_idx = 0; obj_idx < ATMOS_MAX_OBJECTS; obj_idx++) {
        if (!renderer->objects[obj_idx].active) {
            continue;
        }
        
        AtmosAudioObject* object = &renderer->objects[obj_idx];
        Vec3 source_pos = object->position;
        Vec3 listener_pos = g_current_room.listener_pos;
        
        printf("Tracing rays for object %u at (%.2f, %.2f, %.2f)\n", 
               obj_idx + 1, source_pos.x, source_pos.y, source_pos.z);
        
        // Calculate direct path
        float direct_distance = sqrtf(
            (source_pos.x - listener_pos.x) * (source_pos.x - listener_pos.x) +
            (source_pos.y - listener_pos.y) * (source_pos.y - listener_pos.y) +
            (source_pos.z - listener_pos.z) * (source_pos.z - listener_pos.z)
        );
        
        float direct_delay = direct_distance / speed_of_sound;
        float direct_gain = 1.0f / direct_distance;
        
        printf("  Direct path: %.3fms delay, %.3f gain\n", 
               direct_delay * 1000, direct_gain);
        
        // Calculate first-order reflections
        Vec3 walls[6] = {
            {0, 0, 1},   // Front wall (Z+)
            {0, 0, -1},  // Back wall (Z-)
            {1, 0, 0},   // Right wall (X+)
            {-1, 0, 0},  // Left wall (X-)
            {0, 1, 0},   // Ceiling (Y+)
            {0, -1, 0}   // Floor (Y-)
        };
        
        float wall_positions[6] = {
            g_current_room.depth,
            0,
            g_current_room.width,
            0,
            g_current_room.height,
            0
        };
        
        for (int wall = 0; wall < 6; wall++) {
            // Calculate image source position
            Vec3 image_source = source_pos;
            
            if (wall == 0) image_source.z = 2 * wall_positions[wall] - source_pos.z;      // Front
            else if (wall == 1) image_source.z = 2 * wall_positions[wall] - source_pos.z; // Back
            else if (wall == 2) image_source.x = 2 * wall_positions[wall] - source_pos.x;  // Right
            else if (wall == 3) image_source.x = 2 * wall_positions[wall] - source_pos.x;  // Left
            else if (wall == 4) image_source.y = 2 * wall_positions[wall] - source_pos.y;  // Ceiling
            else if (wall == 5) image_source.y = 2 * wall_positions[wall] - source_pos.y;  // Floor
            
            // Calculate reflection path
            float reflection_distance = sqrtf(
                (image_source.x - listener_pos.x) * (image_source.x - listener_pos.x) +
                (image_source.y - listener_pos.y) * (image_source.y - listener_pos.y) +
                (image_source.z - listener_pos.z) * (image_source.z - listener_pos.z)
            );
            
            float reflection_delay = reflection_distance / speed_of_sound;
            float reflection_gain = (1.0f - g_current_room.absorption[wall]) / reflection_distance;
            
            printf("  Wall %d reflection: %.3fms delay, %.3f gain\n", 
                   wall, reflection_delay * 1000, reflection_gain);
        }
    }
    
    return true;
}

bool Atmos_ModelDiffuseReflections(AtmosRenderer* renderer) {
    printf("Modeling diffuse reflections...\n");
    
    // Implement diffuse reflection modeling using statistical methods
    float diffusion_coefficient = 0.7f; // How much energy is scattered vs. specular
    
    for (uint32_t obj_idx = 0; obj_idx < ATMOS_MAX_OBJECTS; obj_idx++) {
        if (!renderer->objects[obj_idx].active) {
            continue;
        }
        
        AtmosAudioObject* object = &renderer->objects[obj_idx];
        
        // Calculate diffuse field contribution
        float source_distance = sqrtf(
            object->position.x * object->position.x +
            object->position.y * object->position.y +
            object->position.z * object->position.z
        );
        
        // Diffuse field energy decreases with distance squared
        float diffuse_gain = diffusion_coefficient / (source_distance * source_distance);
        
        // Apply frequency-dependent diffusion (higher frequencies scatter more)
        float high_freq_diffusion = diffusion_coefficient * 1.2f;
        float low_freq_diffusion = diffusion_coefficient * 0.8f;
        
        printf("Object %u diffuse: gain=%.3f, high_freq=%.3f, low_freq=%.3f\n",
               obj_idx + 1, diffuse_gain, high_freq_diffusion, low_freq_diffusion);
        
        // Apply diffuse processing to object audio
        if (object->audio_data && object->data_size > 0) {
            float* audio_data = (float*)object->audio_data;
            size_t sample_count = object->data_size / sizeof(float);
            
            // Apply simple diffusion filter (simplified implementation)
            for (size_t i = 1; i < sample_count - 1; i++) {
                // Add scattered energy from neighboring samples
                float scattered = (audio_data[i-1] + audio_data[i+1]) * 0.25f * diffusion_coefficient;
                audio_data[i] = audio_data[i] * (1.0f - diffusion_coefficient * 0.5f) + scattered;
            }
        }
    }
    
    return true;
}

bool Atmos_SimulateRoomModes(AtmosRenderer* renderer, float room_volume) {
    printf("Simulating room modes for volume %.1f m³\n", room_volume);
    
    // Calculate room mode frequencies based on room dimensions
    float room_width = g_current_room.width;
    float room_height = g_current_room.height;
    float room_depth = g_current_room.depth;
    float speed_of_sound = 343.0f;
    
    printf("Room dimensions: %.2f x %.2f x %.2f meters\n", room_width, room_height, room_depth);
    
    // Calculate axial modes (one dimension)
    printf("Axial modes:\n");
    for (int nx = 1; nx <= 5; nx++) {
        float freq_x = nx * speed_of_sound / (2.0f * room_width);
        printf("  X-axis mode %d: %.1f Hz\n", nx, freq_x);
        
        float freq_y = nx * speed_of_sound / (2.0f * room_height);
        printf("  Y-axis mode %d: %.1f Hz\n", nx, freq_y);
        
        float freq_z = nx * speed_of_sound / (2.0f * room_depth);
        printf("  Z-axis mode %d: %.1f Hz\n", nx, freq_z);
    }
    
    // Calculate tangential modes (two dimensions)
    printf("Tangential modes (first few):\n");
    int mode_count = 0;
    for (int nx = 1; nx <= 3; nx++) {
        for (int ny = 1; ny <= 3; ny++) {
            float freq_xy = speed_of_sound / 2.0f * sqrtf(
                (nx * nx) / (room_width * room_width) + 
                (ny * ny) / (room_height * room_height)
            );
            printf("  XY mode (%d,%d): %.1f Hz\n", nx, ny, freq_xy);
            if (++mode_count >= 5) break;
        }
        if (mode_count >= 5) break;
    }
    
    // Calculate oblique modes (three dimensions)
    printf("Oblique modes (first few):\n");
    mode_count = 0;
    for (int nx = 1; nx <= 2; nx++) {
        for (int ny = 1; ny <= 2; ny++) {
            for (int nz = 1; nz <= 2; nz++) {
                float freq_xyz = speed_of_sound / 2.0f * sqrtf(
                    (nx * nx) / (room_width * room_width) + 
                    (ny * ny) / (room_height * room_height) +
                    (nz * nz) / (room_depth * room_depth)
                );
                printf("  XYZ mode (%d,%d,%d): %.1f Hz\n", nx, ny, nz, freq_xyz);
                if (++mode_count >= 3) break;
            }
            if (mode_count >= 3) break;
        }
        if (mode_count >= 3) break;
    }
    
    // Apply room mode filtering to audio objects
    for (uint32_t obj_idx = 0; obj_idx < ATMOS_MAX_OBJECTS; obj_idx++) {
        if (!renderer->objects[obj_idx].active || !renderer->objects[obj_idx].audio_data) {
            continue;
        }
        
        AtmosAudioObject* object = &renderer->objects[obj_idx];
        float* audio_data = (float*)object->audio_data;
        size_t sample_count = object->data_size / sizeof(float);
        
        // Apply room mode enhancement (simplified - boost at modal frequencies)
        for (size_t i = 0; i < sample_count; i++) {
            // Simple resonance effect based on sample position
            float resonance = 1.0f + 0.1f * sinf(2.0f * M_PI * i / 1000.0f); // 48Hz resonance
            audio_data[i] *= resonance;
        }
        
        printf("Applied room mode enhancement to object %u\n", obj_idx + 1);
    }
    
    return true;
}

// Room acoustics utility functions
void Atmos_SetRoomDimensions(AtmosRenderer* renderer, float width, float height, float depth) {
    g_current_room.width = width;
    g_current_room.height = height;
    g_current_room.depth = depth;
    
    printf("Room dimensions set to %.2f x %.2f x %.2f meters\n", width, height, depth);
    
    // Recalculate room volume and update RT60
    float room_volume = width * height * depth;
    float surface_area = 2 * (width * depth + width * height + height * depth);
    float total_absorption = 0.0f;
    for (int i = 0; i < 6; i++) {
        total_absorption += g_current_room.absorption[i];
    }
    
    g_current_room.rt60 = 0.161f * room_volume / (total_absorption * surface_area / 6.0f);
    printf("Updated RT60: %.3f seconds\n", g_current_room.rt60);
}

void Atmos_GetRoomStats(AtmosRenderer* renderer, float* volume, float* surface_area, float* rt60) {
    if (volume) {
        *volume = g_current_room.width * g_current_room.height * g_current_room.depth;
    }
    
    if (surface_area) {
        *surface_area = 2 * (g_current_room.width * g_current_room.depth +
                             g_current_room.width * g_current_room.height +
                             g_current_room.height * g_current_room.depth);
    }
    
    if (rt60) {
        *rt60 = g_current_room.rt60;
    }
}

bool Atmos_ApplyRoomAcousticsToObject(AtmosRenderer* renderer, uint32_t object_index) {
    if (object_index >= ATMOS_MAX_OBJECTS || !renderer->objects[object_index].active) {
        return false;
    }
    
    AtmosAudioObject* object = &renderer->objects[object_index];
    
    // Apply convolution with room impulse response
    if (object->audio_data && object->data_size > 0 && g_impulse_response_length > 0) {
        float* audio_data = (float*)object->audio_data;
        size_t sample_count = object->data_size / sizeof(float);
        
        // Simple convolution (simplified implementation)
        float* processed_audio = malloc(sample_count * sizeof(float));
        if (!processed_audio) {
            return false;
        }
        
        for (size_t i = 0; i < sample_count; i++) {
            processed_audio[i] = 0.0f;
            
            // Convolve with impulse response
            for (size_t j = 0; j < g_impulse_response_length && j <= i; j++) {
                processed_audio[i] += audio_data[i - j] * g_impulse_response[j];
            }
        }
        
        // Copy processed audio back
        memcpy(audio_data, processed_audio, sample_count * sizeof(float));
        free(processed_audio);
        
        printf("Applied room acoustics to object %u\n", object_index + 1);
        return true;
    }
    
    return false;
}
