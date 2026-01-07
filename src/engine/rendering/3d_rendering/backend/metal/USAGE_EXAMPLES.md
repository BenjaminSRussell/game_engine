# Metal Pipeline Usage Examples

## Complete Examples

### Example 1: Basic Render Pipeline Creation

```c
#include "mtl_pipeline.h"
#include "mtl_device.h"

void create_basic_render_pipeline_example(void) {
    // 1. Initialize pipeline system
    metal_pipeline_init();
    
    // 2. Get Metal device
    MTLDeviceRef device = metal_device_create();
    if (!device) {
        fprintf(stderr, "Failed to create Metal device\n");
        return;
    }
    
    // 3. Load shader library
    metal_shader_library_t *lib = 
        metal_load_shader_library(device, "assets/shaders/basic.metallib");
    if (!lib) {
        fprintf(stderr, "Failed to load shader library\n");
        return;
    }
    
    // 4. Get shader functions
    MTLFunctionRef vertFunc = metal_get_function(lib, "vertexMain");
    MTLFunctionRef fragFunc = metal_get_function(lib, "fragmentMain");
    
    // 5. Setup vertex descriptor
    metal_vertex_descriptor_t vertexDesc;
    metal_vertex_descriptor_init(&vertexDesc);
    
    // Position: float3 at location 0, offset 0
    metal_vertex_descriptor_add_attribute(&vertexDesc, 0, 
        METAL_VERTEX_FORMAT_FLOAT3, 0, 0);
    
    // Normal: float3 at location 1, offset 12
    metal_vertex_descriptor_add_attribute(&vertexDesc, 1,
        METAL_VERTEX_FORMAT_FLOAT3, 12, 0);
    
    // UV: float2 at location 2, offset 24
    metal_vertex_descriptor_add_attribute(&vertexDesc, 2,
        METAL_VERTEX_FORMAT_FLOAT2, 24, 0);
    
    // Buffer layout: stride = 32 bytes (3*4 + 3*4 + 2*4), per-vertex
    metal_vertex_descriptor_set_layout(&vertexDesc, 0, 32, 0);
    
    // 6. Create pipeline descriptor
    metal_render_pipeline_desc_t pipelineDesc = {
        .vertex_function = vertFunc,
        .fragment_function = fragFunc,
        .color_format = METAL_PIXEL_FORMAT_BGRA8_UNORM,
        .depth_format = METAL_PIXEL_FORMAT_DEPTH32_FLOAT,
        .stencil_format = METAL_PIXEL_FORMAT_INVALID,
        .vertex_descriptor = vertexDesc,
        .cull_mode = METAL_CULL_MODE_BACK,
        .fill_mode = METAL_FILL_MODE_FILL,
        .winding_order = METAL_WINDING_COUNTER_CLOCKWISE,
        .sample_count = 1,  // No MSAA
        .depth_clip_enabled = true,
        .conservative_rasterization_enabled = false
    };
    
    // 7. Setup blend state (optional - for transparency)
    pipelineDesc.blend_state.blend_enabled = false;  // Opaque rendering
    
    // 8. Create pipeline with caching
    metal_pipeline_cache_t cache;
    metal_pipeline_cache_init(&cache);
    metal_pipeline_cache_set_memory_limit(&cache, 32 * 1024 * 1024); // 32MB
    
    metal_render_pipeline_t *pipeline = 
        metal_pipeline_cache_get_render(&cache, device, &pipelineDesc);
    
    if (!pipeline) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return;
    }
    
    // 9. Get statistics
    metal_render_pipeline_stats_t stats = 
        metal_render_pipeline_get_stats(pipeline);
    
    printf("Pipeline created successfully!\n");
    printf("  Memory usage: %zu bytes\n", stats.estimated_memory_bytes);
    printf("  Has vertex shader: %s\n", stats.has_vertex_shader ? "yes" : "no");
    printf("  Has fragment shader: %s\n", stats.has_fragment_shader ? "yes" : "no");
    printf("  Uses depth testing: %s\n", stats.uses_depth_testing ? "yes" : "no");
    
    // 10. Check cache statistics
    metal_pipeline_cache_stats_t cache_stats = 
        metal_pipeline_cache_get_stats(&cache);
    
    printf("\nCache statistics:\n");
    printf("  Total pipelines: %u\n", cache_stats.total_pipelines);
    printf("  Cache hits: %u\n", cache_stats.cache_hits);
    printf("  Cache misses: %u\n", cache_stats.cache_misses);
    printf("  Memory used: %zu bytes\n", cache_stats.total_memory_bytes);
    
    // 11. Cleanup
    metal_pipeline_cache_shutdown(&cache);
    metal_destroy_shader_library(lib);
    metal_pipeline_shutdown();
}
```

### Example 2: Compute Pipeline with Validation

```c
#include "mtl_pipeline.h"

void create_compute_pipeline_with_validation(void) {
    MTLDeviceRef device = metal_device_create();
    
    // Load compute shader
    metal_shader_library_manager_t *mgr = 
        metal_shader_library_manager_create(device);
    
    MTLLibraryRef lib = 
        metal_library_manager_load_file(mgr, "assets/shaders/compute.metallib");
    
    MTLFunctionRef computeFunc = 
        metal_library_get_function(lib, "particleUpdate");
    
    // Query hardware capabilities first
    uint32_t max_threads = metal_get_max_threads_per_threadgroup(device);
    printf("Device supports up to %u threads per threadgroup\n", max_threads);
    
    // Create descriptor
    metal_compute_pipeline_desc_t desc = {
        .compute_function = computeFunc,
        .threadgroup_size_x = 256,  // 16x16 = 256 threads
        .threadgroup_size_y = 1,
        .threadgroup_size_z = 1,
        .threadgroup_memory_length = 16384  // 16KB shared memory
    };
    
    // Validate before creating
    char error_message[512];
    if (!metal_compute_pipeline_validate(device, &desc, 
                                        error_message, sizeof(error_message))) {
        fprintf(stderr, "Pipeline validation failed: %s\n", error_message);
        
        // Adjust and retry
        desc.threadgroup_size_x = 128;
        if (metal_compute_pipeline_validate(device, &desc, 
                                           error_message, sizeof(error_message))) {
            printf("Adjusted threadgroup size to 128 - validation passed\n");
        } else {
            fprintf(stderr, "Still invalid: %s\n", error_message);
            return;
        }
    }
    
    // Create pipeline
    metal_compute_pipeline_t *pipeline = 
        metal_create_compute_pipeline(device, &desc);
    
    if (pipeline) {
        metal_compute_pipeline_stats_t stats = 
            metal_compute_pipeline_get_stats(pipeline);
        
        printf("Compute pipeline created!\n");
        printf("  Max threads per threadgroup: %u\n", 
               stats.max_total_threads_per_threadgroup);
        printf("  Estimated memory: %zu bytes\n", 
               stats.estimated_memory_bytes);
        
        metal_destroy_compute_pipeline(pipeline);
    }
    
    metal_library_manager_destroy(mgr);
}
```

### Example 3: Alpha Blending Pipeline

```c
void create_alpha_blending_pipeline(MTLDeviceRef device) {
    metal_render_pipeline_desc_t desc = {/* ... basic setup ... */};
    
    // Configure alpha blending
    desc.blend_state.blend_enabled = true;
    desc.blend_state.src_rgb_blend = METAL_BLEND_SRC_ALPHA;
    desc.blend_state.dst_rgb_blend = METAL_BLEND_ONE_MINUS_SRC_ALPHA;
    desc.blend_state.rgb_blend_op = METAL_BLEND_OP_ADD;
    desc.blend_state.src_alpha_blend = METAL_BLEND_ONE;
    desc.blend_state.dst_alpha_blend = METAL_BLEND_ZERO;
    desc.blend_state.alpha_blend_op = METAL_BLEND_OP_ADD;
    
    // Disable depth writes for transparent objects
    desc.depth_format = METAL_PIXEL_FORMAT_DEPTH32_FLOAT;
    // Note: depth write is controlled in depth/stencil state
    
    metal_render_pipeline_t *pipeline = 
        metal_create_render_pipeline(device, &desc);
}
```

### Example 4: MSAA (Multisample Anti-Aliasing)

```c
void create_msaa_pipeline(MTLDeviceRef device) {
    metal_render_pipeline_desc_t desc = {/* ... basic setup ... */};
    
    // Enable 4x MSAA
    desc.sample_count = 4;  // Can be 1, 2, 4, or 8
    
    metal_render_pipeline_t *pipeline = 
        metal_create_render_pipeline(device, &desc);
}
```

### Example 5: Depth/Stencil State

```c
void create_depth_stencil_state_example(MTLDeviceRef device) {
    // Standard depth testing
    metal_depth_stencil_desc_t desc = {
        .depth_compare = METAL_COMPARE_LESS,  // Pass if closer
        .depth_write_enabled = true,
        .stencil_enabled = false
    };
    
    metal_depth_stencil_state_t *state = 
        metal_create_depth_stencil_state(device, &desc);
    
    // Use this state when binding to encoder
    // (This would be done in encoder code, not shown here)
    
    metal_destroy_depth_stencil_state(state);
}
```

### Example 6: Shader Library Hot Reload

```c
void setup_hot_reload_system(void) {
    MTLDeviceRef device = metal_device_create();
    
    metal_shader_library_manager_t *mgr = 
        metal_shader_library_manager_create(device);
    
    metal_pipeline_cache_tcache;
    metal_pipeline_cache_init(&cache);
    
    // Load initial shaders
    metal_library_manager_load_file(mgr, "assets/shaders/main.metallib");
    
    // In your game loop or dev mode watcher:
    while (game_is_running()) {
        // Check for shader changes (e.g., every frame in dev mode)
        if (metal_library_manager_check_for_changes(mgr)) {
            printf("Shader files changed - reloading...\n");
            
            uint32_t reloaded = metal_library_manager_reload_changed(mgr);
            printf("Reloaded %u shader libraries\n", reloaded);
            
            // Invalidate all cached pipelines that use these shaders
            metal_pipeline_cache_invalidate_all(&cache);
            printf("Pipeline cache cleared\n");
            
            // Pipelines will be recreated on next use
        }
        
        // ... rest of game loop
    }
    
    metal_pipeline_cache_shutdown(&cache);
    metal_library_manager_destroy(mgr);
}
```

### Example 7: Cache Management

```c
void manage_pipeline_cache(void) {
    metal_pipeline_cache_t cache;
    metal_pipeline_cache_init(&cache);
    
    // Set memory limit (16MB)
    metal_pipeline_cache_set_memory_limit(&cache, 16 * 1024 * 1024);
    
    // Create lots of pipelines... they get cached automatically
    // when using metal_pipeline_cache_get_render/compute()
    
    // Remove a specific pipeline by hash
    metal_render_pipeline_desc_t desc = {/* ... */};
    uint64_t hash = metal_hash_render_pipeline_desc(&desc);
    
    if (metal_pipeline_cache_invalidate_by_hash(&cache, hash)) {
        printf("Pipeline removed from cache\n");
    }
    
    // Check cache effectiveness
    metal_pipeline_cache_stats_t stats = metal_pipeline_cache_get_stats(&cache);
    
    float hit_rate = 0.0f;
    if (stats.cache_hits + stats.cache_misses > 0) {
        hit_rate = 100.0f * stats.cache_hits / 
                   (stats.cache_hits + stats.cache_misses);
    }
    
    printf("Cache Statistics:\n");
    printf("  Hit rate: %.1f%%\n", hit_rate);
    printf("  Total pipelines: %u\n", stats.total_pipelines);
    printf("  Memory used: %.2f MB\n", 
           stats.total_memory_bytes / (1024.0 * 1024.0));
    printf("  Evictions: %u\n", stats.evictions);
    
    // Advance frame counter for LRU tracking
    metal_pipeline_cache_advance_frame(&cache);
    
    metal_pipeline_cache_shutdown(&cache);
}
```

### Example 8: Performance Monitoring

```c
void monitor_pipeline_performance(void) {
    metal_pipeline_cache_t cache;
    metal_pipeline_cache_init(&cache);
    
    // Periodically log statistics
    void log_cache_stats() {
        metal_pipeline_cache_stats_t stats = 
            metal_pipeline_cache_get_stats(&cache);
        
        printf("=== Pipeline Cache Stats ===\n");
        printf("Pipelines: %u\n", stats.total_pipelines);
        printf("Hits: %u, Misses: %u\n", stats.cache_hits, stats.cache_misses);
        printf("Hit rate: %.1f%%\n", 
               100.0f * stats.cache_hits / 
               (stats.cache_hits + stats.cache_misses + 1));
        printf("Memory: %.2f MB\n", 
               stats.total_memory_bytes / (1024.0 * 1024.0));
        printf("Evictions: %u\n", stats.evictions);
    }
    
    // Call periodically (e.g., every 60 frames)
    static int frame_count = 0;
    if (++frame_count >= 60) {
        log_cache_stats();
        frame_count = 0;
    }
}
```

## Best Practices

### 1. Always Validate Compute Pipelines
```c
// GOOD: Validate before creating
if (metal_compute_pipeline_validate(device, &desc, error, sizeof(error))) {
    pipeline = metal_create_compute_pipeline(device, &desc);
}

// BAD: Create without validation
pipeline = metal_create_compute_pipeline(device, &desc);  // Might fail!
```

### 2. Use Pipeline Cache
```c
// GOOD: Use cache for automatic reuse
pipeline = metal_pipeline_cache_get_render(&cache, device, &desc);

// BAD: Create new pipeline every time
pipeline = metal_create_render_pipeline(device, &desc);  // Slow!
```

### 3. Set Memory Limits
```c
// Prevent unbounded cache growth
metal_pipeline_cache_set_memory_limit(&cache, 32 * 1024 * 1024);  // 32MB
```

### 4. Monitor Cache Hit Rates
```c
// Aim for >80% hit rate in production
metal_pipeline_cache_stats_t stats = metal_pipeline_cache_get_stats(&cache);
float hit_rate = 100.0f * stats.cache_hits / (stats.cache_hits + stats.cache_misses);

if (hit_rate < 80.0f) {
    printf("WARNING: Low cache hit rate: %.1f%%\n", hit_rate);
}
```

### 5. Clean Up Resources
```c
// Always destroy pipelines and caches
metal_destroy_render_pipeline(pipeline);
metal_pipeline_cache_shutdown(&cache);
metal_pipeline_shutdown();
```
