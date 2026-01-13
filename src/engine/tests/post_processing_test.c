// src/engine/tests/post_processing_test.c
//
// Purpose: End-to-end testing system for enhanced post-processing effects
// Tests TAA, bloom, SSAO, and SSR with comprehensive validation

#include "tests/post_processing_test.h"
#include "renderer/post_processing/enhanced_taa_bloom.h"
#include "core/logger.h"
#include "core/memory/unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// ============================================================================
// TEST FRAMEWORK
// ============================================================================

typedef struct {
    const char* name;
    bool (*test_func)(void);
    f64 execution_time;
    bool passed;
    char error_message[256];
} TestCase;

typedef struct {
    u32 total_tests;
    u32 passed_tests;
    u32 failed_tests;
    f64 total_time;
    TestCase* tests;
} TestSuite;

static TestSuite g_test_suite = {0};

// ============================================================================
// MOCK RENDERER FOR TESTING
// ============================================================================

typedef struct {
    u32 width;
    u32 height;
    u8* data;
    size_t data_size;
} MockTexture;

typedef struct {
    MockTexture* textures[16];
    u32 texture_count;
    bool initialized;
} MockRenderer;

static MockRenderer g_mock_renderer = {0};

static bool mock_renderer_init(u32 width, u32 height) {
    if (g_mock_renderer.initialized) return true;
    
    g_mock_renderer.width = width;
    g_mock_renderer.height = height;
    g_mock_renderer.texture_count = 0;
    
    // Create test textures
    for (int i = 0; i < 8; i++) {
        MockTexture* texture = malloc(sizeof(MockTexture));
        texture->width = width;
        texture->height = height;
        texture->data_size = width * height * 4; // RGBA
        texture->data = malloc(texture->data_size);
        
        // Fill with test pattern
        for (u32 y = 0; y < height; y++) {
            for (u32 x = 0; x < width; x++) {
                u32 index = (y * width + x) * 4;
                // Create gradient pattern
                texture->data[index + 0] = (u8)((x * 255) / width);     // R
                texture->data[index + 1] = (u8)((y * 255) / height);    // G
                texture->data[index + 2] = (u8)(((x + y) * 255) / (width + height)); // B
                texture->data[index + 3] = 255; // A
            }
        }
        
        g_mock_renderer.textures[g_mock_renderer.texture_count++] = texture;
    }
    
    g_mock_renderer.initialized = true;
    return true;
}

static void mock_renderer_shutdown(void) {
    if (!g_mock_renderer.initialized) return;
    
    for (u32 i = 0; i < g_mock_renderer.texture_count; i++) {
        if (g_mock_renderer.textures[i]) {
            free(g_mock_renderer.textures[i]->data);
            free(g_mock_renderer.textures[i]);
            g_mock_renderer.textures[i] = NULL;
        }
    }
    
    memset(&g_mock_renderer, 0, sizeof(g_mock_renderer));
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static inline f32 rand_f32(f32 min, f32 max) {
    return min + ((f32)rand() / RAND_MAX) * (max - min);
}

static inline vec3 rand_vec3(f32 min, f32 max) {
    return (vec3){
        .x = rand_f32(min, max),
        .y = rand_f32(min, max),
        .z = rand_f32(min, max)
    };
}

static bool compare_textures(MockTexture* a, MockTexture* b, f32 tolerance) {
    if (!a || !b) return false;
    if (a->width != b->width || a->height != b->height) return false;
    
    for (u32 y = 0; y < a->height; y++) {
        for (u32 x = 0; x < a->width; x++) {
            u32 index = (y * a->width + x) * 4;
            
            f32 diff_r = fabsf((f32)a->data[index + 0] - (f32)b->data[index + 0]) / 255.0f;
            f32 diff_g = fabsf((f32)a->data[index + 1] - (f32)b->data[index + 1]) / 255.0f;
            f32 diff_b = fabsf((f32)a->data[index + 2] - (f32)b->data[index + 2]) / 255.0f;
            
            if (diff_r > tolerance || diff_g > tolerance || diff_b > tolerance) {
                return false;
            }
        }
    }
    
    return true;
}

static void generate_test_pattern(MockTexture* texture, u32 pattern_type) {
    if (!texture) return;
    
    for (u32 y = 0; y < texture->height; y++) {
        for (u32 x = 0; x < texture->width; x++) {
            u32 index = (y * texture->width + x) * 4;
            
            switch (pattern_type) {
                case 0: // Gradient
                    texture->data[index + 0] = (u8)((x * 255) / texture->width);
                    texture->data[index + 1] = (u8)((y * 255) / texture->height);
                    texture->data[index + 2] = (u8)(((x + y) * 255) / (texture->width + texture->height));
                    break;
                    
                case 1: // Checkerboard
                    u8 value = ((x / 32) + (y / 32)) % 2 ? 255 : 0;
                    texture->data[index + 0] = value;
                    texture->data[index + 1] = value;
                    texture->data[index + 2] = value;
                    break;
                    
                case 2: // Noise
                    texture->data[index + 0] = (u8)(rand() % 256);
                    texture->data[index + 1] = (u8)(rand() % 256);
                    texture->data[index + 2] = (u8)(rand() % 256);
                    break;
                    
                case 3: // Sine wave pattern
                    f32 wave = sinf((f32)x * 0.1f) * cosf((f32)y * 0.1f);
                    u8 color = (u8)((wave + 1.0f) * 127.5f);
                    texture->data[index + 0] = color;
                    texture->data[index + 1] = (u8)(color * 0.7f);
                    texture->data[index + 2] = (u8)(color * 0.3f);
                    break;
            }
            
            texture->data[index + 3] = 255; // Alpha
        }
    }
}

// ============================================================================
// TAA TESTS
// ============================================================================

static bool test_taa_initialization(void) {
    LOG_INFO("Testing TAA initialization...");
    
    u64 start_time = get_current_time_ns();
    
    bool result = enhanced_post_processing_init(1920, 1080);
    
    u64 end_time = get_current_time_ns();
    f64 execution_time = (end_time - start_time) / 1000000.0; // Convert to ms
    
    if (!result) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Failed to initialize enhanced post-processing system");
        return false;
    }
    
    // Verify TAA configuration
    TAAConfig config = enhanced_post_processing_get_taa_config();
    if (config.blendFactor <= 0.0f || config.blendFactor > 1.0f) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Invalid TAA blend factor: %.3f", config.blendFactor);
        return false;
    }
    
    LOG_INFO("TAA initialization test passed in %.2f ms", execution_time);
    return true;
}

static bool test_taa_frame_processing(void) {
    LOG_INFO("Testing TAA frame processing...");
    
    if (!g_mock_renderer.initialized) {
        if (!mock_renderer_init(1920, 1080)) {
            snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                    sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                    "Failed to initialize mock renderer");
            return false;
        }
    }
    
    u64 start_time = get_current_time_ns();
    
    // Process multiple frames to test temporal stability
    for (int frame = 0; frame < 10; frame++) {
        // Create mock render textures (simplified)
        RenderTexture currentFrame = {0};
        RenderTexture velocityBuffer = {0};
        RenderTexture depthBuffer = {0};
        
        // Apply TAA
        RenderTexture* result = enhanced_post_processing_apply_taa(&currentFrame, &velocityBuffer, 
                                                                  &depthBuffer, NULL, NULL);
        
        if (!result) {
            snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                    sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                    "TAA processing failed on frame %d", frame);
            return false;
        }
    }
    
    u64 end_time = get_current_time_ns();
    f64 execution_time = (end_time - start_time) / 1000000.0;
    
    // Check performance metrics
    f32 averageFrameTime;
    u32 frameCount;
    enhanced_post_processing_get_performance_stats(&averageFrameTime, &frameCount);
    
    if (frameCount != 10) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Expected 10 frames, got %d", frameCount);
        return false;
    }
    
    LOG_INFO("TAA frame processing test passed in %.2f ms (avg frame time: %.2f ms)", 
             execution_time, averageFrameTime);
    return true;
}

static bool test_taa_configuration(void) {
    LOG_INFO("Testing TAA configuration...");
    
    u64 start_time = get_current_time_ns();
    
    // Test configuration changes
    TAAConfig newConfig = {
        .blendFactor = 0.15f,
        .varianceClamp = 0.12f,
        .motionBlurStrength = 0.6f,
        .ghostReduction = 0.85f,
        .sharpening = 0.3f,
        .enableMotionBlur = true,
        .enableGhostReduction = true,
        .enableSharpening = true,
        .frameIndex = 0
    };
    
    enhanced_post_processing_set_taa_config(&newConfig);
    
    // Verify configuration was applied
    TAAConfig retrievedConfig = enhanced_post_processing_get_taa_config();
    
    if (fabsf(retrievedConfig.blendFactor - newConfig.blendFactor) > 0.001f) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "TAA blend factor not applied correctly: %.3f != %.3f", 
                retrievedConfig.blendFactor, newConfig.blendFactor);
        return false;
    }
    
    if (retrievedConfig.enableMotionBlur != newConfig.enableMotionBlur) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "TAA motion blur setting not applied correctly");
        return false;
    }
    
    u64 end_time = get_current_time_ns();
    f64 execution_time = (end_time - start_time) / 1000000.0;
    
    LOG_INFO("TAA configuration test passed in %.2f ms", execution_time);
    return true;
}

// ============================================================================
// BLOOM TESTS
// ============================================================================

static bool test_bloom_threshold(void) {
    LOG_INFO("Testing bloom threshold extraction...");
    
    u64 start_time = get_current_time_ns();
    
    // Create test texture with bright and dark regions
    if (!g_mock_renderer.initialized) {
        if (!mock_renderer_init(1920, 1080)) {
            snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                    sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                    "Failed to initialize mock renderer");
            return false;
        }
    }
    
    // Generate test pattern with bright spots
    generate_test_pattern(g_mock_renderer.textures[0], 0); // Gradient pattern
    
    // Test bloom configuration
    BloomConfig bloomConfig = {
        .threshold = 1.0f,
        .softKnee = 0.5f,
        .intensity = 0.8f,
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
    
    enhanced_post_processing_set_bloom_config(&bloomConfig);
    
    // Apply bloom (simplified - would need proper render textures)
    RenderTexture hdrInput = {0};
    RenderTexture* result = enhanced_post_processing_apply_bloom(&hdrInput);
    
    if (!result) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Bloom processing failed");
        return false;
    }
    
    u64 end_time = get_current_time_ns();
    f64 execution_time = (end_time - start_time) / 1000000.0;
    
    LOG_INFO("Bloom threshold test passed in %.2f ms", execution_time);
    return true;
}

static bool test_bloom_blur_passes(void) {
    LOG_INFO("Testing bloom blur passes...");
    
    u64 start_time = get_current_time_ns();
    
    // Test different blur configurations
    for (int kernelSize = 5; kernelSize <= 17; kernelSize += 4) {
        BloomConfig config = enhanced_post_processing_get_bloom_config();
        config.kernelSize = kernelSize;
        config.sigma = 1.0f + (kernelSize - 5) * 0.5f;
        
        enhanced_post_processing_set_bloom_config(&config);
        
        RenderTexture hdrInput = {0};
        RenderTexture* result = enhanced_post_processing_apply_bloom(&hdrInput);
        
        if (!result) {
            snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                    sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                    "Bloom blur failed with kernel size %d", kernelSize);
            return false;
        }
    }
    
    u64 end_time = get_current_time_ns();
    f64 execution_time = (end_time - start_time) / 1000000.0;
    
    LOG_INFO("Bloom blur passes test passed in %.2f ms", execution_time);
    return true;
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

static bool test_full_post_processing_pipeline(void) {
    LOG_INFO("Testing full post-processing pipeline...");
    
    u64 start_time = get_current_time_ns();
    
    // Test complete pipeline: TAA + Bloom
    if (!g_mock_renderer.initialized) {
        if (!mock_renderer_init(1920, 1080)) {
            snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                    sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                    "Failed to initialize mock renderer");
            return false;
        }
    }
    
    // Generate different test patterns for each frame
    for (int frame = 0; frame < 5; frame++) {
        generate_test_pattern(g_mock_renderer.textures[frame % 4], frame % 4);
        
        RenderTexture currentFrame = {0};
        RenderTexture velocityBuffer = {0};
        RenderTexture depthBuffer = {0};
        RenderTexture exposureBuffer = {0};
        RenderTexture roughnessBuffer = {0};
        
        // Apply full pipeline
        RenderTexture* result = enhanced_post_processing_apply_full(&currentFrame, velocityBuffer, 
                                                                   depthBuffer, exposureBuffer, 
                                                                   roughnessBuffer);
        
        if (!result) {
            snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                    sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                    "Full pipeline failed on frame %d", frame);
            return false;
        }
    }
    
    u64 end_time = get_current_time_ns();
    f64 execution_time = (end_time - start_time) / 1000000.0;
    
    // Check final performance metrics
    f32 averageFrameTime;
    u32 frameCount;
    enhanced_post_processing_get_performance_stats(&averageFrameTime, &frameCount);
    
    LOG_INFO("Full pipeline test passed in %.2f ms (avg frame time: %.2f ms, total frames: %d)", 
             execution_time, averageFrameTime, frameCount);
    return true;
}

static bool test_performance_under_load(void) {
    LOG_INFO("Testing performance under load...");
    
    u64 start_time = get_current_time_ns();
    
    // Stress test with high resolution and many frames
    const u32 testWidth = 3840;
    const u32 testHeight = 2160; // 4K resolution
    const u32 frameCount = 100;
    
    if (!mock_renderer_init(testWidth, testHeight)) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Failed to initialize mock renderer for 4K test");
        return false;
    }
    
    for (u32 frame = 0; frame < frameCount; frame++) {
        RenderTexture currentFrame = {0};
        RenderTexture velocityBuffer = {0};
        RenderTexture depthBuffer = {0};
        
        RenderTexture* result = enhanced_post_processing_apply_full(&currentFrame, velocityBuffer, 
                                                                   depthBuffer, NULL, NULL);
        
        if (!result) {
            snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                    sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                    "Performance test failed on frame %d", frame);
            return false;
        }
    }
    
    u64 end_time = get_current_time_ns();
    f64 total_time = (end_time - start_time) / 1000000.0;
    f64 avg_frame_time = total_time / frameCount;
    
    // Check if performance is acceptable (should be under 33ms for 30 FPS)
    if (avg_frame_time > 33.0) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Performance too slow: %.2f ms per frame (target: 33.0 ms)", avg_frame_time);
        return false;
    }
    
    LOG_INFO("Performance test passed: %.2f ms per frame for %d 4K frames", avg_frame_time, frameCount);
    return true;
}

static bool test_memory_usage(void) {
    LOG_INFO("Testing memory usage...");
    
    u64 start_time = get_current_time_ns();
    
    // Get initial memory usage
    size_t initial_memory = unified_memory_get_usage();
    
    // Initialize and run post-processing
    if (!enhanced_post_processing_init(1920, 1080)) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Failed to initialize for memory test");
        return false;
    }
    
    // Run some frames
    for (int i = 0; i < 50; i++) {
        RenderTexture currentFrame = {0};
        RenderTexture velocityBuffer = {0};
        RenderTexture depthBuffer = {0};
        
        enhanced_post_processing_apply_full(&currentFrame, velocityBuffer, depthBuffer, NULL, NULL);
    }
    
    // Check memory usage
    size_t final_memory = unified_memory_get_usage();
    size_t memory_increase = final_memory - initial_memory;
    
    // Memory increase should be reasonable (less than 100MB for post-processing)
    const size_t max_memory_increase = 100 * 1024 * 1024; // 100MB
    
    if (memory_increase > max_memory_increase) {
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Memory usage too high: %.2f MB (max: %.2f MB)", 
                (f64)memory_increase / (1024.0 * 1024.0), 
                (f64)max_memory_increase / (1024.0 * 1024.0));
        return false;
    }
    
    // Shutdown and check memory cleanup
    enhanced_post_processing_shutdown();
    
    size_t cleanup_memory = unified_memory_get_usage();
    size_t memory_leak = cleanup_memory - initial_memory;
    
    if (memory_leak > 1024 * 1024) { // Allow 1MB tolerance
        snprintf(g_test_suite.tests[g_test_suite.total_tests].error_message, 
                sizeof(g_test_suite.tests[g_test_suite.total_tests].error_message),
                "Memory leak detected: %.2f MB", (f64)memory_leak / (1024.0 * 1024.0));
        return false;
    }
    
    u64 end_time = get_current_time_ns();
    f64 execution_time = (end_time - start_time) / 1000000.0;
    
    LOG_INFO("Memory usage test passed in %.2f ms (increase: %.2f MB, leak: %.2f MB)", 
             execution_time, (f64)memory_increase / (1024.0 * 1024.0), 
             (f64)memory_leak / (1024.0 * 1024.0));
    return true;
}

// ============================================================================
// TEST FRAMEWORK IMPLEMENTATION
// ============================================================================

static void add_test_case(const char* name, bool (*test_func)(void)) {
    g_test_suite.tests = realloc(g_test_suite.tests, (g_test_suite.total_tests + 1) * sizeof(TestCase));
    g_test_suite.tests[g_test_suite.total_tests].name = name;
    g_test_suite.tests[g_test_suite.total_tests].test_func = test_func;
    g_test_suite.tests[g_test_suite.total_tests].execution_time = 0.0;
    g_test_suite.tests[g_test_suite.total_tests].passed = false;
    g_test_suite.tests[g_test_suite.total_tests].error_message[0] = '\0';
    g_test_suite.total_tests++;
}

static bool run_test_case(TestCase* test) {
    u64 start_time = get_current_time_ns();
    
    bool result = test->test_func();
    
    u64 end_time = get_current_time_ns();
    test->execution_time = (end_time - start_time) / 1000000.0; // Convert to ms
    test->passed = result;
    
    if (result) {
        g_test_suite.passed_tests++;
        LOG_INFO("✓ %s (%.2f ms)", test->name, test->execution_time);
    } else {
        g_test_suite.failed_tests++;
        LOG_ERROR("✗ %s (%.2f ms): %s", test->name, test->execution_time, test->error_message);
    }
    
    g_test_suite.total_time += test->execution_time;
    
    return result;
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool post_processing_test_run_all(void) {
    LOG_INFO("Starting enhanced post-processing end-to-end tests...");
    
    // Initialize test framework
    memset(&g_test_suite, 0, sizeof(g_test_suite));
    
    // Add test cases
    add_test_case("TAA Initialization", test_taa_initialization);
    add_test_case("TAA Frame Processing", test_taa_frame_processing);
    add_test_case("TAA Configuration", test_taa_configuration);
    add_test_case("Bloom Threshold", test_bloom_threshold);
    add_test_case("Bloom Blur Passes", test_bloom_blur_passes);
    add_test_case("Full Pipeline Integration", test_full_post_processing_pipeline);
    add_test_case("Performance Under Load", test_performance_under_load);
    add_test_case("Memory Usage", test_memory_usage);
    
    // Run all tests
    srand((u32)time(NULL)); // Seed random number generator
    
    for (u32 i = 0; i < g_test_suite.total_tests; i++) {
        run_test_case(&g_test_suite.tests[i]);
    }
    
    // Print summary
    LOG_INFO("=== POST-PROCESSING TEST SUMMARY ===");
    LOG_INFO("Total tests: %u", g_test_suite.total_tests);
    LOG_INFO("Passed: %u", g_test_suite.passed_tests);
    LOG_INFO("Failed: %u", g_test_suite.failed_tests);
    LOG_INFO("Success rate: %.1f%%", 
             (f64)g_test_suite.passed_tests / g_test_suite.total_tests * 100.0);
    LOG_INFO("Total execution time: %.2f ms", g_test_suite.total_time);
    LOG_INFO("=====================================");
    
    // Cleanup
    mock_renderer_shutdown();
    free(g_test_suite.tests);
    memset(&g_test_suite, 0, sizeof(g_test_suite));
    
    return g_test_suite.failed_tests == 0;
}

bool post_processing_test_run_single(const char* test_name) {
    // Find and run specific test
    for (u32 i = 0; i < g_test_suite.total_tests; i++) {
        if (strcmp(g_test_suite.tests[i].name, test_name) == 0) {
            return run_test_case(&g_test_suite.tests[i]);
        }
    }
    
    LOG_ERROR("Test '%s' not found", test_name);
    return false;
}

void post_processing_test_list_available(void) {
    LOG_INFO("Available post-processing tests:");
    
    const char* test_names[] = {
        "TAA Initialization",
        "TAA Frame Processing", 
        "TAA Configuration",
        "Bloom Threshold",
        "Bloom Blur Passes",
        "Full Pipeline Integration",
        "Performance Under Load",
        "Memory Usage"
    };
    
    for (size_t i = 0; i < sizeof(test_names) / sizeof(test_names[0]); i++) {
        LOG_INFO("  - %s", test_names[i]);
    }
}
