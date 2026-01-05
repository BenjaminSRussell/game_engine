# Phase 2: Geometry & Mesh Systems - Agent Prompts

## Agent 2.1: Mesh Data & Loading

```
TASK: Implement Mesh Data Structures & Asset Loading (Phase 2, Agent 1)

You are implementing mesh data structures and asset loading for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/geometry/mesh/
- src/engine/rendering/3d_rendering/geometry/vertex/
- src/engine/rendering/3d_rendering/asset_system/loading/
- src/engine/rendering/3d_rendering/asset_system/formats/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. mesh_data.c - Mesh vertex/index storage, submesh management
2. mesh_loader.c - Load meshes from disk asynchronously
3. mesh_builder.c - Procedural mesh generation API
4. vertex_format.c - Flexible vertex attribute layouts
5. vertex_buffer_pool.c - Vertex buffer sub-allocation
6. index_buffer_pool.c - Index buffer sub-allocation
7. asset_loader.c - General async asset loading system
8. async_loading.c - Background thread loading
9. mesh_format.c - Binary mesh format read/write
10. mesh_bounds.c - AABB and bounding sphere calculation

IMPLEMENTATION GUIDELINES:
- Support interleaved and separate vertex streams
- Async loading with priority queue
- Mesh format should be GPU-ready (minimal processing on load)
- Calculate bounds during load or generation
- Support 16-bit and 32-bit indices

KEY PATTERNS:
```c
typedef struct vertex_p3n3t2 {
    vec3_t position;
    vec3_t normal;
    vec2_t texcoord;
} vertex_p3n3t2_t;

typedef struct mesh_data {
    void* vertex_data;
    void* index_data;
    uint32_t vertex_count;
    uint32_t index_count;
    vertex_format_t format;
    index_type_t index_type;
    aabb_t bounds;
    submesh_t* submeshes;
    uint32_t submesh_count;
} mesh_data_t;

// Async loading
mesh_handle_t mesh = mesh_load_async("model.mesh", PRIORITY_HIGH);
if (mesh_is_loaded(mesh)) {
    // Use mesh
}
```

FOCUS: Efficient mesh data layout is critical for GPU performance. Get vertex formats right.
```

---

## Agent 2.2: LOD & Streaming

```
TASK: Implement LOD Generation & Mesh Streaming (Phase 2, Agent 2)

You are implementing LOD systems and mesh streaming for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/geometry/lod/
- src/engine/rendering/3d_rendering/geometry/streaming/
- src/engine/rendering/3d_rendering/lod_generation/
- src/engine/rendering/3d_rendering/lod_streaming/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. lod_generator.c - Automatic LOD mesh generation (QEM simplification)
2. lod_selector.c - Runtime LOD selection based on screen size
3. lod_crossfade.c - Dithered LOD transitions
4. screen_size_lod.c - Project bounds to screen, select LOD
5. mesh_streamer.c - Stream mesh LODs on demand
6. stream_priority.c - Priority based on visibility and distance
7. resident_set.c - Track which LODs are memory-resident
8. prefetch_system.c - Predictive loading based on camera
9. lod_bias.c - Quality bias for LOD selection
10. continuous_lod.c - Geomorphing between LODs

IMPLEMENTATION GUIDELINES:
- QEM (Quadric Error Metrics) for mesh simplification
- Screen-space error threshold for LOD selection
- Hysteresis to prevent LOD popping
- Stream highest needed LOD first
- Support forced LOD for debugging

KEY PATTERNS:
```c
typedef struct lod_mesh {
    mesh_handle_t lods[MAX_LOD_LEVELS];
    float screen_sizes[MAX_LOD_LEVELS];  // Threshold to switch
    uint32_t lod_count;
} lod_mesh_t;

// LOD selection
uint32_t select_lod(lod_mesh_t* mesh, float screen_coverage) {
    for (uint32_t i = 0; i < mesh->lod_count - 1; i++) {
        if (screen_coverage > mesh->screen_sizes[i]) {
            return i;
        }
    }
    return mesh->lod_count - 1;
}

// Streaming request
stream_request(mesh, target_lod, PRIORITY_HIGH);
```

FOCUS: Good LOD selection prevents geometry bottlenecks. Streaming enables massive worlds.
```

---

## Agent 2.3: Instancing & Batching

```
TASK: Implement GPU Instancing & Draw Batching (Phase 2, Agent 3)

You are implementing GPU instancing and draw batching for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/geometry/instancing/
- src/engine/rendering/3d_rendering/geometry/bvh/
- src/engine/rendering/3d_rendering/instanced_static_mesh/
- src/engine/rendering/3d_rendering/scene_management/batching/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. instance_buffer.c - Per-instance transform/data buffers
2. instance_culler.c - CPU/GPU instance culling
3. instance_batching.c - Batch instances by mesh+material
4. gpu_instance_data.c - GPU-side instance storage
5. indirect_instancing.c - Indirect draw for GPU-driven
6. bvh_builder.c - BVH construction for scene
7. bvh_traversal.c - BVH traversal for culling
8. static_batching.c - Merge static meshes
9. dynamic_batching.c - Runtime batching of small objects
10. batch_statistics.c - Track batch counts, draw calls

IMPLEMENTATION GUIDELINES:
- Use indirect draws for variable instance counts
- BVH for hierarchical culling
- Sort instances front-to-back for depth
- Batch by pipeline state to minimize switches
- Support per-instance custom data (color, flags)

KEY PATTERNS:
```c
typedef struct instance_data {
    mat4_t transform;
    vec4_t custom;  // User-defined per-instance data
} instance_data_t;

typedef struct instance_batch {
    mesh_handle_t mesh;
    material_handle_t material;
    buffer_handle_t instance_buffer;
    uint32_t instance_count;
    uint32_t instance_offset;
} instance_batch_t;

// Indirect draw
typedef struct draw_indirect_command {
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t first_vertex;
    uint32_t first_instance;
} draw_indirect_command_t;

// GPU fills instance_count after culling
```

FOCUS: Minimize draw calls through batching. GPU culling enables massive instance counts.
```

---

## Agent 2.4: Meshlets & Nanite Foundation

```
TASK: Implement Meshlets & Nanite Cluster System (Phase 2, Agent 4)

You are implementing meshlet generation and Nanite-style cluster system for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/geometry/meshlets/
- src/engine/rendering/3d_rendering/nanite/cluster/
- src/engine/rendering/3d_rendering/nanite/streaming/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. meshlet_builder.c - Generate meshlets from mesh (64 verts, 124 tris)
2. meshlet_bounds.c - Per-meshlet bounding cone/sphere
3. meshlet_culling.c - Cone culling, frustum culling per meshlet
4. cluster_builder.c - Build cluster hierarchy (DAG)
5. cluster_dag.c - Cluster DAG data structure
6. cluster_bounds.c - Cluster bounding volumes
7. cluster_lod.c - Cluster LOD selection
8. nanite_streamer.c - Stream cluster pages
9. page_cache.c - LRU cache for cluster data
10. residency_manager.c - Track resident clusters

IMPLEMENTATION GUIDELINES:
- Meshlet: 64 vertices, 124 triangles max (mesh shader limits)
- Cluster groups share boundaries for seamless LOD
- DAG allows continuous LOD without popping
- Stream clusters based on screen-space error
- Page-based streaming for efficient I/O

KEY PATTERNS:
```c
typedef struct meshlet {
    uint32_t vertex_offset;
    uint32_t vertex_count;
    uint32_t triangle_offset;
    uint32_t triangle_count;
    vec3_t cone_apex;
    vec3_t cone_axis;
    float cone_cutoff;
    sphere_t bounding_sphere;
} meshlet_t;

typedef struct cluster {
    uint32_t meshlet_offset;
    uint32_t meshlet_count;
    uint32_t parent_cluster;
    uint32_t child_clusters[8];
    float error;  // Screen-space error at which to use this LOD
    sphere_t bounds;
} cluster_t;

// Meshlet culling (GPU)
bool cull_meshlet(meshlet_t m, vec3_t view_pos) {
    // Cone culling
    vec3_t to_apex = normalize(m.cone_apex - view_pos);
    if (dot(to_apex, m.cone_axis) < m.cone_cutoff) return true;
    return false;  // Visible
}
```

FOCUS: Meshlets enable mesh shaders. Nanite clusters enable infinite geometry detail.
```

---

## Agent 2.5: Culling Systems

```
TASK: Implement Visibility Culling Systems (Phase 2, Agent 5)

You are implementing culling systems for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/culling/
- src/engine/rendering/3d_rendering/gpu_culling/
- src/engine/rendering/3d_rendering/occlusion/
- src/engine/rendering/3d_rendering/gpu_occlusion/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. frustum_planes.c - Extract frustum planes from view-projection
2. frustum_aabb_test.c - AABB vs frustum test (SIMD)
3. simd_frustum_cull.c - Batch frustum culling with SIMD
4. hzb_builder.c - Build hierarchical Z-buffer
5. hzb_test.c - Test bounds against HZB
6. sw_rasterizer.c - Software rasterizer for occlusion
7. occlusion_query.c - GPU occlusion queries
8. gpu_cull_compute.c - GPU compute culling shader support
9. two_pass_cull.c - Two-phase occlusion culling
10. temporal_occlusion.c - Use previous frame depth

IMPLEMENTATION GUIDELINES:
- Frustum culling should be SIMD (test 4-8 AABBs at once)
- HZB is mip-chain of depth, test at appropriate mip level
- Two-phase: coarse cull, render occluders, fine cull
- GPU culling writes visible indices to buffer
- Temporal reprojection for stable occlusion

KEY PATTERNS:
```c
// Frustum planes
typedef struct frustum {
    vec4_t planes[6];  // left, right, top, bottom, near, far
} frustum_t;

// SIMD AABB test
int frustum_test_aabb_simd(frustum_t* f, aabb_t* aabbs, uint32_t count, uint32_t* results) {
    // Test 4 AABBs at once using SIMD
    // Return count of visible
}

// HZB test
bool hzb_test_bounds(texture_handle_t hzb, aabb_t bounds, mat4_t vp) {
    vec2_t screen_min, screen_max;
    float min_z;
    project_aabb(bounds, vp, &screen_min, &screen_max, &min_z);

    // Select mip level based on screen size
    int mip = calculate_hzb_mip(screen_max - screen_min);
    float hzb_z = sample_hzb(hzb, (screen_min + screen_max) * 0.5, mip);

    return min_z <= hzb_z;  // Visible if closer than HZB
}
```

FOCUS: Culling is critical for performance. Cull as much as possible as early as possible.
```

---

## Agent 2.6: Static & Skeletal Mesh Rendering

```
TASK: Implement Mesh Rendering Passes (Phase 2, Agent 6)

You are implementing the core mesh rendering passes for a high-performance 3D rendering engine in C.

DIRECTORIES TO WORK IN:
- src/engine/rendering/3d_rendering/static_mesh_rendering/
- src/engine/rendering/3d_rendering/skeletal_mesh_rendering/
- src/engine/rendering/3d_rendering/rendering/forward/
- src/engine/rendering/3d_rendering/rendering/deferred/

YOUR RESPONSIBILITIES (implement ~100 TODOs):
1. static_mesh_draw.c - Draw static meshes (batched)
2. skeletal_mesh_draw.c - Draw skinned meshes
3. forward_pass.c - Forward rendering main pass
4. forward_plus.c - Forward+ with light culling
5. forward_transparency.c - Transparent object pass
6. gbuffer_pass.c - G-buffer geometry pass
7. gbuffer_layout.c - G-buffer format definition
8. deferred_lighting.c - Deferred lighting pass
9. depth_prepass.c - Early Z pass
10. mesh_sorting.c - Sort by material, depth, etc.

IMPLEMENTATION GUIDELINES:
- Depth prepass for complex scenes (reduces overdraw)
- G-buffer: albedo (RGB), normal (RG), roughness/metallic (RG), depth
- Forward+ for transparent, deferred for opaque
- Sort opaque front-to-back, transparent back-to-front
- Skeletal meshes need bone matrix upload

KEY PATTERNS:
```c
// G-buffer layout
typedef struct gbuffer {
    texture_handle_t albedo;      // RGBA8: albedo.rgb, ao.a
    texture_handle_t normal;      // RG16F: normal.xy (reconstruct z)
    texture_handle_t material;    // RGBA8: roughness, metallic, flags, ...
    texture_handle_t depth;       // D32F
} gbuffer_t;

// Rendering flow
void render_frame(scene_t* scene, camera_t* camera) {
    // 1. Depth prepass (optional)
    render_depth_prepass(scene, camera);

    // 2. G-buffer pass
    render_gbuffer(scene, camera);

    // 3. Deferred lighting
    render_deferred_lighting(gbuffer, lights);

    // 4. Forward transparency
    render_transparent(scene, camera);
}

// Skeletal mesh
void upload_bone_matrices(skeletal_mesh_t* mesh, mat4_t* bones, uint32_t count) {
    buffer_update(mesh->bone_buffer, bones, count * sizeof(mat4_t));
}
```

FOCUS: This is where triangles become pixels. Get the rendering pipeline flow correct.
```
