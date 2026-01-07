#include "environment/ocean/ocean_fft.h"
#include "environment/ocean/phillips_spectrum.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>
#include <stdio.h>

// =============================================================================
// Default Configurations
// =============================================================================

OceanFFTConfig ocean_fft_default_config(void) {
    OceanFFTConfig config = {0};
    
    config.fft_resolution = OCEAN_FFT_DEFAULT_RESOLUTION;
    config.gravity = OCEAN_GRAVITY;
    config.time_scale = 1.0f;
    
    config.cascade_count = 3;
    config.cascade_scales[0] = 250.0f;   // Near detail (small waves)
    config.cascade_scales[1] = 1000.0f;  // Medium detail
    config.cascade_scales[2] = 4000.0f;  // Far detail (large swells)
    
    config.enable_foam = true;
    config.enable_normals = true;
    
    return config;
}

PhillipsSpectrumParams ocean_fft_default_spectrum(void) {
    PhillipsSpectrumParams params = {0};
    
    params.wind_speed = 20.0f;           // 20 m/s (~45 mph)
    params.wind_direction_x = 1.0f;      // Wind blowing +X
    params.wind_direction_z = 0.0f;
    
    params.amplitude = 1.0f;
    params.wave_suppression = 0.001f;    // Small wave cutoff
    params.fetch = 100000.0f;            // Large fetch (infinite ocean)
    params.choppy_factor = 1.5f;         // Moderate choppiness
    
    return params;
}

// =============================================================================
// Helper Functions
// =============================================================================

static bool ocean_fft_create_textures(OceanFFTSystem* ocean, OceanFFTCascade* cascade, uint32_t resolution) {
    // TODO: Implement texture creation using Metal API
    // For now, this is a placeholder that would call into your Metal backend
    
    // Required textures per cascade:
    // - h0_texture (RG32F, complex)
    // - h0_conj_texture (RG32F, complex)
    // - omega_texture (R32F, real)
    // - ht_height, ht_displacement_x, ht_displacement_z (RG32F, complex)
    // - fft_ping, fft_pong (RG32F, complex)
    // - displacement_map (RGB32F, real)
    // - normal_map (RGBA16F, real)
    
    printf("[OceanFFT] Creating textures for cascade at resolution %u\n", resolution);
    
    // Placeholder - actual implementation would call Metal texture creation
    cascade->h0_texture = NULL;
    cascade->h0_conj_texture = NULL;
    cascade->omega_texture = NULL;
    cascade->ht_height = NULL;
    cascade->ht_displacement_x = NULL;
    cascade->ht_displacement_z = NULL;
    cascade->fft_ping = NULL;
    cascade->fft_pong = NULL;
    cascade->displacement_map = NULL;
    cascade->normal_map = NULL;
    
    return true;  // Would return false on actual failure
}

static void ocean_fft_release_cascade_textures(OceanFFTCascade* cascade) {
    // TODO: Release Metal textures
    // For now, just null out pointers
    
    cascade->h0_texture = NULL;
    cascade->h0_conj_texture = NULL;
    cascade->omega_texture = NULL;
    cascade->ht_height = NULL;
    cascade->ht_displacement_x = NULL;
    cascade->ht_displacement_z = NULL;
    cascade->fft_ping = NULL;
    cascade->fft_pong = NULL;
    cascade->displacement_map = NULL;
    cascade->normal_map = NULL;
}

static bool ocean_fft_load_compute_pipelines(OceanFFTSystem* ocean) {
    // TODO: Load and compile Metal compute shaders
    // Shaders needed:
    // - spectrum_init_pipeline
    // - spectrum_update_pipeline
    // - fft_horizontal_pipeline
    // - fft_vertical_pipeline
    // - displacement_pipeline
    // - normal_pipeline
    // - foam_pipeline
    
    printf("[OceanFFT] Loading compute pipelines\n");
    
    // Placeholder - actual implementation would load .metallib
    ocean->spectrum_init_pipeline = NULL;
    ocean->spectrum_update_pipeline = NULL;
    ocean->fft_horizontal_pipeline = NULL;
    ocean->fft_vertical_pipeline = NULL;
    ocean->displacement_pipeline = NULL;
    ocean->normal_pipeline = NULL;
    ocean->foam_pipeline = NULL;
    
    return true;  // Would return false on actual failure
}

static bool ocean_fft_create_uniform_buffers(OceanFFTSystem* ocean) {
    // TODO: Create Metal uniform buffers
    // - spectrum_params_buffer (sizeof(PhillipsSpectrumParams))
    // - fft_params_buffer (resolution, stage info, etc.)
    
    printf("[OceanFFT] Creating uniform buffers\n");
    
    ocean->spectrum_params_buffer = NULL;
    ocean->fft_params_buffer = NULL;
    
    return true;  // Would return false on actual failure
}

// =============================================================================
// Ocean FFT Public API
// =============================================================================

OceanFFTSystem* ocean_fft_create(
    MTLDeviceRef device,
    MTLCommandQueueRef queue,
    const OceanFFTConfig* config
) {
    if (!device || !queue || !config) {
        fprintf(stderr, "[OceanFFT] Error: Invalid parameters\n");
        return NULL;
    }
    
    // Validate configuration
    if (config->fft_resolution < 64 || config->fft_resolution > 1024) {
        fprintf(stderr, "[OceanFFT] Error: Invalid FFT resolution %u (must be 64-1024)\n", 
                config->fft_resolution);
        return NULL;
    }
    
    if (config->cascade_count < 1 || config->cascade_count > OCEAN_FFT_MAX_CASCADES) {
        fprintf(stderr, "[OceanFFT] Error: Invalid cascade count %u (must be 1-%u)\n",
                config->cascade_count, OCEAN_FFT_MAX_CASCADES);
        return NULL;
    }
    
    // Check resolution is power of 2
    uint32_t res = config->fft_resolution;
    if ((res & (res - 1)) != 0) {
        fprintf(stderr, "[OceanFFT] Error: FFT resolution must be power of 2\n");
        return NULL;
    }
    
    printf("[OceanFFT] Creating ocean system: %ux%u, %u cascades\n",
           config->fft_resolution, config->fft_resolution, config->cascade_count);
    
    // Allocate system
    OceanFFTSystem* ocean = (OceanFFTSystem*)calloc(1, sizeof(OceanFFTSystem));
    if (!ocean) {
        fprintf(stderr, "[OceanFFT] Error: Failed to allocate ocean system\n");
        return NULL;
    }
    
    // Initialize configuration
    ocean->config = *config;
    ocean->spectrum_params = ocean_fft_default_spectrum();
    ocean->device = device;
    ocean->command_queue = queue;
    ocean->cascade_count = config->cascade_count;
    ocean->time = 0.0f;
    ocean->delta_time = 0.0f;
    ocean->last_update_time_ms = 0.0;
    
    // Load compute pipelines
    if (!ocean_fft_load_compute_pipelines(ocean)) {
        fprintf(stderr, "[OceanFFT] Error: Failed to load compute pipelines\n");
        ocean_fft_destroy(ocean);
        return NULL;
    }
    
    // Create uniform buffers
    if (!ocean_fft_create_uniform_buffers(ocean)) {
        fprintf(stderr, "[OceanFFT] Error: Failed to create uniform buffers\n");
        ocean_fft_destroy(ocean);
        return NULL;
    }
    
    // Initialize cascades
    for (uint32_t i = 0; i < config->cascade_count; i++) {
        OceanFFTCascade* cascade = &ocean->cascades[i];
        
        cascade->tile_size = config->cascade_scales[i];
        cascade->blend_start = config->cascade_scales[i] * 0.7f;
        cascade->blend_end = config->cascade_scales[i] * 1.3f;
        
        // Create textures for this cascade
        if (!ocean_fft_create_textures(ocean, cascade, config->fft_resolution)) {
            fprintf(stderr, "[OceanFFT] Error: Failed to create textures for cascade %u\n", i);
            ocean_fft_destroy(ocean);
            return NULL;
        }
        
        // Initialize Phillips spectrum for this cascade
        PhillipsSpectrumParams cascade_params = ocean->spectrum_params;
        // Scale amplitude by cascade to get appropriate detail levels
        cascade_params.amplitude *= (1.0f / (i + 1.0f));
        
        if (!phillips_spectrum_initialize(ocean, cascade, &cascade_params, config->fft_resolution)) {
            fprintf(stderr, "[OceanFFT] Error: Failed to initialize spectrum for cascade %u\n", i);
            ocean_fft_destroy(ocean);
            return NULL;
        }
        
        printf("[OceanFFT] Cascade %u: tile_size=%.1f, blend=[%.1f, %.1f]\n",
               i, cascade->tile_size, cascade->blend_start, cascade->blend_end);
    }
    
    printf("[OceanFFT] Ocean system created successfully\n");
    return ocean;
}

void ocean_fft_destroy(OceanFFTSystem* ocean) {
    if (!ocean) return;
    
    printf("[OceanFFT] Destroying ocean system\n");
    
    // Release cascade resources
    for (uint32_t i = 0; i < ocean->cascade_count; i++) {
        phillips_spectrum_release(&ocean->cascades[i]);
        ocean_fft_release_cascade_textures(&ocean->cascades[i]);
    }
    
    // Release compute pipelines
    // TODO: Actual Metal resource release
    
    // Release uniform buffers
    // TODO: Actual Metal resource release
    
    free(ocean);
}

void ocean_fft_set_spectrum_params(
    OceanFFTSystem* ocean,
    const PhillipsSpectrumParams* params
) {
    if (!ocean || !params) return;
    
    ocean->spectrum_params = *params;
    
    // Update all cascades with new parameters
    for (uint32_t i = 0; i < ocean->cascade_count; i++) {
        PhillipsSpectrumParams cascade_params = *params;
        // Scale amplitude by cascade
        cascade_params.amplitude *= (1.0f / (i + 1.0f));
        
        phillips_spectrum_update_params(ocean, &ocean->cascades[i], &cascade_params);
    }
}

void ocean_fft_update(OceanFFTSystem* ocean, float delta_time) {
    if (!ocean) return;
    
    // TODO: Implement full FFT update pipeline:
    // 1. Update time
    // 2. Dispatch spectrum_update_pipeline to compute h(k,t)
    // 3. Dispatch FFT horizontal passes
    // 4. Dispatch FFT vertical passes  
    // 5. Dispatch displacement assembly
    // 6. Dispatch normal generation
    // 7. Dispatch foam generation (if enabled)
    
    ocean->time += delta_time * ocean->config.time_scale;
    ocean->delta_time = delta_time;
    
    // Placeholder performance tracking
    ocean->last_update_time_ms = 0.0;  // Would measure actual GPU time
}

MTLTextureRef ocean_fft_get_displacement_texture(
    const OceanFFTSystem* ocean,
    uint32_t cascade_index
) {
    if (!ocean || cascade_index >= ocean->cascade_count) {
        return NULL;
    }
    
    return ocean->cascades[cascade_index].displacement_map;
}

MTLTextureRef ocean_fft_get_normal_texture(
    const OceanFFTSystem* ocean,
    uint32_t cascade_index
) {
    if (!ocean || cascade_index >= ocean->cascade_count) {
        return NULL;
    }
    
    return ocean->cascades[cascade_index].normal_map;
}

bool ocean_fft_get_cascade_info(
    const OceanFFTSystem* ocean,
    uint32_t cascade_index,
    float* out_tile_size,
    float* out_blend_start,
    float* out_blend_end
) {
    if (!ocean || cascade_index >= ocean->cascade_count) {
        return false;
    }
    
    const OceanFFTCascade* cascade = &ocean->cascades[cascade_index];
    
    if (out_tile_size) *out_tile_size = cascade->tile_size;
    if (out_blend_start) *out_blend_start = cascade->blend_start;
    if (out_blend_end) *out_blend_end = cascade->blend_end;
    
    return true;
}

float ocean_fft_get_time(const OceanFFTSystem* ocean) {
    return ocean ? ocean->time : 0.0f;
}

double ocean_fft_get_last_update_time_ms(const OceanFFTSystem* ocean) {
    return ocean ? ocean->last_update_time_ms : 0.0;
}
