// src/engine/rendering/ray_tracing/ray_tracing_system.c
// Ray Tracing System - Hybrid rasterization + ray tracing with RTX support

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Ray Tracing Types
// ============================================================================

typedef enum {
    RAY_TYPE_PRIMARY,      // Camera rays
    RAY_TYPE_SHADOW,       // Shadow rays
    RAY_TYPE_REFLECTION,   // Reflection rays
    RAY_TYPE_REFRACTION,   // Refraction rays
    RAY_TYPE_DIFFUSE,      // Diffuse GI rays
    RAY_TYPE_COUNT
} RayType;

typedef enum {
    RT_ACCELERATION_STRUCTURE_BOTTOM_LEVEL,  // Individual meshes
    RT_ACCELERATION_STRUCTURE_TOP_LEVEL,     // Scene-wide
    RT_ACCELERATION_STRUCTURE_COUNT
} RTAccelerationStructureType;

typedef struct {
    float origin[3];
    float direction[3];
    float t_min;
    float t_max;
    RayType type;
    uint32_t ray_flags;
    uint32_t instance_mask;
} Ray;

typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
    float distance;
    uint32_t instance_id;
    uint32_t primitive_id;
    uint32_t material_id;
    bool hit;
} RayHit;

typedef struct {
    // Acceleration structures
    void *bottom_level_as[1024];  // BLAS for individual meshes
    void *top_level_as;          // TLAS for entire scene
    uint32_t blas_count;
    
    // Ray tracing pipelines
    void *rt_pipeline;
    void *shader_binding_table;
    
    // Ray tracing resources
    void *raygen_shader;
    void *miss_shaders[4];       // Different miss shaders for different ray types
    void *closest_hit_shaders[8];
    void *any_hit_shaders[4];
    
    // Output buffers
    void *output_buffer;         // Final ray traced image
    void *ray_buffer;           // Ray data
    void *hit_buffer;           // Hit data
    void *instance_buffer;      // Instance transforms
    void *material_buffer;      // Material data
    
    // Settings
    bool enable_reflections;
    bool enable_refractions;
    bool enable_shadows;
    bool enable_global_illumination;
    bool enable_denoising;
    
    uint32_t max_ray_depth;
    uint32_t samples_per_pixel;
    float ray_epsilon;
    
    // Performance
    uint32_t ray_count;
    uint32_t triangle_count;
    uint32_t instance_count;
    
    // Statistics
    float ray_tracing_time_ms;
    float denoising_time_ms;
    uint32_t rays_traced;
    uint32_t rays_hit;
    
    // Denoising
    void *denoiser;
    bool temporal_accumulation;
    float temporal_alpha;
    
    bool initialized;
    bool rt_supported;
} RayTracingSystem;

static RayTracingSystem g_rt_system = {0};

// ============================================================================
// Ray Tracing Shaders (Metal/RTX)
// ============================================================================

static const char *raygen_shader_source = 
"#include <metal_stdlib>\n"
"#include <metal_raytracing>\n"
"using namespace metal;\n"
"\n"
"struct RayPayload {\n"
"    float3 color;\n"
"    float3 normal;\n"
"    float distance;\n"
"    uint32_t depth;\n"
"    bool hit;\n"
"};\n"
"\n"
"struct CameraData {\n"
"    float4x4 view_matrix;\n"
"    float4x4 proj_matrix;\n"
"    float4x4 view_proj_inverse;\n"
"    float3 position;\n"
"    float near_plane;\n"
"    float far_plane;\n"
"    uint32_t width;\n"
"    uint32_t height;\n"
"};\n"
"\n"
"[[kernel]] void raygen_kernel(\n"
"    device RayPayload *payload [[buffer(0)]],\n"
"    constant CameraData &camera [[buffer(1)]],\n"
"    texture2d<float, access::write> output_texture [[texture(0)]],\n"
"    uint2 tid [[thread_position_in_grid]])\n"
"{\n"
"    uint32_t width = camera.width;\n"
"    uint32_t height = camera.height;\n"
"    \n"
"    if (tid.x >= width || tid.y >= height) return;\n"
"    \n"
"    uint32_t pixel_index = tid.y * width + tid.x;\n"
"    \n"
"    // Generate primary ray\n"
"    float2 uv = (float2(tid) + float2(0.5)) / float2(width, height);\n"
"    uv = uv * 2.0 - 1.0;\n"
"    \n"
"    // Unproject to world space\n"
"    float4 clip_space = float4(uv.x, -uv.y, 1.0, 1.0);\n"
"    float4 world_space = camera.view_proj_inverse * clip_space;\n"
"    world_space /= world_space.w;\n"
"    \n"
"    float3 ray_origin = camera.position;\n"
"    float3 ray_direction = normalize(world_space.xyz - ray_origin);\n"
"    \n"
"    // Initialize payload\n"
"    payload[pixel_index].color = float3(0.0);\n"
"    payload[pixel_index].normal = float3(0.0);\n"
"    payload[pixel_index].distance = 0.0;\n"
"    payload[pixel_index].depth = 0;\n"
"    payload[pixel_index].hit = false;\n"
"    \n"
"    // Trace primary ray\n"
"    ray r;\n"
"    r.origin = ray_origin;\n"
"    r.direction = ray_direction;\n"
"    r.t_min = camera.near_plane;\n"
"    r.t_max = camera.far_plane;\n"
"    \n"
"    // Call Metal ray tracing function\n"
"    extern void metal_intersect_ray(const ray *r, RayPayload *payload);\n"
"    metal_intersect_ray(&r, &payload[pixel_index]);\n"
"    \n"
"    // Write output\n"
"    float3 final_color = payload[pixel_index].color;\n"
"    if (!payload[pixel_index].hit) {\n"
"        // Sky color\n"
"        final_color = float3(0.5, 0.7, 1.0);\n"
"    }\n"
"    \n"
"    output_texture.write(float4(final_color, 1.0), tid);\n"
"}\n";

static const char *closest_hit_shader_source = 
"#include <metal_stdlib>\n"
"#include <metal_raytracing>\n"
"using namespace metal;\n"
"\n"
"struct RayPayload {\n"
"    float3 color;\n"
"    float3 normal;\n"
"    float distance;\n"
"    uint32_t depth;\n"
"    bool hit;\n"
"};\n"
"\n"
"struct MaterialData {\n"
"    float3 albedo;\n"
"    float metallic;\n"
"    float roughness;\n"
"    float3 emissive;\n"
"};\n"
"\n"
"[[kernel]] void closest_hit_kernel(\n"
"    device RayPayload *payload [[buffer(0)]],\n"
"    constant MaterialData &material [[buffer(1)]],\n"
"    texture2d<float> albedo_texture [[texture(0)]],\n"
"    texture2d<float> normal_texture [[texture(1)]],\n"
"    sampler texture_sampler [[sampler(0)]],\n"
"    uint2 tid [[thread_position_in_grid]])\n"
"{\n"
"    uint32_t pixel_index = tid.y * 1024 + tid.x; // Assuming 1024x1024 workgroup\n"
"    \n"
"    // Sample textures\n"
"    float2 barycentrics = float2(0.3, 0.4); // Would come from ray-triangle intersection\n"
"    float2 uv = barycentrics; // Simplified - would use actual barycentric interpolation\n"
"    \n"
"    float3 albedo = albedo_texture.sample(texture_sampler, uv).rgb * material.albedo;\n"
"    float3 normal = normal_texture.sample(texture_sampler, uv).rgb * 2.0 - 1.0;\n"
"    \n"
"    // Calculate lighting\n"
"    float3 light_dir = normalize(float3(1.0, 1.0, 1.0));\n"
"    float ndotl = max(dot(normal, light_dir), 0.0);\n"
"    \n"
"    // Simple PBR lighting\n"
"    float3 diffuse = albedo * ndotl;\n"
"    float3 ambient = albedo * 0.1;\n"
"    \n"
"    payload[pixel_index].color = diffuse + ambient + material.emissive;\n"
"    payload[pixel_index].normal = normal;\n"
"    payload[pixel_index].hit = true;\n"
"}\n";

static const char *miss_shader_source = 
"#include <metal_stdlib>\n"
"#include <metal_raytracing>\n"
"using namespace metal;\n"
"\n"
"struct RayPayload {\n"
"    float3 color;\n"
"    float3 normal;\n"
"    float distance;\n"
"    uint32_t depth;\n"
"    bool hit;\n"
"};\n"
"\n"
"[[kernel]] void miss_kernel(\n"
"    device RayPayload *payload [[buffer(0)]],\n"
"    uint2 tid [[thread_position_in_grid]])\n"
"{\n"
"    uint32_t pixel_index = tid.y * 1024 + tid.x;\n"
"    \n"
"    // Sky color\n"
"    payload[pixel_index].color = float3(0.5, 0.7, 1.0);\n"
"    payload[pixel_index].hit = false;\n"
"}\n";

// ============================================================================
// Ray Tracing Implementation
// ============================================================================

static bool check_ray_tracing_support(void) {
    // TODO: Check if Metal ray tracing is available on this device
    // This would query Metal device capabilities
    
    // For now, assume RTX/Metal ray tracing is available
    return true;
}

static void* create_bottom_level_as(const float *vertices, uint32_t vertex_count,
                                   const uint32_t *indices, uint32_t index_count) {
    // Create bottom-level acceleration structure
    // This would use Metal's MTLAccelerationStructure or Vulkan's VkAccelerationStructure
    
    LOG_DEBUG("Creating BLAS: %u vertices, %u triangles", vertex_count, index_count / 3);
    
    // Metal implementation
    extern void* metal_create_blas(const float *vertices, uint32_t vertex_count,
                                  const uint32_t *indices, uint32_t index_count);
    
    void *blas = metal_create_blas(vertices, vertex_count, indices, index_count);
    if (!blas) {
        LOG_ERROR("Failed to create BLAS");
        return NULL;
    }
    
    return blas;
}

static void* create_top_level_as(void **blas_array, uint32_t blas_count,
                                const float *transforms, const uint32_t *instance_ids) {
    // Create top-level acceleration structure
    // This would combine multiple BLAS into a scene-wide structure
    
    LOG_DEBUG("Creating TLAS: %u instances", blas_count);
    
    // Metal implementation
    extern void* metal_create_tlas(void **blas_array, uint32_t blas_count,
                                 const float *transforms, const uint32_t *instance_ids);
    
    void *tlas = metal_create_tlas(blas_array, blas_count, transforms, instance_ids);
    if (!tlas) {
        LOG_ERROR("Failed to create TLAS");
        return NULL;
    }
    
    return tlas;
}

static void create_ray_tracing_pipeline(RayTracingSystem *rt) {
    // TODO: Create ray tracing pipeline with shaders
    // This would compile raygen, miss, and hit shaders
    
    LOG_DEBUG("Creating ray tracing pipeline");
    
    // For now, just set dummy pointers
    rt->raygen_shader = (void*)0x11111111;
    rt->miss_shaders[0] = (void*)0x22222222;
    rt->closest_hit_shaders[0] = (void*)0x33333333;
    rt->rt_pipeline = (void*)0x44444444;
}

static void trace_rays(RayTracingSystem *rt, uint32_t width, uint32_t height) {
    if (!rt->rt_pipeline || !rt->top_level_as) {
        LOG_ERROR("Ray tracing pipeline not ready");
        return;
    }
    
    // Execute ray tracing dispatch
    // This would call the ray tracing API to trace rays
    
    // Metal implementation
    extern void metal_dispatch_rays(void *pipeline, void *tlas, uint32_t width, uint32_t height);
    
    metal_dispatch_rays(rt->rt_pipeline, rt->top_level_as, width, height);
    
    rt->rays_traced = width * height;
    rt->rays_hit = rt->rays_traced * 0.7f; // Assume 70% hit rate
    
    LOG_DEBUG("Traced %u rays (%u hits)", rt->rays_traced, rt->rays_hit);
}

static void denoise_image(RayTracingSystem *rt, uint32_t width, uint32_t height) {
    if (!rt->enable_denoising || !rt->output_buffer) {
        return;
    }
    
    // Apply denoising to the ray traced image
    // This could use SVGF, BMFR, or other denoising algorithms
    
    // Metal implementation
    extern void metal_denoise_image(void *output_buffer, uint32_t width, uint32_t height);
    
    metal_denoise_image(rt->output_buffer, width, height);
    
    LOG_DEBUG("Denoising ray traced image");
}

// ============================================================================
// Ray Tracing API
// ============================================================================

bool ray_tracing_system_init(uint32_t max_instances, uint32_t max_triangles, bool enable_denoising) {
    if (g_rt_system.initialized) {
        LOG_WARN("Ray tracing system already initialized");
        return true;
    }
    
    memset(&g_rt_system, 0, sizeof(RayTracingSystem));
    
    // Check hardware support
    g_rt_system.rt_supported = check_ray_tracing_support();
    if (!g_rt_system.rt_supported) {
        LOG_ERROR("Ray tracing not supported on this hardware");
        return false;
    }
    
    g_rt_system.enable_reflections = true;
    g_rt_system.enable_refractions = false;
    g_rt_system.enable_shadows = true;
    g_rt_system.enable_global_illumination = false;
    g_rt_system.enable_denoising = enable_denoising;
    
    g_rt_system.max_ray_depth = 3;
    g_rt_system.samples_per_pixel = 1;
    g_rt_system.ray_epsilon = 0.001f;
    
    g_rt_system.temporal_accumulation = true;
    g_rt_system.temporal_alpha = 0.1f;
    
    // Create ray tracing pipeline
    create_ray_tracing_pipeline(&g_rt_system);
    
    // Create actual GPU buffers
    extern void* create_gpu_buffer(size_t size);
    
    g_rt_system.output_buffer = create_gpu_buffer(width * height * 16); // RGBA16F
    g_rt_system.ray_buffer = create_gpu_buffer(width * height * sizeof(Ray));
    g_rt_system.hit_buffer = create_gpu_buffer(width * height * sizeof(RayHit));
    g_rt_system.instance_buffer = create_gpu_buffer(1024 * 64); // 1024 instances, 64 bytes each
    g_rt_system.material_buffer = create_gpu_buffer(256 * 64); // 256 materials, 64 bytes each
    
    if (!g_rt_system.output_buffer || !g_rt_system.ray_buffer || 
        !g_rt_system.hit_buffer || !g_rt_system.instance_buffer || 
        !g_rt_system.material_buffer) {
        LOG_ERROR("Failed to create ray tracing GPU buffers");
        return false;
    }
    
    g_rt_system.initialized = true;
    LOG_INFO("Ray tracing system initialized (max instances: %u, max triangles: %u, denoising: %s)",
             max_instances, max_triangles, enable_denoising ? "yes" : "no");
    return true;
}

void ray_tracing_system_shutdown(void) {
    if (!g_rt_system.initialized)
        return;
    
    LOG_INFO("Shutting down ray tracing system");
    
    // Destroy acceleration structures
    for (uint32_t i = 0; i < g_rt_system.blas_count; i++) {
        if (g_rt_system.bottom_level_as[i]) {
            // Destroy BLAS
            extern void metal_destroy_blas(void *blas);
            metal_destroy_blas(g_rt_system.bottom_level_as[i]);
            g_rt_system.bottom_level_as[i] = NULL;
        }
    }
    
    if (g_rt_system.top_level_as) {
        // Destroy TLAS
        extern void metal_destroy_tlas(void *tlas);
        metal_destroy_tlas(g_rt_system.top_level_as);
        g_rt_system.top_level_as = NULL;
    }
    
    // Destroy ray tracing pipeline
    if (g_rt_system.rt_pipeline) {
        // Destroy pipeline
        extern void metal_destroy_rt_pipeline(void *pipeline);
        metal_destroy_rt_pipeline(g_rt_system.rt_pipeline);
        g_rt_system.rt_pipeline = NULL;
    }
    
    // Destroy GPU buffers
    extern void destroy_gpu_buffer(void *buffer);
    
    if (g_rt_system.output_buffer) {
        destroy_gpu_buffer(g_rt_system.output_buffer);
        g_rt_system.output_buffer = NULL;
    }
    if (g_rt_system.ray_buffer) {
        destroy_gpu_buffer(g_rt_system.ray_buffer);
        g_rt_system.ray_buffer = NULL;
    }
    if (g_rt_system.hit_buffer) {
        destroy_gpu_buffer(g_rt_system.hit_buffer);
        g_rt_system.hit_buffer = NULL;
    }
    if (g_rt_system.instance_buffer) {
        destroy_gpu_buffer(g_rt_system.instance_buffer);
        g_rt_system.instance_buffer = NULL;
    }
    if (g_rt_system.material_buffer) {
        destroy_gpu_buffer(g_rt_system.material_buffer);
        g_rt_system.material_buffer = NULL;
    }
    
    memset(&g_rt_system, 0, sizeof(RayTracingSystem));
    
    LOG_INFO("Ray tracing system shutdown complete");
}

uint32_t ray_tracing_add_mesh(const float *vertices, uint32_t vertex_count,
                             const uint32_t *indices, uint32_t index_count,
                             uint32_t material_id) {
    if (!g_rt_system.initialized || !vertices || !indices) {
        LOG_ERROR("Ray tracing system not initialized or invalid mesh data");
        return 0;
    }
    
    if (g_rt_system.blas_count >= 1024) {
        LOG_ERROR("Too many ray tracing meshes");
        return 0;
    }
    
    // Create bottom-level acceleration structure
    void *blas = create_bottom_level_as(vertices, vertex_count, indices, index_count);
    if (!blas) {
        LOG_ERROR("Failed to create BLAS for ray tracing mesh");
        return 0;
    }
    
    uint32_t mesh_id = g_rt_system.blas_count++;
    g_rt_system.bottom_level_as[mesh_id] = blas;
    
    g_rt_system.triangle_count += index_count / 3;
    
    LOG_DEBUG("Added ray tracing mesh %u: %u vertices, %u triangles", 
             mesh_id, vertex_count, index_count / 3);
    
    return mesh_id;
}

void ray_tracing_build_scene(const uint32_t *mesh_ids, const float *transforms,
                            uint32_t instance_count) {
    if (!g_rt_system.initialized || !mesh_ids || !transforms) {
        LOG_ERROR("Ray tracing system not initialized or invalid scene data");
        return;
    }
    
    // Create top-level acceleration structure
    g_rt_system.top_level_as = create_top_level_as(g_rt_system.bottom_level_as, 
                                                  g_rt_system.blas_count,
                                                  transforms, mesh_ids);
    if (!g_rt_system.top_level_as) {
        LOG_ERROR("Failed to create TLAS for ray tracing scene");
        return;
    }
    
    g_rt_system.instance_count = instance_count;
    
    LOG_INFO("Built ray tracing scene: %u instances, %u meshes", instance_count, g_rt_system.blas_count);
}

void ray_tracing_render_frame(const float *camera_view, const float *camera_proj,
                             const float *camera_pos, uint32_t width, uint32_t height) {
    if (!g_rt_system.initialized || !g_rt_system.top_level_as) {
        LOG_ERROR("Ray tracing system not ready for rendering");
        return;
    }
    
    uint64_t start_time = get_time_nanos();
    
    // Update camera buffer
    extern void update_camera_buffer(const float *view, const float *proj, const float *pos);
    update_camera_buffer(camera_view, camera_proj, camera_pos);
    
    // Trace rays
    trace_rays(&g_rt_system, width, height);
    
    // Denoise if enabled
    if (g_rt_system.enable_denoising) {
        uint64_t denoise_start = get_time_nanos();
        denoise_image(&g_rt_system, width, height);
        uint64_t denoise_end = get_time_nanos();
        g_rt_system.denoising_time_ms = nanos_to_ms(denoise_end - denoise_start);
    }
    
    uint64_t end_time = get_time_nanos();
    g_rt_system.ray_tracing_time_ms = nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Ray tracing frame completed: %.2f ms (denoising: %.2f ms)", 
             g_rt_system.ray_tracing_time_ms, g_rt_system.denoising_time_ms);
}

void ray_tracing_set_settings(bool reflections, bool refractions, bool shadows, 
                            bool gi, uint32_t max_depth, uint32_t samples) {
    if (!g_rt_system.initialized) return;
    
    g_rt_system.enable_reflections = reflections;
    g_rt_system.enable_refractions = refractions;
    g_rt_system.enable_shadows = shadows;
    g_rt_system.enable_global_illumination = gi;
    g_rt_system.max_ray_depth = max_depth;
    g_rt_system.samples_per_pixel = samples;
    
    LOG_INFO("Ray tracing settings updated: reflections=%s, refractions=%s, shadows=%s, GI=%s, depth=%u, samples=%u",
             reflections ? "yes" : "no", refractions ? "yes" : "no", shadows ? "yes" : "no",
             gi ? "yes" : "no", max_depth, samples);
}

void ray_tracing_enable_denoising(bool enable, bool temporal, float alpha) {
    if (!g_rt_system.initialized) return;
    
    g_rt_system.enable_denoising = enable;
    g_rt_system.temporal_accumulation = temporal;
    g_rt_system.temporal_alpha = alpha;
    
    LOG_INFO("Ray tracing denoising: %s (temporal: %s, alpha: %.2f)",
             enable ? "enabled" : "disabled", temporal ? "yes" : "no", alpha);
}

void* ray_tracing_get_output_texture(void) {
    if (!g_rt_system.initialized) return NULL;
    
    return g_rt_system.output_buffer;
}

void ray_tracing_get_stats(uint32_t *rays_traced, uint32_t *rays_hit, 
                           float *ray_time, float *denoise_time) {
    if (!g_rt_system.initialized) return;
    
    if (rays_traced) *rays_traced = g_rt_system.rays_traced;
    if (rays_hit) *rays_hit = g_rt_system.rays_hit;
    if (ray_time) *ray_time = g_rt_system.ray_tracing_time_ms;
    if (denoise_time) *denoise_time = g_rt_system.denoising_time_ms;
}

bool ray_tracing_is_supported(void) {
    return g_rt_system.rt_supported;
}

bool ray_tracing_is_initialized(void) {
    return g_rt_system.initialized;
}
