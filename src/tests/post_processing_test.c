// src/engine/rendering/post_processing/post_processing_test.c
//
// Purpose: End-to-end testing for post-processing effects
// Tests TAA, bloom, SSAO, and SSR compute shader implementations

#include "rendering/post_processing/taa_compute.h"
#include "rendering/post_processing/bloom_compute.h"
#include "rendering/post_processing/ssao_compute.h"
#include "rendering/post_processing/ssr_compute.h"
#include "rendering/core/texture.h"
#include "core/memory/unified_allocator.h"
#include "core/logging/unified_logger.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TEST UTILITIES
// ============================================================================

static TextureID create_test_texture(u32 width, u32 height, vec4 color) {
    TextureDesc desc = {
        .width = width,
        .height = height,
        .format = TEXTURE_FORMAT_RGBA16F,
        .usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_STORAGE,
        .min_filter = FILTER_LINEAR,
        .mag_filter = FILTER_LINEAR
    };
    
    TextureID texture = texture_create(&desc);
    if (!texture) return 0;
    
    // Fill with test data
    f32* data = MALLOC_TEMP(width * height * 4 * sizeof(f32));
    if (!data) {
        texture_destroy(texture);
        return 0;
    }
    
    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            u32 index = (y * width + x) * 4;
            
            // Create a gradient pattern
            f32 fx = (f32)x / width;
            f32 fy = (f32)y / height;
            
            data[index + 0] = color.r * fx;
            data[index + 1] = color.g * fy;
            data[index + 2] = color.b;
            data[index + 3] = color.a;
        }
    }
    
    texture_upload_data(texture, data);
    FREE(data);
    
    return texture;
}

static TextureID create_test_depth_texture(u32 width, u32 height) {
    TextureDesc desc = {
        .width = width,
        .height = height,
        .format = TEXTURE_FORMAT_R32F,
        .usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_STORAGE,
        .min_filter = FILTER_NEAREST,
        .mag_filter = FILTER_NEAREST
    };
    
    TextureID texture = texture_create(&desc);
    if (!texture) return 0;
    
    // Fill with depth data (gradient from near to far)
    f32* data = MALLOC_TEMP(width * height * sizeof(f32));
    if (!data) {
        texture_destroy(texture);
        return 0;
    }
    
    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            u32 index = y * width + x;
            
            // Create depth gradient
            f32 fx = (f32)x / width;
            f32 fy = (f32)y / height;
            data[index] = 0.1f + 0.8f * (fx * fx + fy * fy) * 0.5f;
        }
    }
    
    texture_upload_data(texture, data);
    FREE(data);
    
    return texture;
}

static TextureID create_test_normal_texture(u32 width, u32 height) {
    TextureDesc desc = {
        .width = width,
        .height = height,
        .format = TEXTURE_FORMAT_RGBA8,
        .usage = TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_STORAGE,
        .min_filter = FILTER_LINEAR,
        .mag_filter = FILTER_LINEAR
    };
    
    TextureID texture = texture_create(&desc);
    if (!texture) return 0;
    
    // Fill with normal data
    u8* data = MALLOC_TEMP(width * height * 4 * sizeof(u8));
    if (!data) {
        texture_destroy(texture);
        return 0;
    }
    
    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            u32 index = (y * width + x) * 4;
            
            // Create varying normals
            f32 fx = (f32)x / width;
            f32 fy = (f32)y / height;
            
            // Normal pointing mostly up with some variation
            vec3 normal = (vec3){
                sin(fx * 6.28f) * 0.3f,
                cos(fy * 6.28f) * 0.3f,
                0.9f
            };
            normal = vec3_normalize(normal);
            
            // Convert to [0, 255] range
            data[index + 0] = (u8)((normal.x * 0.5f + 0.5f) * 255);
            data[index + 1] = (u8)((normal.y * 0.5f + 0.5f) * 255);
            data[index + 2] = (u8)((normal.z * 0.5f + 0.5f) * 255);
            data[index + 3] = (u8)(0.5f * 255); // Roughness
        }
    }
    
    texture_upload_data(texture, data);
    FREE(data);
    
    return texture;
}

static bool compare_textures(TextureID tex1, TextureID tex2, f32 tolerance) {
    // Simple comparison - in a real implementation would read back and compare pixels
    // For now, just check if both are valid
    return tex1 != 0 && tex2 != 0;
}

// ============================================================================
// TAA TESTS
// ============================================================================

static bool test_taa_compute(void) {
    LOG_INFO(LOG_CAT_RENDERER, "Testing TAA compute shader");
    
    const u32 width = 1024;
    const u32 height = 768;
    
    // Create TAA context
    TAAComputeContext* taa = taa_compute_create(width, height);
    if (!taa) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create TAA context");
        return false;
    }
    
    // Create test textures
    TextureID current_frame = create_test_texture(width, height, (vec4){1.0f, 0.5f, 0.2f, 1.0f});
    TextureID velocity_buffer = create_test_texture(width, height, (vec4){0.1f, 0.05f, 0.0f, 0.0f});
    TextureID depth_buffer = create_test_depth_texture(width, height);
    
    if (!current_frame || !velocity_buffer || !depth_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create test textures for TAA");
        taa_compute_destroy(taa);
        return false;
    }
    
    // Test TAA processing
    taa_compute_process(taa, current_frame, velocity_buffer, depth_buffer);
    
    // Get output
    TextureID output = taa_compute_get_output(taa);
    if (!output) {
        LOG_ERROR(LOG_CAT_RENDERER, "TAA failed to produce output");
        taa_compute_destroy(taa);
        texture_destroy(current_frame);
        texture_destroy(velocity_buffer);
        texture_destroy(depth_buffer);
        return false;
    }
    
    // Test jitter offset
    f32 jitter_x, jitter_y;
    taa_compute_get_jitter_offset(taa, &jitter_x, &jitter_y);
    
    // Test settings update
    TAAComputeSettings settings = {
        .blend_factor = 0.1f,
        .sharpness = 0.8f,
        .enable_sharpening = true,
        .enable_jitter = true,
        .sample_pattern = 1,
        .jitter_scale = 1.5f
    };
    taa_compute_update_settings(taa, &settings);
    
    // Test reset
    taa_compute_reset(taa);
    
    // Cleanup
    taa_compute_destroy(taa);
    texture_destroy(current_frame);
    texture_destroy(velocity_buffer);
    texture_destroy(depth_buffer);
    
    LOG_INFO(LOG_CAT_RENDERER, "TAA compute test passed");
    return true;
}

// ============================================================================
// BLOOM TESTS
// ============================================================================

static bool test_bloom_compute(void) {
    LOG_INFO(LOG_CAT_RENDERER, "Testing bloom compute shader");
    
    const u32 width = 1024;
    const u32 height = 768;
    
    // Create bloom context
    BloomComputeContext* bloom = bloom_compute_create(width, height);
    if (!bloom) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create bloom context");
        return false;
    }
    
    // Create test HDR texture
    TextureID hdr_input = create_test_texture(width, height, (vec4){2.0f, 1.5f, 1.0f, 1.0f});
    if (!hdr_input) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create HDR test texture");
        bloom_compute_destroy(bloom);
        return false;
    }
    
    // Test bloom processing
    bloom_compute_process(bloom, hdr_input);
    
    // Get output
    TextureID output = bloom_compute_get_output(bloom);
    if (!output) {
        LOG_ERROR(LOG_CAT_RENDERER, "Bloom failed to produce output");
        bloom_compute_destroy(bloom);
        texture_destroy(hdr_input);
        return false;
    }
    
    // Test settings update
    BloomComputeSettings settings = {
        .threshold = 1.5f,
        .soft_knee = 0.3f,
        .intensity = 0.08f,
        .iterations = 7,
        .sigma = 1.5f,
        .color_shift = {1.0f, 0.9f, 0.8f}
    };
    bloom_compute_update_settings(bloom, &settings);
    
    // Test resize
    bloom_compute_resize(bloom, 2048, 1536);
    
    // Cleanup
    bloom_compute_destroy(bloom);
    texture_destroy(hdr_input);
    
    LOG_INFO(LOG_CAT_RENDERER, "Bloom compute test passed");
    return true;
}

// ============================================================================
// SSAO TESTS
// ============================================================================

static bool test_ssao_compute(void) {
    LOG_INFO(LOG_CAT_RENDERER, "Testing SSAO compute shader");
    
    const u32 width = 1024;
    const u32 height = 768;
    
    // Create SSAO context
    SSAOComputeContext* ssao = ssao_compute_create(width, height);
    if (!ssao) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create SSAO context");
        return false;
    }
    
    // Create test textures
    TextureID depth_buffer = create_test_depth_texture(width, height);
    TextureID normal_buffer = create_test_normal_texture(width, height);
    
    if (!depth_buffer || !normal_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create test textures for SSAO");
        ssao_compute_destroy(ssao);
        return false;
    }
    
    // Test SSAO processing
    ssao_compute_process(ssao, depth_buffer, normal_buffer);
    
    // Get output
    TextureID output = ssao_compute_get_output(ssao);
    if (!output) {
        LOG_ERROR(LOG_CAT_RENDERER, "SSAO failed to produce output");
        ssao_compute_destroy(ssao);
        texture_destroy(depth_buffer);
        texture_destroy(normal_buffer);
        return false;
    }
    
    // Test settings update
    SSAOComputeSettings settings = {
        .radius = 0.8f,
        .intensity = 1.5f,
        .bias = 0.05f,
        .sample_count = 24
    };
    ssao_compute_update_settings(ssao, &settings);
    
    // Test projection params
    ssao_compute_set_projection_params(ssao, 0.5f, 200.0f);
    
    // Test resize
    ssao_compute_resize(ssao, 2048, 1536);
    
    // Cleanup
    ssao_compute_destroy(ssao);
    texture_destroy(depth_buffer);
    texture_destroy(normal_buffer);
    
    LOG_INFO(LOG_CAT_RENDERER, "SSAO compute test passed");
    return true;
}

// ============================================================================
// SSR TESTS
// ============================================================================

static bool test_ssr_compute(void) {
    LOG_INFO(LOG_CAT_RENDERER, "Testing SSR compute shader");
    
    const u32 width = 1024;
    const u32 height = 768;
    
    // Create SSR context
    SSRComputeContext* ssr = ssr_compute_create(width, height);
    if (!ssr) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create SSR context");
        return false;
    }
    
    // Create test textures
    TextureID scene_color = create_test_texture(width, height, (vec4){0.8f, 0.6f, 0.4f, 1.0f});
    TextureID normal_roughness = create_test_normal_texture(width, height);
    TextureID depth_buffer = create_test_depth_texture(width, height);
    
    if (!scene_color || !normal_roughness || !depth_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create test textures for SSR");
        ssr_compute_destroy(ssr);
        return false;
    }
    
    // Test SSR processing
    ssr_compute_process(ssr, scene_color, normal_roughness, depth_buffer);
    
    // Get output
    TextureID output = ssr_compute_get_output(ssr);
    if (!output) {
        LOG_ERROR(LOG_CAT_RENDERER, "SSR failed to produce output");
        ssr_compute_destroy(ssr);
        texture_destroy(scene_color);
        texture_destroy(normal_roughness);
        texture_destroy(depth_buffer);
        return false;
    }
    
    // Test settings update
    SSRComputeSettings settings = {
        .max_distance = 75.0f,
        .thickness = 0.15f,
        .max_steps = 96,
        .stride = 1.5f,
        .fade_distance = 40.0f
    };
    ssr_compute_update_settings(ssr, &settings);
    
    // Test projection params
    ssr_compute_set_projection_params(ssr, 0.5f, 200.0f);
    
    // Test resize
    ssr_compute_resize(ssr, 2048, 1536);
    
    // Cleanup
    ssr_compute_destroy(ssr);
    texture_destroy(scene_color);
    texture_destroy(normal_roughness);
    texture_destroy(depth_buffer);
    
    LOG_INFO(LOG_CAT_RENDERER, "SSR compute test passed");
    return true;
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

static bool test_post_processing_integration(void) {
    LOG_INFO(LOG_CAT_RENDERER, "Testing post-processing integration");
    
    const u32 width = 1024;
    const u32 height = 768;
    
    // Create all post-processing contexts
    TAAComputeContext* taa = taa_compute_create(width, height);
    BloomComputeContext* bloom = bloom_compute_create(width, height);
    SSAOComputeContext* ssao = ssao_compute_create(width, height);
    SSRComputeContext* ssr = ssr_compute_create(width, height);
    
    if (!taa || !bloom || !ssao || !ssr) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create post-processing contexts");
        if (taa) taa_compute_destroy(taa);
        if (bloom) bloom_compute_destroy(bloom);
        if (ssao) ssao_compute_destroy(ssao);
        if (ssr) ssr_compute_destroy(ssr);
        return false;
    }
    
    // Create shared test textures
    TextureID scene_color = create_test_texture(width, height, (vec4){1.2f, 0.8f, 0.6f, 1.0f});
    TextureID depth_buffer = create_test_depth_texture(width, height);
    TextureID normal_buffer = create_test_normal_texture(width, height);
    TextureID velocity_buffer = create_test_texture(width, height, (vec4){0.05f, 0.02f, 0.0f, 0.0f});
    
    if (!scene_color || !depth_buffer || !normal_buffer || !velocity_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create shared test textures");
        taa_compute_destroy(taa);
        bloom_compute_destroy(bloom);
        ssao_compute_destroy(ssao);
        ssr_compute_destroy(ssr);
        texture_destroy(scene_color);
        texture_destroy(depth_buffer);
        texture_destroy(normal_buffer);
        texture_destroy(velocity_buffer);
        return false;
    }
    
    // Process full post-processing pipeline
    // 1. TAA
    taa_compute_process(taa, scene_color, velocity_buffer, depth_buffer);
    TextureID taa_output = taa_compute_get_output(taa);
    
    // 2. Bloom
    bloom_compute_process(bloom, taa_output ? taa_output : scene_color);
    TextureID bloom_output = bloom_compute_get_output(bloom);
    
    // 3. SSAO
    ssao_compute_process(ssao, depth_buffer, normal_buffer);
    TextureID ssao_output = ssao_compute_get_output(ssao);
    
    // 4. SSR
    ssr_compute_process(ssr, bloom_output ? bloom_output : scene_color, normal_buffer, depth_buffer);
    TextureID ssr_output = ssr_compute_get_output(ssr);
    
    // Verify all outputs are valid
    bool success = (taa_output != 0) && (bloom_output != 0) && (ssao_output != 0) && (ssr_output != 0);
    
    // Cleanup
    taa_compute_destroy(taa);
    bloom_compute_destroy(bloom);
    ssao_compute_destroy(ssao);
    ssr_compute_destroy(ssr);
    
    texture_destroy(scene_color);
    texture_destroy(depth_buffer);
    texture_destroy(normal_buffer);
    texture_destroy(velocity_buffer);
    
    if (success) {
        LOG_INFO(LOG_CAT_RENDERER, "Post-processing integration test passed");
    } else {
        LOG_ERROR(LOG_CAT_RENDERER, "Post-processing integration test failed");
    }
    
    return success;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

bool run_post_processing_tests(void) {
    LOG_INFO(LOG_CAT_RENDERER, "Starting post-processing tests");
    
    bool all_passed = true;
    
    // Individual component tests
    all_passed &= test_taa_compute();
    all_passed &= test_bloom_compute();
    all_passed &= test_ssao_compute();
    all_passed &= test_ssr_compute();
    
    // Integration test
    all_passed &= test_post_processing_integration();
    
    if (all_passed) {
        LOG_INFO(LOG_CAT_RENDERER, "All post-processing tests passed!");
    } else {
        LOG_ERROR(LOG_CAT_RENDERER, "Some post-processing tests failed");
    }
    
    return all_passed;
}
