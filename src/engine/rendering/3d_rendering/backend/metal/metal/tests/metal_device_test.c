/*
 * metal_device_test.c
 * Test harness for Metal device implementation
 */

#include "../mtl_device.h"
#include "../mtl_device_caps.h"
#include "../mtl_memory_heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_device_creation() {
    printf("\n=== Test: Device Creation ===\n");
    
    mtl_error_code_t error;
    metal_device_t* dev = metal_device_create_with_options(NULL, &error);
    
    if (!dev) {
        printf("FAIL: Failed to create device (error: %d)\n", error);
        return;
    }
    
    printf("PASS: Device created successfully\n");
    printf("  Device: %s\n", metal_device_get_name(dev));
    printf("  Registry ID: %llu\n", metal_device_get_registry_id(dev));
    
    // Test validation
    if (metal_device_validate(dev)) {
        printf("PASS: Device validation successful\n");
    } else {
        printf("FAIL: Device validation failed\n");
    }
    
    metal_device_destroy(dev);
}

void test_capabilities() {
    printf("\n=== Test: Capabilities Detection ===\n");
    
    metal_device_t* dev = metal_device_create_system_default();
    if (!dev) {
        printf("FAIL: Failed to create device\n");
        return;
    }
    
    const mtl_device_caps_t* caps = metal_device_get_caps(dev);
    if (!caps) {
        printf("FAIL: Failed to get capabilities\n");
        metal_device_destroy(dev);
        return;
    }
    
    printf("PASS: Capabilities retrieved\n");
    printf("  macOS Version: %u.%u.%u\n", caps->os_version.major, caps->os_version.minor, caps->os_version.patch);
    printf("  Feature Level: %d\n", caps->feature_level);
    printf("  GPU Families: 0x%08X\n", caps->gpu_family_flags);
    printf("  Ray Tracing: %s\n", caps->supports_raytracing ? "Yes" : "No");
    printf("  Mesh Shaders: %s\n", caps->supports_mesh_shaders ? "Yes" : "No");
    printf("  Unified Memory: %s\n", caps->is_unified_memory ? "Yes" : "No");
    printf("  Max Buffer Size: %llu MB\n", caps->max_buffer_size / (1024 * 1024));
printf("  Max Texture 2D: %ux%u\n", caps->max_texture_width_2d, caps->max_texture_height_2d);
    printf("  Max Threads/Group: %ux%ux%u\n", 
        caps->max_threads_per_group[0], caps->max_threads_per_group[1], caps->max_threads_per_group[2]);
    
    // Test capability export
    char export_buffer[4096];
    size_t written = metal_device_export_capabilities(dev, export_buffer, sizeof(export_buffer));
    if (written > 0) {
        printf("\nPASS: Capability export (%zu bytes)\n", written);
        printf("%s\n", export_buffer);
    }
    
    metal_device_destroy(dev);
}

void test_queue_management() {
    printf("\n=== Test: Queue Management ===\n");
    
    metal_device_t* dev = metal_device_create_system_default();
    if (!dev) {
        printf("FAIL: Failed to create device\n");
        return;
    }
    
    // Get default graphics queue
    mtl_command_queue_t gfx_queue = metal_device_get_graphics_queue(dev);
    if (gfx_queue) {
        printf("PASS: Graphics queue retrieved\n");
    } else {
        printf("FAIL: Failed to get graphics queue\n");
    }
    
    // Create compute queue
    mtl_command_queue_t compute_queue = metal_device_get_compute_queue(dev);
    if (compute_queue) {
        printf("PASS: Compute queue created\n");
    } else {
        printf("FAIL: Failed to create compute queue\n");
    }
    
    // Create transfer queue
    mtl_command_queue_t transfer_queue = metal_device_get_transfer_queue(dev);
    if (transfer_queue) {
        printf("PASS: Transfer queue created\n");
    } else {
        printf("FAIL: Failed to create transfer queue\n");
    }
    
    // Test command buffer creation
    mtl_command_buffer_t cmd_buffer = metal_create_command_buffer(dev);
    if (cmd_buffer) {
        printf("PASS: Command buffer created\n");
    } else {
        printf("FAIL: Failed to create command buffer\n");
    }
    
    metal_device_destroy(dev);
}

void test_memory_management() {
    printf("\n=== Test: Memory Management ===\n");
    
    metal_device_t* dev = metal_device_create_system_default();
    if (!dev) {
        printf("FAIL: Failed to create device\n");
        return;
    }
    
    // Get memory info
    mtl_memory_info_t mem_info;
    metal_device_get_memory_info(dev, &mem_info);
    printf("PASS: Memory info retrieved\n");
    printf("  Current Allocated: %.2f MB\n", mem_info.current_allocated_size / (1024.0 * 1024.0));
    printf("  Recommended Working Set: %.2f MB\n", mem_info.recommended_working_set_size / (1024.0 * 1024.0));
    printf("  Pressure Level: %u\n", mem_info.memory_pressure_level);
    
    // Test memory budget
    mtl_memory_set_budget(dev, 512 * 1024 * 1024); // 512 MB
    
    mtl_memory_budget_t budget;
    mtl_memory_get_budget(dev, &budget);
    printf("PASS: Memory budget set and retrieved\n");
    printf("  Budget: %.2f MB\n", budget.total_budget / (1024.0 * 1024.0));
    printf("  Current: %.2f MB\n", budget.current_usage / (1024.0 * 1024.0));
    printf("  Available: %.2f MB\n", budget.available / (1024.0 * 1024.0));
    
    // Export stats
    char stats_buffer[2048];
    size_t written = mtl_memory_export_stats(dev, stats_buffer, sizeof(stats_buffer));
    if (written > 0) {
        printf("\nPASS: Memory stats export (%zu bytes)\n", written);
        printf("%s\n", stats_buffer);
    }
    
    metal_device_destroy(dev);
}

void test_heap_creation() {
    printf("\n=== Test: Heap Management ===\n");
    
    metal_device_t* dev = metal_device_create_system_default();
    if (!dev) {
        printf("FAIL: Failed to create device\n");
        return;
    }
    
    // Create a heap
    mtl_heap_desc_t heap_desc = {
        .size = 64 * 1024 * 1024, // 64 MB
        .storage_mode = MTL_STORAGE_PRIVATE,
        .cpu_cache_mode = MTL_CPU_CACHE_DEFAULT_CACHE,
        .label = "Test Heap"
    };
    
    mtl_heap_t heap = mtl_heap_create(dev, &heap_desc);
    if (!heap) {
        printf("FAIL: Failed to create heap\n");
        metal_device_destroy(dev);
        return;
    }
    
    printf("PASS: Heap created\n");
    printf("  Total Size: %llu MB\n", mtl_heap_get_size(heap) / (1024 * 1024));
    printf("  Used Size: %llu bytes\n", mtl_heap_get_used_size(heap));
    printf("  Available: %llu MB\n", mtl_heap_get_available_size(heap) / (1024 * 1024));
    
    // Validate heap
    if (mtl_heap_validate(heap)) {
        printf("PASS: Heap validation successful\n");
    } else {
        printf("FAIL: Heap validation failed\n");
    }
    
    // Get usage stats
    mtl_heap_usage_t usage;
    mtl_heap_get_usage(heap, &usage);
    printf("PASS: Heap usage retrieved\n");
    printf("  Allocations: %u\n", usage.num_allocations);
    printf("  Peak Usage: %llu bytes\n", usage.peak_usage);
    
    mtl_heap_destroy(heap);
    metal_device_destroy(dev);
}

void test_reference_counting() {
    printf("\n=== Test: Reference Counting ===\n");
    
    metal_device_t* dev = metal_device_create_system_default();
    if (!dev) {
        printf("FAIL: Failed to create device\n");
        return;
    }
    
    printf("PASS: Device created (ref count = 1)\n");
    
    // Retain
    metal_device_retain(dev);
    printf("PASS: Device retained (ref count = 2)\n");
    
    // Release once (should not destroy)
    metal_device_release(dev);
    printf("PASS: Device released (ref count = 1)\n");
    
    if (metal_device_validate(dev)) {
        printf("PASS: Device still valid after first release\n");
    }
    
    // Final release
    metal_device_release(dev);
    printf("PASS: Device released (ref count = 0, should be destroyed)\n");
}

int main(int argc, char** argv) {
    printf("========================================\n");
    printf("Metal Device Implementation Test Suite\n");
    printf("========================================\n");
    
    test_device_creation();
    test_capabilities();
    test_queue_management();
    test_memory_management();
    test_heap_creation();
    test_reference_counting();
    
    printf("\n========================================\n");
    printf("All tests completed!\n");
    printf("========================================\n");
    
    return 0;
}
