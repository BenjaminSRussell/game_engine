# Phase 4: Materials & Advanced Lighting

## Overview
This phase implements the material system and advanced lighting features including PBR, GI, and volumetric effects.

---

## Agent 4.1: PBR Material System

```
TASK: Implement PBR Material System (Phase 4, Agent 1)

You are implementing the physically-based material system.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/materials/

EXISTING FILES (have partial implementations):
- pbr/metallic_roughness.c
- pbr/pbr_parameters.c

WHAT TO IMPLEMENT:
1. Material instance system with parameter inheritance
2. Texture binding for PBR textures
3. Material parameter buffer management
4. Material sorting for batching
5. Dynamic material creation

METAL MATERIAL SYSTEM:
```c
typedef struct pbr_material_params {
    simd_float4 albedo_factor;
    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float occlusion_strength;
    simd_float3 emissive_factor;
    float alpha_cutoff;
} pbr_material_params_t;

typedef struct material {
    char name[64];
    pbr_material_params_t params;

    // Textures (can be NULL for defaults)
    id<MTLTexture> albedo_texture;
    id<MTLTexture> normal_texture;
    id<MTLTexture> metallic_roughness_texture;
    id<MTLTexture> occlusion_texture;
    id<MTLTexture> emissive_texture;

    // GPU buffer for params
    id<MTLBuffer> param_buffer;

    // Flags
    uint32_t flags;
    bool double_sided;
    bool alpha_blend;
} material_t;

// Default textures
typedef struct material_defaults {
    id<MTLTexture> white_1x1;
    id<MTLTexture> normal_1x1;  // (0.5, 0.5, 1.0)
    id<MTLTexture> black_1x1;
    id<MTLSamplerState> default_sampler;
} material_defaults_t;

material_t* material_create(metal_device_t* dev, const char* name) {
    material_t* mat = calloc(1, sizeof(material_t));
    strncpy(mat->name, name, 63);

    // Default PBR values
    mat->params.albedo_factor = simd_make_float4(1, 1, 1, 1);
    mat->params.metallic_factor = 0.0f;
    mat->params.roughness_factor = 0.5f;
    mat->params.normal_scale = 1.0f;
    mat->params.occlusion_strength = 1.0f;
    mat->params.emissive_factor = simd_make_float3(0, 0, 0);
    mat->params.alpha_cutoff = 0.5f;

    mat->param_buffer = [dev->device newBufferWithLength:sizeof(pbr_material_params_t)
                                                 options:MTLResourceStorageModeShared];

    return mat;
}

void material_update_gpu(material_t* mat) {
    memcpy([mat->param_buffer contents], &mat->params, sizeof(pbr_material_params_t));
}

void material_bind(material_t* mat, id<MTLRenderCommandEncoder> encoder, material_defaults_t* defaults) {
    // Bind textures (use defaults if NULL)
    [encoder setFragmentTexture:(mat->albedo_texture ?: defaults->white_1x1) atIndex:0];
    [encoder setFragmentTexture:(mat->normal_texture ?: defaults->normal_1x1) atIndex:1];
    [encoder setFragmentTexture:(mat->metallic_roughness_texture ?: defaults->white_1x1) atIndex:2];
    [encoder setFragmentTexture:(mat->occlusion_texture ?: defaults->white_1x1) atIndex:3];
    [encoder setFragmentTexture:(mat->emissive_texture ?: defaults->black_1x1) atIndex:4];

    // Bind parameters
    [encoder setFragmentBuffer:mat->param_buffer offset:0 atIndex:0];

    // Bind sampler
    [encoder setFragmentSamplerState:defaults->default_sampler atIndex:0];
}
```

OUTPUT: Complete PBR material system.
```

---

## Agent 4.2: Light Management System

```
TASK: Implement Light Management System (Phase 4, Agent 2)

You are implementing the light source management system.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/lighting/sources/

FILES TO COMPLETE:
- point_light.c/h
- spot_light.c/h
- directional_light.c/h
- light_culling.c/h

METAL LIGHT SYSTEM:
```c
typedef enum light_type {
    LIGHT_TYPE_DIRECTIONAL = 0,
    LIGHT_TYPE_POINT = 1,
    LIGHT_TYPE_SPOT = 2,
} light_type_t;

typedef struct light {
    simd_float3 position;
    float radius;
    simd_float3 direction;
    float inner_cone;
    simd_float3 color;
    float outer_cone;
    float intensity;
    light_type_t type;
    bool cast_shadows;
    uint32_t shadow_map_index;
} light_t;

typedef struct light_system {
    light_t* lights;
    uint32_t light_count;
    uint32_t max_lights;

    id<MTLBuffer> light_buffer;
    id<MTLBuffer> light_grid;  // For clustered culling
    id<MTLBuffer> light_indices;

    // Cluster grid dimensions
    uint32_t cluster_x, cluster_y, cluster_z;
} light_system_t;

light_system_t* light_system_create(metal_device_t* dev, uint32_t max_lights) {
    light_system_t* sys = calloc(1, sizeof(light_system_t));
    sys->max_lights = max_lights;
    sys->lights = calloc(max_lights, sizeof(light_t));

    sys->light_buffer = [dev->device newBufferWithLength:max_lights * sizeof(light_t)
                                                 options:MTLResourceStorageModeShared];

    // Clustered light grid (16x9x24 typical)
    sys->cluster_x = 16;
    sys->cluster_y = 9;
    sys->cluster_z = 24;

    uint32_t cluster_count = sys->cluster_x * sys->cluster_y * sys->cluster_z;
    sys->light_grid = [dev->device newBufferWithLength:cluster_count * sizeof(uint32_t) * 2
                                               options:MTLResourceStorageModeShared];
    sys->light_indices = [dev->device newBufferWithLength:cluster_count * 256 * sizeof(uint32_t)
                                                  options:MTLResourceStorageModeShared];

    return sys;
}

uint32_t light_system_add(light_system_t* sys, light_t* light) {
    if (sys->light_count >= sys->max_lights) return UINT32_MAX;
    sys->lights[sys->light_count] = *light;
    return sys->light_count++;
}

void light_system_update_gpu(light_system_t* sys) {
    memcpy([sys->light_buffer contents], sys->lights, sys->light_count * sizeof(light_t));
}

// Clustered light culling (compute shader)
void light_system_cull_clustered(light_system_t* sys, id<MTLComputeCommandEncoder> encoder,
                                  camera_t* camera, id<MTLComputePipelineState> cull_pipeline) {
    // Compute shader assigns lights to clusters based on frustum intersection
    [encoder setComputePipelineState:cull_pipeline];
    [encoder setBuffer:sys->light_buffer offset:0 atIndex:0];
    [encoder setBuffer:sys->light_grid offset:0 atIndex:1];
    [encoder setBuffer:sys->light_indices offset:0 atIndex:2];

    // Camera uniforms
    struct {
        simd_float4x4 view;
        simd_float4x4 proj;
        float near, far;
        uint32_t light_count;
    } uniforms = {
        .view = camera->view,
        .proj = camera->proj,
        .near = camera->near,
        .far = camera->far,
        .light_count = sys->light_count
    };
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:3];

    MTLSize threads = MTLSizeMake(sys->cluster_x, sys->cluster_y, sys->cluster_z);
    MTLSize groups = MTLSizeMake(1, 1, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];
}
```

OUTPUT: Light management with clustered culling.
```

---

## Agent 4.3: Probe-Based GI

```
TASK: Implement Irradiance Probe System (Phase 4, Agent 3)

You are implementing the irradiance probe system for global illumination.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/lighting/probes/

FILES TO COMPLETE:
- irradiance_probe.c/h
- probe_blending.c/h
- probe_parallax.c/h

METAL PROBE SYSTEM:
```c
typedef struct irradiance_probe {
    simd_float3 position;
    simd_float4 sh_coefficients[9];  // L0, L1, L2 spherical harmonics
    float influence_radius;
} irradiance_probe_t;

typedef struct probe_grid {
    irradiance_probe_t* probes;
    uint32_t probe_count;

    simd_float3 bounds_min;
    simd_float3 bounds_max;
    simd_uint3 resolution;

    id<MTLBuffer> probe_buffer;
    id<MTLTexture> probe_texture;  // 3D texture for GPU sampling
} probe_grid_t;

probe_grid_t* probe_grid_create(metal_device_t* dev, simd_float3 min, simd_float3 max,
                                 simd_uint3 resolution) {
    probe_grid_t* grid = calloc(1, sizeof(probe_grid_t));
    grid->bounds_min = min;
    grid->bounds_max = max;
    grid->resolution = resolution;

    grid->probe_count = resolution.x * resolution.y * resolution.z;
    grid->probes = calloc(grid->probe_count, sizeof(irradiance_probe_t));

    // Initialize probe positions
    simd_float3 cell_size = (max - min) / simd_make_float3(resolution.x, resolution.y, resolution.z);
    for (uint32_t z = 0; z < resolution.z; z++) {
        for (uint32_t y = 0; y < resolution.y; y++) {
            for (uint32_t x = 0; x < resolution.x; x++) {
                uint32_t idx = x + y * resolution.x + z * resolution.x * resolution.y;
                grid->probes[idx].position = min + cell_size * simd_make_float3(x + 0.5f, y + 0.5f, z + 0.5f);
                grid->probes[idx].influence_radius = simd_length(cell_size) * 0.5f;
            }
        }
    }

    grid->probe_buffer = [dev->device newBufferWithLength:grid->probe_count * sizeof(irradiance_probe_t)
                                                  options:MTLResourceStorageModeShared];

    // 3D texture for SH coefficients (RGBA16Float, 9 slices per probe)
    MTLTextureDescriptor* desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                     width:resolution.x
                                    height:resolution.y
                                 mipmapped:NO];
    desc.textureType = MTLTextureType3D;
    desc.depth = resolution.z * 9;  // 9 SH coefficients per probe
    grid->probe_texture = [dev->device newTextureWithDescriptor:desc];

    return grid;
}

// Sample irradiance at a point (for shader use)
// In Metal shader:
```metal
float3 sample_irradiance(float3 world_pos, float3 normal,
                         constant ProbeGridUniforms& grid,
                         texture3d<float> probe_texture) {
    // Convert world pos to grid coordinates
    float3 grid_pos = (world_pos - grid.bounds_min) / (grid.bounds_max - grid.bounds_min);
    grid_pos = saturate(grid_pos) * float3(grid.resolution);

    // Trilinear interpolation of SH
    float3 base = floor(grid_pos);
    float3 frac = grid_pos - base;

    float3 irradiance = float3(0);

    // Sample 8 surrounding probes
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                int3 probe_coord = int3(base) + int3(x, y, z);
                probe_coord = clamp(probe_coord, int3(0), int3(grid.resolution) - 1);

                float weight = (x ? frac.x : 1 - frac.x) *
                              (y ? frac.y : 1 - frac.y) *
                              (z ? frac.z : 1 - frac.z);

                // Sample SH coefficients and evaluate
                float4 sh[9];
                for (int i = 0; i < 9; i++) {
                    int3 sample_coord = int3(probe_coord.x, probe_coord.y,
                                            probe_coord.z * 9 + i);
                    sh[i] = probe_texture.read(uint3(sample_coord));
                }

                irradiance += weight * evaluate_sh(sh, normal);
            }
        }
    }

    return irradiance;
}
```

OUTPUT: Probe-based irradiance system for GI.
```

---

## Agent 4.4: Volumetric Fog

```
TASK: Implement Volumetric Fog System (Phase 4, Agent 4)

You are implementing volumetric fog rendering.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/lighting/volumetric/

FILES TO COMPLETE:
- volumetric_fog.c/h
- froxel_grid.c/h
- temporal_reprojection.c/h

METAL VOLUMETRIC FOG:
```c
typedef struct volumetric_fog {
    id<MTLTexture> froxel_scattering;     // RGBA16F: inscatter.rgb, transmittance.a
    id<MTLTexture> froxel_density;        // R16F: density
    id<MTLTexture> integrated_scattering; // Result of integration

    id<MTLComputePipelineState> inject_pipeline;
    id<MTLComputePipelineState> raymarch_pipeline;
    id<MTLComputePipelineState> integrate_pipeline;

    simd_uint3 resolution;  // e.g., 160x90x128
    float near_plane;
    float far_plane;
    float density_scale;
    simd_float3 fog_color;
} volumetric_fog_t;

volumetric_fog_t* volumetric_fog_create(metal_device_t* dev, uint32_t w, uint32_t h, uint32_t d) {
    volumetric_fog_t* fog = calloc(1, sizeof(volumetric_fog_t));
    fog->resolution = simd_make_uint3(w, h, d);

    MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
    desc.textureType = MTLTextureType3D;
    desc.width = w;
    desc.height = h;
    desc.depth = d;
    desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    desc.storageMode = MTLStorageModePrivate;

    desc.pixelFormat = MTLPixelFormatRGBA16Float;
    fog->froxel_scattering = [dev->device newTextureWithDescriptor:desc];
    fog->integrated_scattering = [dev->device newTextureWithDescriptor:desc];

    desc.pixelFormat = MTLPixelFormatR16Float;
    fog->froxel_density = [dev->device newTextureWithDescriptor:desc];

    fog->near_plane = 0.1f;
    fog->far_plane = 500.0f;
    fog->density_scale = 0.01f;
    fog->fog_color = simd_make_float3(0.5f, 0.6f, 0.7f);

    return fog;
}

void volumetric_fog_render(volumetric_fog_t* fog, id<MTLCommandBuffer> cmd,
                           light_system_t* lights, camera_t* camera,
                           shadow_map_system_t* shadows) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

    // Pass 1: Inject density and lighting into froxels
    [encoder setComputePipelineState:fog->inject_pipeline];
    [encoder setTexture:fog->froxel_scattering atIndex:0];
    [encoder setTexture:fog->froxel_density atIndex:1];
    [encoder setBuffer:lights->light_buffer offset:0 atIndex:0];

    struct {
        simd_float4x4 inv_view_proj;
        simd_float3 camera_pos;
        float near, far;
        simd_uint3 resolution;
        float density_scale;
        simd_float3 fog_color;
        uint32_t light_count;
    } uniforms = {
        .inv_view_proj = simd_inverse(camera->view_proj),
        .camera_pos = camera->position,
        .near = fog->near_plane,
        .far = fog->far_plane,
        .resolution = fog->resolution,
        .density_scale = fog->density_scale,
        .fog_color = fog->fog_color,
        .light_count = lights->light_count
    };
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:1];

    // Bind shadow maps for light visibility
    for (uint32_t i = 0; i < shadows->cascade_count; i++) {
        [encoder setTexture:shadows->cascades[i].depth_texture atIndex:10 + i];
    }

    MTLSize groups = MTLSizeMake((fog->resolution.x + 7) / 8,
                                  (fog->resolution.y + 7) / 8,
                                  (fog->resolution.z + 3) / 4);
    MTLSize threads = MTLSizeMake(8, 8, 4);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];

    // Pass 2: Ray march integration
    [encoder setComputePipelineState:fog->integrate_pipeline];
    [encoder setTexture:fog->froxel_scattering atIndex:0];
    [encoder setTexture:fog->integrated_scattering atIndex:1];

    groups = MTLSizeMake((fog->resolution.x + 7) / 8, (fog->resolution.y + 7) / 8, 1);
    threads = MTLSizeMake(8, 8, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];

    [encoder endEncoding];
}

// Apply to final image (fragment shader)
```metal
fragment float4 apply_volumetric_fog(float4 position [[position]],
                                      texture2d<float> scene_color [[texture(0)]],
                                      texture2d<float> scene_depth [[texture(1)]],
                                      texture3d<float> fog_volume [[texture(2)]],
                                      constant FogUniforms& uniforms [[buffer(0)]]) {
    uint2 coord = uint2(position.xy);
    float4 color = scene_color.read(coord);
    float depth = scene_depth.read(coord).r;

    // Convert depth to froxel z
    float linear_depth = uniforms.near * uniforms.far / (uniforms.far - depth * (uniforms.far - uniforms.near));
    float froxel_z = log(linear_depth / uniforms.near) / log(uniforms.far / uniforms.near);

    float3 froxel_coord = float3(float2(coord) / float2(uniforms.screen_size), froxel_z);
    float4 fog = fog_volume.sample(linear_sampler, froxel_coord);

    // Apply fog
    return float4(color.rgb * fog.a + fog.rgb, color.a);
}
```

OUTPUT: Volumetric fog with light scattering.
```

---

## Agent 4.5: Screen-Space Reflections

```
TASK: Implement Screen-Space Reflections (Phase 4, Agent 5)

You are implementing SSR for real-time reflections.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/postprocess/ssr/

FILES TO CREATE:
- ssr_trace.c/h
- ssr_resolve.c/h
- ssr_fallback.c/h

METAL SSR:
```c
typedef struct ssr_system {
    id<MTLComputePipelineState> trace_pipeline;
    id<MTLComputePipelineState> resolve_pipeline;
    id<MTLTexture> ray_hit_texture;  // RG32F: hit UV
    id<MTLTexture> reflection_texture;

    uint32_t max_steps;
    float max_distance;
    float thickness;
} ssr_system_t;

// SSR tracing compute shader:
```metal
kernel void ssr_trace(
    texture2d<float, access::read> depth_texture [[texture(0)]],
    texture2d<float, access::read> normal_texture [[texture(1)]],
    texture2d<float, access::read> roughness_texture [[texture(2)]],
    texture2d<float, access::write> ray_hit [[texture(3)]],
    constant SSRUniforms& uniforms [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float2 uv = float2(gid) / float2(uniforms.screen_size);
    float depth = depth_texture.read(gid).r;

    if (depth >= 1.0) {
        ray_hit.write(float4(-1, -1, 0, 0), gid);
        return;
    }

    // Reconstruct world position
    float3 world_pos = reconstruct_world_pos(uv, depth, uniforms.inv_view_proj);
    float3 normal = decode_normal(normal_texture.read(gid).xy);
    float roughness = roughness_texture.read(gid).g;

    // Skip very rough surfaces
    if (roughness > 0.7) {
        ray_hit.write(float4(-1, -1, 0, 0), gid);
        return;
    }

    // Calculate reflection direction
    float3 view_dir = normalize(uniforms.camera_pos - world_pos);
    float3 reflect_dir = reflect(-view_dir, normal);

    // Hierarchical ray march
    float3 ray_pos = world_pos + reflect_dir * 0.01;  // Offset to avoid self-intersection
    float step_size = uniforms.initial_step;
    int mip = 4;  // Start at high mip

    for (int i = 0; i < uniforms.max_steps; i++) {
        ray_pos += reflect_dir * step_size;
        float2 ray_uv = world_to_screen(ray_pos, uniforms.view_proj);

        if (ray_uv.x < 0 || ray_uv.x > 1 || ray_uv.y < 0 || ray_uv.y > 1) {
            break;  // Ray left screen
        }

        // Sample depth at ray position
        float scene_depth = depth_texture.read(uint2(ray_uv * float2(uniforms.screen_size)), mip).r;
        float3 scene_pos = reconstruct_world_pos(ray_uv, scene_depth, uniforms.inv_view_proj);

        float dist_to_surface = distance(ray_pos, scene_pos);

        if (ray_pos.z > scene_pos.z && dist_to_surface < uniforms.thickness) {
            // Hit! Refine with binary search at mip 0
            if (mip == 0) {
                ray_hit.write(float4(ray_uv, 1.0, 0), gid);
                return;
            }
            // Step back and decrease mip
            ray_pos -= reflect_dir * step_size;
            mip--;
            step_size *= 0.5;
        }
    }

    ray_hit.write(float4(-1, -1, 0, 0), gid);  // No hit
}
```

C code:
```c
void ssr_render(ssr_system_t* ssr, id<MTLCommandBuffer> cmd,
                gbuffer_t* gbuffer, camera_t* camera,
                id<MTLTexture> scene_color, id<MTLTexture> reflection_output) {

    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

    // Pass 1: Trace
    [encoder setComputePipelineState:ssr->trace_pipeline];
    [encoder setTexture:gbuffer->depth atIndex:0];
    [encoder setTexture:gbuffer->normal atIndex:1];
    [encoder setTexture:gbuffer->material atIndex:2];
    [encoder setTexture:ssr->ray_hit_texture atIndex:3];

    struct {
        simd_float4x4 view_proj;
        simd_float4x4 inv_view_proj;
        simd_float3 camera_pos;
        simd_uint2 screen_size;
        uint32_t max_steps;
        float initial_step;
        float thickness;
    } uniforms = {
        .view_proj = camera->view_proj,
        .inv_view_proj = simd_inverse(camera->view_proj),
        .camera_pos = camera->position,
        .screen_size = simd_make_uint2(gbuffer->width, gbuffer->height),
        .max_steps = ssr->max_steps,
        .initial_step = 0.1f,
        .thickness = ssr->thickness
    };
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];

    MTLSize groups = MTLSizeMake((gbuffer->width + 7) / 8, (gbuffer->height + 7) / 8, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

    // Pass 2: Resolve (sample color at hit points)
    [encoder setComputePipelineState:ssr->resolve_pipeline];
    [encoder setTexture:ssr->ray_hit_texture atIndex:0];
    [encoder setTexture:scene_color atIndex:1];
    [encoder setTexture:reflection_output atIndex:2];

    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

    [encoder endEncoding];
}
```

OUTPUT: SSR system with hierarchical tracing.
```

---

## Agent 4.6: Bloom & Tonemapping

```
TASK: Implement Bloom and Tonemapping (Phase 4, Agent 6)

You are implementing the HDR post-processing pipeline.

WORKING DIRECTORY: src/engine/rendering/3d_rendering/postprocess/

FILES TO COMPLETE:
- bloom/bloom_downsample.c/h
- bloom/bloom_upsample.c/h
- tonemapping/aces_tonemapper.c/h
- tonemapping/exposure_adaptation.c/h

METAL BLOOM & TONEMAPPING:
```c
typedef struct bloom_system {
    id<MTLTexture> mip_chain[8];
    uint32_t mip_count;
    id<MTLComputePipelineState> downsample_pipeline;
    id<MTLComputePipelineState> upsample_pipeline;
    float threshold;
    float intensity;
} bloom_system_t;

typedef struct tonemapper {
    id<MTLComputePipelineState> histogram_pipeline;
    id<MTLComputePipelineState> tonemap_pipeline;
    id<MTLBuffer> histogram_buffer;  // 256 bins
    id<MTLBuffer> exposure_buffer;
    float min_exposure;
    float max_exposure;
    float adaptation_speed;
} tonemapper_t;

// Bloom shader:
```metal
// 13-tap tent filter downsample
kernel void bloom_downsample(
    texture2d<float, access::read> src [[texture(0)]],
    texture2d<float, access::write> dst [[texture(1)]],
    constant float& threshold [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float2 texel_size = 1.0 / float2(src.get_width(), src.get_height());
    float2 uv = (float2(gid) + 0.5) * 2.0 * texel_size;

    // 13-tap filter
    float4 a = src.sample(linear_sampler, uv + float2(-2, -2) * texel_size);
    float4 b = src.sample(linear_sampler, uv + float2( 0, -2) * texel_size);
    float4 c = src.sample(linear_sampler, uv + float2( 2, -2) * texel_size);
    float4 d = src.sample(linear_sampler, uv + float2(-1, -1) * texel_size);
    float4 e = src.sample(linear_sampler, uv + float2( 1, -1) * texel_size);
    float4 f = src.sample(linear_sampler, uv + float2(-2,  0) * texel_size);
    float4 g = src.sample(linear_sampler, uv);
    float4 h = src.sample(linear_sampler, uv + float2( 2,  0) * texel_size);
    float4 i = src.sample(linear_sampler, uv + float2(-1,  1) * texel_size);
    float4 j = src.sample(linear_sampler, uv + float2( 1,  1) * texel_size);
    float4 k = src.sample(linear_sampler, uv + float2(-2,  2) * texel_size);
    float4 l = src.sample(linear_sampler, uv + float2( 0,  2) * texel_size);
    float4 m = src.sample(linear_sampler, uv + float2( 2,  2) * texel_size);

    float4 color = (d + e + i + j) * 0.5 * 0.25 +
                   (a + b + g + f) * 0.25 * 0.25 +
                   (b + c + h + g) * 0.25 * 0.25 +
                   (f + g + l + k) * 0.25 * 0.25 +
                   (g + h + m + l) * 0.25 * 0.25;

    // Apply threshold on first mip
    if (threshold > 0) {
        float brightness = max(color.r, max(color.g, color.b));
        float soft = brightness - threshold + 0.01;
        soft = clamp(soft, 0.0, 2.0 * 0.01);
        soft = soft * soft / (4.0 * 0.01 + 0.0001);
        float contribution = max(soft, brightness - threshold) / max(brightness, 0.0001);
        color.rgb *= contribution;
    }

    dst.write(color, gid);
}

// ACES tonemapping
float3 aces_tonemap(float3 color) {
    float3x3 m1 = float3x3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );
    float3x3 m2 = float3x3(
        1.60475, -0.53108, -0.07367,
        -0.10208, 1.10813, -0.00605,
        -0.00327, -0.07276, 1.07602
    );

    float3 v = m1 * color;
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return m2 * (a / b);
}
```

C code:
```c
void bloom_render(bloom_system_t* bloom, id<MTLCommandBuffer> cmd,
                  id<MTLTexture> hdr_input) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

    // Downsample chain
    [encoder setComputePipelineState:bloom->downsample_pipeline];

    id<MTLTexture> src = hdr_input;
    for (uint32_t i = 0; i < bloom->mip_count; i++) {
        [encoder setTexture:src atIndex:0];
        [encoder setTexture:bloom->mip_chain[i] atIndex:1];

        float threshold = (i == 0) ? bloom->threshold : 0.0f;
        [encoder setBytes:&threshold length:sizeof(float) atIndex:0];

        uint32_t w = bloom->mip_chain[i].width;
        uint32_t h = bloom->mip_chain[i].height;
        MTLSize groups = MTLSizeMake((w + 7) / 8, (h + 7) / 8, 1);
        [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

        src = bloom->mip_chain[i];
    }

    // Upsample chain
    [encoder setComputePipelineState:bloom->upsample_pipeline];

    for (int i = bloom->mip_count - 2; i >= 0; i--) {
        [encoder setTexture:bloom->mip_chain[i + 1] atIndex:0];  // Lower res
        [encoder setTexture:bloom->mip_chain[i] atIndex:1];      // Higher res (also output)

        uint32_t w = bloom->mip_chain[i].width;
        uint32_t h = bloom->mip_chain[i].height;
        MTLSize groups = MTLSizeMake((w + 7) / 8, (h + 7) / 8, 1);
        [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
    }

    [encoder endEncoding];
}

void tonemap_render(tonemapper_t* tm, id<MTLCommandBuffer> cmd,
                    id<MTLTexture> hdr_input, id<MTLTexture> bloom,
                    id<MTLTexture> ldr_output) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

    // Histogram for auto-exposure
    [encoder setComputePipelineState:tm->histogram_pipeline];
    [encoder setTexture:hdr_input atIndex:0];
    [encoder setBuffer:tm->histogram_buffer offset:0 atIndex:0];
    // ... dispatch

    // Tonemap
    [encoder setComputePipelineState:tm->tonemap_pipeline];
    [encoder setTexture:hdr_input atIndex:0];
    [encoder setTexture:bloom atIndex:1];
    [encoder setTexture:ldr_output atIndex:2];
    [encoder setBuffer:tm->exposure_buffer offset:0 atIndex:0];

    uint32_t w = ldr_output.width;
    uint32_t h = ldr_output.height;
    MTLSize groups = MTLSizeMake((w + 7) / 8, (h + 7) / 8, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];

    [encoder endEncoding];
}
```

OUTPUT: HDR bloom and ACES tonemapping.
```
