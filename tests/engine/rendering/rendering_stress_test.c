// Rendering System Stress Test
// Tests rendering performance under complex scenes and high load

#include "rendering/post_processing/post_processing_pipeline.h"
#include "rendering/frame_graph/frame_graph.h"
#include "core/logger.h"
#include "core/timer.h"
#include "core/memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Stress test configuration
typedef struct {
    u32 object_count;
    u32 light_count;
    u32 particle_count;
    u32 resolution_width;
    u32 resolution_height;
    bool enable_taa;
    bool enable_ssao;
    bool enable_ssr;
    bool enable_bloom;
    u32 test_duration_seconds;
    f32 target_frame_time_ms;
} StressTestConfig;

// Performance metrics
typedef struct {
    f64 total_frame_time;
    f64 average_frame_time;
    f64 min_frame_time;
    f64 max_frame_time;
    u64 frame_count;
    u64 dropped_frames;
    f64 memory_usage_mb;
    f64 gpu_utilization;
    bool test_passed;
} PerformanceMetrics;

// Test scene data
typedef struct {
    RenderGraph *render_graph;
    PostProcessingPipeline *post_pipeline;
    TextureID *scene_textures;
    TextureID *depth_textures;
    TextureID *normal_textures;
    TextureID *velocity_textures;
    u32 texture_count;
} TestScene;

// Initialize stress test scene
static TestScene* init_test_scene(const StressTestConfig *config) {
    TestScene *scene = malloc(sizeof(TestScene));
    if (!scene) {
        LOG_ERROR("Failed to allocate test scene");
        return NULL;
    }
    
    memset(scene, 0, sizeof(TestScene));
    
    // Create render graph
    scene->render_graph = rg_create(config->resolution_width, config->resolution_height);
    if (!scene->render_graph) {
        LOG_ERROR("Failed to create render graph");
        free(scene);
        return NULL;
    }
    
    // Create post-processing pipeline
    scene->post_pipeline = post_processing_create(config->resolution_width, config->resolution_height);
    if (!scene->post_pipeline) {
        LOG_ERROR("Failed to create post-processing pipeline");
        rg_destroy(scene->render_graph);
        free(scene);
        return NULL;
    }
    
    // Configure post-processing
    PostProcessingConfig pp_config = {
        .enable_taa = config->enable_taa,
        .enable_ssao = config->enable_ssao,
        .enable_ssr = config->enable_ssr,
        .enable_bloom = config->enable_bloom,
        .bloom_intensity = 0.8f,
        .bloom_iterations = 5,
        .ssao_radius = 0.5f,
        .ssao_strength = 1.0f,
        .ssao_samples = 16,
        .ssr_max_distance = 50.0f,
        .ssr_max_steps = 64,
        .ssr_thickness = 0.1f
    };
    
    post_processing_update_config(scene->post_pipeline, &pp_config);
    
    // Allocate texture arrays for multiple render targets
    scene->texture_count = 10; // Multiple render targets for complex scenes
    scene->scene_textures = malloc(sizeof(TextureID) * scene->texture_count);
    scene->depth_textures = malloc(sizeof(TextureID) * scene->texture_count);
    scene->normal_textures = malloc(sizeof(TextureID) * scene->texture_count);
    scene->velocity_textures = malloc(sizeof(TextureID) * scene->texture_count);
    
    // Create test textures
    for (u32 i = 0; i < scene->texture_count; i++) {
        TextureDesc desc = {
            .width = config->resolution_width,
            .height = config->resolution_height,
            .depth = 1,
            .format = TEXTURE_FORMAT_RGBA16F,
            .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED | TEXTURE_USAGE_RENDER_TARGET,
            .name = "StressTest_Texture"
        };
        
        scene->scene_textures[i] = texture_manager_create(&desc);
        scene->depth_textures[i] = texture_manager_create(&desc); // Simplified - should be depth format
        scene->normal_textures[i] = texture_manager_create(&desc);
        scene->velocity_textures[i] = texture_manager_create(&desc);
    }
    
    LOG_INFO("Test scene initialized with %u objects, %u lights, %u particles", 
             config->object_count, config->light_count, config->particle_count);
    
    return scene;
}

// Simulate complex scene rendering
static void render_complex_frame(TestScene *scene, const StressTestConfig *config) {
    // Begin frame
    rg_begin_frame(scene->render_graph);
    
    // Simulate multiple render passes for complex scene
    for (u32 pass = 0; pass < 3; pass++) {
        // Create render target for this pass
        RGResourceHandle color_target = rg_create_texture(scene->render_graph, &(RGTextureDesc){
            .width = config->resolution_width,
            .height = config->resolution_height,
            .format = TEXTURE_FORMAT_RGBA16F,
            .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED,
            .name = "ComplexPass_Color"
        });
        
        RGResourceHandle depth_target = rg_create_texture(scene->render_graph, &(RGTextureDesc){
            .width = config->resolution_width,
            .height = config->resolution_height,
            .format = TEXTURE_FORMAT_DEPTH32F,
            .usage = TEXTURE_USAGE_STORAGE,
            .name = "ComplexPass_Depth"
        });
        
        // Simulate rendering many objects
        for (u32 i = 0; i < config->object_count; i++) {
            // Add object to render graph (simplified)
            rg_add_draw_call(scene->render_graph, color_target, depth_target);
        }
        
        // Add post-processing for this pass
        if (config->enable_taa || config->enable_ssao || config->enable_ssr || config->enable_bloom) {
            post_processing_add_to_graph(scene->render_graph, scene->post_pipeline, 
                                       color_target, depth_target, color_target);
        }
    }
    
    // Execute render graph
    rg_execute(scene->render_graph);
    
    // End frame
    rg_end_frame(scene->render_graph);
}

// Run stress test
static PerformanceMetrics run_stress_test(const StressTestConfig *config) {
    PerformanceMetrics metrics = {0};
    metrics.min_frame_time = INFINITY;
    
    TestScene *scene = init_test_scene(config);
    if (!scene) {
        metrics.test_passed = false;
        return metrics;
    }
    
    LOG_INFO("Starting stress test: %ux%u, %u seconds", 
             config->resolution_width, config->resolution_height, config->test_duration_seconds);
    
    Timer timer;
    timer_start(&timer);
    
    f64 frame_start_time = timer_get_elapsed_seconds(&timer);
    u64 target_frame_count = (u64)(config->test_duration_seconds * 60.0); // 60 FPS target
    
    for (u64 frame = 0; frame < target_frame_count; frame++) {
        // Render complex frame
        render_complex_frame(scene, config);
        
        // Calculate frame time
        f64 frame_end_time = timer_get_elapsed_seconds(&timer);
        f64 frame_time = (frame_end_time - frame_start_time) * 1000.0; // Convert to ms
        
        // Update metrics
        metrics.total_frame_time += frame_time;
        metrics.frame_count++;
        metrics.min_frame_time = fmin(metrics.min_frame_time, frame_time);
        metrics.max_frame_time = fmax(metrics.max_frame_time, frame_time);
        
        // Check for dropped frames
        if (frame_time > config->target_frame_time_ms * 1.5) {
            metrics.dropped_frames++;
        }
        
        frame_start_time = frame_end_time;
        
        // Log progress every 60 frames (1 second at 60 FPS)
        if (frame % 60 == 0) {
            f64 current_avg = metrics.total_frame_time / metrics.frame_count;
            LOG_DEBUG("Frame %llu: Avg: %.2fms, Min: %.2fms, Max: %.2fms, Dropped: %llu", 
                     frame, current_avg, metrics.min_frame_time, metrics.max_frame_time, metrics.dropped_frames);
        }
    }
    
    // Calculate final metrics
    metrics.average_frame_time = metrics.total_frame_time / metrics.frame_count;
    metrics.memory_usage_mb = memory_get_usage() / (1024.0 * 1024.0);
    metrics.gpu_utilization = 0.0; // Would need GPU API to get this
    
    // Determine if test passed
    metrics.test_passed = (metrics.average_frame_time <= config->target_frame_time_ms) &&
                         (metrics.dropped_frames <= (metrics.frame_count * 0.05)); // Allow 5% dropped frames
    
    timer_stop(&timer);
    
    LOG_INFO("Stress test completed:");
    LOG_INFO("  Average frame time: %.2fms (target: %.2fms)", metrics.average_frame_time, config->target_frame_time_ms);
    LOG_INFO("  Min/Max frame time: %.2fms / %.2fms", metrics.min_frame_time, metrics.max_frame_time);
    LOG_INFO("  Total frames: %llu, Dropped: %llu (%.1f%%)", 
             metrics.frame_count, metrics.dropped_frames, 
             (f64)metrics.dropped_frames / metrics.frame_count * 100.0);
    LOG_INFO("  Memory usage: %.1f MB", metrics.memory_usage_mb);
    LOG_INFO("  Test %s", metrics.test_passed ? "PASSED" : "FAILED");
    
    // Cleanup
    for (u32 i = 0; i < scene->texture_count; i++) {
        texture_manager_destroy(scene->scene_textures[i]);
        texture_manager_destroy(scene->depth_textures[i]);
        texture_manager_destroy(scene->normal_textures[i]);
        texture_manager_destroy(scene->velocity_textures[i]);
    }
    
    free(scene->scene_textures);
    free(scene->depth_textures);
    free(scene->normal_textures);
    free(scene->velocity_textures);
    
    post_processing_destroy(scene->post_pipeline);
    rg_destroy(scene->render_graph);
    free(scene);
    
    return metrics;
}

// Run comprehensive stress test suite
void run_rendering_stress_tests(void) {
    LOG_INFO("=== Rendering System Stress Test Suite ===");
    
    // Test configurations
    StressTestConfig test_configs[] = {
        // Light load test
        {
            .object_count = 1000,
            .light_count = 10,
            .particle_count = 1000,
            .resolution_width = 1920,
            .resolution_height = 1080,
            .enable_taa = true,
            .enable_ssao = true,
            .enable_ssr = false,
            .enable_bloom = true,
            .test_duration_seconds = 10,
            .target_frame_time_ms = 16.67
        },
        // Medium load test
        {
            .object_count = 5000,
            .light_count = 50,
            .particle_count = 5000,
            .resolution_width = 1920,
            .resolution_height = 1080,
            .enable_taa = true,
            .enable_ssao = true,
            .enable_ssr = true,
            .enable_bloom = true,
            .test_duration_seconds = 10,
            .target_frame_time_ms = 16.67
        },
        // Heavy load test
        {
            .object_count = 10000,
            .light_count = 100,
            .particle_count = 10000,
            .resolution_width = 1920,
            .resolution_height = 1080,
            .enable_taa = true,
            .enable_ssao = true,
            .enable_ssr = true,
            .enable_bloom = true,
            .test_duration_seconds = 10,
            .target_frame_time_ms = 16.67
        },
        // 4K test
        {
            .object_count = 5000,
            .light_count = 50,
            .particle_count = 5000,
            .resolution_width = 3840,
            .resolution_height = 2160,
            .enable_taa = true,
            .enable_ssao = true,
            .enable_ssr = true,
            .enable_bloom = true,
            .test_duration_seconds = 10,
            .target_frame_time_ms = 16.67
        }
    };
    
    const char* test_names[] = {
        "Light Load Test",
        "Medium Load Test", 
        "Heavy Load Test",
        "4K Resolution Test"
    };
    
    u32 passed_tests = 0;
    u32 total_tests = sizeof(test_configs) / sizeof(test_configs[0]);
    
    for (u32 i = 0; i < total_tests; i++) {
        LOG_INFO("\n--- %s ---", test_names[i]);
        PerformanceMetrics metrics = run_stress_test(&test_configs[i]);
        
        if (metrics.test_passed) {
            passed_tests++;
            LOG_SUCCESS("✓ %s PASSED", test_names[i]);
        } else {
            LOG_ERROR("✗ %s FAILED", test_names[i]);
        }
    }
    
    LOG_INFO("\n=== Stress Test Summary ===");
    LOG_INFO("Tests passed: %u/%u (%.1f%%)", passed_tests, total_tests, 
             (f64)passed_tests / total_tests * 100.0);
    
    if (passed_tests == total_tests) {
        LOG_SUCCESS("All stress tests PASSED! Rendering system meets performance requirements.");
    } else {
        LOG_ERROR("Some stress tests FAILED! Rendering system needs optimization.");
    }
}

// Main stress test entry point
int main(int argc, char* argv[]) {
    // Initialize systems
    logger_init(LOG_LEVEL_DEBUG);
    memory_init();
    
    // Run stress tests
    run_rendering_stress_tests();
    
    // Cleanup
    memory_shutdown();
    logger_shutdown();
    
    return 0;
}
