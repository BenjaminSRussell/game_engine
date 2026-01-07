#include "ocean_fft.h"
#include "phillips_spectrum.h"
#include "../../backend/metal/mtl_texture.h"
#include "../../backend/metal/mtl_buffer.h"
#include "../../backend/metal/mtl_pipeline.h"
#include "../../backend/metal/mtl_device.h"
#include "../../backend/metal/mtl_command.h"
#include "../../backend/metal/mtl_encoder.h"
#include <stdlib.h>
#include <string.h>
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

static metal_texture_t* create_texture_2d(
    metal_device_t* device,
    uint32_t width, 
    uint32_t height, 
    metal_pixel_format_t format,
    const char* label
) {
    metal_texture_desc_t desc = {0};
    desc.type = METAL_TEXTURE_TYPE_2D;
    desc.format = format;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.array_length = 1;
    desc.mip_levels = 1;
    desc.usage = METAL_TEXTURE_USAGE_SHADER_READ | METAL_TEXTURE_USAGE_SHADER_WRITE;
    desc.storage_mode = METAL_TEXTURE_STORAGE_PRIVATE;
    desc.generate_mipmaps = false;
    desc.label = label;
    
    return metal_texture_create(device, &desc);
}

static bool ocean_fft_create_textures(OceanFFTSystem* ocean, OceanFFTCascade* cascade, uint32_t resolution) {
    printf("[OceanFFT] Creating textures for cascade at resolution %u\n", resolution);
    
    metal_device_t* device = ocean->device;
    
    // Complex textures: RG32Float
    // h0(k) and h0*(-k) - generated on CPU initially, verify storage mode
    metal_texture_desc_t h0_desc = {0};
    h0_desc.type = METAL_TEXTURE_TYPE_2D;
    h0_desc.format = METAL_PIXEL_FORMAT_RG32_FLOAT;
    h0_desc.width = resolution;
    h0_desc.height = resolution;
    h0_desc.depth = 1;
    h0_desc.array_length = 1;
    h0_desc.mip_levels = 1;
    h0_desc.usage = METAL_TEXTURE_USAGE_SHADER_READ | METAL_TEXTURE_USAGE_SHADER_WRITE;
    h0_desc.storage_mode = METAL_TEXTURE_STORAGE_SHARED; // Need CPU access for init
    h0_desc.label = "h0_k";
    
    cascade->h0_texture = metal_texture_create(device, &h0_desc);
    
    h0_desc.label = "h0_conj_k";
    cascade->h0_conj_texture = metal_texture_create(device, &h0_desc);
    
    // Omega (dispersion): R32Float
    metal_texture_desc_t omega_desc = h0_desc;
    omega_desc.format = METAL_PIXEL_FORMAT_R32_FLOAT;
    omega_desc.label = "omega_k";
    cascade->omega_texture = metal_texture_create(device, &omega_desc);
    
    // Time-varying spectra: RG32Float
    cascade->ht_height = create_texture_2d(device, resolution, resolution, METAL_PIXEL_FORMAT_RG32_FLOAT, "ht_height");
    cascade->ht_displacement_x = create_texture_2d(device, resolution, resolution, METAL_PIXEL_FORMAT_RG32_FLOAT, "ht_disp_x");
    cascade->ht_displacement_z = create_texture_2d(device, resolution, resolution, METAL_PIXEL_FORMAT_RG32_FLOAT, "ht_disp_z");
    
    // FFT Ping-Pong buffer: RG32Float (only one needed if we reuse ht_* as dest)
    cascade->fft_ping = create_texture_2d(device, resolution, resolution, METAL_PIXEL_FORMAT_RG32_FLOAT, "fft_ping");
    cascade->fft_pong = create_texture_2d(device, resolution, resolution, METAL_PIXEL_FORMAT_RG32_FLOAT, "fft_pong");
    
    // Final Displacement: RGBA32Float (Dx, height, Dz, unused)
    cascade->displacement_map = create_texture_2d(device, resolution, resolution, METAL_PIXEL_FORMAT_RGBA32_FLOAT, "disp_map");
    
    // Normal Map: RGBA16Float (Nx, Ny, Nz, foam)
    cascade->normal_map = create_texture_2d(device, resolution, resolution, METAL_PIXEL_FORMAT_RGBA16_FLOAT, "normal_map");
    
    if (!cascade->h0_texture || !cascade->h0_conj_texture || !cascade->omega_texture ||
        !cascade->ht_height || !cascade->ht_displacement_x || !cascade->ht_displacement_z ||
        !cascade->fft_ping || !cascade->fft_pong ||
        !cascade->displacement_map || !cascade->normal_map) {
        return false;
    }
    
    return true;
}

static void ocean_fft_release_cascade_textures(OceanFFTCascade* cascade) {
    if (cascade->h0_texture) metal_texture_destroy(cascade->h0_texture);
    if (cascade->h0_conj_texture) metal_texture_destroy(cascade->h0_conj_texture);
    if (cascade->omega_texture) metal_texture_destroy(cascade->omega_texture);
    if (cascade->ht_height) metal_texture_destroy(cascade->ht_height);
    if (cascade->ht_displacement_x) metal_texture_destroy(cascade->ht_displacement_x);
    if (cascade->ht_displacement_z) metal_texture_destroy(cascade->ht_displacement_z);
    if (cascade->fft_ping) metal_texture_destroy(cascade->fft_ping);
    if (cascade->fft_pong) metal_texture_destroy(cascade->fft_pong);
    if (cascade->displacement_map) metal_texture_destroy(cascade->displacement_map);
    if (cascade->normal_map) metal_texture_destroy(cascade->normal_map);
    
    memset(cascade, 0, sizeof(OceanFFTCascade));
}

static metal_compute_pipeline_t* load_compute_pipeline(
    metal_device_t* device, 
    metal_shader_library_t* lib, 
    const char* func_name
) {
    MTLFunctionRef func = metal_get_function(lib, func_name);
    if (!func) {
        fprintf(stderr, "[OceanFFT] Error: Failed to find function '%s'\n", func_name);
        return NULL;
    }
    
    metal_compute_pipeline_desc_t desc = {0};
    desc.compute_function = func;
    
    return metal_create_compute_pipeline(device, &desc);
}

static bool ocean_fft_load_compute_pipelines(OceanFFTSystem* ocean) {
    printf("[OceanFFT] Loading compute pipelines from assets/shaders/ocean.metallib\n");
    
    metal_device_t* device = ocean->device;
    void* raw_device = metal_device_get_mtl_device(device);
    
    metal_shader_library_t* lib = metal_load_shader_library(raw_device, "assets/shaders/ocean.metallib");
    
    if (!lib) {
        fprintf(stderr, "[OceanFFT] Error: Failed to load ocean shader library\n");
        return false;
    }
    
    ocean->spectrum_update_pipeline = load_compute_pipeline(device, lib, "wave_spectrum_update");
    ocean->fft_horizontal_pipeline = load_compute_pipeline(device, lib, "fft_horizontal_full");
    ocean->fft_vertical_pipeline = load_compute_pipeline(device, lib, "fft_vertical_full");
    ocean->displacement_pipeline = load_compute_pipeline(device, lib, "wave_displacement_assembly");
    ocean->normal_pipeline = load_compute_pipeline(device, lib, "wave_normals_generate");
    ocean->foam_pipeline = load_compute_pipeline(device, lib, "wave_foam_generate");
    
    metal_destroy_shader_library(lib);
    
    if (!ocean->spectrum_update_pipeline || !ocean->fft_horizontal_pipeline || 
        !ocean->fft_vertical_pipeline || !ocean->displacement_pipeline ||
        !ocean->normal_pipeline || !ocean->foam_pipeline) {
        return false;
    }
    
    return true;
}

static bool ocean_fft_create_uniform_buffers(OceanFFTSystem* ocean) {
    printf("[OceanFFT] Creating uniform buffers\n");
    
    metal_device_t* device = ocean->device;
    
    metal_buffer_desc_t spectrum_desc = {0};
    spectrum_desc.size = sizeof(PhillipsSpectrumParams) + 64; 
    spectrum_desc.storage_mode = METAL_STORAGE_SHARED;
    spectrum_desc.usage = METAL_BUFFER_USAGE_UNIFORM;
    spectrum_desc.label = "OceanSpectrumParams";
    
    ocean->spectrum_params_buffer = metal_buffer_create(device, &spectrum_desc);
    
    metal_buffer_desc_t fft_desc = {0};
    fft_desc.size = sizeof(uint32_t) * 4; 
    fft_desc.storage_mode = METAL_STORAGE_SHARED;
    fft_desc.usage = METAL_BUFFER_USAGE_UNIFORM;
    fft_desc.label = "OceanFFTParams";
    
    ocean->fft_params_buffer = metal_buffer_create(device, &fft_desc);
    
    if (!ocean->spectrum_params_buffer || !ocean->fft_params_buffer) {
        return false;
    }
    
    return true;
}

// =============================================================================
// Ocean FFT Public API
// =============================================================================

OceanFFTSystem* ocean_fft_create(
    metal_device_t* device,
    mtl_command_queue_t queue,
    const OceanFFTConfig* config
) {
    if (!device || !queue || !config) {
        fprintf(stderr, "[OceanFFT] Error: Invalid parameters\n");
        return NULL;
    }
    
    if (config->fft_resolution < 64 || config->fft_resolution > 1024) {
        return NULL;
    }
    
    if (config->cascade_count < 1 || config->cascade_count > OCEAN_FFT_MAX_CASCADES) {
        return NULL;
    }
    
    uint32_t res = config->fft_resolution;
    if ((res & (res - 1)) != 0) {
        return NULL;
    }
    
    printf("[OceanFFT] Creating ocean system: %ux%u, %u cascades\n",
           config->fft_resolution, config->fft_resolution, config->cascade_count);
    
    OceanFFTSystem* ocean = (OceanFFTSystem*)calloc(1, sizeof(OceanFFTSystem));
    if (!ocean) return NULL;
    
    ocean->config = *config;
    ocean->spectrum_params = ocean_fft_default_spectrum();
    ocean->device = device;
    ocean->command_queue = queue;
    ocean->cascade_count = config->cascade_count;
    
    if (!ocean_fft_load_compute_pipelines(ocean)) {
        ocean_fft_destroy(ocean);
        return NULL;
    }
    
    if (!ocean_fft_create_uniform_buffers(ocean)) {
        ocean_fft_destroy(ocean);
        return NULL;
    }
    
    for (uint32_t i = 0; i < config->cascade_count; i++) {
        OceanFFTCascade* cascade = &ocean->cascades[i];
        
        cascade->tile_size = config->cascade_scales[i];
        cascade->blend_start = config->cascade_scales[i] * 0.7f;
        cascade->blend_end = config->cascade_scales[i] * 1.3f;
        
        if (!ocean_fft_create_textures(ocean, cascade, config->fft_resolution)) {
            ocean_fft_destroy(ocean);
            return NULL;
        }
        
        PhillipsSpectrumParams cascade_params = ocean->spectrum_params;
        cascade_params.amplitude *= (1.0f / (i + 1.0f));
        
        if (!phillips_spectrum_initialize(ocean, cascade, &cascade_params, config->fft_resolution)) {
            ocean_fft_destroy(ocean);
            return NULL;
        }
    }
    
    return ocean;
}

void ocean_fft_destroy(OceanFFTSystem* ocean) {
    if (!ocean) return;
    
    for (uint32_t i = 0; i < ocean->cascade_count; i++) {
        phillips_spectrum_release(&ocean->cascades[i]);
        ocean_fft_release_cascade_textures(&ocean->cascades[i]);
    }
    
    if (ocean->spectrum_params_buffer) metal_buffer_destroy(ocean->spectrum_params_buffer);
    if (ocean->fft_params_buffer) metal_buffer_destroy(ocean->fft_params_buffer);
    
    free(ocean);
}

void ocean_fft_set_spectrum_params(
    OceanFFTSystem* ocean,
    const PhillipsSpectrumParams* params
) {
    if (!ocean || !params) return;
    
    ocean->spectrum_params = *params;
    
    for (uint32_t i = 0; i < ocean->cascade_count; i++) {
        PhillipsSpectrumParams cascade_params = *params;
        cascade_params.amplitude *= (1.0f / (i + 1.0f));
        
        phillips_spectrum_update_params(ocean, &ocean->cascades[i], &cascade_params);
    }
}

// Dispatch helper
static void dispatch_fft_passes(
    OceanFFTSystem* ocean,
    mtl_compute_command_encoder_t encoder,
    OceanFFTCascade* cascade,
    metal_texture_t* input_texture, // Acts as output after vertical pass
    uint32_t resolution
) {
    // 1. Horizontal Pass: Input -> Ping
    metal_compute_encoder_set_compute_pipeline_state(encoder, ocean->fft_horizontal_pipeline->pipeline);
    metal_compute_encoder_set_texture(encoder, input_texture->texture, 0); // Source
    metal_compute_encoder_set_texture(encoder, cascade->fft_ping->texture, 1); // Dest
    metal_compute_encoder_set_buffer(encoder, ocean->fft_params_buffer->buffer, 0, 0);
    
    mtl_dispatch_threadgroups_args_t h_args;
    h_args.threadgroupsPerGrid[0] = 1; 
    h_args.threadgroupsPerGrid[1] = resolution; 
    h_args.threadgroupsPerGrid[2] = 1;
    h_args.threadsPerThreadgroup[0] = resolution / 2; // e.g. 128 threads for 256
    h_args.threadsPerThreadgroup[1] = 1;
    h_args.threadsPerThreadgroup[2] = 1;
    
    metal_compute_encoder_dispatch_threadgroups(encoder, h_args);
    
    // Barrier? (Implicit in new dispatch usually if different pipeline/resources? No, need barrier)
    // metal_compute_encoder_memory_barrier(encoder, cascade->fft_ping->texture, 1);
    
    // 2. Vertical Pass: Ping -> Input (Write back)
    metal_compute_encoder_set_compute_pipeline_state(encoder, ocean->fft_vertical_pipeline->pipeline);
    metal_compute_encoder_set_texture(encoder, cascade->fft_ping->texture, 0); // Source
    metal_compute_encoder_set_texture(encoder, input_texture->texture, 1); // Dest (Write back)
    
    mtl_dispatch_threadgroups_args_t v_args;
    v_args.threadgroupsPerGrid[0] = resolution; 
    v_args.threadgroupsPerGrid[1] = 1; 
    v_args.threadgroupsPerGrid[2] = 1;
    v_args.threadsPerThreadgroup[0] = 1;
    v_args.threadsPerThreadgroup[1] = resolution / 2;
    v_args.threadsPerThreadgroup[2] = 1;
    
    metal_compute_encoder_dispatch_threadgroups(encoder, v_args);
}

void ocean_fft_update(OceanFFTSystem* ocean, float delta_time) {
    if (!ocean) return;
    
    ocean->time += delta_time * ocean->config.time_scale;
    ocean->delta_time = delta_time;
    
    // Create command buffer
    mtl_command_buffer_t buffer = metal_create_command_buffer(ocean->device);
    
    // Create compute encoder
    mtl_compute_command_encoder_t encoder = metal_compute_command_encoder_create(buffer);
    
    // Update Uniforms
    ocean->spectrum_params.time = ocean->time; // Assuming we add time to params
    metal_buffer_update(ocean->spectrum_params_buffer, &ocean->spectrum_params, sizeof(PhillipsSpectrumParams), 0);
    
    uint32_t fft_params[4] = {ocean->config.fft_resolution, 0, 0, 0};
    metal_buffer_update(ocean->fft_params_buffer, fft_params, sizeof(fft_params), 0);
    
    uint32_t res = ocean->config.fft_resolution;
    
    // Common Dispatch Args for 2D kernels
    mtl_dispatch_threadgroups_args_t grid_args;
    grid_args.threadgroupsPerGrid[0] = (res + 15) / 16;
    grid_args.threadgroupsPerGrid[1] = (res + 15) / 16;
    grid_args.threadgroupsPerGrid[2] = 1;
    grid_args.threadsPerThreadgroup[0] = 16;
    grid_args.threadsPerThreadgroup[1] = 16;
    grid_args.threadsPerThreadgroup[2] = 1;
    
    for (uint32_t i = 0; i < ocean->cascade_count; i++) {
        OceanFFTCascade* cascade = &ocean->cascades[i];
        
        // 1. Spectrum Update (h(k,t))
        metal_compute_encoder_set_compute_pipeline_state(encoder, ocean->spectrum_update_pipeline->pipeline);
        metal_compute_encoder_set_texture(encoder, cascade->h0_texture->texture, 0);
        metal_compute_encoder_set_texture(encoder, cascade->h0_conj_texture->texture, 1);
        metal_compute_encoder_set_texture(encoder, cascade->omega_texture->texture, 2);
        metal_compute_encoder_set_texture(encoder, cascade->ht_height->texture, 3);
        metal_compute_encoder_set_texture(encoder, cascade->ht_displacement_x->texture, 4);
        metal_compute_encoder_set_texture(encoder, cascade->ht_displacement_z->texture, 5);
        metal_compute_encoder_set_buffer(encoder, ocean->spectrum_params_buffer->buffer, 0, 0);
        
        metal_compute_encoder_dispatch_threadgroups(encoder, grid_args);
        
        // 2. FFT Passes for Height, DispX, DispZ
        // Each dispatch_fft_passes does H-FFT then V-FFT and writes back to source texture
        dispatch_fft_passes(ocean, encoder, cascade, cascade->ht_height, res);
        dispatch_fft_passes(ocean, encoder, cascade, cascade->ht_displacement_x, res);
        dispatch_fft_passes(ocean, encoder, cascade, cascade->ht_displacement_z, res);
        
        // 3. Displacement Assembly
        metal_compute_encoder_set_compute_pipeline_state(encoder, ocean->displacement_pipeline->pipeline);
        metal_compute_encoder_set_texture(encoder, cascade->ht_displacement_x->texture, 0); // Corrected inputs
        metal_compute_encoder_set_texture(encoder, cascade->ht_height->texture, 1);
        metal_compute_encoder_set_texture(encoder, cascade->ht_displacement_z->texture, 2);
        metal_compute_encoder_set_texture(encoder, cascade->displacement_map->texture, 3);
        metal_compute_encoder_set_buffer(encoder, ocean->spectrum_params_buffer->buffer, 0, 0);
        
        metal_compute_encoder_dispatch_threadgroups(encoder, grid_args);
        
        // 4. Normal Generation
        if (ocean->config.enable_normals) {
            metal_compute_encoder_set_compute_pipeline_state(encoder, ocean->normal_pipeline->pipeline);
            metal_compute_encoder_set_texture(encoder, cascade->displacement_map->texture, 0);
            metal_compute_encoder_set_texture(encoder, cascade->normal_map->texture, 1);
            
            metal_compute_encoder_dispatch_threadgroups(encoder, grid_args);
        }
        
        // 5. Foam Generation
        if (ocean->config.enable_foam) {
            metal_compute_encoder_set_compute_pipeline_state(encoder, ocean->foam_pipeline->pipeline);
            metal_compute_encoder_set_texture(encoder, cascade->displacement_map->texture, 0);
            metal_compute_encoder_set_texture(encoder, cascade->normal_map->texture, 1); // Writes to alpha channel
            // Needs time delta for decay
             metal_compute_encoder_set_buffer(encoder, ocean->spectrum_params_buffer->buffer, 0, 0);
            
            metal_compute_encoder_dispatch_threadgroups(encoder, grid_args);
        }
    }
    
    metal_compute_encoder_end_encoding(encoder);
    metal_command_buffer_commit(buffer);
}

metal_texture_t* ocean_fft_get_displacement_texture(
    const OceanFFTSystem* ocean,
    uint32_t cascade_index
) {
    if (!ocean || cascade_index >= ocean->cascade_count) {
        return NULL;
    }
    
    return ocean->cascades[cascade_index].displacement_map;
}

metal_texture_t* ocean_fft_get_normal_texture(
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
