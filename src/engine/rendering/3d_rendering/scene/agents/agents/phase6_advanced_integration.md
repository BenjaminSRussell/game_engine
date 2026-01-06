# Phase 6: Advanced Systems & Integration (Metal)

## Overview
Phase 6 completes the rendering engine with advanced GPU-driven techniques, animation systems, and editor/asset tools. All implementations use Metal API exclusively.

---

## Agent 6.1: GPU-Driven Rendering (Nanite-Style)

### Objective
Implement a GPU-driven mesh rendering system with cluster-based LOD, software rasterization for small triangles, and persistent culling.

### Files to Implement
Located in `geometry/nanite/` and `geometry/cluster/`:
- `cluster_builder.c` - Build mesh clusters at import time
- `cluster_hierarchy.c` - BVH-style cluster DAG
- `cluster_lod.c` - Runtime LOD selection on GPU
- `visibility_buffer.c` - Visibility buffer rendering
- `software_rasterizer.c` - Compute-based small triangle raster
- `cluster_streaming.c` - Virtual geometry streaming

### Implementation Requirements

```c
// cluster_builder.c
#define CLUSTER_TRIANGLE_COUNT 128
#define CLUSTER_VERTEX_COUNT 256

typedef struct mesh_cluster {
    uint32_t vertex_offset;
    uint32_t index_offset;
    uint32_t triangle_count;
    simd_float3 bounds_center;
    simd_float3 bounds_extent;
    float lod_error;           // Screen-space error threshold
    uint32_t parent_cluster;   // For DAG
    uint32_t child_clusters[8];
    uint32_t child_count;
} mesh_cluster_t;

typedef struct cluster_mesh {
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
    id<MTLBuffer> cluster_buffer;
    uint32_t cluster_count;
    uint32_t total_triangles;
} cluster_mesh_t;

// Build clusters from input mesh using meshoptimizer-style algorithm
cluster_mesh_t* cluster_mesh_build(id<MTLDevice> device,
                                   const vertex_t* vertices, uint32_t vertex_count,
                                   const uint32_t* indices, uint32_t index_count);
```

```c
// visibility_buffer.c
typedef struct visibility_buffer {
    id<MTLTexture> visibility;    // R32UI - cluster_id | triangle_id
    id<MTLTexture> depth;         // Depth32Float
    uint32_t width, height;
} visibility_buffer_t;

// Render clusters to visibility buffer
void visibility_render(id<MTLRenderCommandEncoder> encoder,
                       visibility_buffer_t* vis_buffer,
                       const cluster_mesh_t* mesh,
                       id<MTLBuffer> visible_clusters,
                       uint32_t visible_count);

// Material pass - resolve visibility to final color
void visibility_material_pass(id<MTLRenderCommandEncoder> encoder,
                              visibility_buffer_t* vis_buffer,
                              gbuffer_t* gbuffer);
```

```c
// Cluster culling compute shader (Metal Shading Language)
/*
kernel void cull_clusters(device const mesh_cluster* clusters [[buffer(0)]],
                          device uint* visible_clusters [[buffer(1)]],
                          device atomic_uint* visible_count [[buffer(2)]],
                          constant CullUniforms& uniforms [[buffer(3)]],
                          uint cluster_id [[thread_position_in_grid]])
{
    mesh_cluster cluster = clusters[cluster_id];

    // Frustum cull
    if (!frustum_test(cluster.bounds_center, cluster.bounds_extent, uniforms.frustum))
        return;

    // Occlusion cull against HZB
    if (is_occluded(cluster.bounds_center, cluster.bounds_extent, uniforms.hzb))
        return;

    // LOD selection - compare screen-space error
    float screen_error = compute_screen_error(cluster.lod_error,
                                               cluster.bounds_center,
                                               uniforms.view_pos);
    if (screen_error > uniforms.lod_threshold && cluster.child_count > 0)
        return; // Children will be rendered instead

    // Add to visible list
    uint idx = atomic_fetch_add_explicit(visible_count, 1, memory_order_relaxed);
    visible_clusters[idx] = cluster_id;
}
*/
```

### Success Criteria
- [ ] Meshes automatically clustered at 128 triangles per cluster
- [ ] GPU cluster culling with frustum + occlusion
- [ ] Visibility buffer rendered correctly
- [ ] Material pass resolves to G-buffer
- [ ] Performance: <1ms for 1M triangles on M1

---

## Agent 6.2: Skeletal Animation System

### Objective
Implement GPU-accelerated skeletal animation with bone matrices, skinning, and animation blending.

### Files to Implement
Located in `character/animation/` and `character/skeleton/`:
- `skeleton_data.c` - Bone hierarchy and bind pose
- `animation_clip.c` - Keyframe animation data
- `animation_sampler.c` - Sample animation at time
- `animation_blender.c` - Blend multiple animations
- `gpu_skinning.c` - Compute shader skinning
- `animation_state_machine.c` - State transitions

### Implementation Requirements

```c
// skeleton_data.c
#define MAX_BONES 256
#define MAX_BONE_INFLUENCES 4

typedef struct bone {
    char name[64];
    int32_t parent_index;      // -1 for root
    simd_float4x4 local_bind;  // Local bind pose
    simd_float4x4 inv_bind;    // Inverse bind matrix
} bone_t;

typedef struct skeleton {
    bone_t* bones;
    uint32_t bone_count;
    uint32_t root_bone;
} skeleton_t;

skeleton_t* skeleton_create(const bone_t* bones, uint32_t count);
void skeleton_destroy(skeleton_t* skeleton);
```

```c
// animation_clip.c
typedef struct keyframe {
    float time;
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
} keyframe_t;

typedef struct bone_track {
    uint32_t bone_index;
    keyframe_t* keyframes;
    uint32_t keyframe_count;
} bone_track_t;

typedef struct animation_clip {
    char name[64];
    float duration;
    float ticks_per_second;
    bone_track_t* tracks;
    uint32_t track_count;
    bool looping;
} animation_clip_t;

// Sample bone transform at time
void animation_sample(const animation_clip_t* clip, float time,
                      simd_float4x4* bone_transforms, uint32_t bone_count);
```

```c
// gpu_skinning.c
typedef struct skinned_mesh {
    id<MTLBuffer> vertex_buffer;      // Source vertices with bone weights
    id<MTLBuffer> skinned_buffer;     // Output skinned vertices
    id<MTLBuffer> bone_matrices;      // Current pose matrices
    uint32_t vertex_count;
} skinned_mesh_t;

// Upload bone matrices and dispatch skinning compute
void skinned_mesh_update(id<MTLComputeCommandEncoder> encoder,
                         skinned_mesh_t* mesh,
                         const simd_float4x4* bone_matrices,
                         uint32_t bone_count);
```

```c
// Skinning compute shader
/*
struct SkinVertex {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
    uint4 bone_indices;
    float4 bone_weights;
};

kernel void compute_skinning(device const SkinVertex* input [[buffer(0)]],
                             device Vertex* output [[buffer(1)]],
                             device const float4x4* bones [[buffer(2)]],
                             uint vid [[thread_position_in_grid]])
{
    SkinVertex v = input[vid];

    float4x4 skin_matrix = bones[v.bone_indices.x] * v.bone_weights.x
                         + bones[v.bone_indices.y] * v.bone_weights.y
                         + bones[v.bone_indices.z] * v.bone_weights.z
                         + bones[v.bone_indices.w] * v.bone_weights.w;

    output[vid].position = (skin_matrix * float4(v.position, 1.0)).xyz;
    output[vid].normal = normalize((skin_matrix * float4(v.normal, 0.0)).xyz);
    output[vid].tangent = float4(normalize((skin_matrix * float4(v.tangent.xyz, 0.0)).xyz),
                                  v.tangent.w);
    output[vid].uv = v.uv;
}
*/
```

### Success Criteria
- [ ] Skeleton with up to 256 bones supported
- [ ] Animation clips sample correctly with interpolation
- [ ] GPU skinning compute shader functional
- [ ] Animation blending between states
- [ ] Performance: <0.5ms for 100 animated characters

---

## Agent 6.3: Cloth & Hair Simulation

### Objective
Implement GPU-based cloth simulation and hair strand rendering.

### Files to Implement
Located in `character/cloth/` and `character/hair/`:
- `cloth_simulation.c` - Position-based dynamics cloth
- `cloth_constraints.c` - Distance and bending constraints
- `cloth_collision.c` - Sphere/capsule collision
- `hair_strand.c` - Hair strand representation
- `hair_simulation.c` - Hair physics simulation
- `hair_rendering.c` - Hair shading model

### Implementation Requirements

```c
// cloth_simulation.c
#define CLOTH_SOLVER_ITERATIONS 4

typedef struct cloth_particle {
    simd_float3 position;
    simd_float3 prev_position;
    simd_float3 velocity;
    float inv_mass;            // 0 for pinned particles
} cloth_particle_t;

typedef struct cloth_constraint {
    uint32_t particle_a;
    uint32_t particle_b;
    float rest_length;
    float stiffness;
} cloth_constraint_t;

typedef struct cloth_mesh {
    id<MTLBuffer> particles;
    id<MTLBuffer> constraints;
    id<MTLBuffer> vertex_output;
    uint32_t particle_count;
    uint32_t constraint_count;
    uint32_t width, height;    // Grid dimensions
} cloth_mesh_t;

void cloth_simulate_step(id<MTLComputeCommandEncoder> encoder,
                         cloth_mesh_t* cloth, float delta_time,
                         simd_float3 gravity, simd_float3 wind);
```

```c
// hair_strand.c
#define HAIR_SEGMENTS_PER_STRAND 16

typedef struct hair_strand {
    simd_float3 positions[HAIR_SEGMENTS_PER_STRAND];
    simd_float3 velocities[HAIR_SEGMENTS_PER_STRAND];
    float length;
    float stiffness;
} hair_strand_t;

typedef struct hair_system {
    id<MTLBuffer> strands;
    id<MTLBuffer> render_vertices;
    uint32_t strand_count;
    id<MTLTexture> density_map;    // Where hair grows
} hair_system_t;

// Render hair using line strips or tessellation
void hair_render(id<MTLRenderCommandEncoder> encoder,
                 hair_system_t* hair,
                 const simd_float4x4* view_proj);
```

```c
// Cloth solver compute shader
/*
kernel void cloth_solve_constraints(device ClothParticle* particles [[buffer(0)]],
                                    device const ClothConstraint* constraints [[buffer(1)]],
                                    uint cid [[thread_position_in_grid]])
{
    ClothConstraint c = constraints[cid];

    float3 p1 = particles[c.particle_a].position;
    float3 p2 = particles[c.particle_b].position;

    float3 delta = p2 - p1;
    float current_length = length(delta);
    float diff = (current_length - c.rest_length) / current_length;

    float w1 = particles[c.particle_a].inv_mass;
    float w2 = particles[c.particle_b].inv_mass;
    float w_sum = w1 + w2;

    if (w_sum > 0.0001) {
        float3 correction = delta * diff * c.stiffness;
        particles[c.particle_a].position += correction * (w1 / w_sum);
        particles[c.particle_b].position -= correction * (w2 / w_sum);
    }
}
*/
```

### Success Criteria
- [ ] Cloth simulation with distance/bending constraints
- [ ] Pinned particles for attachment points
- [ ] Collision with character capsules
- [ ] Hair strands with proper physics
- [ ] Kajiya-Kay or Marschner hair shading
- [ ] Performance: <2ms for moderate cloth + hair

---

## Agent 6.4: Asset Import Pipeline

### Objective
Implement asset importers for common 3D formats with Metal resource creation.

### Files to Implement
Located in `assets/import/` and `assets/processing/`:
- `gltf_importer.c` - glTF 2.0 importer
- `obj_importer.c` - Wavefront OBJ importer
- `texture_importer.c` - Image loading to MTLTexture
- `mesh_optimizer.c` - Optimize meshes for GPU
- `material_converter.c` - Convert to PBR materials
- `asset_cache.c` - Compiled asset caching

### Implementation Requirements

```c
// gltf_importer.c
typedef struct gltf_import_result {
    mesh_t** meshes;
    uint32_t mesh_count;
    material_t** materials;
    uint32_t material_count;
    skeleton_t* skeleton;
    animation_clip_t** animations;
    uint32_t animation_count;
    scene_node_t* root_node;
} gltf_import_result_t;

// Import glTF file, creating Metal resources
gltf_import_result_t* gltf_import(id<MTLDevice> device,
                                   const char* filepath);
void gltf_import_result_destroy(gltf_import_result_t* result);
```

```c
// texture_importer.c
typedef struct texture_import_options {
    bool generate_mipmaps;
    bool srgb;
    MTLPixelFormat force_format;  // 0 for auto
    bool compress_bc;             // Use BC compression if supported
} texture_import_options_t;

id<MTLTexture> texture_import(id<MTLDevice> device,
                               const char* filepath,
                               const texture_import_options_t* options);

id<MTLTexture> texture_import_hdr(id<MTLDevice> device,
                                   const char* filepath);
```

```c
// mesh_optimizer.c
typedef struct mesh_optimize_options {
    bool optimize_vertex_cache;
    bool optimize_overdraw;
    bool optimize_fetch;
    bool generate_lods;
    uint32_t lod_count;
    float lod_error_threshold;
} mesh_optimize_options_t;

// Optimize mesh and optionally generate LODs
void mesh_optimize(mesh_t* mesh, const mesh_optimize_options_t* options);
```

```c
// asset_cache.c
// Cache compiled assets to avoid reimport
typedef struct asset_cache {
    char cache_directory[PATH_MAX];
    // Hash map of source path -> cached binary
} asset_cache_t;

// Check if cached version exists and is newer than source
bool asset_cache_check(asset_cache_t* cache, const char* source_path,
                       uint64_t* cached_timestamp);

// Load cached binary directly into Metal resources
bool asset_cache_load(asset_cache_t* cache, const char* source_path,
                      id<MTLDevice> device, void** out_asset);

// Save compiled asset to cache
void asset_cache_save(asset_cache_t* cache, const char* source_path,
                      const void* asset_data, size_t asset_size);
```

### Success Criteria
- [ ] glTF 2.0 import with meshes, materials, animations
- [ ] OBJ import for simple meshes
- [ ] PNG/JPG/HDR texture import
- [ ] Mesh optimization with LOD generation
- [ ] Asset caching for fast reload
- [ ] Material conversion to PBR format

---

## Agent 6.5: Editor Tools & Debug Visualization

### Objective
Implement editor tools including gizmos, object picking, and debug visualization.

### Files to Implement
Located in `editor/gizmos/` and `editor/debug/`:
- `transform_gizmo.c` - Translate/rotate/scale gizmos
- `object_picker.c` - Mouse picking with GPU
- `debug_renderer.c` - Debug lines, shapes, text
- `grid_renderer.c` - Editor grid
- `bounds_renderer.c` - AABB/OBB visualization
- `profiler_overlay.c` - GPU timing overlay

### Implementation Requirements

```c
// transform_gizmo.c
typedef enum gizmo_mode {
    GIZMO_MODE_TRANSLATE,
    GIZMO_MODE_ROTATE,
    GIZMO_MODE_SCALE
} gizmo_mode_t;

typedef enum gizmo_space {
    GIZMO_SPACE_LOCAL,
    GIZMO_SPACE_WORLD
} gizmo_space_t;

typedef struct transform_gizmo {
    gizmo_mode_t mode;
    gizmo_space_t space;
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
    int active_axis;           // -1 = none, 0=X, 1=Y, 2=Z, 3=XY, etc.
    bool is_dragging;
} transform_gizmo_t;

void gizmo_render(id<MTLRenderCommandEncoder> encoder,
                  transform_gizmo_t* gizmo,
                  const simd_float4x4* view,
                  const simd_float4x4* proj);

// Returns axis index if hit, -1 otherwise
int gizmo_pick(transform_gizmo_t* gizmo, simd_float3 ray_origin, simd_float3 ray_dir);

// Update transform based on mouse drag
void gizmo_drag(transform_gizmo_t* gizmo, simd_float3 ray_origin, simd_float3 ray_dir,
                simd_float4x4* out_transform);
```

```c
// object_picker.c
typedef struct object_picker {
    id<MTLTexture> id_buffer;      // R32UI - object ID per pixel
    id<MTLBuffer> readback_buffer;
    uint32_t width, height;
} object_picker_t;

// Render scene with object IDs
void picker_render_ids(id<MTLRenderCommandEncoder> encoder,
                       object_picker_t* picker,
                       const renderable_t* objects, uint32_t count);

// Read back object ID at screen position
uint32_t picker_query(object_picker_t* picker, uint32_t x, uint32_t y);
```

```c
// debug_renderer.c
typedef struct debug_renderer {
    id<MTLBuffer> line_buffer;
    id<MTLBuffer> text_buffer;
    uint32_t line_count;
    uint32_t max_lines;
    id<MTLTexture> font_atlas;
} debug_renderer_t;

void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end,
                     simd_float4 color);
void debug_draw_box(debug_renderer_t* dbg, simd_float3 min, simd_float3 max,
                    simd_float4 color);
void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius,
                       simd_float4 color);
void debug_draw_text_3d(debug_renderer_t* dbg, simd_float3 position,
                        const char* text, simd_float4 color);

void debug_render(id<MTLRenderCommandEncoder> encoder, debug_renderer_t* dbg,
                  const simd_float4x4* view_proj);
void debug_clear(debug_renderer_t* dbg);
```

```c
// profiler_overlay.c
typedef struct gpu_profiler {
    id<MTLCounterSampleBuffer> counter_buffer;  // If available
    double frame_times[120];
    uint32_t frame_index;
    // Per-pass timing
    struct {
        char name[32];
        double time_ms;
    } pass_times[32];
    uint32_t pass_count;
} gpu_profiler_t;

void profiler_begin_frame(gpu_profiler_t* profiler);
void profiler_begin_pass(gpu_profiler_t* profiler, const char* name);
void profiler_end_pass(gpu_profiler_t* profiler);
void profiler_end_frame(gpu_profiler_t* profiler);

void profiler_render_overlay(id<MTLRenderCommandEncoder> encoder,
                             gpu_profiler_t* profiler);
```

### Success Criteria
- [ ] Transform gizmo for translate/rotate/scale
- [ ] GPU-based object picking
- [ ] Debug line/shape rendering
- [ ] 3D text rendering
- [ ] Infinite grid for editor
- [ ] GPU profiler with frame graph

---

## Agent 6.6: Integration & Testing

### Objective
Integrate all systems and create comprehensive test framework.

### Files to Implement
Located in `integration/` and `testing/`:
- `render_world.c` - World rendering orchestration
- `scene_renderer.c` - Complete scene render path
- `render_config.c` - Runtime configuration
- `render_tests.c` - Unit tests for rendering
- `visual_tests.c` - Visual regression tests
- `benchmark_suite.c` - Performance benchmarks

### Implementation Requirements

```c
// render_world.c
typedef struct render_world {
    // All rendering systems
    metal_device_t* device;
    render_graph_t* graph;
    gbuffer_t* gbuffer;
    shadow_system_t* shadows;
    light_system_t* lights;
    material_system_t* materials;

    // Effect systems
    particle_system_t* particles;
    ocean_system_t* ocean;
    atmosphere_system_t* atmosphere;

    // Post-processing
    taa_state_t* taa;
    bloom_state_t* bloom;

    // Editor
    debug_renderer_t* debug;
    transform_gizmo_t* gizmo;

    // Config
    render_config_t config;
} render_world_t;

render_world_t* render_world_create(id<MTLDevice> device, uint32_t width, uint32_t height);
void render_world_destroy(render_world_t* world);
void render_world_resize(render_world_t* world, uint32_t width, uint32_t height);
void render_world_render(render_world_t* world, const scene_t* scene, const camera_t* camera);
```

```c
// scene_renderer.c
// Complete frame rendering sequence
void scene_render_frame(render_world_t* world, const scene_t* scene, const camera_t* camera) {
    // 1. Update animations
    animation_system_update(world, scene);

    // 2. Cull visible objects
    visibility_cull(world, scene, camera);

    // 3. Update GPU particles
    particles_update(world->particles, delta_time);

    // 4. Shadow pass
    shadows_render(world->shadows, scene);

    // 5. G-buffer pass
    gbuffer_render(world->gbuffer, scene, camera);

    // 6. Deferred lighting
    deferred_lighting_render(world, camera);

    // 7. Forward transparent
    forward_render_transparent(world, scene, camera);

    // 8. Environment (sky, ocean)
    atmosphere_render(world->atmosphere, camera);
    ocean_render(world->ocean, camera);

    // 9. Post-processing
    taa_resolve(world->taa);
    bloom_apply(world->bloom);
    tonemap_apply(world);

    // 10. Editor overlays
    debug_render(world->debug, camera);

    // 11. Present
    swapchain_present(world);
}
```

```c
// render_tests.c
typedef struct render_test_context {
    id<MTLDevice> device;
    render_world_t* world;
    id<MTLTexture> reference_image;
    float psnr_threshold;
} render_test_context_t;

// Visual regression test
bool test_visual_regression(render_test_context_t* ctx, const char* test_name,
                            const scene_t* scene, const camera_t* camera);

// Performance benchmark
typedef struct benchmark_result {
    char name[64];
    double min_ms;
    double max_ms;
    double avg_ms;
    uint32_t sample_count;
} benchmark_result_t;

benchmark_result_t benchmark_run(render_test_context_t* ctx, const char* name,
                                  void (*bench_func)(void*), void* user_data,
                                  uint32_t iterations);
```

### Success Criteria
- [ ] Complete render_world integration
- [ ] Full frame rendering path works end-to-end
- [ ] Runtime quality/performance settings
- [ ] Unit tests for each system
- [ ] Visual regression test framework
- [ ] Performance benchmark suite
- [ ] All systems working together without errors

---

## Phase 6 Completion Checklist

When all agents complete, verify:
- [ ] GPU-driven cluster rendering functional
- [ ] Skeletal animation with GPU skinning
- [ ] Cloth/hair simulation running
- [ ] Asset import pipeline working
- [ ] Editor tools functional
- [ ] Full integration tested
- [ ] Performance targets met
- [ ] No Vulkan/D3D12 references remaining
- [ ] All files properly organized in subdirectories
