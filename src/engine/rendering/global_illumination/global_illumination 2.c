// src/engine/rendering/global_illumination/global_illumination.c
// Global Illumination System - Real-time GI with DDGI and ReSTIR

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Global Illumination Types
// ============================================================================

typedef enum {
    GI_TYPE_DDGI,           // Dynamic Diffuse Global Illumination
    GI_TYPE_RESTIR,          // Reservoir Temporal Importance Resampling
    GI_TYPE_LPV,            // Light Propagation Volumes
    GI_TYPE_VXGI,           // Voxel Cone Tracing
    GI_TYPE_SVOGI,          // Sparse Voxel Octree Global Illumination
    GI_TYPE_HYBRID          // Hybrid approach
} GIType;

typedef struct {
    float position[3];
    float normal[3];
    float albedo[3];
    float emissive[3];
    float indirect_lighting[3];
    float distance;
    float confidence;
} GIVoxel;

typedef struct {
    // DDGI (Dynamic Diffuse Global Illumination)
    struct {
        GIVoxel *voxels;
        uint32_t voxel_count_x, voxel_count_y, voxel_count_z;
        float voxel_size;
        float3 world_origin;
        float3 world_size;
        
        // Irradiance and distance textures
        void *irradiance_texture;
        void *distance_texture;
        
        // Probe management
        float3 *probe_positions;
        uint32_t probe_count;
        float probe_spacing;
        
        // Temporal accumulation
        float temporal_alpha;
        uint32_t temporal_frame;
        
        // Ray tracing settings
        uint32_t rays_per_probe;
        float max_ray_distance;
        float bias;
    } ddgi;
    
    // ReSTIR (Reservoir Temporal Importance Resampling)
    struct {
        void *reservoir_buffer;
        uint32_t reservoir_size;
        uint32_t temporal_frames;
        float spatial_radius;
        float temporal_bias;
        float alpha;
        
        // Importance sampling
        void *importance_buffer;
        uint32_t importance_samples;
        
        // Temporal reuse
        void *history_buffer;
        uint32_t history_frames;
    } restir;
    
    // General settings
    GIType type;
    bool enable_gi;
    bool enable_shadows;
    bool enable_bounces;
    uint32_t max_bounces;
    
    // Performance
    uint32_t gi_resolution;
    float gi_update_rate;
    float gi_intensity;
    
    // Statistics
    float gi_time_ms;
    uint32_t voxels_updated;
    uint32_t rays_traced;
    uint32_t samples_generated;
    
    // GPU resources
    void *gi_compute_shader;
    void *gi_render_target;
    void *gi_buffer;
    
    bool initialized;
} GISystem;

static GISystem g_gi_system = {0};

// ============================================================================
// DDGI Implementation
// ============================================================================

static void initialize_ddgi(GISystem *gi, uint32_t voxel_count_x, uint32_t voxel_count_y, uint32_t voxel_count_z,
                         float voxel_size, const float3 world_origin) {
    gi->ddgi.voxel_count_x = voxel_count_x;
    gi->ddgi.voxel_count_y = voxel_count_y;
    gi->ddgi.voxel_count_z = voxel_count_z;
    gi->ddgi.voxel_size = voxel_size;
    gi->ddgi.world_origin = world_origin;
    
    gi->ddgi.world_size.x = voxel_count_x * voxel_size;
    gi->ddgi.world_size.y = voxel_count_y * voxel_size;
    gi->ddgi.world_size.z = voxel_count_z * voxel_size;
    
    uint32_t total_voxels = voxel_count_x * voxel_count_y * voxel_count_z;
    gi->ddgi.voxels = calloc(total_voxels, sizeof(GIVoxel));
    
    if (!gi->ddgi.voxels) {
        LOG_ERROR("Failed to allocate DDGI voxels");
        return;
    }
    
    // Initialize probe grid
    gi->ddgi.probe_spacing = voxel_size * 2.0f;
    gi->ddgi.probe_count_x = voxel_count_x / 2;
    gi->ddgi.probe_count_y = voxel_count_y / 2;
    gi->ddgi.probe_count_z = voxel_count_z / 2;
    gi->ddgi.probe_count = gi->ddgi.probe_count_x * gi->ddgi.probe_count_y * gi->ddgi.probe_count_z;
    
    gi->ddgi.probe_positions = malloc(gi->ddgi.probe_count * sizeof(float3));
    if (!gi->ddgi.probe_positions) {
        LOG_ERROR("Failed to allocate probe positions");
        free(gi->ddgi.voxels);
        gi->ddgi.voxels = NULL;
        return;
    }
    
    // Generate probe positions
    for (uint32_t z = 0; z < gi->ddgi.probe_count_z; z++) {
        for (uint32_t y = 0; y < gi->ddgi.probe_count_y; y++) {
            for (uint32_t x = 0; x < gi->ddgi.probe_count_x; x++) {
                uint32_t index = (z * gi->ddgi.probe_count_y + y) * gi->ddgi.probe_count_x + x;
                gi->ddgi.probe_positions[index].x = world_origin.x + (x + 0.5f) * gi->ddgi.probe_spacing;
                gi->ddgi.probe_positions[index].y = world_origin.y + (y + 0.5f) * gi->ddgi.probe_spacing;
                gi->ddgi.probe_positions[index].z = world_origin.z + (z + 0.5f) * gi->ddgi.probe_spacing;
            }
        }
    }
    
    gi->ddgi.rays_per_probe = 32;
    gi->ddgi.max_ray_distance = voxel_size * 4.0f;
    gi->ddgi.bias = 0.001f;
    gi->ddgi.temporal_alpha = 0.1f;
    
    // Create irradiance and distance textures
    gi->ddgi.irradiance_texture = create_texture_3d(voxel_count_x, voxel_count_y, voxel_count_z, FORMAT_RGBA16F);
    gi->ddgi.distance_texture = create_texture_3d(voxel_count_x, voxel_count_y, voxel_count_z, FORMAT_R16F);
    
    if (!gi->ddgi.irradiance_texture || !gi->ddgi.distance_texture) {
        LOG_ERROR("Failed to create DDGI textures");
        free(gi->ddgi.voxels);
        free(gi->ddgi.probe_positions);
        gi->ddgi.voxels = NULL;
        gi->ddgi.probe_positions = NULL;
        return;
    }
    
    // Initialize texture data
    clear_texture_3d(gi->ddgi.irradiance_texture, (float[]){0.0f, 0.0f, 0.0f, 0.0f});
    clear_texture_3d(gi->ddgi.distance_texture, (float[]){gi->ddgi.max_ray_distance});
    
    LOG_INFO("Initialized DDGI: %ux%ux%u voxels, %u probes", 
             voxel_count_x, voxel_count_y, voxel_count_z, gi->ddgi.probe_count);
}

static void update_ddgi_probes(GISystem *gi, const float *camera_pos, const float *view_matrix) {
    if (!gi->ddgi.voxels || !gi->ddgi.probe_positions) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Implement actual DDGI update algorithm
    // Trace rays from each probe position
    for (uint32_t i = 0; i < gi->ddgi.probe_count; i++) {
        const float3 *probe_pos = &gi->ddgi.probe_positions[i];
        
        // Check if probe is within range of camera
        float3 to_probe = {
            probe_pos->x - camera_pos[0],
            probe_pos->y - camera_pos[1],
            probe_pos->z - camera_pos[2]
        };
        float distance = sqrtf(to_probe.x * to_probe.x + to_probe.y * to_probe.y + to_probe.z * to_probe.z);
        
        if (distance < gi->ddgi.max_ray_distance * 2.0f) {
            // Trace rays and gather radiance
            float3 total_radiance = {0.0f, 0.0f, 0.0f};
            float total_distance = 0.0f;
            uint32_t valid_rays = 0;
            
            for (uint32_t ray = 0; ray < gi->ddgi.rays_per_probe; ray++) {
                // Generate ray direction using hemisphere sampling
                float u = (float)rand() / RAND_MAX;
                float v = (float)rand() / RAND_MAX;
                
                float theta = 2.0f * M_PI * u;
                float phi = acosf(2.0f * v - 1.0f);
                
                float3 ray_dir = {
                    sinf(phi) * cosf(theta),
                    cosf(phi),
                    sinf(phi) * sinf(theta)
                };
                
                // Trace ray and get radiance
                float3 radiance;
                float ray_distance;
                if (trace_ray_ddgi(probe_pos, &ray_dir, &radiance, &ray_distance)) {
                    total_radiance.x += radiance.x;
                    total_radiance.y += radiance.y;
                    total_radiance.z += radiance.z;
                    total_distance += ray_distance;
                    valid_rays++;
                }
            }
            
            // Average the results
            if (valid_rays > 0) {
                float inv_valid_rays = 1.0f / (float)valid_rays;
                total_radiance.x *= inv_valid_rays;
                total_radiance.y *= inv_valid_rays;
                total_radiance.z *= inv_valid_rays;
                total_distance *= inv_valid_rays;
                
                // Update probe data
                update_ddgi_probe(gi, i, &total_radiance, total_distance);
            }
            
            probes_updated++;
            rays_traced += gi->ddgi.rays_per_probe;
        }
    }
    
    // Update voxel data from probe results
    // TODO: Interpolate probe data to voxels
    
    uint64_t end_time = get_time_nanos();
    gi->gi_time_ms = nanos_to_ms(end_time - start_time);
    gi->voxels_updated = probes_updated;
    gi->rays_traced = rays_traced;
    
    LOG_DEBUG("DDGI update: %u probes, %u rays, %.2f ms", probes_updated, rays_traced, gi->gi_time_ms);
}

static float3 sample_ddgi_irradiance(GISystem *gi, const float3 world_pos, const float3 normal) {
    if (!gi->ddgi.irradiance_texture || !gi->ddgi.voxels) {
        return (float3){0.0f, 0.0f, 0.0f};
    }
    
    // Convert world position to voxel coordinates
    float3 voxel_pos = {
        (world_pos.x - gi->ddgi.world_origin.x) / gi->ddgi.voxel_size,
        (world_pos.y - gi->ddgi.world_origin.y) / gi->ddgi.voxel_size,
        (world_pos.z - gi->ddgi.world_origin.z) / gi->ddgi.voxel_size
    };
    
    // Clamp to voxel grid
    voxel_pos.x = fmaxf(0.0f, fminf(voxel_pos.x, gi->ddgi.voxel_count_x - 1.0f));
    voxel_pos.y = fmaxf(0.0f, fminf(voxel_pos.y, gi->ddgi.voxel_count_y - 1.0f));
    voxel_pos.z = fmaxf(0.0f, fminf(voxel_pos.z, gi->ddgi.voxel_count_z - 1.0f));
    
    // Trilinear interpolation
    uint32_t ix = (uint32_t)voxel_pos.x;
    uint32_t iy = (uint32_t)voxel_pos.y;
    uint32_t iz = (uint32_t)voxel_pos.z;
    
    float fx = voxel_pos.x - ix;
    float fy = voxel_pos.y - iy;
    float fz = voxel_pos.z - iz;
    
    // Sample 8 neighboring voxels
    float3 irradiance000 = gi->ddgi.voxels[(iz * gi->ddgi.voxel_count_y + iy) * gi->ddgi.voxel_count_x + ix].indirect_lighting;
    float3 irradiance100 = gi->ddgi.voxels[(iz * gi->ddgi.voxel_count_y + iy) * gi->ddgi.voxel_count_x + ix + 1].indirect_lighting;
    float3 irradiance010 = gi->ddgi.voxels[(iz * gi->ddgi.voxel_count_y + iy + 1) * gi->ddgi.voxel_count_x + ix].indirect_lighting;
    float3 irradiance110 = gi->ddgi.voxels[(iz * gi->ddgi.voxel_count_y + iy + 1) * gi->ddgi.voxel_count_x + ix + 1].indirect_lighting;
    float3 irradiance001 = gi->ddgi.voxels[((iz + 1) * gi->ddgi.voxel_count_y + iy) * gi->ddgi.voxel_count_x + ix].indirect_lighting;
    float3 irradiance101 = gi->ddgi.voxels[((iz + 1) * gi->ddgi.voxel_count_y + iy) * gi->ddgi.voxel_count_x + ix + 1].indirect_lighting;
    float3 irradiance011 = gi->ddgi.voxels[((iz + 1) * gi->ddgi.voxel_count_y + iy + 1) * gi->ddgi.voxel_count_x + ix].indirect_lighting;
    float3 irradiance111 = gi->ddgi.voxels[((iz + 1) * gi->ddgi.voxel_count_y + iy + 1) * gi->ddgi.voxel_count_x + ix + 1].indirect_lighting;
    
    // Trilinear interpolation
    float3 irradiance00 = lerp(irradiance000, irradiance100, fx);
    float3 irradiance01 = lerp(irradiance010, irradiance110, fx);
    float3 irradiance0 = lerp(irradiance00, irradiance01, fy);
    
    float3 irradiance10 = lerp(irradiance001, irradiance101, fx);
    float3 irradiance11 = lerp(irradiance011, irradiance111, fx);
    float3 irradiance1 = lerp(irradiance10, irradiance11, fy);
    
    float3 final_irradiance = lerp(irradiance0, irradiance1, fz);
    
    return final_irradiance * gi->gi_intensity;
}

// ============================================================================
// ReSTIR Implementation
// ============================================================================

static void initialize_restir(GISystem *gi, uint32_t reservoir_size, uint32_t temporal_frames) {
    gi->restir.reservoir_size = reservoir_size;
    gi->restir.temporal_frames = temporal_frames;
    gi->restir.spatial_radius = 10.0f;
    gi->restir.temporal_bias = 0.001f;
    gi->restir.alpha = 0.1f;
    
    // Create reservoir buffer
    gi->restir.reservoir_buffer = create_buffer(reservoir_size * sizeof(Reservoir));
    if (!gi->restir.reservoir_buffer) {
        LOG_ERROR("Failed to create ReSTIR reservoir buffer");
        return;
    }
    
    // Initialize reservoirs
    Reservoir *reservoirs = map_buffer(gi->restir.reservoir_buffer);
    for (uint32_t i = 0; i < reservoir_size; i++) {
        reservoirs[i].weight_sum = 0.0f;
        reservoirs[i].sample_count = 0;
        reservoirs[i].M = 0.0f;
        reservoirs[i].light_index = UINT32_MAX;
        reservoirs[i].random_hash = 0;
    }
    unmap_buffer(gi->restir.reservoir_buffer);
    
    // Create importance buffer
    gi->restir.importance_buffer = create_buffer(reservoir_size * sizeof(float));
    if (!gi->restir.importance_buffer) {
        LOG_ERROR("Failed to create ReSTIR importance buffer");
        destroy_buffer(gi->restir.reservoir_buffer);
        gi->restir.reservoir_buffer = NULL;
        return;
    }
    
    // Initialize importance values
    float *importance = map_buffer(gi->restir.importance_buffer);
    for (uint32_t i = 0; i < reservoir_size; i++) {
        importance[i] = 1.0f; // Equal importance initially
    }
    unmap_buffer(gi->restir.importance_buffer);
    
    // Create history buffer
    uint32_t history_size = reservoir_size * temporal_frames;
    gi->restir.history_buffer = create_buffer(history_size * sizeof(float4));
    if (!gi->restir.history_buffer) {
        LOG_ERROR("Failed to create ReSTIR history buffer");
        destroy_buffer(gi->restir.reservoir_buffer);
        destroy_buffer(gi->restir.importance_buffer);
        gi->restir.reservoir_buffer = NULL;
        gi->restir.importance_buffer = NULL;
        return;
    }
    
    // Initialize history with zeros
    float4 *history = map_buffer(gi->restir.history_buffer);
    memset(history, 0, history_size * sizeof(float4));
    unmap_buffer(gi->restir.history_buffer);
    
    LOG_INFO("Initialized ReSTIR: %u reservoirs, %u temporal frames", reservoir_size, temporal_frames);
}

static void update_restir(GISystem *gi, const float *camera_pos, const float *view_matrix) {
    if (!gi->restir.reservoir_buffer) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Implement ReSTIR update algorithm
    Reservoir *reservoirs = map_buffer(gi->restir.reservoir_buffer);
    float *importance = map_buffer(gi->restir.importance_buffer);
    float4 *history = map_buffer(gi->restir.history_buffer);
    
    // Generate new samples and update reservoirs
    for (uint32_t i = 0; i < gi->restir.reservoir_size; i++) {
        // Generate importance sample
        float u = (float)rand() / RAND_MAX;
        float v = (float)rand() / RAND_MAX;
        
        // Sample light source
        LightSample sample = sample_light_source(u, v);
        
        // Calculate sample weight
        float weight = calculate_sample_weight(&sample, importance[i]);
        
        // Update reservoir using temporal reuse
        update_reservoir(&reservoirs[i], &sample, weight, gi->restir.temporal_bias);
        
        // Apply spatial resampling
        if (gi->restir.spatial_radius > 0.0f) {
            spatial_resample_reservoirs(gi, i, reservoirs, importance);
        }
        
        // Update history
        uint32_t history_index = (gi->restir.temporal_frames * i) + (gi->restir.temporal_frames - 1);
        history[history_index] = (float4){sample.radiance.x, sample.radiance.y, sample.radiance.z, weight};
    }
    
    unmap_buffer(gi->restir.reservoir_buffer);
    unmap_buffer(gi->restir.importance_buffer);
    unmap_buffer(gi->restir.history_buffer);
    
    uint64_t end_time = get_time_nanos();
    gi->gi_time_ms += nanos_to_ms(end_time - start_time);
    gi->samples_generated = gi->restir.reservoir_size;
    
    LOG_DEBUG("ReSTIR update: %u samples, %.2f ms", gi->restir.reservoir_size, gi->gi_time_ms);
}

// ============================================================================
// Global Illumination API
// ============================================================================

bool gi_system_init(GIType type, uint32_t resolution, float update_rate, float intensity) {
    if (g_gi_system.initialized) {
        LOG_WARN("Global illumination system already initialized");
        return true;
    }
    
    memset(&g_gi_system, 0, sizeof(GISystem));
    
    g_gi_system.type = type;
    g_gi_system.enable_gi = true;
    g_gi_system.enable_shadows = true;
    g_gi_system.enable_bounces = true;
    g_gi_system.max_bounces = 2;
    g_gi_system.gi_resolution = resolution;
    g_gi_system.gi_update_rate = update_rate;
    g_gi_system.gi_intensity = intensity;
    
    switch (type) {
        case GI_TYPE_DDGI:
            // Initialize DDGI with 32x32x32 voxel grid
            initialize_ddgi(&g_gi_system, 32, 32, 32, 1.0f, (float3){0.0f, 0.0f, 0.0f});
            break;
            
        case GI_TYPE_RESTIR:
            // Initialize ReSTIR
            initialize_restir(&g_gi_system, 1024, 8);
            break;
            
        case GI_TYPE_HYBRID:
            // Initialize both DDGI and ReSTIR
            initialize_ddgi(&g_gi_system, 32, 32, 32, 1.0f, (float3){0.0f, 0.0f, 0.0f});
            initialize_restir(&g_gi_system, 1024, 8);
            break;
            
        default:
            LOG_ERROR("Unsupported GI type: %d", (int)type);
            return false;
    }
    
    // Create GI compute shader
    g_gi_system.gi_compute_shader = load_compute_shader("global_illumination");
    if (!g_gi_system.gi_compute_shader) {
        LOG_ERROR("Failed to load GI compute shader");
        return false;
    }
    
    // Set compute shader constants
    bind_compute_shader(g_gi_system.gi_compute_shader);
    set_compute_uniform("gi_type", (int)type);
    set_compute_uniform("gi_intensity", intensity);
    set_compute_uniform("max_bounces", g_gi_system.max_bounces);
    set_compute_uniform("gi_resolution", (int)resolution);
    unbind_compute_shader();
    
    // Create GI render target
    g_gi_system.gi_render_target = create_render_target(resolution, resolution, FORMAT_RGBA16F);
    if (!g_gi_system.gi_render_target) {
        LOG_ERROR("Failed to create GI render target");
        return false;
    }
    
    // Create GI buffer
    g_gi_system.gi_buffer = create_buffer(sizeof(GIData));
    if (!g_gi_system.gi_buffer) {
        LOG_ERROR("Failed to create GI buffer");
        return false;
    }
    
    // Initialize GI data
    GIData *gi_data = map_buffer(g_gi_system.gi_buffer);
    gi_data->gi_type = type;
    gi_data->intensity = intensity;
    gi_data->max_bounces = g_gi_system.max_bounces;
    gi_data->resolution = resolution;
    gi_data->update_rate = update_rate;
    unmap_buffer(g_gi_system.gi_buffer);
    
    g_gi_system.initialized = true;
    LOG_INFO("Global illumination system initialized (type: %d, resolution: %u, update_rate: %.2f, intensity: %.2f)",
             (int)type, resolution, update_rate, intensity);
    return true;
}

void gi_system_shutdown(void) {
    if (!g_gi_system.initialized)
        return;
    
    LOG_INFO("Shutting down global illumination system");
    
    // Cleanup DDGI
    if (g_gi_system.ddgi.voxels) {
        free(g_gi_system.ddgi.voxels);
        g_gi_system.ddgi.voxels = NULL;
    }
    
    if (g_gi_system.ddgi.probe_positions) {
        free(g_gi_system.ddgi.probe_positions);
        g_gi_system.ddgi.probe_positions = NULL;
    }
    
    // Destroy textures
    if (g_gi_system.ddgi.irradiance_texture) {
        destroy_texture(g_gi_system.ddgi.irradiance_texture);
        g_gi_system.ddgi.irradiance_texture = NULL;
    }
    if (g_gi_system.ddgi.distance_texture) {
        destroy_texture(g_gi_system.ddgi.distance_texture);
        g_gi_system.ddgi.distance_texture = NULL;
    }
    
    // Cleanup ReSTIR
    if (g_gi_system.restir.reservoir_buffer) {
        destroy_buffer(g_gi_system.restir.reservoir_buffer);
        g_gi_system.restir.reservoir_buffer = NULL;
    }
    if (g_gi_system.restir.importance_buffer) {
        destroy_buffer(g_gi_system.restir.importance_buffer);
        g_gi_system.restir.importance_buffer = NULL;
    }
    if (g_gi_system.restir.history_buffer) {
        destroy_buffer(g_gi_system.restir.history_buffer);
        g_gi_system.restir.history_buffer = NULL;
    }
    if (g_gi_system.gi_buffer) {
        destroy_buffer(g_gi_system.gi_buffer);
        g_gi_system.gi_buffer = NULL;
    }
    
    // Destroy GPU resources
    if (g_gi_system.gi_compute_shader) {
        destroy_shader(g_gi_system.gi_compute_shader);
        g_gi_system.gi_compute_shader = NULL;
    }
    if (g_gi_system.gi_render_target) {
        destroy_render_target(g_gi_system.gi_render_target);
        g_gi_system.gi_render_target = NULL;
    }
    
    memset(&g_gi_system, 0, sizeof(GISystem));
    
    LOG_INFO("Global illumination system shutdown complete");
}

void gi_system_update(const float *camera_pos, const float *view_matrix, const float *proj_matrix, float dt) {
    if (!g_gi_system.initialized || !g_gi_system.enable_gi) {
        return;
    }
    
    // Update based on update rate
    static float update_timer = 0.0f;
    update_timer += dt;
    
    if (update_timer < 1.0f / g_gi_system.gi_update_rate) {
        return;
    }
    
    update_timer = 0.0f;
    
    switch (g_gi_system.type) {
        case GI_TYPE_DDGI:
            update_ddgi_probes(&g_gi_system, camera_pos, view_matrix);
            break;
            
        case GI_TYPE_RESTIR:
            update_restir(&g_gi_system, camera_pos, view_matrix);
            break;
            
        case GI_TYPE_HYBRID:
            update_ddgi_probes(&g_gi_system, camera_pos, view_matrix);
            update_restir(&g_gi_system, camera_pos, view_matrix);
            break;
            
        default:
            break;
    }
}

float3 gi_system_sample_indirect_lighting(const float3 world_pos, const float3 normal, const float3 view_dir) {
    if (!g_gi_system.initialized || !g_gi_system.enable_gi) {
        return (float3){0.0f, 0.0f, 0.0f};
    }
    
    switch (g_gi_system.type) {
        case GI_TYPE_DDGI:
        return sample_ddgi_irradiance(&g_gi_system, world_pos, normal);
            
            // TODO: Sample ReSTIR
            Reservoir *reservoirs = map_buffer(g_gi_system.restir.reservoir_buffer);
            uint32_t pixel_index = (uint32_t)(world_pos.x * g_gi_system.gi_resolution) + 
                                 (uint32_t)(world_pos.z * g_gi_system.gi_resolution);
            
            if (pixel_index < g_gi_system.restir.reservoir_size) {
                Reservoir *reservoir = &reservoirs[pixel_index];
                if (reservoir->sample_count > 0) {
                    // Sample from reservoir
                    float u = (float)rand() / RAND_MAX;
                    float weight_threshold = u * reservoir->weight_sum;
                    
                    float accumulated_weight = 0.0f;
                    for (uint32_t i = 0; i < reservoir->sample_count; i++) {
                        accumulated_weight += reservoir->weights[i];
                        if (accumulated_weight >= weight_threshold) {
                            // Return selected sample
                            return reservoir->samples[i].radiance;
                        }
                    }
                    
                    // Fallback to first sample
                    return reservoir->samples[0].radiance;
                }
            }
            
            unmap_buffer(g_gi_system.restir.reservoir_buffer);
            return (float3){0.0f, 0.0f, 0.0f};
            
        case GI_TYPE_HYBRID:
            // Combine DDGI and ReSTIR
            float3 ddgi_contribution = sample_ddgi_irradiance(&g_gi_system, world_pos, normal);
            
            // Add ReSTIR contribution
            Reservoir *reservoirs = map_buffer(g_gi_system.restir.reservoir_buffer);
            uint32_t pixel_index = (uint32_t)(world_pos.x * g_gi_system.gi_resolution) + 
                                 (uint32_t)(world_pos.z * g_gi_system.gi_resolution);
            
            float3 restir_contribution = {0.0f, 0.0f, 0.0f};
            if (pixel_index < g_gi_system.restir.reservoir_size) {
                Reservoir *reservoir = &reservoirs[pixel_index];
                if (reservoir->sample_count > 0) {
                    // Weighted blend of samples
                    for (uint32_t i = 0; i < reservoir->sample_count; i++) {
                        restir_contribution.x += reservoir->samples[i].radiance.x * reservoir->weights[i];
                        restir_contribution.y += reservoir->samples[i].radiance.y * reservoir->weights[i];
                        restir_contribution.z += reservoir->samples[i].radiance.z * reservoir->weights[i];
                    }
                    
                    float weight_sum = reservoir->weight_sum > 0.0f ? reservoir->weight_sum : 1.0f;
                    restir_contribution.x /= weight_sum;
                    restir_contribution.y /= weight_sum;
                    restir_contribution.z /= weight_sum;
                }
            }
            
            unmap_buffer(g_gi_system.restir.reservoir_buffer);
            
            // Blend contributions
            float3 final_result = {
                ddgi_contribution.x * 0.7f + restir_contribution.x * 0.3f,
                ddgi_contribution.y * 0.7f + restir_contribution.y * 0.3f,
                ddgi_contribution.z * 0.7f + restir_contribution.z * 0.3f
            };
            
            return final_result;
            
        default:
            return (float3){0.0f, 0.0f, 0.0f};
    }
}

void gi_system_set_intensity(float intensity) {
    if (!g_gi_system.initialized) return;
    
    g_gi_system.gi_intensity = intensity;
    LOG_DEBUG("GI intensity set to %.2f", intensity);
}

void gi_system_set_update_rate(float update_rate) {
    if (!g_gi_system.initialized) return;
    
    g_gi_system.gi_update_rate = update_rate;
    LOG_DEBUG("GI update rate set to %.2f Hz", update_rate);
}

void gi_system_enable_shadows(bool enable) {
    if (!g_gi_system.initialized) return;
    
    g_gi_system.enable_shadows = enable;
    LOG_DEBUG("GI shadows %s", enable ? "enabled" : "disabled");
}

void gi_system_set_max_bounces(uint32_t max_bounces) {
    if (!g_gi_system.initialized) return;
    
    g_gi_system.max_bounces = max_bounces;
    LOG_DEBUG("GI max bounces set to %u", max_bounces);
}

void gi_system_get_stats(float *gi_time, uint32_t *voxels_updated, uint32_t *rays_traced, uint32_t *samples_generated) {
    if (!g_gi_system.initialized) return;
    
    if (gi_time) *gi_time = g_gi_system.gi_time_ms;
    if (voxels_updated) *voxels_updated = g_gi_system.voxels_updated;
    if (rays_traced) *rays_traced = g_gi_system.rays_traced;
    if (samples_generated) *samples_generated = g_gi_system.samples_generated;
}

bool gi_system_is_initialized(void) {
    return g_gi_system.initialized;
}

GIType gi_system_get_type(void) {
    return g_gi_system.type;
}
