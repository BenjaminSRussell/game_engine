// src/engine/renderer/post_processing/enhanced_taa_bloom.c
//
// Purpose: Enhanced TAA and bloom post-processing system with compute shaders
// Provides high-quality temporal anti-aliasing and advanced bloom effects

#include "renderer/post_processing/enhanced_taa_bloom.h"
#include "core/logger.h"
#include "core/memory/unified_memory_allocator.h"
#include "renderer/vulkan/vulkan_renderer.h"
#include "renderer/metal/metal_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// INTERNAL STRUCTURES AND STATE
// ============================================================================

typedef struct {
    // TAA resources
    RenderTexture* currentFrameTexture;
    RenderTexture* historyFrameTexture;
    RenderTexture* velocityBuffer;
    RenderTexture* depthBuffer;
    RenderTexture* exposureBuffer;
    RenderTexture* roughnessBuffer;
    RenderTexture* taaOutputTexture;
    
    // Bloom resources
    RenderTexture* thresholdTexture;
    RenderTexture* blurTextures[4]; // Multiple blur passes
    RenderTexture* bloomCompositeTexture;
    
    // Lens effects
    Texture* lensDirtTexture;
    Texture* lensFlareTexture;
    Texture* vignetteTexture;
    
    // Compute pipelines
    ComputePipeline* taaPipeline;
    ComputePipeline* bloomThresholdPipeline;
    ComputePipeline* bloomBlurPipeline;
    ComputePipeline* bloomCompositePipeline;
    
    // Configuration
    TAAConfig taaConfig;
    BloomConfig bloomConfig;
    
    // Performance metrics
    u64 lastFrameTime;
    f32 averageFrameTime;
    u32 frameCount;
    
    bool initialized;
} EnhancedPostProcessingState;

static EnhancedPostProcessingState g_postProcessState = {0};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static inline f32 lerp(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

static inline vec3 lerp_vec3(vec3 a, vec3 b, f32 t) {
    return (vec3){
        .x = lerp(a.x, b.x, t),
        .y = lerp(a.y, b.y, t),
        .z = lerp(a.z, b.z, t)
    };
}

static inline f32 clamp_f32(f32 value, f32 min, f32 max) {
    return (value < min) ? min : (value > max) ? max : value;
}

static inline vec3 clamp_vec3(vec3 value, f32 min, f32 max) {
    return (vec3){
        .x = clamp_f32(value.x, min, max),
        .y = clamp_f32(value.y, min, max),
        .z = clamp_f32(value.z, min, max)
    };
}

static vec3 rgb_to_hsv(vec3 rgb) {
    f32 max_val = fmaxf(fmaxf(rgb.x, rgb.y), rgb.z);
    f32 min_val = fminf(fminf(rgb.x, rgb.y), rgb.z);
    f32 delta = max_val - min_val;
    
    vec3 hsv = {0};
    hsv.z = max_val;
    
    if (delta > 0.00001f) {
        hsv.y = delta / max_val;
        
        if (max_val == rgb.x) {
            hsv.x = fmodf((rgb.y - rgb.z) / delta, 6.0f);
        } else if (max_val == rgb.y) {
            hsv.x = (rgb.z - rgb.x) / delta + 2.0f;
        } else {
            hsv.x = (rgb.x - rgb.y) / delta + 4.0f;
        }
        
        hsv.x = hsv.x / 6.0f;
        if (hsv.x < 0.0f) hsv.x += 1.0f;
    }
    
    return hsv;
}

static vec3 hsv_to_rgb(vec3 hsv) {
    f32 c = hsv.z * hsv.y;
    f32 x = c * (1.0f - fabsf(fmodf(hsv.x * 6.0f, 2.0f) - 1.0f));
    f32 m = hsv.z - c;
    
    vec3 rgb = {0};
    
    if (hsv.x < 1.0f / 6.0f) {
        rgb = (vec3){c, x, 0};
    } else if (hsv.x < 2.0f / 6.0f) {
        rgb = (vec3){x, c, 0};
    } else if (hsv.x < 3.0f / 6.0f) {
        rgb = (vec3){0, c, x};
    } else if (hsv.x < 4.0f / 6.0f) {
        rgb = (vec3){0, x, c};
    } else if (hsv.x < 5.0f / 6.0f) {
        rgb = (vec3){x, 0, c};
    } else {
        rgb = (vec3){c, 0, x};
    }
    
    rgb.x += m;
    rgb.y += m;
    rgb.z += m;
    
    return rgb;
}

// ============================================================================
// TAA IMPLEMENTATION
// ============================================================================

static bool create_taa_resources(u32 width, u32 height) {
    Renderer* renderer = get_current_renderer();
    if (!renderer) return false;
    
    // Create render textures for TAA
    TextureDesc desc = {
        .width = width,
        .height = height,
        .format = TEXTURE_FORMAT_RGBA16F,
        .usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_STORAGE,
        .name = "TAA Output"
    };
    
    g_postProcessState.taaOutputTexture = renderer->create_render_texture(renderer, &desc);
    if (!g_postProcessState.taaOutputTexture) return false;
    
    // History texture (double-buffered)
    desc.name = "TAA History";
    g_postProcessState.historyFrameTexture = renderer->create_render_texture(renderer, &desc);
    if (!g_postProcessState.historyFrameTexture) return false;
    
    return true;
}

static bool create_taa_pipeline(void) {
    Renderer* renderer = get_current_renderer();
    if (!renderer) return false;
    
    ComputePipelineDesc desc = {
        .shader_path = "assets/shaders/post_processing/taa_enhanced.comp",
        .name = "Enhanced TAA"
    };
    
    g_postProcessState.taaPipeline = renderer->create_compute_pipeline(renderer, &desc);
    return g_postProcessState.taaPipeline != NULL;
}

static void update_taa_config(void) {
    // Adaptive configuration based on performance
    if (g_postProcessState.averageFrameTime > 16.67f) { // Below 60 FPS
        // Reduce quality for better performance
        g_postProcessState.taaConfig.blendFactor = lerp(g_postProcessState.taaConfig.blendFactor, 0.05f, 0.1f);
        g_postProcessState.taaConfig.enableMotionBlur = false;
        g_postProcessState.taaConfig.enableSharpening = false;
    } else if (g_postProcessState.averageFrameTime < 10.0f) { // Above 100 FPS
        // Increase quality for better visuals
        g_postProcessState.taaConfig.blendFactor = lerp(g_postProcessState.taaConfig.blendFactor, 0.1f, 0.1f);
        g_postProcessState.taaConfig.enableMotionBlur = true;
        g_postProcessState.taaConfig.enableSharpening = true;
    }
    
    // Update frame index for temporal variation
    g_postProcessState.taaConfig.frameIndex++;
    
    // Generate jitter offset for TAA
    // Using Halton sequence for better distribution
    u32 index = g_postProcessState.taaConfig.frameIndex;
    f32 u, v;
    
    // Halton(2, 3) sequence
    u = 0.0f;
    f32 f = 0.5f;
    while (index > 0) {
        u += f * (index & 1);
        index >>= 1;
        f *= 0.5f;
    }
    
    index = g_postProcessState.taaConfig.frameIndex;
    f = 1.0f / 3.0f;
    while (index > 0) {
        v += f * (index % 3);
        index /= 3;
        f /= 3.0f;
    }
    
    g_postProcessState.taaConfig.jitterOffset = (vec2){
        .x = (u - 0.5f) * 2.0f,
        .y = (v - 0.5f) * 2.0f
    };
}

static void dispatch_taa(RenderTexture* currentFrame, RenderTexture* velocityBuffer,
                         RenderTexture* depthBuffer, RenderTexture* exposureBuffer,
                         RenderTexture* roughnessBuffer) {
    Renderer* renderer = get_current_renderer();
    if (!renderer || !g_postProcessState.taaPipeline) return;
    
    // Update TAA configuration
    update_taa_config();
    
    // Bind resources
    renderer->bind_texture_to_compute(renderer, g_postProcessState.taaPipeline, 0, currentFrame->texture);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.taaPipeline, 1, g_postProcessState.historyFrameTexture->texture);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.taaPipeline, 2, velocityBuffer->texture);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.taaPipeline, 3, depthBuffer->texture);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.taaPipeline, 4, exposureBuffer ? exposureBuffer->texture : NULL);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.taaPipeline, 5, roughnessBuffer ? roughnessBuffer->texture : NULL);
    
    renderer->bind_image_to_compute(renderer, g_postProcessState.taaPipeline, 0, g_postProcessState.taaOutputTexture);
    renderer->bind_image_to_compute(renderer, g_postProcessState.taaPipeline, 1, g_postProcessState.historyFrameTexture);
    
    // Set push constants
    TAAParams params = {
        .jitterOffset = g_postProcessState.taaConfig.jitterOffset,
        .blendFactor = g_postProcessState.taaConfig.blendFactor,
        .varianceClamp = g_postProcessState.taaConfig.varianceClamp,
        .motionBlurStrength = g_postProcessState.taaConfig.motionBlurStrength,
        .ghostReduction = g_postProcessState.taaConfig.ghostReduction,
        .sharpening = g_postProcessState.taaConfig.sharpening,
        .texelSize = (vec2){1.0f / currentFrame->texture->width, 1.0f / currentFrame->texture->height},
        .frameIndex = g_postProcessState.taaConfig.frameIndex,
        .enableMotionBlur = g_postProcessState.taaConfig.enableMotionBlur ? 1 : 0,
        .enableGhostReduction = g_postProcessState.taaConfig.enableGhostReduction ? 1 : 0,
        .enableSharpening = g_postProcessState.taaConfig.enableSharpening ? 1 : 0
    };
    
    renderer->set_compute_push_constants(renderer, g_postProcessState.taaPipeline, &params, sizeof(params));
    
    // Dispatch compute shader
    u32 groupCountX = (currentFrame->texture->width + 15) / 16;
    u32 groupCountY = (currentFrame->texture->height + 15) / 16;
    renderer->dispatch_compute(renderer, g_postProcessState.taaPipeline, groupCountX, groupCountY, 1);
    
    // Barrier to ensure completion
    renderer->memory_barrier(renderer, MEMORY_BARRIER_ALL);
}

// ============================================================================
// BLOOM IMPLEMENTATION
// ============================================================================

static bool create_bloom_resources(u32 width, u32 height) {
    Renderer* renderer = get_current_renderer();
    if (!renderer) return false;
    
    TextureDesc desc = {
        .width = width,
        .height = height,
        .format = TEXTURE_FORMAT_RGBA16F,
        .usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_STORAGE,
        .name = "Bloom Texture"
    };
    
    // Threshold texture
    desc.name = "Bloom Threshold";
    g_postProcessState.thresholdTexture = renderer->create_render_texture(renderer, &desc);
    if (!g_postProcessState.thresholdTexture) return false;
    
    // Blur textures (multiple passes for better quality)
    for (int i = 0; i < 4; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Bloom Blur %d", i);
        desc.name = name;
        g_postProcessState.blurTextures[i] = renderer->create_render_texture(renderer, &desc);
        if (!g_postProcessState.blurTextures[i]) return false;
    }
    
    // Composite texture
    desc.name = "Bloom Composite";
    g_postProcessState.bloomCompositeTexture = renderer->create_render_texture(renderer, &desc);
    if (!g_postProcessState.bloomCompositeTexture) return false;
    
    return true;
}

static bool create_bloom_pipelines(void) {
    Renderer* renderer = get_current_renderer();
    if (!renderer) return false;
    
    ComputePipelineDesc desc = {
        .shader_path = "assets/shaders/post_processing/bloom_threshold_enhanced.comp",
        .name = "Enhanced Bloom Threshold"
    };
    
    g_postProcessState.bloomThresholdPipeline = renderer->create_compute_pipeline(renderer, &desc);
    if (!g_postProcessState.bloomThresholdPipeline) return false;
    
    desc.shader_path = "assets/shaders/post_processing/bloom_blur_enhanced.comp";
    desc.name = "Enhanced Bloom Blur";
    g_postProcessState.bloomBlurPipeline = renderer->create_compute_pipeline(renderer, &desc);
    if (!g_postProcessState.bloomBlurPipeline) return false;
    
    desc.shader_path = "assets/shaders/post_processing/bloom_composite_enhanced.comp";
    desc.name = "Enhanced Bloom Composite";
    g_postProcessState.bloomCompositePipeline = renderer->create_compute_pipeline(renderer, &desc);
    if (!g_postProcessState.bloomCompositePipeline) return false;
    
    return true;
}

static void dispatch_bloom_threshold(RenderTexture* hdrInput) {
    Renderer* renderer = get_current_renderer();
    if (!renderer || !g_postProcessState.bloomThresholdPipeline) return;
    
    // Bind resources
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomThresholdPipeline, 0, hdrInput->texture);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomThresholdPipeline, 1, 
                                    g_postProcessState.lensDirtTexture ? g_postProcessState.lensDirtTexture : NULL);
    renderer->bind_image_to_compute(renderer, g_postProcessState.bloomThresholdPipeline, 0, g_postProcessState.thresholdTexture);
    
    // Set push constants
    BloomThresholdParams params = {
        .threshold = g_postProcessState.bloomConfig.threshold,
        .softKnee = g_postProcessState.bloomConfig.softKnee,
        .intensity = g_postProcessState.bloomConfig.intensity,
        .saturation = g_postProcessState.bloomConfig.saturation,
        .tint = g_postProcessState.bloomConfig.tint,
        .texelSize = (vec2){1.0f / hdrInput->texture->width, 1.0f / hdrInput->texture->height},
        .enableLensDirt = g_postProcessState.bloomConfig.enableLensDirt ? 1 : 0,
        .enableAdaptive = g_postProcessState.bloomConfig.enableAdaptive ? 1 : 0,
        .adaptiveStrength = g_postProcessState.bloomConfig.adaptiveStrength
    };
    
    renderer->set_compute_push_constants(renderer, g_postProcessState.bloomThresholdPipeline, &params, sizeof(params));
    
    // Dispatch
    u32 groupCountX = (hdrInput->texture->width + 15) / 16;
    u32 groupCountY = (hdrInput->texture->height + 15) / 16;
    renderer->dispatch_compute(renderer, g_postProcessState.bloomThresholdPipeline, groupCountX, groupCountY, 1);
}

static void dispatch_bloom_blur(RenderTexture* input, RenderTexture* output, vec2 direction) {
    Renderer* renderer = get_current_renderer();
    if (!renderer || !g_postProcessState.bloomBlurPipeline) return;
    
    // Bind resources
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomBlurPipeline, 0, input->texture);
    renderer->bind_image_to_compute(renderer, g_postProcessState.bloomBlurPipeline, 0, output);
    
    // Set push constants
    BloomBlurParams params = {
        .texelSize = (vec2){1.0f / input->texture->width, 1.0f / input->texture->height},
        .direction = direction,
        .sigma = g_postProcessState.bloomConfig.sigma,
        .anamorphicStrength = g_postProcessState.bloomConfig.anamorphicStrength,
        .kernelSize = g_postProcessState.bloomConfig.kernelSize,
        .enableAnamorphic = g_postProcessState.bloomConfig.enableAnamorphic ? 1 : 0
    };
    
    renderer->set_compute_push_constants(renderer, g_postProcessState.bloomBlurPipeline, &params, sizeof(params));
    
    // Dispatch
    u32 groupCountX = (input->texture->width + 15) / 16;
    u32 groupCountY = (input->texture->height + 15) / 16;
    renderer->dispatch_compute(renderer, g_postProcessState.bloomBlurPipeline, groupCountX, groupCountY, 1);
}

static void dispatch_bloom_composite(RenderTexture* originalScene, RenderTexture* blurredBloom) {
    Renderer* renderer = get_current_renderer();
    if (!renderer || !g_postProcessState.bloomCompositePipeline) return;
    
    // Bind resources
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomCompositePipeline, 0, originalScene->texture);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomCompositePipeline, 1, blurredBloom->texture);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomCompositePipeline, 2, 
                                    g_postProcessState.lensDirtTexture ? g_postProcessState.lensDirtTexture : NULL);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomCompositePipeline, 3, 
                                    g_postProcessState.lensFlareTexture ? g_postProcessState.lensFlareTexture : NULL);
    renderer->bind_texture_to_compute(renderer, g_postProcessState.bloomCompositePipeline, 4, 
                                    g_postProcessState.vignetteTexture ? g_postProcessState.vignetteTexture : NULL);
    renderer->bind_image_to_compute(renderer, g_postProcessState.bloomCompositePipeline, 0, g_postProcessState.bloomCompositeTexture);
    
    // Set push constants
    BloomCompositeParams params = {
        .intensity = g_postProcessState.bloomConfig.intensity,
        .dirtIntensity = g_postProcessState.bloomConfig.dirtIntensity,
        .flareIntensity = g_postProcessState.bloomConfig.flareIntensity,
        .vignetteIntensity = g_postProcessState.bloomConfig.vignetteIntensity,
        .colorShift = g_postProcessState.bloomConfig.colorShift,
        .bloomTint = g_postProcessState.bloomConfig.bloomTint,
        .flareTint = g_postProcessState.bloomConfig.flareTint,
        .texelSize = (vec2){1.0f / originalScene->texture->width, 1.0f / originalScene->texture->height},
        .enableLensDirt = g_postProcessState.bloomConfig.enableLensDirt ? 1 : 0,
        .enableLensFlare = g_postProcessState.bloomConfig.enableLensFlare ? 1 : 0,
        .enableVignette = g_postProcessState.bloomConfig.enableVignette ? 1 : 0,
        .enableHDR = g_postProcessState.bloomConfig.enableHDR ? 1 : 0,
        .hdrExposure = g_postProcessState.bloomConfig.hdrExposure,
        .hdrGamma = g_postProcessState.hdrGamma
    };
    
    renderer->set_compute_push_constants(renderer, g_postProcessState.bloomCompositePipeline, &params, sizeof(params));
    
    // Dispatch
    u32 groupCountX = (originalScene->texture->width + 15) / 16;
    u32 groupCountY = (originalScene->texture->height + 15) / 16;
    renderer->dispatch_compute(renderer, g_postProcessState.bloomCompositePipeline, groupCountX, groupCountY, 1);
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool enhanced_post_processing_init(u32 width, u32 height) {
    if (g_postProcessState.initialized) {
        return true; // Already initialized
    }
    
    LOG_INFO("Initializing enhanced post-processing system...");
    
    // Initialize default configurations
    g_postProcessState.taaConfig = (TAAConfig){
        .blendFactor = 0.1f,
        .varianceClamp = 0.1f,
        .motionBlurStrength = 0.5f,
        .ghostReduction = 0.8f,
        .sharpening = 0.25f,
        .enableMotionBlur = true,
        .enableGhostReduction = true,
        .enableSharpening = true,
        .frameIndex = 0
    };
    
    g_postProcessState.bloomConfig = (BloomConfig){
        .threshold = 1.0f,
        .softKnee = 0.5f,
        .intensity = 0.5f,
        .saturation = 1.2f,
        .sigma = 2.0f,
        .kernelSize = 9,
        .anamorphicStrength = 0.0f,
        .tint = (vec3){1.0f, 1.0f, 1.0f},
        .colorShift = (vec3){1.0f, 0.9f, 0.8f},
        .bloomTint = (vec3){1.0f, 0.95f, 0.85f},
        .flareTint = (vec3){1.0f, 0.8f, 0.6f},
        .dirtIntensity = 0.3f,
        .flareIntensity = 0.5f,
        .vignetteIntensity = 0.8f,
        .enableLensDirt = false,
        .enableLensFlare = false,
        .enableVignette = false,
        .enableAnamorphic = false,
        .enableAdaptive = true,
        .enableHDR = true,
        .adaptiveStrength = 0.5f,
        .hdrExposure = 1.0f,
        .hdrGamma = 2.2f
    };
    
    // Create TAA resources and pipelines
    if (!create_taa_resources(width, height)) {
        LOG_ERROR("Failed to create TAA resources");
        return false;
    }
    
    if (!create_taa_pipeline()) {
        LOG_ERROR("Failed to create TAA pipeline");
        return false;
    }
    
    // Create bloom resources and pipelines
    if (!create_bloom_resources(width, height)) {
        LOG_ERROR("Failed to create bloom resources");
        return false;
    }
    
    if (!create_bloom_pipelines()) {
        LOG_ERROR("Failed to create bloom pipelines");
        return false;
    }
    
    // Load lens effect textures (optional)
    Renderer* renderer = get_current_renderer();
    if (renderer) {
        g_postProcessState.lensDirtTexture = renderer->load_texture(renderer, "assets/textures/lens_dirt.png");
        g_postProcessState.lensFlareTexture = renderer->load_texture(renderer, "assets/textures/lens_flare.png");
        g_postProcessState.vignetteTexture = renderer->load_texture(renderer, "assets/textures/vignette.png");
        
        if (g_postProcessState.lensDirtTexture) {
            g_postProcessState.bloomConfig.enableLensDirt = true;
        }
        if (g_postProcessState.lensFlareTexture) {
            g_postProcessState.bloomConfig.enableLensFlare = true;
        }
        if (g_postProcessState.vignetteTexture) {
            g_postProcessState.bloomConfig.enableVignette = true;
        }
    }
    
    g_postProcessState.initialized = true;
    g_postProcessState.frameCount = 0;
    g_postProcessState.averageFrameTime = 16.67f; // Assume 60 FPS initially
    
    LOG_INFO("Enhanced post-processing system initialized successfully");
    return true;
}

void enhanced_post_processing_shutdown(void) {
    if (!g_postProcessState.initialized) {
        return;
    }
    
    LOG_INFO("Shutting down enhanced post-processing system...");
    
    Renderer* renderer = get_current_renderer();
    if (renderer) {
        // Destroy TAA resources
        if (g_postProcessState.taaOutputTexture) {
            renderer->destroy_render_texture(renderer, g_postProcessState.taaOutputTexture);
        }
        if (g_postProcessState.historyFrameTexture) {
            renderer->destroy_render_texture(renderer, g_postProcessState.historyFrameTexture);
        }
        
        // Destroy bloom resources
        if (g_postProcessState.thresholdTexture) {
            renderer->destroy_render_texture(renderer, g_postProcessState.thresholdTexture);
        }
        for (int i = 0; i < 4; i++) {
            if (g_postProcessState.blurTextures[i]) {
                renderer->destroy_render_texture(renderer, g_postProcessState.blurTextures[i]);
            }
        }
        if (g_postProcessState.bloomCompositeTexture) {
            renderer->destroy_render_texture(renderer, g_postProcessState.bloomCompositeTexture);
        }
        
        // Destroy pipelines
        if (g_postProcessState.taaPipeline) {
            renderer->destroy_compute_pipeline(renderer, g_postProcessState.taaPipeline);
        }
        if (g_postProcessState.bloomThresholdPipeline) {
            renderer->destroy_compute_pipeline(renderer, g_postProcessState.bloomThresholdPipeline);
        }
        if (g_postProcessState.bloomBlurPipeline) {
            renderer->destroy_compute_pipeline(renderer, g_postProcessState.bloomBlurPipeline);
        }
        if (g_postProcessState.bloomCompositePipeline) {
            renderer->destroy_compute_pipeline(renderer, g_postProcessState.bloomCompositePipeline);
        }
        
        // Destroy lens effect textures
        if (g_postProcessState.lensDirtTexture) {
            renderer->destroy_texture(renderer, g_postProcessState.lensDirtTexture);
        }
        if (g_postProcessState.lensFlareTexture) {
            renderer->destroy_texture(renderer, g_postProcessState.lensFlareTexture);
        }
        if (g_postProcessState.vignetteTexture) {
            renderer->destroy_texture(renderer, g_postProcessState.vignetteTexture);
        }
    }
    
    memset(&g_postProcessState, 0, sizeof(g_postProcessState));
    
    LOG_INFO("Enhanced post-processing system shutdown complete");
}

RenderTexture* enhanced_post_processing_apply_taa(RenderTexture* currentFrame, 
                                                   RenderTexture* velocityBuffer,
                                                   RenderTexture* depthBuffer,
                                                   RenderTexture* exposureBuffer,
                                                   RenderTexture* roughnessBuffer) {
    if (!g_postProcessState.initialized || !currentFrame || !velocityBuffer || !depthBuffer) {
        return currentFrame;
    }
    
    // Update performance metrics
    u64 currentTime = get_current_time_ns();
    if (g_postProcessState.lastFrameTime > 0) {
        f32 frameTime = (currentTime - g_postProcessState.lastFrameTime) / 1000000.0f; // Convert to milliseconds
        g_postProcessState.averageFrameTime = lerp(g_postProcessState.averageFrameTime, frameTime, 0.1f);
    }
    g_postProcessState.lastFrameTime = currentTime;
    g_postProcessState.frameCount++;
    
    // Dispatch TAA
    dispatch_taa(currentFrame, velocityBuffer, depthBuffer, exposureBuffer, roughnessBuffer);
    
    return g_postProcessState.taaOutputTexture;
}

RenderTexture* enhanced_post_processing_apply_bloom(RenderTexture* hdrInput) {
    if (!g_postProcessState.initialized || !hdrInput) {
        return hdrInput;
    }
    
    // Step 1: Threshold extraction
    dispatch_bloom_threshold(hdrInput);
    
    // Step 2: Multi-pass blur (horizontal and vertical)
    RenderTexture* currentBlur = g_postProcessState.thresholdTexture;
    
    for (int pass = 0; pass < 4; pass++) {
        // Horizontal blur
        dispatch_bloom_blur(currentBlur, g_postProcessState.blurTextures[pass], (vec2){1.0f, 0.0f});
        
        // Vertical blur
        dispatch_bloom_blur(g_postProcessState.blurTextures[pass], g_postProcessState.blurTextures[pass], (vec2){0.0f, 1.0f});
        
        currentBlur = g_postProcessState.blurTextures[pass];
    }
    
    // Step 3: Composite with original scene
    dispatch_bloom_composite(hdrInput, currentBlur);
    
    return g_postProcessState.bloomCompositeTexture;
}

RenderTexture* enhanced_post_processing_apply_full(RenderTexture* currentFrame,
                                                   RenderTexture* velocityBuffer,
                                                   RenderTexture* depthBuffer,
                                                   RenderTexture* exposureBuffer,
                                                   RenderTexture* roughnessBuffer) {
    if (!g_postProcessState.initialized || !currentFrame) {
        return currentFrame;
    }
    
    // Apply TAA first
    RenderTexture* taaResult = enhanced_post_processing_apply_taa(currentFrame, velocityBuffer, 
                                                                  depthBuffer, exposureBuffer, roughnessBuffer);
    
    // Then apply bloom to the TAA result
    return enhanced_post_processing_apply_bloom(taaResult);
}

void enhanced_post_processing_set_taa_config(const TAAConfig* config) {
    if (config) {
        g_postProcessState.taaConfig = *config;
    }
}

void enhanced_post_processing_set_bloom_config(const BloomConfig* config) {
    if (config) {
        g_postProcessState.bloomConfig = *config;
    }
}

TAAConfig enhanced_post_processing_get_taa_config(void) {
    return g_postProcessState.taaConfig;
}

BloomConfig enhanced_post_processing_get_bloom_config(void) {
    return g_postProcessState.bloomConfig;
}

void enhanced_post_processing_get_performance_stats(f32* averageFrameTime, u32* frameCount) {
    if (averageFrameTime) *averageFrameTime = g_postProcessState.averageFrameTime;
    if (frameCount) *frameCount = g_postProcessState.frameCount;
}
