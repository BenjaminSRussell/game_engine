/*
 * tonemapping_pipeline.c
 * Unified tonemapping post-processing pipeline implementation
 */

#include "postprocess/tonemapping_pipeline.h"
#include "postprocess/tonemapping/aces_tonemapper.h"
#include "postprocess/tonemapping/reinhard_tonemapper.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

struct tonemapping_pipeline {
    // Settings
    tonemapping_settings_t settings;
    
    // Component handles
    postprocessing_aces_tonemapper_handle_t aces_handle;
    postprocessing_reinhard_tonemapper_handle_t reinhard_handle;
    
    // LUT data (if enabled)
    float* lut_data;                  // 3D LUT texture data
    uint32_t lut_size;                // Size per dimension (e.g., 32 for 32x32x32)
    float lut_blend_factor;           // Blend factor for LUT transitions
    
    // Stats
    float avg_luminance;
    float current_exposure;
    float last_process_time_ms;
};

static bool g_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

// Filmic/Unreal tone mapping (Uncharted 2 formula)
static void tonemap_filmic_color(float r, float g, float b, float exposure,
                                 float* out_r, float* out_g, float* out_b) {
    // Apply exposure
    r *= exposure;
    g *= exposure;
    b *= exposure;
    
    // Uncharted 2 filmic curve
    const float A = 0.15f;  // Shoulder strength
    const float B = 0.50f;  // Linear strength
    const float C = 0.10f;  // Linear angle
    const float D = 0.20f;  // Toe strength
    const float E = 0.02f;  // Toe numerator
    const float F = 0.30f;  // Toe denominator
    
    #define FILMIC_CURVE(x) (((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F)
    
    float r_mapped = FILMIC_CURVE(r);
    float g_mapped = FILMIC_CURVE(g);
    float b_mapped = FILMIC_CURVE(b);
    
    // White point normalization
    const float W = 11.2f;
    float white_scale = 1.0f / FILMIC_CURVE(W);
    
    *out_r = r_mapped * white_scale;
    *out_g = g_mapped * white_scale;
    *out_b = b_mapped * white_scale;
    
    #undef FILMIC_CURVE
}

// Linear tone mapping (just exposure + clamp)
static void tonemap_linear_color(float r, float g, float b, float exposure,
                                 float* out_r, float* out_g, float* out_b) {
    *out_r = fminf(r * exposure, 1.0f);
    *out_g = fminf(g * exposure, 1.0f);
    *out_b = fminf(b * exposure, 1.0f);
}

// Apply contrast adjustment
static void adjust_contrast(float* r, float* g, float* b, float contrast) {
    const float mid = 0.5f;
    *r = mid + ((*r - mid) * contrast);
    *g = mid + ((*g - mid) * contrast);
    *b = mid + ((*b - mid) * contrast);
}

// Apply saturation adjustment
static void adjust_saturation(float* r, float* g, float* b, float saturation) {
    // Calculate luminance
    float lum = 0.2126f * (*r) + 0.7152f * (*g) + 0.0722f * (*b);
    
    // Interpolate between grayscale and original color
    *r = lum + (*r - lum) * saturation;
    *g = lum + (*g - lum) * saturation;
    *b = lum + (*b - lum) * saturation;
}

// Apply gamma correction 
static void apply_gamma(float* r, float* g, float* b, float gamma) {
    float inv_gamma = 1.0f / gamma;
    *r = powf(fmaxf(*r, 0.0f), inv_gamma);
    *g = powf(fmaxf(*g, 0.0f), inv_gamma);
    *b = powf(fmaxf(*b, 0.0f), inv_gamma);
}

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int tonemapping_pipeline_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    // Initialize component subsystems
    if (postprocessing_aces_tonemapper_init() != 0) {
        return -1;
    }
    
    if (postprocessing_reinhard_tonemapper_init() != 0) {
        postprocessing_aces_tonemapper_shutdown();
        return -1;
    }
    
    g_initialized = true;
    return 0;
}

void tonemapping_pipeline_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    postprocessing_reinhard_tonemapper_shutdown();
    postprocessing_aces_tonemapper_shutdown();
    
    g_initialized = false;
}

/* ============================================================================
 * LIFECYCLE
 * ============================================================================ */

tonemapping_settings_t tonemapping_pipeline_default_settings(void) {
    tonemapping_settings_t settings = {
        .operator_type = TONEMAP_OPERATOR_ACES,
        .exposure = 1.0f,
        .auto_exposure = false,
        .contrast = 1.0f,
        .saturation = 1.0f,
        .use_lut = false,
        .gamma = 2.2f
    };
    return settings;
}

tonemapping_pipeline_t* tonemapping_pipeline_create(const tonemapping_settings_t* settings) {
    if (!g_initialized) {
        return NULL;
    }
    
    tonemapping_pipeline_t* pipeline = (tonemapping_pipeline_t*)calloc(1, sizeof(tonemapping_pipeline_t));
    if (!pipeline) {
        return NULL;
    }
    
    // Apply settings
    if (settings) {
        pipeline->settings = *settings;
    } else {
        pipeline->settings = tonemapping_pipeline_default_settings();
    }
    
    // Create component handles
    aces_tonemap_params_t aces_params = {
        .exposure = pipeline->settings.exposure,
        .white_point = 11.2f,
        .saturation = pipeline->settings.saturation
    };
    
    postprocessing_aces_tonemapper_desc_t aces_desc = {
        .flags = 0,
        .initial_params = aces_params,
        .user_data = NULL
    };
    
    if (postprocessing_aces_tonemapper_create(&pipeline->aces_handle, &aces_desc) != 0) {
        free(pipeline);
        return NULL;
    }
    
    reinhard_tonemap_params_t reinhard_params = {
        .white_point = 1.0f,
        .exposure = pipeline->settings.exposure
    };
    
    postprocessing_reinhard_tonemapper_desc_t reinhard_desc = {
        .flags = 0,
        .initial_params = reinhard_params,
        .user_data = NULL
    };
    
    if (postprocessing_reinhard_tonemapper_create(&pipeline->reinhard_handle, &reinhard_desc) != 0) {
        postprocessing_aces_tonemapper_destroy(pipeline->aces_handle);
        free(pipeline);
        return NULL;
    }
    
    // Initialize stats
    pipeline->current_exposure = pipeline->settings.exposure;
    pipeline->avg_luminance = 0.18f;
    
    return pipeline;
}

void tonemapping_pipeline_destroy(tonemapping_pipeline_t* pipeline) {
    if (!pipeline) {
        return;
    }
    
    // Destroy component handles
    if (postprocessing_reinhard_tonemapper_is_valid(pipeline->reinhard_handle)) {
        postprocessing_reinhard_tonemapper_destroy(pipeline->reinhard_handle);
    }
    
    if (postprocessing_aces_tonemapper_is_valid(pipeline->aces_handle)) {
        postprocessing_aces_tonemapper_destroy(pipeline->aces_handle);
    }
    
    // Free LUT data
    if (pipeline->lut_data) {
        free(pipeline->lut_data);
    }
    
    free(pipeline);
}

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

void tonemapping_pipeline_set_settings(tonemapping_pipeline_t* pipeline, 
                                      const tonemapping_settings_t* settings) {
    if (!pipeline || !settings) {
        return;
    }
    
    pipeline->settings = *settings;
    pipeline->current_exposure = settings->exposure;
    
    // Update component params
    aces_tonemap_params_t aces_params = {
        .exposure = settings->exposure,
        .white_point = 11.2f,
        .saturation = settings->saturation
    };
    postprocessing_aces_tonemapper_set_params(pipeline->aces_handle, &aces_params);
    
    reinhard_tonemap_params_t reinhard_params = {
        .white_point = 1.0f,
        .exposure = settings->exposure
    };
    postprocessing_reinhard_tonemapper_set_params(pipeline->reinhard_handle, &reinhard_params);
}

void tonemapping_pipeline_get_settings(const tonemapping_pipeline_t* pipeline, 
                                      tonemapping_settings_t* out_settings) {
    if (!pipeline || !out_settings) {
        return;
    }
    
    *out_settings = pipeline->settings;
}

/* ============================================================================
 * LUT MANAGEMENT
 * ============================================================================ */

int tonemapping_pipeline_load_lut(tonemapping_pipeline_t* pipeline, const char* filepath) {
    if (!pipeline || !filepath) {
        return -1;
    }
    
    // TODO: Implement .cube LUT file loading
    // For now, just allocate identity LUT
    pipeline->lut_size = 32;
    size_t lut_elements = pipeline->lut_size * pipeline->lut_size * pipeline->lut_size * 3;
    
    if (pipeline->lut_data) {
        free(pipeline->lut_data);
    }
    
    pipeline->lut_data = (float*)malloc(lut_elements * sizeof(float));
    if (!pipeline->lut_data) {
        return -1;
    }
    
    // Create identity LUT
    for (uint32_t b = 0; b < pipeline->lut_size; b++) {
        for (uint32_t g = 0; g < pipeline->lut_size; g++) {
            for (uint32_t r = 0; r < pipeline->lut_size; r++) {
                uint32_t idx = (b * pipeline->lut_size * pipeline->lut_size + g * pipeline->lut_size + r) * 3;
                pipeline->lut_data[idx + 0] = (float)r / (float)(pipeline->lut_size - 1);
                pipeline->lut_data[idx + 1] = (float)g / (float)(pipeline->lut_size - 1);
                pipeline->lut_data[idx + 2] = (float)b / (float)(pipeline->lut_size - 1);
            }
        }
    }
    
    return 0;
}

void tonemapping_pipeline_set_lut_blend(tonemapping_pipeline_t* pipeline, float blend_factor) {
    if (!pipeline) {
        return;
    }
    
    pipeline->lut_blend_factor = fmaxf(0.0f, fminf(1.0f, blend_factor));
}

/* ============================================================================
 * PROCESSING
 * ============================================================================ */

int tonemapping_pipeline_process(tonemapping_pipeline_t* pipeline,
                                const float* input_hdr,
                                float* output_ldr,
                                uint32_t pixel_count) {
    if (!pipeline || !input_hdr || !output_ldr) {
        return -1;
    }
    
    float exposure = pipeline->current_exposure;
   
    // Process each pixel
    for (uint32_t i = 0; i < pixel_count; i++) {
        float r = input_hdr[i * 3 + 0];
        float g = input_hdr[i * 3 + 1];  
        float b = input_hdr[i * 3 + 2];
        
        // Apply tone mapping operator
        float r_out, g_out, b_out;
        
        switch (pipeline->settings.operator_type) {
            case TONEMAP_OPERATOR_ACES: {
                aces_tonemap_params_t params = {
                    .exposure = exposure,
                    .white_point = 11.2f,
                    .saturation = 1.0f  // Will apply saturation later
                };
                postprocessing_aces_tonemap_color(&params, r, g, b, &r_out, &g_out, &b_out);
                break;
            }
            
            case TONEMAP_OPERATOR_REINHARD: {
                reinhard_tonemap_params_t params = {
                    .white_point = 1.0f,
                    .exposure = exposure
                };
                postprocessing_reinhard_tonemap_color(&params, r, g, b, &r_out, &g_out, &b_out);
                break;
            }
            
            case TONEMAP_OPERATOR_FILMIC:
                tonemap_filmic_color(r, g, b, exposure, &r_out, &g_out, &b_out);
                break;
            
            case TONEMAP_OPERATOR_LINEAR:
            default:
                tonemap_linear_color(r, g, b, exposure, &r_out, &g_out, &b_out);
                break;
        }
        
        // Apply contrast
        if (pipeline->settings.contrast != 1.0f) {
            adjust_contrast(&r_out, &g_out, &b_out, pipeline->settings.contrast);
        }
        
        // Apply saturation 
        if (pipeline->settings.saturation != 1.0f) {
            adjust_saturation(&r_out, &g_out, &b_out, pipeline->settings.saturation);
        }
        
        // TODO: Apply LUT if enabled
        // if (pipeline->settings.use_lut && pipeline->lut_data) {
        //     apply_lut_3d(&r_out, &g_out, &b_out, pipeline->lut_data, pipeline->lut_size);
        // }
        
        // Apply gamma correction
        if (pipeline->settings.gamma != 1.0f) {
            apply_gamma(&r_out, &g_out, &b_out, pipeline->settings.gamma);
        }
        
        // Clamp to [0, 1]
        output_ldr[i * 3 + 0] = fmaxf(0.0f, fminf(1.0f, r_out));
        output_ldr[i * 3 + 1] = fmaxf(0.0f, fminf(1.0f, g_out));
        output_ldr[i * 3 + 2] = fmaxf(0.0f, fminf(1.0f, b_out));
    }
    
    return 0;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

void tonemapping_pipeline_get_stats(const tonemapping_pipeline_t* pipeline,
                                   float* out_avg_luminance,
                                   float* out_current_exposure,
                                   float* out_last_process_time_ms) {
    if (!pipeline) {
        return;
    }
    
    if (out_avg_luminance) *out_avg_luminance = pipeline->avg_luminance;
    if (out_current_exposure) *out_current_exposure = pipeline->current_exposure;
    if (out_last_process_time_ms) *out_last_process_time_ms = pipeline->last_process_time_ms;
}

/* ============================================================================
 * UTILITY
 * ============================================================================ */

float tonemapping_ev_to_linear(float ev) {
    return powf(2.0f, ev);
}

float tonemapping_linear_to_ev(float linear) {
    return log2f(fmaxf(linear, 1e-6f));
}
