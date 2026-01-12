/*
 * mtl_sync_example.c
 * Comprehensive usage example for Metal synchronization system
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import "mtl_sync_manager.h"
#import "mtl_device.h"
#import <Metal/Metal.h>
#import <stdio.h>

/**
 * Example: Complete frame rendering with synchronization
 */
void example_complete_frame_rendering(void) {
    // Create Metal device
    metal_device_t* device = metal_device_create_system_default();
    if (!device) {
        fprintf(stderr, "Failed to create Metal device\n");
        return;
    }
    
    id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)metal_get_device(device);
    
    // Configure synchronization manager
    metal_sync_manager_config_t sync_config = {
        .max_frames_in_flight = 3,           // Triple buffering
        .target_fps = 60,                    // 60 FPS target
        .enable_vsync = true,                // Enable VSync
        .enable_hazard_tracking = true,      // Automatic barrier insertion
        .enable_gpu_timestamps = true,       // GPU profiling
        .barrier_policy = METAL_BARRIER_POLICY_CONSERVATIVE
    };
    
    metal_sync_manager_t* sync_mgr = metal_sync_manager_create(mtl_device, &sync_config);
    if (!sync_mgr) {
        fprintf(stderr, "Failed to create sync manager\n");
        return;
    }
    
    // Get timestamp pool for profiling
    metal_timestamp_pool_t* timestamps = metal_sync_manager_get_timestamp_pool(sync_mgr);
    
    // Main render loop
    bool running = true;
    uint32_t frame_count = 0;
    
    while (running && frame_count < 600) { // Run for 10 seconds at 60fps
        // Begin frame - waits if too many frames in flight
        if (!metal_sync_manager_begin_frame(sync_mgr)) {
            continue; // Frame skipped due to adaptive pacing
        }
        
        // Create command buffer
        metal_command_buffer_t* cmd_buffer = metal_create_command_buffer(device);
        if (!cmd_buffer) continue;
        
        // --- GPU Timestamp Example ---
        uint32_t frame_query = metal_timestamp_begin(timestamps, cmd_buffer, "Full Frame");
        uint32_t render_query = metal_timestamp_begin(timestamps, cmd_buffer, "Render Pass");
        
        // Encode rendering commands here...
        // (render pass encoding would go here)
        
        metal_timestamp_end(timestamps, cmd_buffer, render_query);
        
        uint32_t compute_query = metal_timestamp_begin(timestamps, cmd_buffer, "Post-Process");
        
        // Encode compute pass...
        // (compute encoding would go here)
        
        metal_timestamp_end(timestamps, cmd_buffer, compute_query);
        metal_timestamp_end(timestamps, cmd_buffer, frame_query);
        
        // End frame and submit
        metal_sync_manager_end_frame(sync_mgr, cmd_buffer);
        
        frame_count++;
        
        // Print stats every second
        if (frame_count % 60 == 0) {
            printf("\n--- Frame %u Stats ---\n", frame_count);
            
            metal_frame_timing_stats_t frame_stats;
            metal_hazard_report_t hazard_report;
            metal_vsync_stats_t vsync_stats;
            
            metal_sync_manager_get_stats(sync_mgr, &frame_stats, &hazard_report, &vsync_stats);
            
            printf("FPS: %u, Avg Frame Time: %.2f ms\n", 
                   frame_stats.current_fps, frame_stats.avg_frame_time_ms);
            printf("Frame Drop Rate: %.2f%%\n", 
                   (double)frame_stats.dropped_frames / frame_stats.total_frames * 100.0);
            printf("VSync Jitter: %.3f ms\n", vsync_stats.jitter_ms);
            printf("Hazards Detected: %u, Barriers Inserted: %u\n",
                   hazard_report.total_hazards, hazard_report.barriers_inserted);
        }
    }
    
    // Final report
    printf("\n=== Final Statistics ===\n");
    metal_sync_manager_print_report(sync_mgr);
    
    // Cleanup
    metal_sync_manager_destroy(sync_mgr);
    metal_device_release(device);
}

/**
 * Example: Manual synchronization with individual components
 */
void example_manual_synchronization(void) {
    metal_device_t* device = metal_device_create_system_default();
    id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)metal_get_device(device);
    
    // Create individual sync components
    metal_frame_sync_t* frame_sync = metal_frame_sync_create(mtl_device, 3);
    
    metal_frame_pacing_config_t pacer_config = {
        .target_fps = 144,
        .frame_time_budget_ms = 1000.0 / 144.0,
        .enable_vsync = false,
        .enable_adaptive_pacing = true,
        .stats_window_size = 144,
        .adaptive_threshold_ms = 10.0
    };
    metal_frame_pacer_t* pacer = metal_frame_pacer_create(&pacer_config);
    
    // Render loop
    for (int i = 0; i < 1000; i++) {
        metal_frame_begin(frame_sync);
        metal_frame_pacer_begin_frame(pacer);
        
        // Create command buffer and render...
        metal_command_buffer_t* cmd = metal_create_command_buffer(device);
        
        // Submit
        metal_frame_end(frame_sync, cmd);
        metal_frame_pacer_end_frame(pacer, 0.0);
    }
    
    // Get statistics
    metal_frame_timing_stats_t stats;
    metal_frame_pacer_get_stats(pacer, &stats);
    
    printf("Average FPS: %.2f\n", 1000.0 / stats.avg_frame_time_ms);
    printf("95th percentile: %.2f ms\n", stats.percentile_95_ms);
    printf("99th percentile: %.2f ms\n", stats.percentile_99_ms);
    
    // Cleanup
    metal_frame_pacer_destroy(pacer);
    metal_frame_sync_destroy(frame_sync);
    metal_device_release(device);
}

/**
 * Example: Hazard tracking and automatic barrier insertion
 */
void example_hazard_tracking(void) {
    metal_device_t* device = metal_device_create_system_default();
    id<MTLDevice> mtl_device = (__bridge id<MTLDevice>)metal_get_device(device);
    
    // Create barrier manager
    metal_barrier_manager_t* barriers = metal_barrier_manager_create(
        mtl_device, 
        METAL_BARRIER_POLICY_CONSERVATIVE
    );
    
    // Simulate resource accesses
    void* texture = NULL; // Would be actual MTLTexture
    metal_command_buffer_t* cmd = metal_create_command_buffer(device);
    
    // Write to texture
    metal_resource_access_desc_t write_access = {
        .resource = texture,
        .access_flags = METAL_ACCESS_WRITE,
        .pipeline_stages = METAL_STAGE_FRAGMENT,
        .frame_index = 1,
        .pass_index = 0
    };
    
    uint32_t barriers_inserted = metal_barrier_manager_access_resource(
        barriers, &write_access, cmd
    );
    printf("Barriers inserted after write: %u\n", barriers_inserted);
    
    // Read from texture (should detect RAW hazard)
    metal_resource_access_desc_t read_access = {
        .resource = texture,
        .access_flags = METAL_ACCESS_READ,
        .pipeline_stages = METAL_STAGE_FRAGMENT,
        .frame_index = 1,
        .pass_index = 1
    };
    
    barriers_inserted = metal_barrier_manager_access_resource(
        barriers, &read_access, cmd
    );
    printf("Barriers inserted after read: %u\n", barriers_inserted);
    
    // Get statistics
    uint32_t total_barriers, total_hazards;
    metal_barrier_manager_get_stats(barriers, &total_barriers, &total_hazards);
    
    printf("Total hazards detected: %u\n", total_hazards);
    printf("Total barriers inserted: %u\n", total_barriers);
    
    // Generate report
    metal_hazard_report_t report;
    metal_generate_hazard_report(barriers, &report);
    metal_print_hazard_report(&report);
    
    // Cleanup
    metal_barrier_manager_destroy(barriers);
    metal_device_release(device);
}

int main(int argc, char** argv) {
    printf("=== Metal Synchronization Examples ===\n\n");
    
    printf("Example 1: Complete Frame Rendering\n");
    example_complete_frame_rendering();
    
    printf("\n\nExample 2: Manual Synchronization\n");
    example_manual_synchronization();
    
    printf("\n\nExample 3: Hazard Tracking\n");
    example_hazard_tracking();
    
    return 0;
}
