/*
 * instancing_example.c
 * Example integration of instancing and GPU-driven rendering system
 *
 * This demonstrates how to use the complete instancing pipeline:
 * 1. Instance data management
 * 2. GPU culling
 * 3. Batch generation
 * 4. Indirect rendering
 * 5. Performance profiling
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "../geometry/instancing/instance_data.h"
#include "../geometry/instancing/gpu_culling.h"
#include "../geometry/instancing/performance_profiling.h"
#include "../rendering/3d_rendering/rendering/gpu_driven/multi_draw_indirect.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

/* ============================================================================
 * EXAMPLE: FOREST SCENE WITH 1000 TREES
 * ============================================================================ */

typedef struct forest_scene {
    // Instance data
    instance_buffer_t* tree_instances;
    uint32_t tree_count;
    
    // GPU culling
    gpu_culling_context_t* culling_context;
    
    // Indirect rendering
    rendering_multi_draw_indirect_handle_t indirect_handle;
    
    // Performance profiling
    performance_profiler_t* profiler;
    
    // Metal resources
    void* metal_device;
    void* command_queue;
} forest_scene_t;

/**
 * Initialize a scene with N tree instances
 */
forest_scene_t* forest_scene_create(uint32_t tree_count, void* metal_device) {
    forest_scene_t* scene = (forest_scene_t*)calloc(1, sizeof(forest_scene_t));
    if (!scene) {
        return NULL;
    }
    
    scene->metal_device = metal_device;
    scene->tree_count = tree_count;
    
    // Initialize systems
    instance_data_init();
    gpu_culling_init();
    rendering_multi_draw_indirect_init();
    
    // Create instance buffer
    instance_buffer_desc_t instance_desc = {0};
    instance_desc.max_instances = tree_count;
    instance_desc.initial_count = 0;
    instance_desc.allow_resize = false;
    instance_desc.gpu_writable = false;
    instance_desc.label = "TreeInstances";
    
    scene->tree_instances = instance_buffer_create(
        (metal_device_t*)metal_device, &instance_desc);
    
    if (!scene->tree_instances) {
        fprintf(stderr, "[Example] Failed to create instance buffer\n");
        free(scene);
        return NULL;
    }
    
    // Create GPU culling context
    gpu_culling_config_t culling_config = {0};
    culling_config.enable_frustum_culling = true;
    culling_config.enable_distance_culling = true;
    culling_config.enable_backface_culling = false;
    culling_config.enable_occlusion_culling = false;
    culling_config.near_distance = 0.1f;
    culling_config.far_distance = 500.0f;
    culling_config.lod0_distance = 50.0f;
    culling_config.lod1_distance = 150.0f;
    culling_config.lod2_distance = 300.0f;
    culling_config.max_visible_instances = tree_count;
    
    scene->culling_context = gpu_culling_context_create(
        (metal_device_t*)metal_device,
        tree_count,
        &culling_config);
    
    // Create indirect rendering handle
    rendering_multi_draw_indirect_desc_t indirect_desc = {0};
    indirect_desc.max_draw_count = tree_count;
    indirect_desc.use_indirect_command_buffer = true;
    indirect_desc.indexed = true;
    indirect_desc.flags = 0;
    
    if (rendering_multi_draw_indirect_create(&scene->indirect_handle, &indirect_desc) != 0) {
        fprintf(stderr, "[Example] Failed to create indirect rendering handle\n");
    }
    
    // Create performance profiler
    scene->profiler = performance_profiler_create(1000, 60); // 1000 frame history, 60 frame window
    
    printf("[Example] Forest scene created with %u trees\n", tree_count);
    
    return scene;
}

/**
 * Populate scene with tree instances
 */
void forest_scene_populate(forest_scene_t* scene) {
    if (!scene || !scene->tree_instances) {
        return;
    }
    
    printf("[Example] Populating forest with trees...\n");
    
    // Create tree instances in a grid with random variation
    instance_data_t* instances = (instance_data_t*)malloc(
        scene->tree_count * sizeof(instance_data_t));
    
    uint32_t grid_size = (uint32_t)sqrt(scene->tree_count);
    float spacing = 10.0f;
    
    for (uint32_t i = 0; i < scene->tree_count; i++) {
        instance_data_t* inst = &instances[i];
        memset(inst, 0, sizeof(instance_data_t));
        
        // Position in grid
        uint32_t x = i % grid_size;
        uint32_t z = i / grid_size;
        
        float pos_x = (x - grid_size / 2) * spacing + ((rand() % 100) / 100.0f - 0.5f) * spacing;
        float pos_y = 0.0f;
        float pos_z = (z - grid_size / 2) * spacing + ((rand() % 100) / 100.0f - 0.5f) * spacing;
        
        // Create transform matrix (identity with translation)
        inst->transform.matrix[0] = 1.0f; // Scale X
        inst->transform.matrix[5] = 1.0f; // Scale Y
        inst->transform.matrix[10] = 1.0f; // Scale Z
        inst->transform.matrix[12] = pos_x; // Translation X
        inst->transform.matrix[13] = pos_y; // Translation Y
        inst->transform.matrix[14] = pos_z; // Translation Z
        inst->transform.matrix[15] = 1.0f; // W
        
        // Material parameters
        inst->material_params.base_color[0] = 0.2f + (rand() % 100) / 500.0f; // R
        inst->material_params.base_color[1] = 0.6f + (rand() % 100) / 500.0f; // G
        inst->material_params.base_color[2] = 0.2f + (rand() % 100) / 500.0f; // B
        inst->material_params.base_color[3] = 1.0f; // A
        inst->material_params.roughness = 0.8f;
        inst->material_params.metallic = 0.0f;
        
        // Visibility flags
        inst->visibility_flags.visible = 1;
        inst->visibility_flags.cast_shadow = 1;
        inst->visibility_flags.receive_shadow = 1;
        inst->visibility_flags.culling_enabled = 1;
        
        // Mesh and material IDs
        inst->mesh_id = 0; // All trees use same mesh
        inst->material_id = 0; // All trees use same material
    }
    
    // Upload to GPU
    instance_buffer_update(scene->tree_instances, instances, scene->tree_count, 0);
    
    free(instances);
    
    printf("[Example] Populated %u tree instances\n", scene->tree_count);
}

/**
 * Render one frame of the forest scene
 */
void forest_scene_render_frame(forest_scene_t* scene, 
                                const float camera_position[3],
                                const float view_proj_matrix[16]) {
    if (!scene || !scene->profiler) {
        return;
    }
    
    // Begin frame profiling
    performance_profiler_begin_frame(scene->profiler);
    
    // Extract frustum from view-projection matrix
    camera_frustum_t frustum;
    gpu_culling_extract_frustum(view_proj_matrix, &frustum);
    
    // GPU Culling pass
    performance_profiler_begin_culling(scene->profiler);
    
#ifdef __OBJC__
    @autoreleasepool {
        // Create command buffer (simplified - would come from render pass)
        id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)scene->command_queue;
        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        
        // Execute GPU culling
        gpu_culling_execute(
            scene->culling_context,
            (__bridge MTLCommandBuffer*)commandBuffer,
            scene->tree_instances,
            camera_position,
            &frustum,
            NULL); // No hi-z texture yet
        
        // Record culling time
        performance_profiler_end_culling(scene->profiler);
        
        // Get culling results
        gpu_culling_result_t culling_result;
        gpu_culling_get_result(scene->culling_context, &culling_result);
        
        // Record instance counts
        performance_profiler_record_instances(
            scene->profiler,
            culling_result.stats.total_instances,
            culling_result.stats.visible_instances);
        
        // Record culling statistics
        performance_profiler_record_culling(
            scene->profiler,
            culling_result.stats.frustum_culled,
            culling_result.stats.distance_culled,
            culling_result.stats.occlusion_culled,
            culling_result.stats.total_instances);
        
        // Record draw calls (before = visible instances, after = batches)
        performance_profiler_record_draw_calls(
            scene->profiler,
            culling_result.stats.visible_instances, // Without instancing
            1); // With instancing (1 batch for all trees)
        
        // Note: Actual render encoder and command execution would go here
        
        [commandBuffer commit];
    }
#endif
    
    // End frame profiling
    performance_profiler_end_frame(scene->profiler);
}

/**
 * Print performance report
 */
void forest_scene_print_stats(forest_scene_t* scene) {
    if (!scene || !scene->profiler) {
        return;
    }
    
    printf("\n");
    performance_profiler_print_report(scene->profiler);
    printf("\n");
    
    // Print culling stats
    gpu_culling_debug_print(scene->culling_context);
    
    // Print indirect rendering stats
    rendering_multi_draw_indirect_debug_print();
}

/**
 * Clean up scene
 */
void forest_scene_destroy(forest_scene_t* scene) {
    if (!scene) {
        return;
    }
    
    if (scene->tree_instances) {
        instance_buffer_destroy(scene->tree_instances);
    }
    
    if (scene->culling_context) {
        gpu_culling_context_destroy(scene->culling_context);
    }
    
    rendering_multi_draw_indirect_destroy(scene->indirect_handle);
    
    if (scene->profiler) {
        performance_profiler_destroy(scene->profiler);
    }
    
    // Shutdown systems
    rendering_multi_draw_indirect_shutdown();
    gpu_culling_shutdown();
    instance_data_shutdown();
    
    free(scene);
    
    printf("[Example] Forest scene destroyed\n");
}

/* ============================================================================
 * MAIN EXAMPLE
 * ============================================================================ */

#ifdef EXAMPLE_STANDALONE

int main(int argc, char** argv) {
    printf("=== Instancing System Example ===\n\n");
    
    // In a real application, you would get the Metal device from your renderer
    // For this example, we'll use NULL (non-functional but demonstrates API)
    void* metal_device = NULL;
    
    // Create forest scene with 1000 trees
    forest_scene_t* scene = forest_scene_create(1000, metal_device);
    if (!scene) {
        fprintf(stderr, "Failed to create scene\n");
        return 1;
    }
    
    // Populate with tree instances
    forest_scene_populate(scene);
    
    // Simulate several frames
    float camera_pos[3] = {0.0f, 10.0f, 0.0f};
    float view_proj[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    }; // Identity matrix (simplified)
    
    printf("\nSimulating 60 frames...\n");
    for (int frame = 0; frame < 60; frame++) {
        // Move camera in a circle
        camera_pos[0] = cosf(frame * 0.1f) * 50.0f;
        camera_pos[2] = sinf(frame * 0.1f) * 50.0f;
        
        forest_scene_render_frame(scene, camera_pos, view_proj);
        
        if (frame % 10 == 0) {
            printf("  Frame %d rendered\n", frame);
        }
    }
    
    // Print final statistics
    forest_scene_print_stats(scene);
    
    // Export CSV
    performance_profiler_export_csv(scene->profiler, "forest_performance.csv");
    
    // Cleanup
    forest_scene_destroy(scene);
    
    printf("\n=== Example Complete ===\n");
    return 0;
}

#endif // EXAMPLE_STANDALONE
