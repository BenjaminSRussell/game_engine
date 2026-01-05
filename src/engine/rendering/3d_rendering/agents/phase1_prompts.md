# Phase 1: Core Infrastructure & Foundation - Agent Prompts

## Agent 1.1: Core Device & Memory

```
TASK: Implement Core Device & Memory Management (Phase 1, Agent 1)

You are implementing the foundational GPU device and memory systems for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/core/device/
- src/engine/rendering/3d_rendering/core/memory/
- src/engine/rendering/3d_rendering/memory/
- src/engine/rendering/3d_rendering/platform/vulkan/
- src/engine/rendering/3d_rendering/platform/metal/
- src/engine/rendering/3d_rendering/platform/d3d12/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. device_context.c - Initialize GPU device, enumerate physical devices, create logical device
2. device_capabilities.c - Query GPU features, limits, format support
3. device_memory.c - GPU memory type enumeration, allocation strategies
4. gpu_allocator.c - Implement slab allocator, buddy allocator, pool allocator
5. staging_buffer.c - Ring buffer for CPU->GPU transfers
6. memory_pool.c - Fixed-size block allocation
7. heap_manager.c - Multiple heap management (device local, host visible, etc.)
8. vk_instance.c, vk_device.c - Vulkan instance/device creation
9. mtl_device.c - Metal device initialization (macOS)
10. dx_device.c - D3D12 device creation (Windows)

IMPLEMENTATION GUIDELINES:
- Use abstract render device interface that backends implement
- All allocations must be tracked for leak detection
- Support memory budgets and automatic eviction
- Thread-safe allocation with minimal locking
- Alignment requirements must be respected (16-byte minimum, 256 for uniform buffers)

STRUCT PATTERNS TO USE:
```c
typedef struct render_device {
    void* backend_handle;  // VkDevice, id<MTLDevice>, ID3D12Device*
    render_device_caps_t caps;
    render_memory_allocator_t* allocator;
    uint32_t queue_family_indices[QUEUE_TYPE_COUNT];
} render_device_t;

typedef struct gpu_allocation {
    uint64_t offset;
    uint64_t size;
    uint32_t heap_index;
    uint32_t flags;
    void* mapped_ptr;  // NULL if not mapped
} gpu_allocation_t;
```

FOCUS: Get device initialization and memory allocation working correctly. This is the foundation everything else builds on.
```

---

## Agent 1.2: Command & Synchronization

```
TASK: Implement Command Buffers & Synchronization (Phase 1, Agent 2)

You are implementing command buffer recording and GPU synchronization for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/core/command/
- src/engine/rendering/3d_rendering/core/sync/
- src/engine/rendering/3d_rendering/synchronization/
- src/engine/rendering/3d_rendering/core/pipeline/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. command_buffer.c - Command buffer allocation, recording, reset
2. command_pool.c - Pool-based command buffer management per thread
3. command_queue.c - Queue submission with dependencies
4. command_encoder.c - High-level encoding (begin/end render pass, bind pipeline, draw)
5. fence_pool.c - Fence allocation, waiting, recycling
6. semaphore_manager.c - Timeline semaphores for GPU-GPU sync
7. barrier_batch.c - Resource barrier coalescing and optimization
8. frame_sync.c - Frame-in-flight synchronization (double/triple buffering)
9. pipeline_cache.c - PSO caching and retrieval
10. pipeline_layout.c - Pipeline layout management

IMPLEMENTATION GUIDELINES:
- Command pools are per-thread (no locking needed within pool)
- Use timeline semaphores for async compute coordination
- Batch barriers to minimize sync points
- Support secondary command buffers for parallel recording
- Implement automatic resource state tracking

KEY PATTERNS:
```c
typedef struct command_buffer {
    void* backend_handle;
    command_pool_t* pool;
    command_buffer_state_t state;
    resource_state_tracker_t* state_tracker;
} command_buffer_t;

typedef struct fence {
    void* backend_handle;
    uint64_t signal_value;
    bool signaled;
} fence_t;

// Usage pattern
cmd_begin(cmd);
cmd_begin_render_pass(cmd, &pass_info);
cmd_bind_pipeline(cmd, pipeline);
cmd_draw(cmd, vertex_count, instance_count, 0, 0);
cmd_end_render_pass(cmd);
cmd_end(cmd);
queue_submit(queue, cmd, fence);
```

FOCUS: Correct synchronization is critical. Ensure no race conditions and proper barrier placement.
```

---

## Agent 1.3: Resource Management

```
TASK: Implement Resource Management & Descriptors (Phase 1, Agent 3)

You are implementing GPU resource management and descriptor binding for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/resource_management/
- src/engine/rendering/3d_rendering/core/descriptor/
- src/engine/rendering/3d_rendering/bindless_rendering/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. resource_handle.c - Typed handles for buffers, textures, samplers
2. resource_pool.c - Resource pooling and recycling
3. resource_lifetime.c - Reference counting, deferred deletion
4. descriptor_set_layout.c - Layout creation and caching
5. descriptor_pool.c - Descriptor pool management
6. descriptor_writer.c - Batched descriptor updates
7. bindless_manager.c - Global bindless descriptor heap
8. push_constants.c - Push constant management
9. dynamic_descriptors.c - Dynamic uniform/storage buffers

IMPLEMENTATION GUIDELINES:
- All resources accessed via opaque handles (not raw pointers)
- Deferred deletion (wait N frames before actual free)
- Bindless: single global descriptor table indexed by handle
- Support descriptor indexing for material system
- Cache descriptor set layouts by hash

KEY PATTERNS:
```c
// Handle-based resource access
typedef struct { uint32_t index; uint32_t generation; } buffer_handle_t;
typedef struct { uint32_t index; uint32_t generation; } texture_handle_t;

// Bindless indexing
typedef struct bindless_heap {
    VkDescriptorSet descriptor_set;
    uint32_t texture_count;
    uint32_t buffer_count;
    free_list_t free_indices;
} bindless_heap_t;

uint32_t bindless_register_texture(bindless_heap_t* heap, texture_handle_t tex);
// Returns index to use in shader: textures[index]
```

FOCUS: Handle-based resources prevent dangling pointers. Bindless enables flexible material system.
```

---

## Agent 1.4: Render Graph & Frame Management

```
TASK: Implement Render Graph & Frame Management (Phase 1, Agent 4)

You are implementing the render graph system and frame management for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/rendering/render_graph/
- src/engine/rendering/3d_rendering/framebuffer/
- src/engine/rendering/3d_rendering/rendering/output/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. render_pass_node.c - Render graph pass node definition
2. resource_node.c - Transient resource declarations
3. graph_compiler.c - Graph compilation, barrier insertion
4. graph_executor.c - Execute compiled graph
5. resource_aliasing.c - Memory aliasing for transients
6. render_target.c - Render target management
7. swapchain.c - Swapchain creation and presentation
8. dynamic_resolution.c - DRS scaling
9. hdr_output.c - HDR10/Dolby Vision output
10. frame_pacing.c - Frame timing and VSync

IMPLEMENTATION GUIDELINES:
- Declare passes and resources, graph figures out barriers
- Transient resources can alias memory (reuse between passes)
- Support async compute passes in graph
- Automatic render target pooling
- Handle swapchain resize gracefully

KEY PATTERNS:
```c
// Render graph usage
render_graph_t* graph = render_graph_create();

texture_handle_t gbuffer_albedo = rg_create_texture(graph, "GBuffer_Albedo", &desc);
texture_handle_t gbuffer_normal = rg_create_texture(graph, "GBuffer_Normal", &desc);

rg_add_pass(graph, "GBufferPass", PASS_TYPE_GRAPHICS, &(rg_pass_desc_t){
    .color_outputs = { gbuffer_albedo, gbuffer_normal },
    .depth_output = depth,
    .execute = gbuffer_pass_execute,
});

rg_add_pass(graph, "LightingPass", PASS_TYPE_GRAPHICS, &(rg_pass_desc_t){
    .texture_inputs = { gbuffer_albedo, gbuffer_normal, depth },
    .color_outputs = { hdr_color },
    .execute = lighting_pass_execute,
});

rg_compile(graph);
rg_execute(graph, cmd);
```

FOCUS: The render graph is the backbone of the frame. Get pass ordering and barriers right.
```

---

## Agent 1.5: Math & Utilities

```
TASK: Implement Math Library & Debug Utilities (Phase 1, Agent 5)

You are implementing math utilities and debugging infrastructure for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/math/
- src/engine/rendering/3d_rendering/statistics/
- src/engine/rendering/3d_rendering/profiling/
- src/engine/rendering/3d_rendering/debugging/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. vec2.c, vec3.c, vec4.c - Vector math with SIMD
2. mat3.c, mat4.c - Matrix operations
3. quat.c - Quaternion math
4. frustum.c - Frustum planes extraction
5. aabb.c - Axis-aligned bounding box
6. gpu_timer.c - GPU timestamp queries
7. frame_timer.c - CPU frame timing
8. perf_overlay.c - On-screen performance display
9. debug_lines.c - Debug line rendering
10. debug_shapes.c - Debug sphere/box/cone rendering

IMPLEMENTATION GUIDELINES:
- Use SIMD intrinsics (SSE/NEON) for vector math
- Matrices are column-major (OpenGL/Vulkan convention)
- Profile markers for all major passes
- Debug rendering should be zero-cost when disabled
- Statistics should track min/max/avg over time

KEY PATTERNS:
```c
// SIMD vec4
typedef union vec4 {
    struct { float x, y, z, w; };
    float e[4];
    __m128 simd;
} vec4_t;

static inline vec4_t vec4_add(vec4_t a, vec4_t b) {
    return (vec4_t){ .simd = _mm_add_ps(a.simd, b.simd) };
}

// GPU profiling
gpu_scope_begin(cmd, "ShadowPass");
// ... shadow rendering ...
gpu_scope_end(cmd);

// Debug visualization
debug_draw_aabb(aabb, COLOR_GREEN);
debug_draw_frustum(frustum, COLOR_YELLOW);
```

FOCUS: SIMD math is essential for performance. Debug tools are essential for development.
```

---

## Agent 1.6: Basic Shading Infrastructure

```
TASK: Implement Shader System & PBR Basics (Phase 1, Agent 6)

You are implementing the shader compilation system and PBR shading foundation for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/shading/brdf/
- src/engine/rendering/3d_rendering/materials/shaders/
- src/engine/rendering/3d_rendering/materials/pbr/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. shader_compiler.c - HLSL/GLSL compilation to SPIR-V
2. shader_reflection.c - Extract binding info, push constants
3. shader_cache.c - Compiled shader caching (disk + memory)
4. shader_variants.c - Permutation system (#ifdef handling)
5. ggx_brdf.c - GGX microfacet BRDF
6. diffuse_brdf.c - Lambert, Disney diffuse
7. fresnel.c - Schlick Fresnel
8. pbr_parameters.c - Albedo, roughness, metallic, normal
9. metallic_roughness.c - Metal/rough workflow
10. energy_conservation.c - Multi-scatter compensation

IMPLEMENTATION GUIDELINES:
- Hot-reload shaders during development
- Cache shaders by content hash
- GGX with height-correlated Smith visibility
- Support both metal/rough and spec/gloss workflows
- Implement energy-conserving sheen

KEY PATTERNS:
```c
// Shader loading
shader_handle_t vs = shader_compile("mesh.vert", SHADER_STAGE_VERTEX);
shader_handle_t fs = shader_compile("pbr.frag", SHADER_STAGE_FRAGMENT);

// Shader permutations
#define PERM_HAS_NORMAL_MAP (1 << 0)
#define PERM_HAS_EMISSIVE   (1 << 1)
shader_handle_t fs = shader_get_variant("pbr.frag", PERM_HAS_NORMAL_MAP);

// PBR BRDF (in shader, but implement supporting C code)
// D = GGX distribution
// G = Smith height-correlated
// F = Schlick Fresnel
vec3 specular = D * G * F / (4.0 * NdotV * NdotL);
vec3 diffuse = albedo / PI * (1.0 - F) * (1.0 - metallic);
```

FOCUS: Shaders are the heart of rendering. Get the PBR math right from the start.
```
