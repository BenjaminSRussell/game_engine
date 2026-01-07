# GPU-Driven Rendering - Quick Reference

## Phase 1 Implementation Summary

All foundational GPU-driven rendering infrastructure is now in place.

## Core Headers to Include

```c
#include "rendering/core/gpu_types.h"           // GPU data structures
#include "rendering/core/gpu_scene.h"           // GPU scene management
#include "rendering/core/gpu_data_transfer.h"   // Data transfer
#include "rendering/gpu_driven/draw_command_gen.h"  // Indirect commands
#include "geometry/instancing/instance_batching.h"  // Batching
```

## Key Data Structures

### Instance Data (128 bytes)
```c
GPUInstanceData {
    float transform[16];           // 4x4 matrix
    float bounds_min[4];           // AABB min
    float bounds_max[4];           // AABB max
    uint32_t material_id;
    uint32_t visibility_flags;
    uint32_t mesh_id;
    uint32_t instance_custom_data;
}
```

### Material Data (64 bytes)
```c
GPUMaterialData {
    float base_color[4];
    float metallic_roughness_ao[4];
    uint32_t texture_indices[8];   // Bindless textures
}
```

### Indirect Draw Arguments (20 bytes)
```c
IndirectDrawArgs {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t indexStart;
    int32_t baseVertex;
    uint32_t baseInstance;
}
```

## GPU Scene API Usage

```c
// 1. Initialize system
rendering_gpu_scene_init();

// 2. Create a GPU scene
rendering_gpu_scene_desc_t desc = {
    .max_instances = 100000,
    .max_materials = 256,
    .max_draw_calls = 1024,
};
rendering_gpu_scene_handle_t scene;
rendering_gpu_scene_create(&scene, &desc);

// 3. Add instances
GPUInstanceData instance_data = {
    .transform = {...},           // Fill with actual data
    .bounds_min = {-0.5f, -0.5f, -0.5f, 1.0f},
    .bounds_max = {0.5f, 0.5f, 0.5f, 1.0f},
    .material_id = 0,
    .mesh_id = 0,
};
rendering_gpu_scene_add_instance(scene, instance_id, &instance_data);

// 4. Set materials
GPUMaterialData material = {
    .base_color = {0.8f, 0.8f, 0.8f, 1.0f},
    .metallic_roughness_ao = {0.0f, 0.5f, 1.0f, 0.0f},
    .texture_indices = {0, 1, 2, 3, 4, 5, 6, 7},
};
rendering_gpu_scene_set_material(scene, 0, &material);

// 5. Mark dirty and upload
rendering_gpu_scene_mark_instances_dirty(scene, 0, instance_count);
rendering_gpu_scene_upload_dirty_data(scene);

// 6. Get buffers for GPU use
void* instance_buffer = rendering_gpu_scene_get_instance_buffer(scene);
void* material_buffer = rendering_gpu_scene_get_material_buffer(scene);

// 7. Get stats
rendering_gpu_scene_stats_t stats;
rendering_gpu_scene_get_stats(scene, &stats);
printf("Instances: %u, Visible: %u, Draws: %u\n",
       stats.total_instances, stats.visible_instances, stats.draw_call_count);
```

## Batching Integration

```c
// 1. Initialize batching
geometry_instance_batching_init();

// 2. Create a batcher
geometry_instance_batching_handle_t batcher;
geometry_instance_batching_desc_t batch_desc = {
    .flags = 0,
};
geometry_instance_batching_create(&batcher, &batch_desc);

// 3. Add instances to batches
geometry_instance_batching_add_instance(
    batcher,
    instance_id,           // Unique ID
    mesh_id,              // Which mesh
    material_id,          // Which material
    lod_level,            // LOD 0-4
    shader_variant,       // Shader variant
    depth                 // For sorting
);

// 4. Get batch information
uint32_t batch_count = geometry_instance_batching_get_batch_count(batcher);
uint32_t total_instances = geometry_instance_batching_get_instance_count(batcher);

// 5. Iterate batches
for (uint32_t i = 0; i < batch_count; i++) {
    geometry_instance_batch_data_t batch_data;
    geometry_instance_batching_get_batch_data(batcher, i, &batch_data);

    // Process batch
    printf("Batch %u: %u instances, mesh=%u, material=%u, lod=%u\n",
           i, batch_data.batch_count, batch_data.mesh_id,
           batch_data.material_id, batch_data.lod_level);
}
```

## Data Transfer API

```c
// 1. Initialize
rendering_gpu_transfer_init();

// 2. Create transfer context
rendering_gpu_transfer_handle_t transfer;
rendering_gpu_transfer_desc_t transfer_desc = {
    .staging_buffer_size = 64 * 1024 * 1024,  // 64MB
    .use_async_transfer = true,
};
rendering_gpu_transfer_create(&transfer, &transfer_desc);

// 3. Upload instances
GPUInstanceData* instances = ...;
uint32_t instance_count = ...;
rendering_gpu_transfer_upload_instances(
    transfer, scene, instances, instance_count, 0);

// 4. Upload materials
GPUMaterialData* materials = ...;
uint32_t material_count = ...;
rendering_gpu_transfer_upload_materials(
    transfer, scene, materials, material_count, 0);

// 5. Wait for completion
rendering_gpu_transfer_wait_for_transfers(transfer);
```

## Draw Command Generation

```c
// 1. Initialize
rendering_draw_command_gen_init();

// 2. Create command generator
rendering_draw_command_gen_handle_t gen;
rendering_draw_command_gen_desc_t gen_desc = {
    .flags = 0,
};
rendering_draw_command_gen_create(&gen, &gen_desc);

// 3. Get indirect args buffer
uint32_t arg_count;
IndirectDrawArgs* args = rendering_draw_command_gen_get_indirect_buffer(gen, &arg_count);

// 4. Get draw count for multi-draw
uint32_t draw_count = rendering_draw_command_gen_get_draw_count(gen);
```

## Metal Shader Utilities

In your compute shaders (gpu_culling.metal):

```metal
#include "../core/gpu_types.metal"

// Frustum testing
if (aabb_in_frustum(bounds_center, bounds_extents, frustum)) {
    results[tid].visible_flag = 1u;
}

// LOD selection
uint lod = select_lod_with_hysteresis(distance, current_lod, lod_data);
results[tid].lod_selected = lod;

// Atomic operations (lock-free)
uint visible_idx = atomic_fetch_add_explicit(
    &visible_count[0], 1u, memory_order_relaxed);
```

## Compute Kernel Signatures

Available in gpu_culling.metal:

```metal
kernel void cull_frustum(
    device GPUInstanceData* instances [[buffer(0)]],
    device CullingResult* results [[buffer(1)]],
    constant FrustumData& frustum [[buffer(2)]],
    constant uint& instance_count [[buffer(3)]],
    uint tid [[thread_position_in_grid]])

kernel void select_lod(
    device GPUInstanceData* instances [[buffer(0)]],
    device CullingResult* results [[buffer(1)]],
    constant LODSelectionData& lod_data [[buffer(2)]],
    constant float3& camera_pos [[buffer(3)]],
    constant uint& instance_count [[buffer(4)]],
    uint tid [[thread_position_in_grid]])

kernel void compact_visible(
    device CullingResult* results [[buffer(0)]],
    device IndirectDrawArgs* indirect_args [[buffer(1)]],
    device uint* visible_indices [[buffer(2)]],
    device atomic_uint* visible_count [[buffer(3)]],
    constant uint& instance_count [[buffer(4)]],
    uint tid [[thread_position_in_grid]])

kernel void populate_indirect_args(
    device IndirectDrawArgs* indirect_args [[buffer(0)]],
    device uint* batch_instance_counts [[buffer(1)]],
    device uint* batch_index_offsets [[buffer(2)]],
    device uint* batch_instance_offsets [[buffer(3)]],
    constant uint& batch_count [[buffer(4)]],
    uint tid [[thread_position_in_grid]])
```

## Constants

```c
#define MAX_INSTANCES_PER_SCENE 1000000
#define MAX_MATERIALS_PER_SCENE 4096
#define MAX_DRAW_CALLS_PER_FRAME 100000
#define MAX_VISIBLE_INSTANCES_PER_FRAME 1000000

// Buffer sizes
#define INSTANCE_BUFFER_SIZE (MAX_INSTANCES_PER_SCENE * sizeof(GPUInstanceData))
#define MATERIAL_BUFFER_SIZE (MAX_MATERIALS_PER_SCENE * sizeof(GPUMaterialData))
#define INDIRECT_BUFFER_SIZE (MAX_DRAW_CALLS_PER_FRAME * sizeof(IndirectDrawArgs))
#define CULLING_RESULT_BUFFER_SIZE (MAX_INSTANCES_PER_SCENE * sizeof(CullingResult))
```

## Error Codes

All functions return `int` with these conventions:
- `0` = Success
- `-1` = Invalid parameter
- `-2` = Not initialized
- `-3` = Capacity exceeded / Out of range
- `-4` = Allocation failed / Memory error
- Negative values indicate failures

## Memory Estimates

For 1 million instances:
- Instance buffer: 128MB
- Material buffer (4096 materials): 256KB
- Culling results: 16MB
- Indirect args (100K draws): 2MB
- **Total: ~145MB**

For 100k instances (typical scene):
- Instance buffer: 12.8MB
- Culling results: 1.6MB
- **Total: ~14.4MB**

## Next Phase (Phase 2)

The following requires Metal backend integration:
1. Actual GPU buffer allocation via `metal_buffer_create()`
2. GPU data upload via blit/compute encoders
3. Compute shader compilation
4. Indirect command buffer (ICB) creation
5. Render loop integration

## Reference Implementation

The GPU particle system (`effects/gpu_particles/gpu_particle_system.c`) is a complete example of:
- Structure-of-Arrays layout
- Atomic operations
- Compute shader integration
- Indirect drawing
- Metal backend usage

## Debugging

```c
// Print batch information
geometry_instance_batching_debug_print();

// Print command generation status
rendering_draw_command_gen_debug_print();

// Get memory usage
size_t scene_mem = rendering_gpu_scene_get_memory_usage();
size_t batch_mem = geometry_instance_batching_get_memory_usage();
size_t transfer_mem = rendering_gpu_transfer_get_memory_usage();
```

## File Organization

```
src/engine/rendering/
├── core/
│   ├── gpu_types.h              // Data structures
│   ├── gpu_scene.h/c            // GPU scene management
│   └── gpu_data_transfer.h/c    // Data transfer
├── gpu_driven/
│   └── draw_command_gen.h/c     // Indirect commands
└── shaders/
    └── gpu_culling.metal         // Compute kernels

src/engine/geometry/
└── instancing/
    └── instance_batching.h/c     // Batching system
```

## Compilation Notes

- Include paths must have access to both `rendering/` and `geometry/` directories
- Metal shader file (gpu_culling.metal) requires Metal compilation step
- All C code compiles with standard C11
- No external dependencies beyond Metal framework
