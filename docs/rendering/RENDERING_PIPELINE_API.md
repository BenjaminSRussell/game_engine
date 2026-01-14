# Rendering Pipeline API Documentation

## Overview

The rendering pipeline provides a comprehensive, multi-threaded, GPU-accelerated rendering system with support for forward, deferred, and hybrid rendering approaches. The system includes advanced features like ray tracing, variable rate shading, mesh shaders, and sophisticated culling techniques.

## Core Rendering Pipeline

### Pipeline Management

```c
// Initialize rendering pipeline
bool render_pipeline_init(const RenderPipelineConfig* config);

// Shutdown rendering pipeline
void render_pipeline_shutdown(void);

// Begin frame rendering
void render_pipeline_begin_frame(void);

// End frame rendering
void render_pipeline_end_frame(void);

// Set rendering mode
void render_pipeline_set_mode(RenderMode mode);

typedef enum {
    RENDER_MODE_FORWARD,
    RENDER_MODE_DEFERRED,
    RENDER_MODE_HYBRID,
    RENDER_MODE_RAY_TRACED
} RenderMode;
```

### Pipeline Configuration

```c
typedef struct {
    u32 width;
    u32 height;
    u32 msaa_samples;
    bool enable_ray_tracing;
    bool enable_variable_rate_shading;
    bool enable_mesh_shaders;
    bool enable_gpu_culling;
    bool enable_async_compute;
    f32 render_scale;
    RenderQuality quality;
} RenderPipelineConfig;

typedef enum {
    RENDER_QUALITY_LOW,
    RENDER_QUALITY_MEDIUM,
    RENDER_QUALITY_HIGH,
    RENDER_QUALITY_ULTRA,
    RENDER_QUALITY_CUSTOM
} RenderQuality;
```

## Render Graph System

### Graph Management

```c
// Create render graph
RenderGraph* render_graph_create(const char* name);

// Destroy render graph
void render_graph_destroy(RenderGraph* graph);

// Add pass to graph
RenderPass* render_graph_add_pass(RenderGraph* graph, const char* name, RenderPassType type);

// Build graph dependencies
bool render_graph_build(RenderGraph* graph);

// Execute graph
void render_graph_execute(RenderGraph* graph, const RenderContext* context);

typedef enum {
    RENDER_PASS_TYPE_GRAPHICS,
    RENDER_PASS_TYPE_COMPUTE,
    RENDER_PASS_TYPE_RAY_TRACING,
    RENDER_PASS_TYPE_COPY
} RenderPassType;
```

### Pass Configuration

```c
typedef struct {
    char name[64];
    RenderPassType type;
    RenderPassFlags flags;
    
    // Input/output resources
    RenderResource* inputs[16];
    RenderResource* outputs[16];
    u32 input_count;
    u32 output_count;
    
    // Shader and pipeline
    Shader* shader;
    PipelineState* pipeline;
    
    // Execution callback
    void (*execute)(RenderPass* pass, const RenderContext* context);
    
    // User data
    void* user_data;
} RenderPass;

typedef enum {
    RENDER_PASS_FLAG_NONE = 0,
    RENDER_PASS_FLAG_CLEAR_COLOR = 1 << 0,
    RENDER_PASS_FLAG_CLEAR_DEPTH = 1 << 1,
    RENDER_PASS_FLAG_CLEAR_STENCIL = 1 << 2,
    RENDER_PASS_FLAG_READ_ONLY_DEPTH = 1 << 3,
    RENDER_PASS_FLAG_READ_ONLY_STENCIL = 1 << 4
} RenderPassFlags;
```

## Resource Management

### Buffer Management

```c
// Create buffer
RenderBuffer* render_buffer_create(const RenderBufferDesc* desc);

// Destroy buffer
void render_buffer_destroy(RenderBuffer* buffer);

// Map buffer memory
void* render_buffer_map(RenderBuffer* buffer, u32 offset, u32 size);

// Unmap buffer memory
void render_buffer_unmap(RenderBuffer* buffer);

// Update buffer data
void render_buffer_update(RenderBuffer* buffer, const void* data, u32 size, u32 offset);

typedef struct {
    u32 size;
    u32 usage;
    BufferType type;
    MemoryType memory_type;
} RenderBufferDesc;

typedef enum {
    BUFFER_TYPE_VERTEX,
    BUFFER_TYPE_INDEX,
    BUFFER_TYPE_UNIFORM,
    BUFFER_TYPE_STORAGE,
    BUFFER_TYPE_INDIRECT
} BufferType;

typedef enum {
    MEMORY_TYPE_CPU_VISIBLE,
    MEMORY_TYPE_GPU_LOCAL,
    MEMORY_TYPE_CPU_TO_GPU,
    MEMORY_TYPE_GPU_TO_CPU
} MemoryType;
```

### Texture Management

```c
// Create texture
RenderTexture* render_texture_create(const RenderTextureDesc* desc);

// Destroy texture
void render_texture_destroy(RenderTexture* texture);

// Update texture data
void render_texture_update(RenderTexture* texture, const void* data, u32 size, u32 mip_level);

// Generate mipmaps
void render_texture_generate_mipmaps(RenderTexture* texture);

typedef struct {
    u32 width;
    u32 height;
    u32 depth;
    u32 mip_levels;
    u32 array_size;
    TextureFormat format;
    TextureType type;
    TextureUsage usage;
} RenderTextureDesc;

typedef enum {
    TEXTURE_TYPE_1D,
    TEXTURE_TYPE_2D,
    TEXTURE_TYPE_3D,
    TEXTURE_TYPE_CUBE,
    TEXTURE_TYPE_1D_ARRAY,
    TEXTURE_TYPE_2D_ARRAY,
    TEXTURE_TYPE_CUBE_ARRAY
} TextureType;

typedef enum {
    TEXTURE_USAGE_SAMPLED = 1 << 0,
    TEXTURE_USAGE_STORAGE = 1 << 1,
    TEXTURE_USAGE_COLOR_ATTACHMENT = 1 << 2,
    TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT = 1 << 3
} TextureUsage;
```

## Shader System

### Shader Management

```c
// Create shader from source
Shader* shader_create_from_source(const char* vertex_source, const char* fragment_source);

// Create shader from file
Shader* shader_create_from_file(const char* vertex_path, const char* fragment_path);

// Create compute shader
Shader* shader_create_compute(const char* compute_source);

// Destroy shader
void shader_destroy(Shader* shader);

// Set shader uniform
void shader_set_uniform(Shader* shader, const char* name, const void* value, UniformType type);

// Bind shader
void shader_bind(Shader* shader);

typedef enum {
    UNIFORM_TYPE_FLOAT,
    UNIFORM_TYPE_VEC2,
    UNIFORM_TYPE_VEC3,
    UNIFORM_TYPE_VEC4,
    UNIFORM_TYPE_INT,
    UNIFORM_TYPE_IVEC2,
    UNIFORM_TYPE_IVEC3,
    UNIFORM_TYPE_IVEC4,
    UNIFORM_TYPE_UINT,
    UNIFORM_TYPE_UIVEC2,
    UNIFORM_TYPE_UIVEC3,
    UNIFORM_TYPE_UIVEC4,
    UNIFORM_TYPE_MAT3,
    UNIFORM_TYPE_MAT4
} UniformType;
```

### Pipeline State

```c
// Create pipeline state
PipelineState* pipeline_state_create(const PipelineStateDesc* desc);

// Destroy pipeline state
void pipeline_state_destroy(PipelineState* state);

// Bind pipeline state
void pipeline_state_bind(PipelineState* state);

typedef struct {
    Shader* shader;
    BlendState blend;
    RasterizerState rasterizer;
    DepthStencilState depth_stencil;
    InputLayout input_layout;
    PrimitiveTopology topology;
    u32 render_target_count;
    TextureFormat render_target_formats[8];
    TextureFormat depth_format;
} PipelineStateDesc;

typedef struct {
    bool enabled;
    BlendOp color_op;
    BlendOp alpha_op;
    BlendFactor src_color;
    BlendFactor dst_color;
    BlendFactor src_alpha;
    BlendFactor dst_alpha;
} BlendState;

typedef struct {
    FillMode fill_mode;
    CullMode cull_mode;
    bool front_face_counter_clockwise;
    f32 depth_bias;
    f32 depth_bias_clamp;
    f32 slope_scaled_depth_bias;
    bool depth_clip_enable;
    bool scissor_enable;
    bool multisample_enable;
    bool antialiased_line_enable;
} RasterizerState;

typedef struct {
    bool depth_enable;
    bool depth_write_enable;
    ComparisonFunc depth_func;
    bool stencil_enable;
    u8 stencil_read_mask;
    u8 stencil_write_mask;
    StencilOpDesc front_face;
    StencilOpDesc back_face;
} DepthStencilState;
```

## Advanced Rendering Features

### Ray Tracing

```c
// Create ray tracing pipeline
RayTracingPipeline* ray_tracing_pipeline_create(const RayTracingPipelineDesc* desc);

// Destroy ray tracing pipeline
void ray_tracing_pipeline_destroy(RayTracingPipeline* pipeline);

// Create acceleration structure
AccelerationStructure* acceleration_structure_create(const AccelerationStructureDesc* desc);

// Destroy acceleration structure
void acceleration_structure_destroy(AccelerationStructure* as);

// Trace rays
void ray_tracing_trace_rays(RayTracingPipeline* pipeline, const RayTracingDispatchDesc* desc);

typedef struct {
    Shader* ray_gen_shader;
    Shader* miss_shaders[8];
    Shader* closest_hit_shaders[8];
    Shader* any_hit_shaders[8];
    Shader* intersection_shaders[8];
    u32 miss_shader_count;
    u32 closest_hit_shader_count;
    u32 any_hit_shader_count;
    u32 intersection_shader_count;
} RayTracingPipelineDesc;

typedef struct {
    AccelerationStructure* tlas;
    AccelerationStructure* blas_instances[64];
    u32 blas_instance_count;
    RenderTexture* output_texture;
    u32 width;
    u32 height;
    u32 depth;
} RayTracingDispatchDesc;
```

### Variable Rate Shading

```c
// Create VRS texture
RenderTexture* vrs_texture_create(u32 width, u32 height, VRSRate initial_rate);

// Update VRS rates
void vrs_texture_update_rates(RenderTexture* vrs_texture, const VRSRate* rates, u32 count);

// Set VRS shading rate
void vrs_set_shading_rate(VRSRate rate);

typedef enum {
    VRS_RATE_1X1,
    VRS_RATE_1X2,
    VRS_RATE_2X1,
    VRS_RATE_2X2,
    VRS_RATE_2X4,
    VRS_RATE_4X2,
    VRS_RATE_4X4
} VRSRate;
```

### Mesh Shaders

```c
// Create mesh shader pipeline
MeshShaderPipeline* mesh_shader_pipeline_create(const MeshShaderPipelineDesc* desc);

// Destroy mesh shader pipeline
void mesh_shader_pipeline_destroy(MeshShaderPipeline* pipeline);

// Dispatch mesh shaders
void mesh_shader_dispatch(MeshShaderPipeline* pipeline, u32 thread_group_x, u32 thread_group_y, u32 thread_group_z);

typedef struct {
    Shader* mesh_shader;
    Shader* amplification_shader;
    Shader* pixel_shader;
    u32 max_vertices;
    u32 max_primitives;
    u32 max_meshlets;
} MeshShaderPipelineDesc;
```

## Culling Systems

### Frustum Culling

```c
// Create frustum culler
FrustumCuller* frustum_culler_create(void);

// Destroy frustum culler
void frustum_culler_destroy(FrustumCuller* culler);

// Update frustum from camera
void frustum_culler_update(FrustumCuller* culler, const Camera* camera);

// Cull objects
u32 frustum_culler_cull(FrustumCuller* culler, const Cullable* objects, u32 object_count, 
                        bool* out_visible);

typedef struct {
    Vec3 center;
    f32 radius;
    AABB bounds;
    u32 lod_level;
    void* user_data;
} Cullable;
```

### Occlusion Culling

```c
// Create occlusion culler
OcclusionCuller* occlusion_culler_create(u32 query_count);

// Destroy occlusion culler
void occlusion_culler_destroy(OcclusionCuller* culler);

// Begin occlusion query
void occlusion_culler_begin_query(OcclusionCuller* culler, const AABB* bounds);

// End occlusion query
void occlusion_culler_end_query(OcclusionCuller* culler, u32 query_id);

// Get query result
bool occlusion_culler_get_result(OcclusionCuller* culler, u32 query_id);
```

### GPU Culling

```c
// Create GPU culler
GPCuller* gpu_culler_create(const GPCullerDesc* desc);

// Destroy GPU culler
void gpu_culler_destroy(GPCuller* culler);

// Update and execute GPU culling
void gpu_culler_execute(GPCuller* culler, const Camera* camera, u32 object_count);

// Get culled results
const GPCullResult* gpu_culler_get_results(GPCuller* culler, u32* out_count);

typedef struct {
    RenderBuffer* instance_buffer;
    RenderBuffer* command_buffer;
    RenderBuffer* indirect_buffer;
    Shader* cull_shader;
    u32 max_objects;
    bool enable_frustum_culling;
    bool enable_occlusion_culling;
} GPCullerDesc;

typedef struct {
    u32 instance_count;
    u32 start_instance;
    u32 start_index;
    u32 index_count;
} GPCullResult;
```

## Post-Processing

### Post-Processing Pipeline

```c
// Create post-processing pipeline
PostProcessPipeline* post_process_pipeline_create(const PostProcessPipelineDesc* desc);

// Destroy post-processing pipeline
void post_process_pipeline_destroy(PostProcessPipeline* pipeline);

// Add post-processing effect
void post_process_pipeline_add_effect(PostProcessPipeline* pipeline, PostProcessEffect* effect);

// Execute post-processing
void post_process_pipeline_execute(PostProcessPipeline* pipeline, RenderTexture* input, RenderTexture* output);

typedef struct {
    u32 max_effects;
    bool enable_tone_mapping;
    bool enable_bloom;
    bool enable_anti_aliasing;
    bool enable_dof;
    bool enable_motion_blur;
} PostProcessPipelineDesc;
```

### Effects

```c
// Create bloom effect
BloomEffect* bloom_effect_create(const BloomDesc* desc);

// Create tone mapping effect
ToneMapEffect* tone_map_effect_create(const ToneMapDesc* desc);

// Create depth of field effect
DOFEffect* dof_effect_create(const DOFDesc* desc);

// Create motion blur effect
MotionBlurEffect* motion_blur_effect_create(const MotionBlurDesc* desc);

typedef struct {
    f32 threshold;
    f32 intensity;
    u32 blur_iterations;
    f32 blur_scale;
} BloomDesc;

typedef struct {
    ToneMapOperator operator;
    f32 exposure;
    f32 gamma;
    f32 white_point;
} ToneMapDesc;

typedef enum {
    TONE_MAP_OPERATOR_NONE,
    TONE_MAP_OPERATOR_REINHARD,
    TONE_MAP_OPERATOR_ACES,
    TONE_MAP_OPERATOR_UNCHARTED2
} ToneMapOperator;
```

## Performance and Debugging

### Performance Monitoring

```c
// Get rendering statistics
const RenderStats* render_get_stats(void);

// Reset statistics
void render_reset_stats(void);

// Enable/disable performance profiling
void render_set_profiling_enabled(bool enabled);

typedef struct {
    f64 frame_time_ms;
    f64 cpu_time_ms;
    f64 gpu_time_ms;
    u32 draw_calls;
    u32 triangles;
    u32 vertices;
    u32 compute_dispatches;
    u32 ray_tracing_dispatches;
    u64 memory_used;
    u64 memory_allocated;
    f64 culling_time_ms;
    f64 shadow_time_ms;
    f64 post_process_time_ms;
} RenderStats;
```

### Debug Rendering

```c
// Enable debug rendering
void render_debug_enable(bool enabled);

// Add debug line
void render_debug_add_line(Vec3 start, Vec3 end, Vec3 color, f32 lifetime);

// Add debug box
void render_debug_add_box(Vec3 min_bounds, Vec3 max_bounds, Vec3 color, f32 lifetime);

// Add debug sphere
void render_debug_add_sphere(Vec3 center, f32 radius, Vec3 color, f32 lifetime);

// Clear debug primitives
void render_debug_clear(void);

// Render debug primitives
void render_debug_render(const Camera* camera);
```

## Integration Examples

### Basic Forward Rendering

```c
// Initialize pipeline
RenderPipelineConfig config = {
    .width = 1920,
    .height = 1080,
    .msaa_samples = 4,
    .enable_ray_tracing = false,
    .enable_variable_rate_shading = false,
    .enable_mesh_shaders = false,
    .enable_gpu_culling = true,
    .render_scale = 1.0f,
    .quality = RENDER_QUALITY_HIGH
};

render_pipeline_init(&config);

// Create render graph
RenderGraph* graph = render_graph_create("forward_rendering");

// Add geometry pass
RenderPass* geometry_pass = render_graph_add_pass(graph, "geometry", RENDER_PASS_TYPE_GRAPHICS);
geometry_pass->flags = RENDER_PASS_FLAG_CLEAR_COLOR | RENDER_PASS_FLAG_CLEAR_DEPTH;
geometry_pass->shader = shader_create_from_file("forward.vert", "forward.frag");

// Add post-processing pass
RenderPass* post_process_pass = render_graph_add_pass(graph, "post_process", RENDER_PASS_TYPE_GRAPHICS);
post_process_pass->shader = shader_create_from_file("post_process.vert", "post_process.frag");

// Build and execute graph
render_graph_build(graph);

// Main loop
while (running) {
    render_pipeline_begin_frame();
    
    RenderContext context = get_render_context();
    render_graph_execute(graph, &context);
    
    render_pipeline_end_frame();
}
```

### Deferred Rendering with Ray Tracing

```c
// Initialize pipeline with ray tracing
RenderPipelineConfig config = {
    .width = 1920,
    .height = 1080,
    .msaa_samples = 1,
    .enable_ray_tracing = true,
    .enable_variable_rate_shading = true,
    .enable_mesh_shaders = true,
    .enable_gpu_culling = true,
    .render_scale = 1.0f,
    .quality = RENDER_QUALITY_ULTRA
};

render_pipeline_init(&config);

// Create G-buffer textures
RenderTextureDesc gbuffer_desc = {
    .width = 1920,
    .height = 1080,
    .format = TEXTURE_FORMAT_RGBA16F,
    .type = TEXTURE_TYPE_2D,
    .usage = TEXTURE_USAGE_COLOR_ATTACHMENT | TEXTURE_USAGE_SAMPLED
};

RenderTexture* albedo_texture = render_texture_create(&gbuffer_desc);
RenderTexture* normal_texture = render_texture_create(&gbuffer_desc);
RenderTexture* material_texture = render_texture_create(&gbuffer_desc);

// Create ray tracing pipeline
RayTracingPipelineDesc rt_desc = {
    .ray_gen_shader = shader_create_from_file("ray_gen.rgen"),
    .miss_shaders[0] = shader_create_from_file("miss.rmiss"),
    .closest_hit_shaders[0] = shader_create_from_file("closest_hit.rchit"),
    .miss_shader_count = 1,
    .closest_hit_shader_count = 1
};

RayTracingPipeline* rt_pipeline = ray_tracing_pipeline_create(&rt_desc);

// Create render graph
RenderGraph* graph = render_graph_create("deferred_ray_traced");

// Add G-buffer pass
RenderPass* gbuffer_pass = render_graph_add_pass(graph, "gbuffer", RENDER_PASS_TYPE_GRAPHICS);
gbuffer_pass->outputs[0] = albedo_texture;
gbuffer_pass->outputs[1] = normal_texture;
gbuffer_pass->outputs[2] = material_texture;
gbuffer_pass->output_count = 3;

// Add ray tracing pass
RenderPass* rt_pass = render_graph_add_pass(graph, "ray_tracing", RENDER_PASS_TYPE_RAY_TRACING);
rt_pass->outputs[0] = get_output_texture();
rt_pass->output_count = 1;
rt_pass->execute = ray_tracing_pass_execute;

render_graph_build(graph);
```

## Best Practices

1. **Resource Management**: Use descriptor sets and binding layouts for efficient resource access
2. **Command Buffer Recording**: Batch similar operations and minimize state changes
3. **Memory Management**: Use memory pools and avoid frequent allocations
4. **Synchronization**: Use proper pipeline barriers and synchronization primitives
5. **Culling**: Implement multiple culling layers for maximum performance
6. **LOD**: Use level-of-detail systems for distant objects
7. **Batching**: Group similar objects for instanced rendering
8. **Profiling**: Monitor performance metrics and optimize bottlenecks

## Error Handling

```c
typedef enum {
    RENDER_ERROR_NONE = 0,
    RENDER_ERROR_OUT_OF_MEMORY,
    RENDER_ERROR_INVALID_PARAMETER,
    RENDER_ERROR_DEVICE_LOST,
    RENDER_ERROR_INITIALIZATION_FAILED,
    RENDER_ERROR_SHADER_COMPILATION_FAILED,
    RENDER_ERROR_PIPELINE_CREATION_FAILED,
    RENDER_ERROR_RESOURCE_CREATION_FAILED
} RenderError;

// Get last error
RenderError render_get_last_error(void);

// Get error string
const char* render_get_error_string(RenderError error);

// Set error callback
void render_set_error_callback(RenderErrorCallback callback);
```

This documentation provides comprehensive coverage of the rendering pipeline API, including advanced features like ray tracing, variable rate shading, mesh shaders, and sophisticated culling techniques.
