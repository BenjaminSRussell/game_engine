# Phase 2: Directory Reorganization & Geometry Systems

## Overview
This phase reorganizes the 154 flat directories into a clean hierarchy and implements the geometry pipeline.

---

## Agent 2.1: Directory Reorganization

```
TASK: Reorganize 3d_rendering Directory Structure (Phase 2, Agent 1)

You are reorganizing the rendering engine directory structure from 154 flat folders into a clean hierarchy.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/

CURRENT PROBLEM:
- 154 directories at top level
- No clear organization
- Related systems scattered

TARGET STRUCTURE:
```
3d_rendering/
├── backend/              # GPU API backends
│   └── metal/           # (move from platform/metal)
│
├── core/                # Core systems (keep existing)
│   ├── command/
│   ├── device/
│   ├── memory/
│   ├── pipeline/
│   ├── descriptor/
│   └── sync/
│
├── geometry/            # All geometry-related
│   ├── mesh/           # (keep)
│   ├── vertex/         # (keep)
│   ├── lod/            # (keep)
│   ├── bvh/            # (keep)
│   ├── meshlets/       # (keep)
│   ├── instancing/     # (keep)
│   ├── streaming/      # (keep)
│   └── nanite/         # (move from nanite/)
│
├── lighting/           # All lighting (keep + merge)
│   ├── sources/
│   ├── shadows/
│   ├── cascades/
│   ├── probes/
│   ├── volumetric/
│   ├── lightmaps/
│   ├── gi/            # (merge global_illumination)
│   ├── lumen/         # (move from lumen/)
│   └── vsm/           # (move from virtual_shadow_maps/)
│
├── materials/          # Materials & shading
│   ├── shaders/
│   ├── pbr/
│   ├── layering/
│   ├── textures/
│   └── instances/
│
├── shading/           # BRDF and shading models (keep)
│   ├── brdf/
│   ├── subsurface/
│   ├── cloth/
│   ├── hair/
│   ├── clearcoat/
│   └── special/
│
├── rendering/         # Render passes (keep + expand)
│   ├── forward/
│   ├── deferred/
│   ├── visibility/
│   ├── gpu_driven/
│   ├── render_graph/
│   └── output/
│
├── effects/           # Visual effects
│   ├── particles/
│   ├── gpu_particles/
│   ├── weather/
│   ├── decals/
│   ├── trails/
│   ├── fire/          # (move from fire_rendering/)
│   ├── smoke/         # (move from smoke_rendering/)
│   ├── water/         # (move from water/)
│   ├── destruction/   # (move from destruction/)
│   └── volumetric/
│
├── postprocess/       # Post-processing
│   ├── tonemapping/
│   ├── bloom/
│   ├── anti_aliasing/
│   ├── motion_blur/
│   ├── dof/
│   ├── ao/
│   └── ssr/
│
├── environment/       # Environment rendering
│   ├── landscape/     # (merge landscape_*)
│   ├── vegetation/
│   ├── ocean/         # (move from ocean_waves/)
│   ├── atmosphere/    # (move from atmosphere/, sky_atmosphere/)
│   └── clouds/        # (move from volumetric_clouds/)
│
├── character/         # Character rendering
│   ├── animation/     # (move from animation/)
│   ├── skin/          # (move from skin_rendering/)
│   ├── eyes/          # (move from eye_rendering/)
│   ├── hair/          # (move from hair_system/, hair_rendering/)
│   └── cloth/         # (move from cloth_system/)
│
├── assets/            # Asset system
│   ├── loading/
│   ├── formats/
│   ├── import/
│   ├── streaming/
│   └── compression/
│
├── editor/            # Editor tools (keep)
│   ├── viewport/
│   ├── gizmos/
│   ├── debug/
│   └── tools/
│
├── profiling/         # Performance (keep)
│
└── test/              # Unit tests (keep)
```

EXECUTION:
Create a Python script that:
1. Creates new directory structure
2. Moves files preserving git history (git mv)
3. Updates #include paths in all .c/.h files
4. Generates a mapping file for verification

OUTPUT: Clean directory structure with ~20 top-level folders instead of 154.
```

---

## Agent 2.2: Mesh System Implementation

```
TASK: Complete Mesh Data System (Phase 2, Agent 2)

You are completing the mesh data system implementation.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/geometry/mesh/

EXISTING FILES (have partial implementations):
- mesh_data.c (502 lines - good start)
- mesh_loader.c
- mesh_builder.c
- mesh_optimizer.c
- mesh_bounds.c
- submesh_manager.c

WHAT'S IMPLEMENTED:
- Basic mesh data structures
- Some vertex/index handling

WHAT NEEDS COMPLETION:
1. Metal buffer integration (use MTLBuffer for vertex/index data)
2. Async mesh loading with completion handlers
3. Mesh optimization (vertex cache, overdraw)
4. Proper submesh management with material IDs
5. Bounding volume computation (AABB, sphere)

METAL INTEGRATION:
```c
typedef struct mesh_gpu_data {
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
    uint32_t vertex_count;
    uint32_t index_count;
    MTLIndexType index_type;
    MTLPrimitiveType primitive_type;
} mesh_gpu_data_t;

// Upload mesh to GPU
mesh_gpu_data_t* mesh_upload_to_gpu(metal_device_t* dev, mesh_data_t* mesh) {
    mesh_gpu_data_t* gpu = calloc(1, sizeof(mesh_gpu_data_t));

    size_t vb_size = mesh->vertex_count * mesh->vertex_stride;
    gpu->vertex_buffer = [dev->device newBufferWithBytes:mesh->vertices
                                                  length:vb_size
                                                 options:MTLResourceStorageModeShared];

    size_t ib_size = mesh->index_count * (mesh->index_type == INDEX_U16 ? 2 : 4);
    gpu->index_buffer = [dev->device newBufferWithBytes:mesh->indices
                                                 length:ib_size
                                                options:MTLResourceStorageModeShared];

    gpu->vertex_count = mesh->vertex_count;
    gpu->index_count = mesh->index_count;
    gpu->index_type = mesh->index_type == INDEX_U16 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;

    return gpu;
}
```

OUTPUT: Complete mesh system that loads data and uploads to Metal buffers.
```

---

## Agent 2.3: Vertex Format System

```
TASK: Implement Flexible Vertex Format System (Phase 2, Agent 3)

You are implementing a flexible vertex format system for Metal.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/geometry/vertex/

FILES TO COMPLETE:
- vertex_format.c/h - Vertex attribute configuration
- vertex_buffer_pool.c/h - Buffer sub-allocation
- vertex_compression.c/h - Vertex data compression

WHAT TO IMPLEMENT:
1. Vertex format descriptor matching Metal's MTLVertexDescriptor
2. Common formats: P3N3T2, P3N3T2T4 (tangent), P3N3T2W4B4 (skinned)
3. Vertex buffer pooling for efficient allocation
4. Quantized position compression (16-bit)
5. Octahedron normal compression

METAL VERTEX DESCRIPTOR:
```c
typedef enum vertex_attribute {
    VERTEX_ATTR_POSITION = 0,
    VERTEX_ATTR_NORMAL = 1,
    VERTEX_ATTR_TEXCOORD0 = 2,
    VERTEX_ATTR_TEXCOORD1 = 3,
    VERTEX_ATTR_TANGENT = 4,
    VERTEX_ATTR_COLOR = 5,
    VERTEX_ATTR_BONE_WEIGHTS = 6,
    VERTEX_ATTR_BONE_INDICES = 7,
} vertex_attribute_t;

typedef struct vertex_format {
    uint32_t attribute_mask;
    uint32_t stride;
    MTLVertexFormat formats[8];
    uint32_t offsets[8];
} vertex_format_t;

MTLVertexDescriptor* vertex_format_to_metal(vertex_format_t* fmt) {
    MTLVertexDescriptor* desc = [[MTLVertexDescriptor alloc] init];

    for (int i = 0; i < 8; i++) {
        if (fmt->attribute_mask & (1 << i)) {
            desc.attributes[i].format = fmt->formats[i];
            desc.attributes[i].offset = fmt->offsets[i];
            desc.attributes[i].bufferIndex = 0;
        }
    }

    desc.layouts[0].stride = fmt->stride;
    desc.layouts[0].stepRate = 1;
    desc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    return desc;
}

// Common format: Position + Normal + UV
vertex_format_t vertex_format_p3n3t2(void) {
    return (vertex_format_t){
        .attribute_mask = (1 << VERTEX_ATTR_POSITION) | (1 << VERTEX_ATTR_NORMAL) | (1 << VERTEX_ATTR_TEXCOORD0),
        .stride = sizeof(float) * 8,  // 3 + 3 + 2
        .formats = { MTLVertexFormatFloat3, MTLVertexFormatFloat3, MTLVertexFormatFloat2 },
        .offsets = { 0, 12, 24 },
    };
}
```

OUTPUT: Flexible vertex format system integrated with Metal.
```

---

## Agent 2.4: LOD System

```
TASK: Implement LOD Generation and Selection (Phase 2, Agent 4)

You are implementing the LOD (Level of Detail) system.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/geometry/lod/

FILES TO COMPLETE:
- lod_generator.c - Mesh simplification (QEM)
- lod_selector.c - Runtime LOD selection
- lod_crossfade.c - Smooth transitions

WHAT TO IMPLEMENT:
1. Quadric Error Metrics mesh simplification
2. Screen-size based LOD selection
3. Dithered crossfade between LODs
4. Hysteresis to prevent rapid switching
5. LOD bias for quality/performance tuning

IMPLEMENTATION:
```c
typedef struct lod_chain {
    mesh_gpu_data_t* lods[8];
    float switch_distances[8];  // Screen coverage thresholds
    uint32_t lod_count;
} lod_chain_t;

// Generate LOD chain
lod_chain_t* lod_generate_chain(mesh_data_t* source, float* ratios, uint32_t count) {
    lod_chain_t* chain = calloc(1, sizeof(lod_chain_t));
    chain->lod_count = count + 1;  // +1 for original

    // LOD 0 is original
    chain->lods[0] = mesh_upload_to_gpu(g_device, source);
    chain->switch_distances[0] = 1.0f;  // Use at 100%+ screen coverage

    for (uint32_t i = 0; i < count; i++) {
        mesh_data_t* simplified = qem_simplify(source, ratios[i]);
        chain->lods[i + 1] = mesh_upload_to_gpu(g_device, simplified);
        chain->switch_distances[i + 1] = ratios[i] * 0.5f;  // Heuristic
        mesh_data_destroy(simplified);
    }

    return chain;
}

// Select LOD based on screen coverage
uint32_t lod_select(lod_chain_t* chain, float screen_coverage, float bias) {
    float adjusted = screen_coverage * bias;

    for (uint32_t i = 0; i < chain->lod_count; i++) {
        if (adjusted >= chain->switch_distances[i]) {
            return i;
        }
    }
    return chain->lod_count - 1;  // Lowest LOD
}
```

OUTPUT: LOD system that generates and selects appropriate detail levels.
```

---

## Agent 2.5: BVH and Culling

```
TASK: Complete BVH and Culling System (Phase 2, Agent 5)

You are completing the BVH construction and culling system.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/geometry/bvh/

EXISTING: bvh_builder.c (533 lines - good foundation)

WHAT NEEDS COMPLETION:
1. BVH traversal for frustum culling
2. SIMD-optimized AABB tests (use ARM NEON on Apple Silicon)
3. BVH updates for dynamic objects
4. Integration with Metal compute for GPU culling

APPLE SILICON SIMD:
```c
#include <simd/simd.h>

// Test 4 AABBs against frustum at once
void frustum_cull_simd(
    simd_float4 frustum_planes[6],
    simd_float4 aabb_min_x, simd_float4 aabb_min_y, simd_float4 aabb_min_z,
    simd_float4 aabb_max_x, simd_float4 aabb_max_y, simd_float4 aabb_max_z,
    simd_int4* out_visible
) {
    simd_int4 visible = simd_make_int4(-1, -1, -1, -1);  // All visible initially

    for (int p = 0; p < 6; p++) {
        simd_float4 plane = frustum_planes[p];

        // Select positive vertex for each AABB
        simd_float4 px = simd_select(aabb_min_x, aabb_max_x, plane.x > 0);
        simd_float4 py = simd_select(aabb_min_y, aabb_max_y, plane.y > 0);
        simd_float4 pz = simd_select(aabb_min_z, aabb_max_z, plane.z > 0);

        // Dot product with plane
        simd_float4 dist = px * plane.x + py * plane.y + pz * plane.z + plane.w;

        // If all positive vertices are behind plane, AABB is culled
        visible = simd_and(visible, simd_make_int4(dist.x >= 0 ? -1 : 0,
                                                    dist.y >= 0 ? -1 : 0,
                                                    dist.z >= 0 ? -1 : 0,
                                                    dist.w >= 0 ? -1 : 0));
    }

    *out_visible = visible;
}
```

OUTPUT: Fast BVH-based culling ready for scene traversal.
```

---

## Agent 2.6: Instancing System

```
TASK: Implement GPU Instancing System (Phase 2, Agent 6)

You are implementing GPU instancing for efficient rendering of repeated geometry.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/geometry/instancing/

FILES TO COMPLETE:
- instance_buffer.c - Per-instance data management
- instance_culler.c - GPU instance culling
- indirect_instancing.c - Indirect draw support

METAL INSTANCING:
```c
typedef struct instance_data {
    simd_float4x4 transform;
    simd_float4 custom;  // User data (color, ID, etc.)
} instance_data_t;

typedef struct instance_batch {
    mesh_gpu_data_t* mesh;
    id<MTLBuffer> instance_buffer;
    id<MTLBuffer> indirect_buffer;  // MTLDrawIndexedPrimitivesIndirectArguments
    uint32_t max_instances;
    uint32_t visible_count;
} instance_batch_t;

// Create instance batch
instance_batch_t* instance_batch_create(metal_device_t* dev, mesh_gpu_data_t* mesh, uint32_t max_instances) {
    instance_batch_t* batch = calloc(1, sizeof(instance_batch_t));
    batch->mesh = mesh;
    batch->max_instances = max_instances;

    batch->instance_buffer = [dev->device newBufferWithLength:max_instances * sizeof(instance_data_t)
                                                      options:MTLResourceStorageModeShared];

    batch->indirect_buffer = [dev->device newBufferWithLength:sizeof(MTLDrawIndexedPrimitivesIndirectArguments)
                                                      options:MTLResourceStorageModeShared];

    return batch;
}

// Draw instanced
void instance_batch_draw(metal_command_buffer_t* cmd, instance_batch_t* batch) {
    [cmd->render_encoder setVertexBuffer:batch->mesh->vertex_buffer offset:0 atIndex:0];
    [cmd->render_encoder setVertexBuffer:batch->instance_buffer offset:0 atIndex:1];

    [cmd->render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                    indexCount:batch->mesh->index_count
                                     indexType:batch->mesh->index_type
                                   indexBuffer:batch->mesh->index_buffer
                             indexBufferOffset:0
                                 instanceCount:batch->visible_count];
}

// GPU culling (compute shader writes visible instances + count)
void instance_batch_cull_gpu(metal_command_buffer_t* cmd, instance_batch_t* batch, frustum_t* frustum) {
    // Dispatch compute shader that:
    // 1. Tests each instance against frustum
    // 2. Appends visible instances to output buffer
    // 3. Updates indirect args with visible count
}
```

OUTPUT: GPU instancing system for efficient foliage, debris, etc.
```
