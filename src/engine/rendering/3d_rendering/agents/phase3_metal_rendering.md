# Phase 3: Metal Rendering Pipeline

## Overview
This phase implements the actual rendering pipeline using Metal - forward rendering, deferred rendering, and the render graph system.

---

## Agent 3.1: Render Graph System

```
TASK: Implement Metal Render Graph (Phase 3, Agent 1)

You are implementing a render graph system for automatic resource management and pass scheduling.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/rendering/render_graph/

FILES TO CREATE/MODIFY:
- render_pass_node.c/h
- resource_node.c/h
- graph_compiler.c/h
- graph_executor.c/h

WHAT TO IMPLEMENT:
1. Declare render passes with inputs/outputs
2. Automatic texture/buffer creation for transients
3. Barrier placement between passes
4. Resource aliasing for memory efficiency
5. Async compute pass support

METAL RENDER GRAPH:
```c
typedef enum rg_resource_type {
    RG_RESOURCE_TEXTURE,
    RG_RESOURCE_BUFFER,
} rg_resource_type_t;

typedef struct rg_resource {
    char name[64];
    rg_resource_type_t type;
    union {
        struct {
            uint32_t width, height;
            MTLPixelFormat format;
            MTLTextureUsage usage;
        } texture;
        struct {
            size_t size;
        } buffer;
    };
    id<MTLTexture> mtl_texture;
    id<MTLBuffer> mtl_buffer;
    bool is_imported;  // External resource vs transient
} rg_resource_t;

typedef struct rg_pass {
    char name[64];
    void (*execute)(struct rg_pass* pass, id<MTLCommandBuffer> cmd);
    rg_resource_t* color_outputs[8];
    rg_resource_t* depth_output;
    rg_resource_t* texture_inputs[16];
    rg_resource_t* buffer_inputs[8];
    uint32_t color_output_count;
    uint32_t texture_input_count;
    void* user_data;
    bool is_compute;
} rg_pass_t;

typedef struct render_graph {
    rg_resource_t* resources;
    uint32_t resource_count;
    rg_pass_t* passes;
    uint32_t pass_count;
    uint32_t* execution_order;
    metal_device_t* device;
} render_graph_t;

// API
rg_resource_t* rg_create_texture(render_graph_t* graph, const char* name,
                                  uint32_t w, uint32_t h, MTLPixelFormat format);
rg_pass_t* rg_add_pass(render_graph_t* graph, const char* name,
                       void (*execute)(rg_pass_t*, id<MTLCommandBuffer>));
void rg_pass_add_color_output(rg_pass_t* pass, rg_resource_t* tex);
void rg_pass_add_texture_input(rg_pass_t* pass, rg_resource_t* tex);

void rg_compile(render_graph_t* graph);
void rg_execute(render_graph_t* graph, id<MTLCommandBuffer> cmd);

// Example usage:
void setup_render_graph(render_graph_t* graph) {
    // Create transient resources
    rg_resource_t* gbuffer_albedo = rg_create_texture(graph, "GBuffer_Albedo", 1920, 1080, MTLPixelFormatRGBA8Unorm);
    rg_resource_t* gbuffer_normal = rg_create_texture(graph, "GBuffer_Normal", 1920, 1080, MTLPixelFormatRG16Float);
    rg_resource_t* depth = rg_create_texture(graph, "Depth", 1920, 1080, MTLPixelFormatDepth32Float);
    rg_resource_t* hdr_color = rg_create_texture(graph, "HDR_Color", 1920, 1080, MTLPixelFormatRGBA16Float);

    // G-buffer pass
    rg_pass_t* gbuffer_pass = rg_add_pass(graph, "GBuffer", execute_gbuffer);
    rg_pass_add_color_output(gbuffer_pass, gbuffer_albedo);
    rg_pass_add_color_output(gbuffer_pass, gbuffer_normal);
    gbuffer_pass->depth_output = depth;

    // Lighting pass
    rg_pass_t* lighting_pass = rg_add_pass(graph, "Lighting", execute_lighting);
    rg_pass_add_texture_input(lighting_pass, gbuffer_albedo);
    rg_pass_add_texture_input(lighting_pass, gbuffer_normal);
    rg_pass_add_texture_input(lighting_pass, depth);
    rg_pass_add_color_output(lighting_pass, hdr_color);
}
```

OUTPUT: Render graph that manages the entire frame automatically.
```

---

## Agent 3.2: G-Buffer System

```
TASK: Implement Deferred Rendering G-Buffer (Phase 3, Agent 2)

You are implementing the G-buffer system for deferred rendering.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/rendering/deferred/

FILES TO COMPLETE:
- gbuffer_pass.c/h - Geometry pass
- gbuffer_layout.c/h - G-buffer format

WHAT TO IMPLEMENT:
1. G-buffer texture layout (albedo, normal, material, depth)
2. G-buffer render pass descriptor
3. Geometry rendering to G-buffer
4. G-buffer sampling utilities

METAL G-BUFFER:
```c
typedef struct gbuffer {
    id<MTLTexture> albedo;      // RGBA8: albedo.rgb, ambient_occlusion.a
    id<MTLTexture> normal;      // RG16F: encoded normal
    id<MTLTexture> material;    // RGBA8: roughness, metallic, material_id, flags
    id<MTLTexture> depth;       // Depth32Float
    id<MTLTexture> velocity;    // RG16F: motion vectors
    uint32_t width, height;
} gbuffer_t;

gbuffer_t* gbuffer_create(metal_device_t* dev, uint32_t width, uint32_t height) {
    gbuffer_t* gb = calloc(1, sizeof(gbuffer_t));
    gb->width = width;
    gb->height = height;

    MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
    desc.textureType = MTLTextureType2D;
    desc.width = width;
    desc.height = height;
    desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    desc.storageMode = MTLStorageModePrivate;

    desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
    gb->albedo = [dev->device newTextureWithDescriptor:desc];

    desc.pixelFormat = MTLPixelFormatRG16Float;
    gb->normal = [dev->device newTextureWithDescriptor:desc];

    desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
    gb->material = [dev->device newTextureWithDescriptor:desc];

    desc.pixelFormat = MTLPixelFormatRG16Float;
    gb->velocity = [dev->device newTextureWithDescriptor:desc];

    desc.pixelFormat = MTLPixelFormatDepth32Float;
    desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    gb->depth = [dev->device newTextureWithDescriptor:desc];

    return gb;
}

MTLRenderPassDescriptor* gbuffer_render_pass_descriptor(gbuffer_t* gb) {
    MTLRenderPassDescriptor* desc = [MTLRenderPassDescriptor renderPassDescriptor];

    desc.colorAttachments[0].texture = gb->albedo;
    desc.colorAttachments[0].loadAction = MTLLoadActionClear;
    desc.colorAttachments[0].storeAction = MTLStoreActionStore;

    desc.colorAttachments[1].texture = gb->normal;
    desc.colorAttachments[1].loadAction = MTLLoadActionClear;
    desc.colorAttachments[1].storeAction = MTLStoreActionStore;

    desc.colorAttachments[2].texture = gb->material;
    desc.colorAttachments[2].loadAction = MTLLoadActionClear;
    desc.colorAttachments[2].storeAction = MTLStoreActionStore;

    desc.colorAttachments[3].texture = gb->velocity;
    desc.colorAttachments[3].loadAction = MTLLoadActionClear;
    desc.colorAttachments[3].storeAction = MTLStoreActionStore;

    desc.depthAttachment.texture = gb->depth;
    desc.depthAttachment.loadAction = MTLLoadActionClear;
    desc.depthAttachment.storeAction = MTLStoreActionStore;
    desc.depthAttachment.clearDepth = 1.0;

    return desc;
}
```

OUTPUT: G-buffer system ready for deferred rendering.
```

---

## Agent 3.3: Deferred Lighting Pass

```
TASK: Implement Deferred Lighting (Phase 3, Agent 3)

You are implementing the deferred lighting pass that reads the G-buffer and computes lighting.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/rendering/deferred/

FILES TO CREATE:
- deferred_lighting.c/h

WHAT TO IMPLEMENT:
1. Full-screen lighting pass
2. Point/spot/directional light handling
3. Clustered light culling integration
4. Shadow sampling
5. PBR lighting computation

METAL SHADERS (create as .metal files):
```metal
// deferred_lighting.metal
#include <metal_stdlib>
using namespace metal;

struct LightData {
    float3 position;
    float radius;
    float3 color;
    float intensity;
};

struct LightingUniforms {
    float4x4 inv_view_proj;
    float3 camera_pos;
    uint light_count;
};

fragment float4 deferred_lighting_fragment(
    float4 position [[position]],
    texture2d<float> gbuffer_albedo [[texture(0)]],
    texture2d<float> gbuffer_normal [[texture(1)]],
    texture2d<float> gbuffer_material [[texture(2)]],
    depth2d<float> gbuffer_depth [[texture(3)]],
    constant LightingUniforms& uniforms [[buffer(0)]],
    constant LightData* lights [[buffer(1)]]
) {
    uint2 coord = uint2(position.xy);

    // Sample G-buffer
    float4 albedo_ao = gbuffer_albedo.read(coord);
    float2 encoded_normal = gbuffer_normal.read(coord).xy;
    float4 material = gbuffer_material.read(coord);
    float depth = gbuffer_depth.read(coord);

    // Reconstruct world position
    float2 uv = float2(coord) / float2(gbuffer_albedo.get_width(), gbuffer_albedo.get_height());
    float4 clip_pos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip_pos.y = -clip_pos.y;
    float4 world_pos = uniforms.inv_view_proj * clip_pos;
    world_pos /= world_pos.w;

    // Decode normal (octahedron encoding)
    float3 N = decode_octahedron(encoded_normal);
    float3 V = normalize(uniforms.camera_pos - world_pos.xyz);

    float3 albedo = albedo_ao.rgb;
    float ao = albedo_ao.a;
    float roughness = material.r;
    float metallic = material.g;

    // Accumulate lighting
    float3 Lo = float3(0);

    for (uint i = 0; i < uniforms.light_count; i++) {
        LightData light = lights[i];

        float3 L = light.position - world_pos.xyz;
        float distance = length(L);
        L /= distance;

        if (distance < light.radius) {
            float attenuation = 1.0 / (distance * distance + 1.0);
            attenuation *= saturate(1.0 - distance / light.radius);

            float3 radiance = light.color * light.intensity * attenuation;
            Lo += brdf_ggx(N, V, L, albedo, roughness, metallic) * radiance;
        }
    }

    // Ambient
    Lo += albedo * 0.03 * ao;

    return float4(Lo, 1.0);
}
```

C CODE:
```c
typedef struct deferred_lighting {
    id<MTLRenderPipelineState> pipeline;
    id<MTLBuffer> light_buffer;
    id<MTLBuffer> uniform_buffer;
    uint32_t max_lights;
} deferred_lighting_t;

void deferred_lighting_execute(deferred_lighting_t* dl, gbuffer_t* gb,
                                id<MTLRenderCommandEncoder> encoder,
                                light_t* lights, uint32_t light_count,
                                camera_t* camera) {
    // Update uniforms
    LightingUniforms uniforms = {
        .inv_view_proj = simd_inverse(camera->view_proj),
        .camera_pos = camera->position,
        .light_count = light_count
    };
    memcpy([dl->uniform_buffer contents], &uniforms, sizeof(uniforms));

    // Update lights
    memcpy([dl->light_buffer contents], lights, light_count * sizeof(light_t));

    // Bind pipeline
    [encoder setRenderPipelineState:dl->pipeline];

    // Bind G-buffer textures
    [encoder setFragmentTexture:gb->albedo atIndex:0];
    [encoder setFragmentTexture:gb->normal atIndex:1];
    [encoder setFragmentTexture:gb->material atIndex:2];
    [encoder setFragmentTexture:gb->depth atIndex:3];

    // Bind uniforms
    [encoder setFragmentBuffer:dl->uniform_buffer offset:0 atIndex:0];
    [encoder setFragmentBuffer:dl->light_buffer offset:0 atIndex:1];

    // Draw fullscreen triangle
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}
```

OUTPUT: Deferred lighting pass with PBR and multiple lights.
```

---

## Agent 3.4: Forward Rendering Path

```
TASK: Implement Forward Rendering Path (Phase 3, Agent 4)

You are implementing forward rendering for transparent objects and simpler scenes.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/rendering/forward/

FILES TO COMPLETE:
- forward_pass.c/h
- forward_lighting.c/h
- forward_transparency.c/h

WHAT TO IMPLEMENT:
1. Forward opaque pass
2. Forward+ clustered lighting
3. Transparent object rendering (sorted)
4. Alpha blending modes

METAL FORWARD RENDERING:
```c
typedef struct forward_renderer {
    id<MTLRenderPipelineState> opaque_pipeline;
    id<MTLRenderPipelineState> transparent_pipeline;
    id<MTLDepthStencilState> depth_state_opaque;
    id<MTLDepthStencilState> depth_state_transparent;
    id<MTLBuffer> light_grid_buffer;  // Clustered light indices
    id<MTLBuffer> light_data_buffer;
} forward_renderer_t;

void forward_render_opaque(forward_renderer_t* fr, id<MTLRenderCommandEncoder> encoder,
                           drawable_list_t* drawables, camera_t* camera) {
    [encoder setRenderPipelineState:fr->opaque_pipeline];
    [encoder setDepthStencilState:fr->depth_state_opaque];
    [encoder setCullMode:MTLCullModeBack];

    for (uint32_t i = 0; i < drawables->count; i++) {
        drawable_t* d = &drawables->items[i];

        // Set per-object uniforms
        [encoder setVertexBytes:&d->transform length:sizeof(simd_float4x4) atIndex:2];

        // Bind mesh
        [encoder setVertexBuffer:d->mesh->vertex_buffer offset:0 atIndex:0];

        // Bind material textures
        [encoder setFragmentTexture:d->material->albedo_texture atIndex:0];
        [encoder setFragmentTexture:d->material->normal_texture atIndex:1];

        // Draw
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:d->mesh->index_count
                             indexType:d->mesh->index_type
                           indexBuffer:d->mesh->index_buffer
                     indexBufferOffset:0];
    }
}

void forward_render_transparent(forward_renderer_t* fr, id<MTLRenderCommandEncoder> encoder,
                                drawable_list_t* drawables, camera_t* camera) {
    // Sort back-to-front
    sort_drawables_by_depth(drawables, camera->position, false);

    [encoder setRenderPipelineState:fr->transparent_pipeline];
    [encoder setDepthStencilState:fr->depth_state_transparent];  // Write off, test on
    [encoder setCullMode:MTLCullModeNone];  // Two-sided

    for (uint32_t i = 0; i < drawables->count; i++) {
        drawable_t* d = &drawables->items[i];
        // ... same as opaque
    }
}
```

OUTPUT: Forward rendering path for transparent and simple rendering.
```

---

## Agent 3.5: Shadow Rendering

```
TASK: Implement Shadow Mapping (Phase 3, Agent 5)

You are implementing shadow mapping with cascaded shadows for directional lights.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/lighting/shadows/

FILES TO COMPLETE:
- shadow_atlas.c/h
- shadow_caster.c/h
- cascade_splits.c/h
- pcf_filter.c/h

METAL SHADOW MAPPING:
```c
typedef struct shadow_cascade {
    id<MTLTexture> depth_texture;
    simd_float4x4 view_proj;
    float split_near;
    float split_far;
} shadow_cascade_t;

typedef struct shadow_map_system {
    shadow_cascade_t cascades[4];
    id<MTLRenderPipelineState> shadow_pipeline;
    id<MTLDepthStencilState> shadow_depth_state;
    uint32_t cascade_count;
    uint32_t resolution;  // e.g., 2048
} shadow_map_system_t;

shadow_map_system_t* shadow_system_create(metal_device_t* dev, uint32_t resolution, uint32_t cascades) {
    shadow_map_system_t* sys = calloc(1, sizeof(shadow_map_system_t));
    sys->resolution = resolution;
    sys->cascade_count = cascades;

    MTLTextureDescriptor* desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                     width:resolution
                                    height:resolution
                                 mipmapped:NO];
    desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    desc.storageMode = MTLStorageModePrivate;

    for (uint32_t i = 0; i < cascades; i++) {
        sys->cascades[i].depth_texture = [dev->device newTextureWithDescriptor:desc];
    }

    return sys;
}

void shadow_system_render_cascade(shadow_map_system_t* sys, uint32_t cascade_index,
                                  id<MTLCommandBuffer> cmd, drawable_list_t* shadow_casters) {
    shadow_cascade_t* cascade = &sys->cascades[cascade_index];

    MTLRenderPassDescriptor* desc = [MTLRenderPassDescriptor renderPassDescriptor];
    desc.depthAttachment.texture = cascade->depth_texture;
    desc.depthAttachment.loadAction = MTLLoadActionClear;
    desc.depthAttachment.storeAction = MTLStoreActionStore;
    desc.depthAttachment.clearDepth = 1.0;

    id<MTLRenderCommandEncoder> encoder = [cmd renderCommandEncoderWithDescriptor:desc];
    [encoder setRenderPipelineState:sys->shadow_pipeline];
    [encoder setDepthStencilState:sys->shadow_depth_state];
    [encoder setCullMode:MTLCullModeFront];  // Front-face culling for shadows

    // Set cascade view-proj
    [encoder setVertexBytes:&cascade->view_proj length:sizeof(simd_float4x4) atIndex:1];

    for (uint32_t i = 0; i < shadow_casters->count; i++) {
        drawable_t* d = &shadow_casters->items[i];
        [encoder setVertexBuffer:d->mesh->vertex_buffer offset:0 atIndex:0];
        [encoder setVertexBytes:&d->transform length:sizeof(simd_float4x4) atIndex:2];
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:d->mesh->index_count
                             indexType:d->mesh->index_type
                           indexBuffer:d->mesh->index_buffer
                     indexBufferOffset:0];
    }

    [encoder endEncoding];
}

// Calculate cascade matrices
void shadow_system_update_cascades(shadow_map_system_t* sys, camera_t* camera,
                                   simd_float3 light_dir, float shadow_distance) {
    float lambda = 0.5f;  // Practical split scheme blend

    for (uint32_t i = 0; i < sys->cascade_count; i++) {
        float p = (float)(i + 1) / sys->cascade_count;
        float log_split = camera->near * powf(shadow_distance / camera->near, p);
        float uniform_split = camera->near + (shadow_distance - camera->near) * p;

        sys->cascades[i].split_far = lambda * log_split + (1 - lambda) * uniform_split;
        sys->cascades[i].split_near = (i == 0) ? camera->near : sys->cascades[i-1].split_far;

        // Calculate view-proj for this cascade...
        sys->cascades[i].view_proj = calculate_cascade_matrix(camera, light_dir,
                                                               sys->cascades[i].split_near,
                                                               sys->cascades[i].split_far);
    }
}
```

OUTPUT: Cascaded shadow mapping system.
```

---

## Agent 3.6: Metal Shader Compilation

```
TASK: Implement Metal Shader Pipeline (Phase 3, Agent 6)

You are implementing the shader compilation and management system for Metal.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/materials/shaders/

FILES TO COMPLETE:
- shader_compiler.c/h (rewrite for Metal)
- shader_cache.c/h
- shader_variants.c/h

WHAT TO IMPLEMENT:
1. Load .metallib shader libraries
2. Create MTLFunction objects
3. Shader permutation system via function constants
4. Shader hot-reload for development
5. Shader reflection for binding info

METAL SHADER SYSTEM:
```c
typedef struct metal_shader_library {
    id<MTLLibrary> library;
    char path[256];
    time_t last_modified;
} metal_shader_library_t;

typedef struct metal_shader {
    id<MTLFunction> function;
    char name[64];
    MTLFunctionType type;  // Vertex, Fragment, Kernel
} metal_shader_t;

typedef struct shader_system {
    metal_shader_library_t* libraries;
    uint32_t library_count;
    metal_device_t* device;
} shader_system_t;

// Load shader library
metal_shader_library_t* shader_load_library(shader_system_t* sys, const char* path) {
    metal_shader_library_t* lib = calloc(1, sizeof(metal_shader_library_t));
    strncpy(lib->path, path, 255);

    NSError* error = nil;
    NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    lib->library = [sys->device->device newLibraryWithURL:url error:&error];

    if (error) {
        NSLog(@"Failed to load shader library: %@", error);
        free(lib);
        return NULL;
    }

    return lib;
}

// Get shader function with constants (permutations)
metal_shader_t* shader_get_function(metal_shader_library_t* lib, const char* name,
                                     MTLFunctionConstantValues* constants) {
    metal_shader_t* shader = calloc(1, sizeof(metal_shader_t));
    strncpy(shader->name, name, 63);

    NSError* error = nil;
    if (constants) {
        shader->function = [lib->library newFunctionWithName:[NSString stringWithUTF8String:name]
                                              constantValues:constants
                                                       error:&error];
    } else {
        shader->function = [lib->library newFunctionWithName:[NSString stringWithUTF8String:name]];
    }

    if (!shader->function) {
        NSLog(@"Failed to get function %s: %@", name, error);
        free(shader);
        return NULL;
    }

    shader->type = [shader->function functionType];
    return shader;
}

// Shader permutations via function constants
id<MTLFunction> shader_get_permutation(metal_shader_library_t* lib, const char* name,
                                        bool has_normal_map, bool has_emissive) {
    MTLFunctionConstantValues* constants = [[MTLFunctionConstantValues alloc] init];

    [constants setConstantValue:&has_normal_map type:MTLDataTypeBool atIndex:0];
    [constants setConstantValue:&has_emissive type:MTLDataTypeBool atIndex:1];

    NSError* error = nil;
    return [lib->library newFunctionWithName:[NSString stringWithUTF8String:name]
                              constantValues:constants
                                       error:&error];
}

// Hot reload (development only)
void shader_system_check_reload(shader_system_t* sys) {
    for (uint32_t i = 0; i < sys->library_count; i++) {
        metal_shader_library_t* lib = &sys->libraries[i];

        struct stat st;
        if (stat(lib->path, &st) == 0 && st.st_mtime > lib->last_modified) {
            // Reload library
            NSError* error = nil;
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:lib->path]];
            id<MTLLibrary> new_lib = [sys->device->device newLibraryWithURL:url error:&error];

            if (!error) {
                lib->library = new_lib;
                lib->last_modified = st.st_mtime;
                NSLog(@"Reloaded shader library: %s", lib->path);
            }
        }
    }
}
```

METAL SHADERS (.metal files to create):
```metal
// shaders/pbr.metal
#include <metal_stdlib>
using namespace metal;

// Function constants for permutations
constant bool HAS_NORMAL_MAP [[function_constant(0)]];
constant bool HAS_EMISSIVE [[function_constant(1)]];

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texcoord [[attribute(2)]];
    float4 tangent [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 world_pos;
    float3 normal;
    float2 texcoord;
    float3 tangent;
    float3 bitangent;
};

vertex VertexOut pbr_vertex(VertexIn in [[stage_in]],
                            constant float4x4& model [[buffer(1)]],
                            constant float4x4& view_proj [[buffer(2)]]) {
    VertexOut out;
    float4 world = model * float4(in.position, 1.0);
    out.position = view_proj * world;
    out.world_pos = world.xyz;
    out.normal = normalize((model * float4(in.normal, 0.0)).xyz);
    out.texcoord = in.texcoord;

    if (HAS_NORMAL_MAP) {
        out.tangent = normalize((model * float4(in.tangent.xyz, 0.0)).xyz);
        out.bitangent = cross(out.normal, out.tangent) * in.tangent.w;
    }

    return out;
}

fragment float4 pbr_fragment(VertexOut in [[stage_in]],
                              texture2d<float> albedo_tex [[texture(0)]],
                              texture2d<float> normal_tex [[texture(1), function_constant(HAS_NORMAL_MAP)]],
                              texture2d<float> metallic_roughness_tex [[texture(2)]],
                              texture2d<float> emissive_tex [[texture(3), function_constant(HAS_EMISSIVE)]],
                              sampler samp [[sampler(0)]]) {
    float4 albedo = albedo_tex.sample(samp, in.texcoord);

    float3 N = normalize(in.normal);
    if (HAS_NORMAL_MAP) {
        float3 T = normalize(in.tangent);
        float3 B = normalize(in.bitangent);
        float3x3 TBN = float3x3(T, B, N);
        float3 normal_sample = normal_tex.sample(samp, in.texcoord).xyz * 2.0 - 1.0;
        N = normalize(TBN * normal_sample);
    }

    float4 mr = metallic_roughness_tex.sample(samp, in.texcoord);
    float metallic = mr.b;
    float roughness = mr.g;

    // ... PBR lighting ...

    float3 color = albedo.rgb;  // Simplified

    if (HAS_EMISSIVE) {
        color += emissive_tex.sample(samp, in.texcoord).rgb;
    }

    return float4(color, albedo.a);
}
```

OUTPUT: Metal shader compilation and permutation system.
```
