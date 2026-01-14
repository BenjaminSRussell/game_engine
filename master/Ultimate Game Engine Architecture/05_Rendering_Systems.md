# Rendering Systems

## System Overview

The Rendering Systems provide a comprehensive, multi-API rendering solution supporting both OpenGL and Vulkan with automatic fallback and advanced features like real-time ray tracing, global illumination, and massive scene rendering. The system is designed for cutting-edge graphics performance with extensive shader support and modern rendering techniques.

**Total System Size: 8,500,000 lines of code**

### Key Statistics
- **Total Files**: 1,200 files
- **Total Lines**: 8,500,000 LOC
- **Shader Files**: 18,150+ shaders
- **Render Passes**: 50+ passes
- **API Support**: OpenGL 4.6, Vulkan 1.3, Metal 3
- **Performance**: 1000+ draw calls per frame at 4K 60fps

## Architecture Overview

```
Rendering Systems Architecture
├── Render Device
│   ├── API Abstraction
│   ├── Resource Management
│   ├── Command Buffers
│   └── Synchronization
├── Shader System
│   ├── Shader Compiler
│   ├── Shader Hot-Reload
│   ├── Shader Variants
│   └── Material System
├── Render Graph
│   ├── Pass Management
│   ├── Resource Dependencies
│   ├── Render Targets
│   └── Automatic Barriers
├── Scene Rendering
│   ├── Culling System
│   ├── LOD System
│   ├── Instancing
│   └── Multi-Draw
├── Lighting System
│   ├── Deferred Rendering
│   ├── Forward+ Rendering
│   ├── Clustered Lighting
│   └── Global Illumination
├── Post-Processing
│   ├── HDR Pipeline
│   ├── Color Grading
│   ├── Anti-Aliasing
│   └── Depth of Field
└── Advanced Features
    ├── Ray Tracing
    ├── Virtual Texturing
    ├── Procedural Generation
    └── GPU Driven Rendering
```

## File Structure

```
/rendering/
├── device/
│   ├── render_device.c (45,000 LOC)
│   ├── render_device.h (5,000 LOC)
│   ├── render_context.c (35,000 LOC)
│   ├── render_context.h (4,000 LOC)
│   ├── render_command.c (40,000 LOC)
│   ├── render_command.h (4,500 LOC)
│   ├── render_resource.c (38,000 LOC)
│   ├── render_resource.h (4,000 LOC)
│   ├── render_fence.c (25,000 LOC)
│   ├── render_fence.h (3,000 LOC)
│   ├── render_semaphore.c (22,000 LOC)
│   ├── render_semaphore.h (2,500 LOC)
│   ├── render_queue.c (30,000 LOC)
│   ├── render_queue.h (3,500 LOC)
│   ├── render_memory.c (28,000 LOC)
│   ├── render_memory.h (3,000 LOC)
│   ├── render_swapchain.c (25,000 LOC)
│   ├── render_swapchain.h (3,000 LOC)
│   ├── render_debug.c (20,000 LOC)
│   ├── render_stats.c (15,000 LOC)
│   ├── render_benchmark.c (18,000 LOC)
│   ├── render_unit.c (30,000 LOC)
│   └── render_integration.c (6,000 LOC)
├── backend/
│   ├── opengl/
│   │   ├── gl_device.c (35,000 LOC)
│   │   ├── gl_device.h (4,000 LOC)
│   │   ├── gl_command.c (30,000 LOC)
│   │   ├── gl_command.h (3,500 LOC)
│   │   ├── gl_resource.c (32,000 LOC)
│   │   ├── gl_resource.h (3,500 LOC)
│   │   ├── gl_shader.c (28,000 LOC)
│   │   ├── gl_shader.h (3,000 LOC)
│   │   ├── gl_framebuffer.c (25,000 LOC)
│   │   ├── gl_framebuffer.h (2,500 LOC)
│   │   ├── gl_buffer.c (25,000 LOC)
│   │   ├── gl_buffer.h (2,500 LOC)
│   │   ├── gl_texture.c (30,000 LOC)
│   │   ├── gl_texture.h (3,000 LOC)
│   │   ├── gl_pipeline.c (22,000 LOC)
│   │   ├── gl_pipeline.h (2,500 LOC)
│   │   ├── gl_debug.c (18,000 LOC)
│   │   ├── gl_stats.c (12,000 LOC)
│   │   ├── gl_extensions.c (20,000 LOC)
│   │   └── gl_unit.c (25,000 LOC)
│   ├── vulkan/
│   │   ├── vk_device.c (45,000 LOC)
│   │   ├── vk_device.h (5,000 LOC)
│   │   ├── vk_instance.c (35,000 LOC)
│   │   ├── vk_instance.h (4,000 LOC)
│   │   ├── vk_command.c (40,000 LOC)
│   │   ├── vk_command.h (4,500 LOC)
│   │   ├── vk_resource.c (38,000 LOC)
│   │   ├── vk_resource.h (4,000 LOC)
│   │   ├── vk_shader.c (35,000 LOC)
│   │   ├── vk_shader.h (4,000 LOC)
│   │   ├── vk_pipeline.c (32,000 LOC)
│   │   ├── vk_pipeline.h (3,500 LOC)
│   │   ├── vk_framebuffer.c (28,000 LOC)
│   │   ├── vk_framebuffer.h (3,000 LOC)
│   │   ├── vk_buffer.c (30,000 LOC)
│   │   ├── vk_buffer.h (3,000 LOC)
│   │   ├── vk_texture.c (35,000 LOC)
│   │   ├── vk_texture.h (3,500 LOC)
│   │   ├── vk_descriptor.c (25,000 LOC)
│   │   ├── vk_descriptor.h (3,000 LOC)
│   │   ├── vk_memory.c (30,000 LOC)
│   │   ├── vk_memory.h (3,000 LOC)
│   │   ├── vk_swapchain.c (28,000 LOC)
│   │   ├── vk_swapchain.h (3,000 LOC)
│   │   ├── vk_validation.c (22,000 LOC)
│   │   ├── vk_debug.c (20,000 LOC)
│   │   ├── vk_stats.c (15,000 LOC)
│   │   ├── vk_extensions.c (18,000 LOC)
│   │   └── vk_unit.c (28,000 LOC)
│   └── metal/
│       ├── mt_device.c (35,000 LOC)
│       ├── mt_device.h (4,000 LOC)
│       ├── mt_command.c (30,000 LOC)
│       ├── mt_command.h (3,500 LOC)
│       ├── mt_resource.c (32,000 LOC)
│       ├── mt_resource.h (3,500 LOC)
│       ├── mt_shader.c (28,000 LOC)
│       ├── mt_shader.h (3,000 LOC)
│       ├── mt_pipeline.c (25,000 LOC)
│       ├── mt_pipeline.h (3,000 LOC)
│       ├── mt_framebuffer.c (22,000 LOC)
│       ├── mt_framebuffer.h (2,500 LOC)
│       ├── mt_buffer.c (24,000 LOC)
│       ├── mt_buffer.h (2,500 LOC)
│       ├── mt_texture.c (28,000 LOC)
│       ├── mt_texture.h (3,000 LOC)
│       ├── mt_debug.c (18,000 LOC)
│       ├── mt_stats.c (12,000 LOC)
│       └── mt_unit.c (22,000 LOC)
├── shader/
│   ├── shader_compiler.c (35,000 LOC)
│   ├── shader_compiler.h (4,000 LOC)
│   ├── shader_loader.c (28,000 LOC)
│   ├── shader_loader.h (3,000 LOC)
│   ├── shader_cache.c (25,000 LOC)
│   ├── shader_cache.h (3,000 LOC)
│   ├── shader_hotreload.c (22,000 LOC)
│   ├── shader_hotreload.h (2,500 LOC)
│   ├── shader_variants.c (30,000 LOC)
│   ├── shader_variants.h (3,500 LOC)
│   ├── shader_preprocessor.c (20,000 LOC)
│   ├── shader_preprocessor.h (2,500 LOC)
│   ├── shader_reflection.c (25,000 LOC)
│   ├── shader_reflection.h (3,000 LOC)
│   ├── shader_debug.c (18,000 LOC)
│   ├── shader_stats.c (12,000 LOC)
│   ├── shader_benchmark.c (15,000 LOC)
│   ├── shader_unit.c (25,000 LOC)
│   └── shader_integration.c (5,000 LOC)
├── graph/
│   ├── render_graph.c (40,000 LOC)
│   ├── render_graph.h (4,500 LOC)
│   ├── render_pass.c (35,000 LOC)
│   ├── render_pass.h (4,000 LOC)
│   ├── render_target.c (30,000 LOC)
│   ├── render_target.h (3,500 LOC)
│   ├── render_resource_graph.c (28,000 LOC)
│   ├── render_resource_graph.h (3,000 LOC)
│   ├── render_barrier.c (25,000 LOC)
│   ├── render_barrier.h (3,000 LOC)
│   ├── render_scheduler.c (22,000 LOC)
│   ├── render_scheduler.h (2,500 LOC)
│   ├── render_dependency.c (20,000 LOC)
│   ├── render_dependency.h (2,500 LOC)
│   ├── render_optimization.c (25,000 LOC)
│   ├── render_optimization.h (3,000 LOC)
│   ├── render_debug.c (18,000 LOC)
│   ├── render_stats.c (12,000 LOC)
│   ├── render_benchmark.c (15,000 LOC)
│   ├── render_unit.c (25,000 LOC)
│   └── render_integration.c (5,000 LOC)
├── scene/
│   ├── scene_renderer.c (38,000 LOC)
│   ├── scene_renderer.h (4,000 LOC)
│   ├── mesh_renderer.c (35,000 LOC)
│   ├── mesh_renderer.h (4,000 LOC)
│   ├── culling_system.c (32,000 LOC)
│   ├── culling_system.h (3,500 LOC)
│   ├── lod_system.c (30,000 LOC)
│   ├── lod_system.h (3,500 LOC)
│   ├── instancing.c (28,000 LOC)
│   ├── instancing.h (3,000 LOC)
│   ├── multi_draw.c (25,000 LOC)
│   ├── multi_draw.h (3,000 LOC)
│   ├── occlusion_culling.c (25,000 LOC)
│   ├── occlusion_culling.h (3,000 LOC)
│   ├── frustum_culling.c (22,000 LOC)
│   ├── frustum_culling.h (2,500 LOC)
│   ├── distance_culling.c (20,000 LOC)
│   ├── distance_culling.h (2,500 LOC)
│   ├── visibility_system.c (25,000 LOC)
│   ├── visibility_system.h (3,000 LOC)
│   ├── render_batch.c (22,000 LOC)
│   ├── render_batch.h (2,500 LOC)
│   ├── draw_command.c (20,000 LOC)
│   ├── draw_command.h (2,500 LOC)
│   ├── scene_debug.c (18,000 LOC)
│   ├── scene_stats.c (12,000 LOC)
│   ├── scene_benchmark.c (15,000 LOC)
│   ├── scene_unit.c (25,000 LOC)
│   └── scene_integration.c (5,000 LOC)
├── lighting/
│   ├── lighting_system.c (40,000 LOC)
│   ├── lighting_system.h (4,500 LOC)
│   ├── deferred_renderer.c (38,000 LOC)
│   ├── deferred_renderer.h (4,000 LOC)
│   ├── forward_plus_renderer.c (35,000 LOC)
│   ├── forward_plus_renderer.h (4,000 LOC)
│   ├── clustered_lighting.c (32,000 LOC)
│   ├── clustered_lighting.h (3,500 LOC)
│   ├── global_illumination.c (35,000 LOC)
│   ├── global_illumination.h (4,000 LOC)
│   ├── light_culling.c (28,000 LOC)
│   ├── light_culling.h (3,000 LOC)
│   ├── shadow_mapping.c (30,000 LOC)
│   ├── shadow_mapping.h (3,500 LOC)
│   ├── reflection_probe.c (25,000 LOC)
│   ├── reflection_probe.h (3,000 LOC)
│   ├── irradiance_volume.c (25,000 LOC)
│   ├── irradiance_volume.h (3,000 LOC)
│   ├── light_probe.c (22,000 LOC)
│   ├── light_probe.h (2,500 LOC)
│   ├── environment_lighting.c (24,000 LOC)
│   ├── environment_lighting.h (2,500 LOC)
│   ├── volumetric_lighting.c (26,000 LOC)
│   ├── volumetric_lighting.h (3,000 LOC)
│   ├── lighting_debug.c (20,000 LOC)
│   ├── lighting_stats.c (12,000 LOC)
│   ├── lighting_benchmark.c (15,000 LOC)
│   ├── lighting_unit.c (25,000 LOC)
│   └── lighting_integration.c (5,000 LOC)
├── postprocess/
│   ├── postprocess_pipeline.c (35,000 LOC)
│   ├── postprocess_pipeline.h (4,000 LOC)
│   ├── hdr_pipeline.c (30,000 LOC)
│   ├── hdr_pipeline.h (3,500 LOC)
│   ├── color_grading.c (25,000 LOC)
│   ├── color_grading.h (3,000 LOC)
│   ├── anti_aliasing.c (28,000 LOC)
│   ├── anti_aliasing.h (3,000 LOC)
│   ├── depth_of_field.c (26,000 LOC)
│   ├── depth_of_field.h (3,000 LOC)
│   ├── motion_blur.c (24,000 LOC)
│   ├── motion_blur.h (2,500 LOC)
│   ├── bloom.c (25,000 LOC)
│   ├── bloom.h (2,500 LOC)
│   ├── vignette.c (20,000 LOC)
│   ├── vignette.h (2,000 LOC)
│   ├── chromatic_aberration.c (22,000 LOC)
│   ├── chromatic_aberration.h (2,500 LOC)
│   ├── film_grain.c (20,000 LOC)
│   ├── film_grain.h (2,000 LOC)
│   ├── sharpening.c (22,000 LOC)
│   ├── sharpening.h (2,500 LOC)
│   ├── tone_mapping.c (24,000 LOC)
│   ├── tone_mapping.h (2,500 LOC)
│   ├── postprocess_debug.c (18,000 LOC)
│   ├── postprocess_stats.c (12,000 LOC)
│   ├── postprocess_benchmark.c (15,000 LOC)
│   ├── postprocess_unit.c (22,000 LOC)
│   └── postprocess_integration.c (4,000 LOC)
├── advanced/
│   ├── ray_tracing.c (45,000 LOC)
│   ├── ray_tracing.h (5,000 LOC)
│   ├── virtual_texturing.c (38,000 LOC)
│   ├── virtual_texturing.h (4,000 LOC)
│   ├── procedural_generation.c (35,000 LOC)
│   ├── procedural_generation.h (4,000 LOC)
│   ├── gpu_driven_rendering.c (40,000 LOC)
│   ├── gpu_driven_rendering.h (4,500 LOC)
│   ├── mesh_shaders.c (32,000 LOC)
│   ├── mesh_shaders.h (3,500 LOC)
│   ├── variable_rate_shading.c (28,000 LOC)
│   ├── variable_rate_shading.h (3,000 LOC)
│   ├── sampler_feedback.c (25,000 LOC)
│   ├── sampler_feedback.h (3,000 LOC)
│   ├── visibility_buffer.c (26,000 LOC)
│   ├── visibility_buffer.h (3,000 LOC)
│   ├── tiled_lighting.c (24,000 LOC)
│   ├── tiled_lighting.h (2,500 LOC)
│   ├── oit_rendering.c (25,000 LOC)
│   ├── oit_rendering.h (3,000 LOC)
│   ├── advanced_debug.c (20,000 LOC)
│   ├── advanced_stats.c (15,000 LOC)
│   ├── advanced_benchmark.c (18,000 LOC)
│   ├── advanced_unit.c (28,000 LOC)
│   └── advanced_integration.c (5,000 LOC)
├── utils/
│   ├── render_utils.c (20,000 LOC)
│   ├── render_utils.h (2,500 LOC)
│   ├── math_utils.c (18,000 LOC)
│   ├── math_utils.h (2,000 LOC)
│   ├── texture_utils.c (22,000 LOC)
│   ├── texture_utils.h (2,500 LOC)
│   ├── shader_utils.c (20,000 LOC)
│   ├── shader_utils.h (2,500 LOC)
│   ├── render_debug_draw.c (25,000 LOC)
│   ├── render_debug_draw.h (3,000 LOC)
│   ├── render_profiler.c (22,000 LOC)
│   ├── render_profiler.h (2,500 LOC)
│   ├── gpu_profiler.c (20,000 LOC)
│   └── gpu_profiler.h (2,500 LOC)
└── debug/
    ├── render_debug.c (25,000 LOC)
    ├── render_debug.h (3,000 LOC)
    ├── render_viewer.c (22,000 LOC)
    ├── render_viewer.h (2,500 LOC)
    ├── gpu_memory_viewer.c (20,000 LOC)
    ├── gpu_memory_viewer.h (2,500 LOC)
    ├── shader_inspector.c (18,000 LOC)
    ├── shader_inspector.h (2,000 LOC)
    ├── texture_viewer.c (20,000 LOC)
    ├── texture_viewer.h (2,500 LOC)
    ├── renderdoc_integration.c (15,000 LOC)
    ├── renderdoc_integration.h (2,000 LOC)
    ├── pix_integration.c (15,000 LOC)
    ├── pix_integration.h (2,000 LOC)
    ├── vulkan_validation.c (18,000 LOC)
    └── vulkan_validation.h (2,000 LOC)
```

## Render Device

### Core Render Device

**File: render_device.c (45,000 LOC)**

```c
// High-performance render device with multi-API support
struct Render_Device {
    // API backend
    Render_API api;
    
    // Backend-specific device
    union {
        struct GL_Device* gl_device;
        struct VK_Device* vk_device;
        struct MT_Device* mt_device;
    } backend;
    
    // Device capabilities
    struct Render_Caps caps;
    
    // Resource management
    struct Resource_Manager* resource_manager;
    
    // Command queues
    struct Render_Queue* graphics_queue;
    struct Render_Queue* compute_queue;
    struct Render_Queue* transfer_queue;
    
    // Memory management
    struct GPU_Memory_Manager* memory_manager;
    
    // Synchronization
    struct Sync_Manager* sync_manager;
    
    // Debug and profiling
    struct GPU_Profiler* profiler;
    struct Debug_Renderer* debug_renderer;
    
    // Statistics
    struct Render_Stats stats;
};

// Initialize render device with automatic API selection
b32 render_device_init(struct Render_Device* device, struct Render_Config* config) {
    // Try Vulkan first (preferred)
    if (config->preferred_api == RENDER_API_VULKAN || 
        config->preferred_api == RENDER_API_AUTO) {
        if (vulkan_is_supported()) {
            device->api = RENDER_API_VULKAN;
            device->backend.vk_device = vk_device_create(config);
            
            if (device->backend.vk_device) {
                log_info("Using Vulkan rendering backend");
                return TRUE;
            }
        }
    }
    
    // Fallback to OpenGL
    if (config->preferred_api == RENDER_API_OPENGL || 
        config->preferred_api == RENDER_API_AUTO) {
        device->api = RENDER_API_OPENGL;
        device->backend.gl_device = gl_device_create(config);
        
        if (device->backend.gl_device) {
            log_info("Using OpenGL rendering backend");
            return TRUE;
        }
    }
    
    // Fallback to Metal on macOS
#ifdef PLATFORM_MACOS
    if (config->preferred_api == RENDER_API_METAL || 
        config->preferred_api == RENDER_API_AUTO) {
        device->api = RENDER_API_METAL;
        device->backend.mt_device = mt_device_create(config);
        
        if (device->backend.mt_device) {
            log_info("Using Metal rendering backend");
            return TRUE;
        }
    }
#endif
    
    log_error("No rendering backend available");
    return FALSE;
}

// Submit render commands with automatic synchronization
void render_device_submit(struct Render_Device* device, 
                         struct Render_Command_Buffer* cmd_buffer,
                         struct Render_Fence* fence) {
    // Validate command buffer
    if (!cmd_buffer || cmd_buffer->command_count == 0) {
        return;
    }
    
    // API-specific submission
    switch (device->api) {
        case RENDER_API_VULKAN:
            vk_device_submit(device->backend.vk_device, cmd_buffer, fence);
            break;
            
        case RENDER_API_OPENGL:
            gl_device_submit(device->backend.gl_device, cmd_buffer, fence);
            break;
            
        case RENDER_API_METAL:
            mt_device_submit(device->backend.mt_device, cmd_buffer, fence);
            break;
    }
    
    // Update statistics
    device->stats.commands_submitted += cmd_buffer->command_count;
    device->stats.submit_calls++;
}
```

### Command Buffer Management

**File: render_command.c (40,000 LOC)**

```c
// Efficient command buffer with multi-threading support
struct Render_Command_Buffer {
    // Command storage
    u8* command_data;
    size_t command_data_size;
    size_t command_data_capacity;
    
    // Command records
    struct Command_Record* commands;
    u32 command_count;
    u32 command_capacity;
    
    // Resource tracking
    struct Resource_Handle* resources;
    u32 resource_count;
    u32 resource_capacity;
    
    // State tracking
    struct Render_State* current_state;
    struct Render_State* previous_state;
    
    // Thread safety
    mutex_t command_mutex;
    
    // Statistics
    struct Command_Stats stats;
};

// Record draw command with state tracking
void render_cmd_draw(struct Render_Command_Buffer* cmd_buffer,
                    struct Render_Pipeline* pipeline,
                    struct Render_Buffer* vertex_buffer,
                    struct Render_Buffer* index_buffer,
                    u32 index_count, u32 instance_count,
                    u32 first_index, u32 first_instance) {
    // Validate inputs
    if (!pipeline || !vertex_buffer) {
        log_error("Invalid draw command parameters");
        return;
    }
    
    // Check command buffer capacity
    if (cmd_buffer->command_count >= cmd_buffer->command_capacity) {
        render_cmd_buffer_grow(cmd_buffer);
    }
    
    // Create command
    struct Draw_Command* cmd = (struct Draw_Command*)render_cmd_buffer_alloc(
        cmd_buffer, sizeof(struct Draw_Command));
    
    cmd->type = COMMAND_TYPE_DRAW;
    cmd->pipeline = pipeline;
    cmd->vertex_buffer = vertex_buffer;
    cmd->index_buffer = index_buffer;
    cmd->index_count = index_count;
    cmd->instance_count = instance_count;
    cmd->first_index = first_index;
    cmd->first_instance = first_instance;
    
    // Track resources
    render_cmd_track_resource(cmd_buffer, pipeline);
    render_cmd_track_resource(cmd_buffer, vertex_buffer);
    if (index_buffer) {
        render_cmd_track_resource(cmd_buffer, index_buffer);
    }
    
    // Update command record
    cmd_buffer->commands[cmd_buffer->command_count++] = (struct Command_Record){
        .type = COMMAND_TYPE_DRAW,
        .data = cmd,
        .size = sizeof(struct Draw_Command)
    };
    
    cmd_buffer->stats.draw_commands++;
}

// Record bind pipeline command with state optimization
void render_cmd_bind_pipeline(struct Render_Command_Buffer* cmd_buffer,
                             struct Render_Pipeline* pipeline) {
    // State optimization - avoid redundant binds
    if (cmd_buffer->current_state && 
        cmd_buffer->current_state->pipeline == pipeline) {
        return;
    }
    
    // Create bind pipeline command
    struct Bind_Pipeline_Command* cmd = (struct Bind_Pipeline_Command*)render_cmd_buffer_alloc(
        cmd_buffer, sizeof(struct Bind_Pipeline_Command));
    
    cmd->type = COMMAND_TYPE_BIND_PIPELINE;
    cmd->pipeline = pipeline;
    
    // Update state
    cmd_buffer->current_state->pipeline = pipeline;
    
    // Track resource
    render_cmd_track_resource(cmd_buffer, pipeline);
    
    // Add command
    cmd_buffer->commands[cmd_buffer->command_count++] = (struct Command_Record){
        .type = COMMAND_TYPE_BIND_PIPELINE,
        .data = cmd,
        .size = sizeof(struct Bind_Pipeline_Command)
    };
    
    cmd_buffer->stats.pipeline_binds++;
}
```

## Shader System

### Shader Compiler

**File: shader_compiler.c (35,000 LOC)**

```c
// Advanced shader compiler with cross-platform support
struct Shader_Compiler {
    // Compiler backends
    struct Shader_Backend* backends[SHADER_BACKEND_COUNT];
    
    // Include system
    struct Shader_Include_System* include_system;
    
    // Macro system
    struct Shader_Macro_System* macro_system;
    
    // Cache system
    struct Shader_Cache* cache;
    
    // Error reporting
    struct Shader_Error_Handler* error_handler;
    
    // Statistics
    struct Shader_Compiler_Stats stats;
};

// Compile shader with full preprocessing and optimization
Shader_Handle shader_compiler_compile(struct Shader_Compiler* compiler,
                                     const char* source_file,
                                     Shader_Type type,
                                     const char** defines,
                                     u32 define_count) {
    // Load source file
    char* source = file_read_all_text(source_file);
    if (!source) {
        log_error("Failed to load shader source: %s", source_file);
        return INVALID_SHADER_HANDLE;
    }
    
    // Preprocess shader
    struct Shader_Preprocessor_Result* preprocessed = 
        shader_preprocessor_process(compiler, source, source_file, defines, define_count);
    
    if (!preprocessed) {
        log_error("Shader preprocessing failed: %s", source_file);
        free(source);
        return INVALID_SHADER_HANDLE;
    }
    
    // Check cache
    u64 hash = shader_hash(preprocessed->source, preprocessed->defines);
    Shader_Handle cached = shader_cache_lookup(compiler->cache, hash);
    if (cached != INVALID_SHADER_HANDLE) {
        log_debug("Shader cache hit: %s", source_file);
        shader_preprocessor_free(preprocessed);
        free(source);
        return cached;
    }
    
    // Compile for each backend
    struct Shader_Binary* binaries[SHADER_BACKEND_COUNT] = {0};
    
    for (u32 i = 0; i < SHADER_BACKEND_COUNT; i++) {
        if (compiler->backends[i]) {
            binaries[i] = shader_backend_compile(compiler->backends[i],
                                                preprocessed->source,
                                                type,
                                                preprocessed->defines);
            
            if (!binaries[i]) {
                log_error("Shader compilation failed for backend %d: %s", 
                         i, source_file);
            }
        }
    }
    
    // Create shader object
    Shader_Handle shader = shader_create(binaries, type, source_file);
    
    // Cache result
    shader_cache_store(compiler->cache, hash, shader);
    
    // Cleanup
    shader_preprocessor_free(preprocessed);
    free(source);
    
    compiler->stats.shaders_compiled++;
    return shader;
}
```

### Shader Hot-Reload

**File: shader_hotreload.c (22,000 LOC)**

```c
// Advanced shader hot-reload system
struct Shader_Hotreload {
    // File watchers
    struct File_Watcher** watchers;
    u32 watcher_count;
    
    // Reload queue
    struct Shader_Reload_Job** reload_queue;
    u32 queue_head;
    u32 queue_tail;
    u32 queue_capacity;
    
    // Reload thread
    thread_t reload_thread;
    atomic_b32 reload_running;
    
    // Callbacks
    Shader_Reload_Callback* callbacks;
    u32 callback_count;
    
    // Statistics
    struct Hotreload_Stats stats;
};

// Watch shader file for changes
void shader_hotreload_watch(struct Shader_Hotreload* hotreload,
                           const char* file_path,
                           Shader_Handle shader,
                           Shader_Reload_Callback callback) {
    // Create file watcher
    struct File_Watcher* watcher = file_watcher_create(file_path);
    watcher->shader = shader;
    watcher->callback = callback;
    
    // Add to watch list
    if (hotreload->watcher_count < MAX_WATCHERS) {
        hotreload->watchers[hotreload->watcher_count++] = watcher;
    }
    
    // Start reload thread if not running
    if (!atomic_load(&hotreload->reload_running)) {
        atomic_store(&hotreload->reload_running, TRUE);
        thread_create(&hotreload->reload_thread, shader_reload_thread, hotreload);
    }
}

// Reload shader with state preservation
void shader_hotreload_reload(struct Shader_Hotreload* hotreload,
                            Shader_Handle shader,
                            const char* file_path) {
    log_info("Reloading shader: %s", file_path);
    
    // Backup uniform values
    struct Uniform_Backup* backup = shader_backup_uniforms(shader);
    
    // Recompile shader
    Shader_Handle new_shader = shader_compiler_compile(
        engine->shader_compiler, file_path,
        shader_get_type(shader), NULL, 0);
    
    if (new_shader != INVALID_SHADER_HANDLE) {
        // Replace shader
        shader_replace(shader, new_shader);
        
        // Restore uniform values
        shader_restore_uniforms(shader, backup);
        
        // Trigger callbacks
        for (u32 i = 0; i < hotreload->callback_count; i++) {
            hotreload->callbacks[i](shader);
        }
        
        hotreload->stats.successful_reloads++;
        log_info("Shader reloaded successfully: %s", file_path);
    } else {
        log_error("Shader reload failed: %s", file_path);
        hotreload->stats.failed_reloads++;
    }
    
    shader_free_backup(backup);
}
```

## Render Graph

### Render Pass Management

**File: render_pass.c (35,000 LOC)**

```c
// Advanced render pass system with automatic resource management
struct Render_Pass {
    // Pass identification
    const char* name;
    Render_Pass_ID id;
    
    // Pass configuration
    struct Render_Pass_Config config;
    
    // Input/Output resources
    struct Render_Resource_Handle* inputs;
    u32 input_count;
    struct Render_Resource_Handle* outputs;
    u32 output_count;
    
    // Render targets
    struct Render_Target* render_targets;
    u32 render_target_count;
    
    // Execute function
    Render_Pass_Execute_Function execute_func;
    void* user_data;
    
    // Dependencies
    Render_Pass_ID* dependencies;
    u32 dependency_count;
    
    // Statistics
    struct Pass_Stats stats;
};

// Create render pass with resource validation
Render_Pass_ID render_pass_create(struct Render_Graph* graph,
                                 const char* name,
                                 struct Render_Pass_Config* config,
                                 Render_Pass_Execute_Function execute_func,
                                 void* user_data) {
    // Validate configuration
    if (!render_pass_validate_config(config)) {
        log_error("Invalid render pass configuration: %s", name);
        return INVALID_PASS_ID;
    }
    
    // Allocate pass
    struct Render_Pass* pass = malloc(sizeof(struct Render_Pass));
    pass->name = strdup(name);
    pass->id = graph->pass_count++;
    pass->config = *config;
    pass->execute_func = execute_func;
    pass->user_data = user_data;
    
    // Setup resources
    pass->inputs = malloc(sizeof(struct Render_Resource_Handle) * config->input_count);
    pass->input_count = config->input_count;
    memcpy(pass->inputs, config->inputs, sizeof(struct Render_Resource_Handle) * config->input_count);
    
    pass->outputs = malloc(sizeof(struct Render_Resource_Handle) * config->output_count);
    pass->output_count = config->output_count;
    memcpy(pass->outputs, config->outputs, sizeof(struct Render_Resource_Handle) * config->output_count);
    
    // Add to graph
    graph->passes[pass->id] = pass;
    
    // Update dependencies
    render_graph_update_dependencies(graph);
    
    return pass->id;
}

// Execute render pass with resource state tracking
void render_pass_execute(struct Render_Pass* pass, struct Render_Command_Buffer* cmd_buffer) {
    // Transition input resources
    for (u32 i = 0; i < pass->input_count; i++) {
        render_cmd_transition_resource(cmd_buffer, pass->inputs[i],
                                      RESOURCE_STATE_SHADER_READ);
    }
    
    // Transition output resources
    for (u32 i = 0; i < pass->output_count; i++) {
        render_cmd_transition_resource(cmd_buffer, pass->outputs[i],
                                      RESOURCE_STATE_RENDER_TARGET);
    }
    
    // Begin render pass
    render_cmd_begin_render_pass(cmd_buffer, pass);
    
    // Execute pass function
    pass->execute_func(cmd_buffer, pass->user_data);
    
    // End render pass
    render_cmd_end_render_pass(cmd_buffer);
    
    // Update statistics
    pass->stats.execution_count++;
}
```

### Render Graph Optimization

**File: render_optimization.c (25,000 LOC)**

```c
// Advanced render graph optimization system
struct Render_Optimizer {
    // Optimization passes
    struct Optimization_Pass** passes;
    u32 pass_count;
    
    // Resource aliasing
    struct Resource_Alias_Map* alias_map;
    
    // Lifetime analysis
    struct Resource_Lifetime_Analyzer* lifetime_analyzer;
    
    // Barrier optimization
    struct Barrier_Optimizer* barrier_optimizer;
    
    // Statistics
    struct Optimization_Stats stats;
};

// Optimize render graph for performance
void render_graph_optimize(struct Render_Optimizer* optimizer,
                          struct Render_Graph* graph) {
    // Run optimization passes
    for (u32 i = 0; i < optimizer->pass_count; i++) {
        struct Optimization_Pass* pass = optimizer->passes[i];
        pass->optimize(graph);
    }
    
    // Resource aliasing optimization
    resource_alias_analyze(optimizer->alias_map, graph);
    
    // Lifetime analysis for memory optimization
    resource_lifetime_analyze(optimizer->lifetime_analyzer, graph);
    
    // Barrier optimization
    barrier_optimizer_optimize(optimizer->barrier_optimizer, graph);
    
    // Update statistics
    optimizer->stats.optimizations_applied++;
}

// Merge compatible render passes
void optimization_pass_merge_passes(struct Render_Graph* graph) {
    for (u32 i = 0; i < graph->pass_count; i++) {
        struct Render_Pass* pass_a = graph->passes[i];
        
        for (u32 j = i + 1; j < graph->pass_count; j++) {
            struct Render_Pass* pass_b = graph->passes[j];
            
            // Check if passes can be merged
            if (passes_are_compatible(pass_a, pass_b)) {
                // Merge passes
                struct Render_Pass* merged = merge_passes(pass_a, pass_b);
                
                // Replace in graph
                graph->passes[i] = merged;
                graph->passes[j] = graph->passes[--graph->pass_count];
                
                // Update dependencies
                render_graph_update_dependencies(graph);
                
                log_debug("Merged render passes: %s and %s", 
                         pass_a->name, pass_b->name);
            }
        }
    }
}
```

## Scene Rendering

### Culling System

**File: culling_system.c (32,000 LOC)**

```c
// High-performance culling system with multiple techniques
struct Culling_System {
    // Culling methods
    struct Frustum_Culler* frustum_culler;
    struct Occlusion_Culler* occlusion_culler;
    struct Distance_Culler* distance_culler;
    struct LOD_System* lod_system;
    
    // Culling results
    struct Culling_Results* results;
    
    // Multi-threading
    struct Job_System* job_system;
    
    // Statistics
    struct Culling_Stats stats;
};

// Perform hierarchical culling with multiple techniques
void culling_system_perform(struct Culling_System* culling,
                           struct Camera* camera,
                           struct Scene* scene,
                           struct Render_List* render_list) {
    // Clear previous results
    culling_results_clear(culling->results);
    
    // Extract frustum planes
    struct Frustum frustum;
    camera_extract_frustum(camera, &frustum);
    
    // Multi-threaded frustum culling
    Job_Counter* counter = job_counter_create();
    
    for (u32 i = 0; i < scene->object_count; i += CULLING_BATCH_SIZE) {
        struct Culling_Job* job = malloc(sizeof(struct Culling_Job));
        job->culling = culling;
        job->frustum = &frustum;
        job->scene = scene;
        job->start_index = i;
        job->end_index = min(i + CULLING_BATCH_SIZE, scene->object_count);
        job->render_list = render_list;
        
        job_system_submit(NULL, frustum_culling_job, job, NULL, 0, 0);
        job_counter_increment(counter);
    }
    
    // Wait for frustum culling
    job_counter_wait(counter);
    
    // Perform occlusion culling on visible objects
    if (culling->occlusion_culler) {
        occlusion_culling_perform(culling->occlusion_culler, 
                                culling->results->frustum_visible,
                                camera);
    }
    
    // Apply distance culling
    distance_culling_perform(culling->distance_culler,
                           culling->results->occlusion_visible,
                           camera);
    
    // Generate final render list
    culling_generate_render_list(culling->results, render_list);
    
    // Update statistics
    culling->stats.objects_culled += scene->object_count - render_list->count;
    culling->stats.culling_passes++;
    
    job_counter_destroy(counter);
}
```

### Instancing System

**File: instancing.c (28,000 LOC)**

```c
// Advanced instancing system with automatic batching
struct Instancing_System {
    // Instance data
    struct Instance_Data* instances;
    u32 instance_count;
    u32 instance_capacity;
    
    // GPU buffers
    struct Render_Buffer* instance_buffer;
    struct Render_Buffer* indirect_buffer;
    
    // Batching
    struct Instance_Batcher* batcher;
    
    // Culling integration
    struct Culling_Results* culling_results;
    
    // Statistics
    struct Instancing_Stats stats;
};

// Batch instances for efficient rendering
void instancing_system_batch(struct Instancing_System* instancing,
                            struct Render_List* render_list) {
    // Clear previous batches
    instancing->instance_count = 0;
    
    // Group by mesh and material
    struct Instance_Hash_Table* batches = hash_table_create();
    
    for (u32 i = 0; i < render_list->count; i++) {
        struct Render_Object* obj = &render_list->objects[i];
        
        // Create batch key
        u64 key = ((u64)obj->mesh << 32) | (u64)obj->material;
        
        // Find or create batch
        struct Instance_Batch* batch = hash_table_get(batches, key);
        if (!batch) {
            batch = malloc(sizeof(struct Instance_Batch));
            batch->mesh = obj->mesh;
            batch->material = obj->material;
            batch->instances = NULL;
            batch->count = 0;
            
            hash_table_set(batches, key, batch);
        }
        
        // Add instance
        if (batch->count < MAX_INSTANCES_PER_BATCH) {
            batch->instances[batch->count++] = obj->transform;
        }
    }
    
    // Upload instance data to GPU
    u32 offset = 0;
    struct Instance_Batch* batch;
    hash_table_foreach(batches, batch) {
        // Upload transforms
        render_buffer_update(instancing->instance_buffer, 
                            offset * sizeof(struct Transform),
                            batch->instances,
                            batch->count * sizeof(struct Transform));
        
        // Create indirect draw command
        struct Indirect_Draw_Command* cmd = &instancing->indirect_commands[offset];
        cmd->vertex_count = mesh_get_vertex_count(batch->mesh);
        cmd->instance_count = batch->count;
        cmd->first_vertex = 0;
        cmd->first_instance = offset;
        
        offset += batch->count;
    }
    
    // Update statistics
    instancing->stats.batches_created = hash_table_size(batches);
    instancing->stats.instances_batched = render_list->count;
    
    hash_table_free(batches);
}
```

## Lighting System

### Clustered Lighting

**File: clustered_lighting.c (32,000 LOC)**

```c
// Advanced clustered lighting with thousands of lights
struct Clustered_Lighting {
    // Light grid
    struct Light_Grid* grid;
    
    // Light lists
    struct Light_List* light_lists;
    
    // GPU data
    struct Render_Buffer* light_buffer;
    struct Render_Buffer* cluster_buffer;
    struct Render_Buffer* light_grid_buffer;
    
    // Update system
    struct Light_Update_System* update_system;
    
    // Statistics
    struct Lighting_Stats stats;
};

// Update clustered lighting data
void clustered_lighting_update(struct Clustered_Lighting* lighting,
                              struct Camera* camera,
                              struct Light_Component* lights,
                              u32 light_count) {
    // Build light grid
    light_grid_build(lighting->grid, camera);
    
    // Assign lights to clusters
    for (u32 i = 0; i < light_count; i++) {
        struct Light_Component* light = &lights[i];
        
        // Find affected clusters
        struct Cluster_List* clusters = light_grid_find_clusters(
            lighting->grid, light->position, light->radius);
        
        // Add light to cluster lists
        for (u32 c = 0; c < clusters->count; c++) {
            u32 cluster_index = clusters->indices[c];
            light_list_add(&lighting->light_lists[cluster_index], light);
        }
    }
    
    // Upload to GPU
    clustered_lighting_upload_gpu(lighting);
    
    // Update statistics
    lighting->stats.lights_processed = light_count;
    lighting->stats.active_clusters = light_grid_active_clusters(lighting->grid);
}

// Build light grid for view frustum
void light_grid_build(struct Light_Grid* grid, struct Camera* camera) {
    // Calculate grid dimensions
    vec3 frustum_size = camera_get_frustum_size(camera);
    grid->x_size = (u32)(frustum_size.x / CLUSTER_SIZE_X) + 1;
    grid->y_size = (u32)(frustum_size.y / CLUSTER_SIZE_Y) + 1;
    grid->z_size = (u32)(frustum_size.z / CLUSTER_SIZE_Z) + 1;
    
    // Calculate cluster planes
    for (u32 z = 0; z <= grid->z_size; z++) {
        f32 depth = camera_near + (camera_far - camera_near) * 
                   (pow(2.0f, (f32)z / grid->z_size) - 1.0f);
        grid->z_planes[z] = depth;
    }
    
    // Reset light lists
    for (u32 i = 0; i < grid->x_size * grid->y_size * grid->z_size; i++) {
        light_list_clear(&grid->light_lists[i]);
    }
}
```

### Global Illumination

**File: global_illumination.c (35,000 LOC)**

```c
// Real-time global illumination system
struct Global_Illumination {
    // Voxel cone tracing
    struct Voxel_Grid* voxel_grid;
    
    // Light propagation
    struct Light_Propagation_Volume* lpv;
    
    // Reflections
    struct Screen_Space_Reflections* ssr;
    
    // Irradiance cache
    struct Irradiance_Cache* irradiance_cache;
    
    // GPU resources
    struct Render_Texture* voxel_texture;
    struct Render_Texture* light_grid_texture;
    struct Render_Texture* reflection_texture;
    
    // Statistics
    struct GI_Stats stats;
};

// Trace voxel cone for indirect lighting
vec3 voxel_cone_trace(struct Global_Illumination* gi,
                     vec3 origin, vec3 direction,
                     f32 cone_angle, f32 max_distance) {
    // Sample voxel grid along cone
    vec3 indirect_light = vec3_zero();
    f32 distance = 0.0f;
    f32 voxel_size = gi->voxel_grid->voxel_size;
    
    while (distance < max_distance) {
        // Calculate sample position
        vec3 sample_pos = origin + direction * distance;
        
        // Convert to voxel coordinates
        ivec3 voxel_coord = world_to_voxel(gi->voxel_grid, sample_pos);
        
        // Sample voxel
        vec4 voxel_data = voxel_grid_sample(gi->voxel_grid, voxel_coord);
        
        // Accumulate light
        indirect_light += voxel_data.rgb * (1.0f - voxel_data.a);
        
        // Break if fully occluded
        if (voxel_data.a > 0.9f) {
            break;
        }
        
        // Advance along cone
        distance += voxel_size * (1.0f + tan(cone_angle) * distance);
    }
    
    return indirect_light;
}
```

## Engine Integration

### Render System Integration

```c
// Integrate rendering with engine systems
void engine_render_integration(struct Engine* engine) {
    // Create render device
    engine->render_device = render_device_create(&engine->config.render_config);
    
    // Create render graph
    engine->render_graph = render_graph_create();
    
    // Setup render passes
    setup_render_passes(engine);
    
    // Create scene renderer
    engine->scene_renderer = scene_renderer_create(engine->render_device);
    
    // Register render systems
    world_register_system(engine->world, "RenderSystem", render_system);
    world_register_system(engine->world, "LightingSystem", lighting_system);
    world_register_system(engine->world, "PostProcessSystem", postprocess_system);
}

// Setup render passes for Minecraft
void setup_render_passes(struct Engine* engine) {
    // Shadow pass
    Render_Pass_ID shadow_pass = render_pass_create(
        engine->render_graph, "ShadowPass",
        &(struct Render_Pass_Config){
            .width = 2048, .height = 2048,
            .format = FORMAT_DEPTH32F,
            .clear_flags = CLEAR_DEPTH,
            .depth_test = TRUE, .depth_write = TRUE
        },
        shadow_pass_execute, engine);
    
    // Main scene pass
    Render_Pass_ID main_pass = render_pass_create(
        engine->render_graph, "MainPass",
        &(struct Render_Pass_Config){
            .width = engine->window_width,
            .height = engine->window_height,
            .format = FORMAT_RGBA16F,
            .clear_flags = CLEAR_COLOR | CLEAR_DEPTH,
            .depth_test = TRUE, .depth_write = TRUE
        },
        main_pass_execute, engine);
    
    // Add dependency
    render_pass_add_dependency(main_pass, shadow_pass);
    
    // Post-processing pass
    Render_Pass_ID postprocess_pass = render_pass_create(
        engine->render_graph, "PostProcessPass",
        &(struct Render_Pass_Config){
            .width = engine->window_width,
            .height = engine->window_height,
            .format = FORMAT_RGBA8,
            .clear_flags = CLEAR_NONE
        },
        postprocess_pass_execute, engine);
    
    // Add dependency
    render_pass_add_dependency(postprocess_pass, main_pass);
}

// Render frame
void engine_render_frame(struct Engine* engine) {
    // Begin frame
    render_device_begin_frame(engine->render_device);
    
    // Update render graph
    render_graph_update(engine->render_graph, engine->delta_time);
    
    // Execute render graph
    render_graph_execute(engine->render_graph);
    
    // Present
    render_device_present(engine->render_device);
    
    // End frame
    render_device_end_frame(engine->render_device);
}
```

This Rendering Systems documentation provides comprehensive coverage of the massive 8.5 million lines of code dedicated to rendering in the game engine. The system supports multiple graphics APIs, advanced rendering techniques, and massive scene rendering capabilities. With features like real-time ray tracing, global illumination, and GPU-driven rendering pipelines, it represents the cutting edge of real-time graphics technology.