/*
 * bloom_pipeline.c
 * Unified bloom post-processing pipeline implementation
 */

#include "postprocess/bloom_pipeline.h"
#include "postprocess/bloom/bloom_threshold.h"
#include "postprocess/bloom/bloom_downsample.h"
#include "postprocess/bloom/bloom_upsample.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct bloom_mip_level {
    float* buffer;
    uint32_t width;
    uint32_t height;
    size_t size_bytes;
} bloom_mip_level_t;

struct bloom_pipeline {
    // Settings
    bloom_settings_t settings;
    
    // Dimensions
    uint32_t width;
    uint32_t height;
    
    // Threshold buffer
    float* threshold_buffer;
    uint32_t threshold_width;
    uint32_t threshold_height;
    
    // Mip chain (downsampled levels)
    bloom_mip_level_t mip_chain[BLOOM_MAX_MIP_LEVELS];
    uint32_t active_mip_count;
    
    // Temporary upsample buffer
    float* upsample_buffer;
    
    // Component handles
    postprocessing_bloom_threshold_handle_t threshold_handle;
    postprocessing_bloom_downsample_handle_t downsample_handle;
    postprocessing_bloom_upsample_handle_t upsample_handle;
    
    // Stats
    size_t total_memory;
    float last_process_time_ms;
};

static bool g_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static uint32_t get_mip_count_for_quality(bloom_quality_t quality) {
    switch (quality) {
        case BLOOM_QUALITY_LOW:    return 3;
        case BLOOM_QUALITY_MEDIUM: return 5;
        case BLOOM_QUALITY_HIGH:   return 6;
        case BLOOM_QUALITY_ULTRA:  return 8;
        default:                   return 6;
    }
}

static void apply_color_shift(float* rgb, uint32_t pixel_count, 
                              float r_shift, float g_shift, float b_shift) {
    for (uint32_t i = 0; i < pixel_count; i++) {
        rgb[i * 3 + 0] *= r_shift;
        rgb[i * 3 + 1] *= g_shift;
        rgb[i * 3 + 2] *= b_shift;
    }
}

static void composite_bloom(const float* original, const float* bloom, 
                           float* output, uint32_t pixel_count,
                           float intensity) {
    for (uint32_t i = 0; i < pixel_count; i++) {
        output[i * 3 + 0] = original[i * 3 + 0] + bloom[i * 3 + 0] * intensity;
        output[i * 3 + 1] = original[i * 3 + 1] + bloom[i * 3 + 1] * intensity;
        output[i * 3 + 2] = original[i * 3 + 2] + bloom[i * 3 + 2] * intensity;
    }
}

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int bloom_pipeline_init(void) {
    if (g_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize component subsystems
    if (postprocessing_bloom_threshold_init() != 0) {
        return -1;
    }
    
    if (postprocessing_bloom_downsample_init() != 0) {
        postprocessing_bloom_threshold_shutdown();
        return -1;
    }
    
    if (postprocessing_bloom_upsample_init() != 0) {
        postprocessing_bloom_downsample_shutdown();
        postprocessing_bloom_threshold_shutdown();
        return -1;
    }
    
    g_initialized = true;
    return 0;
}

void bloom_pipeline_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    postprocessing_bloom_upsample_shutdown();
    postprocessing_bloom_downsample_shutdown();
    postprocessing_bloom_threshold_shutdown();
    
    g_initialized = false;
}

/* ============================================================================
 * LIFECYCLE
 * ============================================================================ */

bloom_settings_t bloom_pipeline_default_settings(void) {
    bloom_settings_t settings = {
        .threshold = 1.0f,
        .soft_knee = 0.5f,
        .intensity = 0.04f,
        .scatter = 0.7f,
        .color_shift_r = 1.0f,
        .color_shift_g = 1.0f,
        .color_shift_b = 1.0f,
        .quality = BLOOM_QUALITY_HIGH,
        .mip_count = 0, // Use quality preset
        .enable_anamorphic = false,
        .enable_lens_dirt = false,
        .enable_lens_flares = false,
        .dirt_intensity = 0.0f,
        .flare_intensity = 0.0f
    };
    return settings;
}

bloom_pipeline_t* bloom_pipeline_create(uint32_t width, uint32_t height, 
                                        const bloom_settings_t* settings) {
    if (!g_initialized) {
        return NULL;
    }
    
    bloom_pipeline_t* pipeline = (bloom_pipeline_t*)calloc(1, sizeof(bloom_pipeline_t));
    if (!pipeline) {
        return NULL;
    }
    
    pipeline->width = width;
    pipeline->height = height;
    
    // Apply settings
    if (settings) {
        pipeline->settings = *settings;
    } else {
        pipeline->settings = bloom_pipeline_default_settings();
    }
    
    // Determine mip count
    if (pipeline->settings.mip_count == 0) {
        pipeline->active_mip_count = get_mip_count_for_quality(pipeline->settings.quality);
    } else {
        pipeline->active_mip_count = pipeline->settings.mip_count;
        if (pipeline->active_mip_count > BLOOM_MAX_MIP_LEVELS) {
            pipeline->active_mip_count = BLOOM_MAX_MIP_LEVELS;
        }
    }
    
    // Allocate threshold buffer (full resolution)
    pipeline->threshold_width = width;
    pipeline->threshold_height = height;
    size_t threshold_size = width * height * 3 * sizeof(float);
    pipeline->threshold_buffer = (float*)malloc(threshold_size);
    if (!pipeline->threshold_buffer) {
        free(pipeline);
        return NULL;
    }
    pipeline->total_memory = threshold_size;
    
    // Allocate mip chain
    uint32_t mip_w = width / 2;
    uint32_t mip_h = height / 2;
    
    for (uint32_t i = 0; i < pipeline->active_mip_count; i++) {
        if (mip_w < 4 || mip_h < 4) {
            pipeline->active_mip_count = i;
            break;
        }
        
        pipeline->mip_chain[i].width = mip_w;
        pipeline->mip_chain[i].height = mip_h;
        pipeline->mip_chain[i].size_bytes = mip_w * mip_h * 3 * sizeof(float);
        pipeline->mip_chain[i].buffer = (float*)malloc(pipeline->mip_chain[i].size_bytes);
        
        if (!pipeline->mip_chain[i].buffer) {
            // Cleanup on allocation failure
            for (uint32_t j = 0; j < i; j++) {
                free(pipeline->mip_chain[j].buffer);
            }
            free(pipeline->threshold_buffer);
            free(pipeline);
            return NULL;
        }
        
        pipeline->total_memory += pipeline->mip_chain[i].size_bytes;
        
        mip_w /= 2;
        mip_h /= 2;
    }
    
    // Allocate upsample temp buffer (full resolution)
    pipeline->upsample_buffer = (float*)malloc(width * height * 3 * sizeof(float));
    if (!pipeline->upsample_buffer) {
        for (uint32_t i = 0; i < pipeline->active_mip_count; i++) {
            free(pipeline->mip_chain[i].buffer);
        }
        free(pipeline->threshold_buffer);
        free(pipeline);
        return NULL;
    }
    pipeline->total_memory += width * height * 3 * sizeof(float);
    
    // Create component handles
    postprocessing_bloom_threshold_desc_t threshold_desc = {
        .flags = 0,
        .initial_params = {
            .threshold = pipeline->settings.threshold,
            .knee = pipeline->settings.soft_knee
        },
        .user_data = NULL
    };
    
    if (postprocessing_bloom_threshold_create(&pipeline->threshold_handle, &threshold_desc) != 0) {
        bloom_pipeline_destroy(pipeline);
        return NULL;
    }
    
    postprocessing_bloom_downsample_desc_t downsample_desc = {
        .flags = 0,
        .user_data = NULL
    };
    
    if (postprocessing_bloom_downsample_create(&pipeline->downsample_handle, &downsample_desc) != 0) {
        postprocessing_bloom_threshold_destroy(pipeline->threshold_handle);
        bloom_pipeline_destroy(pipeline);
        return NULL;
    }
    
    bloom_upsample_params_t upsample_params = {
        .filter_radius = 0.005f * pipeline->settings.scatter
    };
    
    postprocessing_bloom_upsample_desc_t upsample_desc = {
        .flags = 0,
        .initial_params = upsample_params,
        .user_data = NULL
    };
    
    if (postprocessing_bloom_upsample_create(&pipeline->upsample_handle, &upsample_desc) != 0) {
        postprocessing_bloom_downsample_destroy(pipeline->downsample_handle);
        postprocessing_bloom_threshold_destroy(pipeline->threshold_handle);
        bloom_pipeline_destroy(pipeline);
        return NULL;
    }
    
    return pipeline;
}

void bloom_pipeline_destroy(bloom_pipeline_t* pipeline) {
    if (!pipeline) {
        return;
    }
    
    // Destroy component handles
    if (postprocessing_bloom_upsample_is_valid(pipeline->upsample_handle)) {
        postprocessing_bloom_upsample_destroy(pipeline->upsample_handle);
    }
    
    if (postprocessing_bloom_downsample_is_valid(pipeline->downsample_handle)) {
        postprocessing_bloom_downsample_destroy(pipeline->downsample_handle);
    }
    
    if (postprocessing_bloom_threshold_is_valid(pipeline->threshold_handle)) {
        postprocessing_bloom_threshold_destroy(pipeline->threshold_handle);
    }
    
    // Free buffers
    free(pipeline->upsample_buffer);
    free(pipeline->threshold_buffer);
    
    for (uint32_t i = 0; i < pipeline->active_mip_count; i++) {
        free(pipeline->mip_chain[i].buffer);
    }
    
    free(pipeline);
}

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

void bloom_pipeline_set_settings(bloom_pipeline_t* pipeline, const bloom_settings_t* settings) {
    if (!pipeline || !settings) {
        return;
    }
    
    pipeline->settings = *settings;
    
    // Update threshold params
    bloom_threshold_params_t threshold_params = {
        .threshold = settings->threshold,
        .knee = settings->soft_knee
    };
    postprocessing_bloom_threshold_set_params(pipeline->threshold_handle, &threshold_params);
    
    // Update upsample params
    bloom_upsample_params_t upsample_params = {
        .filter_radius = 0.005f * settings->scatter
    };
    postprocessing_bloom_upsample_set_params(pipeline->upsample_handle, &upsample_params);
}

void bloom_pipeline_get_settings(const bloom_pipeline_t* pipeline, bloom_settings_t* out_settings) {
    if (!pipeline || !out_settings) {
        return;
    }
    
    *out_settings = pipeline->settings;
}

/* ============================================================================
 * PROCESSING
 * ============================================================================ */

int bloom_pipeline_process(bloom_pipeline_t* pipeline, 
                          const float* input_hdr, 
                          float* output_hdr, 
                          uint32_t width, 
                          uint32_t height) {
    if (!pipeline || !input_hdr || !output_hdr) {
        return -1;
    }
    
    if (width != pipeline->width || height != pipeline->height) {
        return -1; // Size mismatch
    }
    
    // Step 1: Threshold extraction
    bloom_threshold_params_t threshold_params = {
        .threshold = pipeline->settings.threshold,
        .knee = pipeline->settings.soft_knee
    };
    
    uint32_t pixel_count = width * height;
    postprocessing_bloom_apply_threshold_buffer(&threshold_params, input_hdr, 
                                               pipeline->threshold_buffer, pixel_count);
    
    // Step 2: Downsample chain
    const float* downsample_input = pipeline->threshold_buffer;
    uint32_t downsample_input_w = pipeline->threshold_width;
    uint32_t downsample_input_h = pipeline->threshold_height;
    
    for (uint32_t i = 0; i < pipeline->active_mip_count; i++) {
        postprocessing_bloom_downsample_buffer(
            downsample_input, downsample_input_w, downsample_input_h,
            pipeline->mip_chain[i].buffer, pipeline->mip_chain[i].width, pipeline->mip_chain[i].height
        );
        
        downsample_input = pipeline->mip_chain[i].buffer;
        downsample_input_w = pipeline->mip_chain[i].width;
        downsample_input_h = pipeline->mip_chain[i].height;
    }
    
    // Step 3: Upsample chain with blending
    // Start from smallest mip and work up
    if (pipeline->active_mip_count > 0) {
        // Initialize with smallest mip
        uint32_t smallest_mip = pipeline->active_mip_count - 1;
        memcpy(pipeline->upsample_buffer, 
               pipeline->mip_chain[smallest_mip].buffer,
               pipeline->mip_chain[smallest_mip].size_bytes);
        
        bloom_upsample_params_t upsample_params = {
            .filter_radius = 0.005f * pipeline->settings.scatter
        };
        
        // Progressively upsample and blend
        for (int i = (int)smallest_mip - 1; i >= 0; i--) {
            uint32_t target_w = pipeline->mip_chain[i].width;
            uint32_t target_h = pipeline->mip_chain[i].height;
            
            // Allocate temp for upsampled result
            float* temp = (float*)malloc(target_w * target_h * 3 * sizeof(float));
            if (!temp) {
                return -1;
            }
            
            // Upsample current level
            uint32_t src_w = (i == (int)smallest_mip - 1) ? 
                           pipeline->mip_chain[smallest_mip].width :
                           pipeline->mip_chain[i + 1].width;
            uint32_t src_h = (i == (int)smallest_mip - 1) ?
                           pipeline->mip_chain[smallest_mip].height :
                           pipeline->mip_chain[i + 1].height;
            
            postprocessing_bloom_upsample_buffer(&upsample_params, 
                                                pipeline->upsample_buffer, src_w, src_h,
                                                temp, target_w, target_h);
            
            // Blend with current mip level
            for (uint32_t p = 0; p < target_w * target_h; p++) {
                temp[p * 3 + 0] += pipeline->mip_chain[i].buffer[p * 3 + 0];
                temp[p * 3 + 1] += pipeline->mip_chain[i].buffer[p * 3 + 1];
                temp[p * 3 + 2] += pipeline->mip_chain[i].buffer[p * 3 + 2];
            }
            
            memcpy(pipeline->upsample_buffer, temp, target_w * target_h * 3 * sizeof(float));
            free(temp);
        }
        
        // Final upsample to full resolution
        float* final_bloom = (float*)malloc(width * height * 3 * sizeof(float));
        if (!final_bloom) {
            return -1;
        }
        
        postprocessing_bloom_upsample_buffer(&upsample_params,
                                            pipeline->upsample_buffer, 
                                            pipeline->mip_chain[0].width,
                                            pipeline->mip_chain[0].height,
                                            final_bloom, width, height);
        
        // Step 4: Apply color shift
        if (pipeline->settings.color_shift_r != 1.0f ||
            pipeline->settings.color_shift_g != 1.0f ||
            pipeline->settings.color_shift_b != 1.0f) {
            apply_color_shift(final_bloom, pixel_count,
                            pipeline->settings.color_shift_r,
                            pipeline->settings.color_shift_g,
                            pipeline->settings.color_shift_b);
        }
        
        // Step 5: Composite with original
        composite_bloom(input_hdr, final_bloom, output_hdr, pixel_count, 
                       pipeline->settings.intensity);
        
        free(final_bloom);
    } else {
        // No mips, just copy input to output
        memcpy(output_hdr, input_hdr, pixel_count * 3 * sizeof(float));
    }
    
    return 0;
}

/* ============================================================================
 * DEBUG ACCESS
 * ============================================================================ */

const float* bloom_pipeline_get_threshold_buffer(const bloom_pipeline_t* pipeline, 
                                                 uint32_t* out_width, 
                                                 uint32_t* out_height) {
    if (!pipeline) {
        return NULL;
    }
    
    if (out_width) *out_width = pipeline->threshold_width;
    if (out_height) *out_height = pipeline->threshold_height;
    
    return pipeline->threshold_buffer;
}

const float* bloom_pipeline_get_mip_buffer(const bloom_pipeline_t* pipeline, 
                                          uint32_t mip_level,
                                          uint32_t* out_width, 
                                          uint32_t* out_height) {
    if (!pipeline || mip_level >= pipeline->active_mip_count) {
        return NULL;
    }
    
    if (out_width) *out_width = pipeline->mip_chain[mip_level].width;
    if (out_height) *out_height = pipeline->mip_chain[mip_level].height;
    
    return pipeline->mip_chain[mip_level].buffer;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

void bloom_pipeline_get_stats(const bloom_pipeline_t* pipeline, 
                             size_t* out_memory_used,
                             uint32_t* out_mip_count,
                             float* out_last_process_time_ms) {
    if (!pipeline) {
        return;
    }
    
    if (out_memory_used) *out_memory_used = pipeline->total_memory;
    if (out_mip_count) *out_mip_count = pipeline->active_mip_count;
    if (out_last_process_time_ms) *out_last_process_time_ms = pipeline->last_process_time_ms;
}
