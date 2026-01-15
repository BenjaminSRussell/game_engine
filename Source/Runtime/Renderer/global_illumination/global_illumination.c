// src/engine/rendering/global_illumination/global_illumination.c
// Global Illumination System - Real-time GI with DDGI and ReSTIR

#include "engine/include/core/logger.h"
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
        gi->ddgi.voxels = NULL;
        free(gi->ddgi.probe_positions);
        gi->ddgi.probe_positions = NULL;
        return;
    }
    
    LOG_INFO("Initialized DDGI: %ux%ux%u voxels, %u probes", 
             voxel_count_x, voxel_count_y, voxel_count_z, gi->ddgi.probe_count);
}

static void update_ddgi_probes(GISystem *gi, const float *camera_pos, const float *view_matrix) {
    if (!gi->ddgi.voxels || !gi->ddgi.probe_positions) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Update DDGI probes based on camera movement
    // Implement actual DDGI update algorithm
    // Trace rays from each probe position and gather radiance
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
            // Trace rays from this probe
            float3 total_irradiance = {0.0f, 0.0f, 0.0f};
            float total_distance = 0.0f;
            uint32_t valid_samples = 0;
            
            for (uint32_t ray = 0; ray < gi->ddgi.rays_per_probe; ray++) {
                // Generate cosine-weighted hemisphere sample
                float u = (float)rand() / RAND_MAX;
                float v = (float)rand() / RAND_MAX;
                float theta = 2.0f * M_PI * u;
                float phi = acosf(2.0f * v - 1.0f);
                
                float3 ray_dir = {
                    sinf(phi) * cosf(theta),
                    sinf(phi) * sinf(theta),
                    cosf(phi)
                };
                
                // Trace ray and get radiance
                float3 hit_point = {
                    probe_pos->x + ray_dir.x * gi->ddgi.max_ray_distance,
                    probe_pos->y + ray_dir.y * gi->ddgi.max_ray_distance,
                    probe_pos->z + ray_dir.z * gi->ddgi.max_ray_distance
                };
                
                // Simple radiance calculation (would use actual ray tracing in production)
                float3 radiance = {
                    fmaxf(0.0f, ray_dir.y) * 0.8f + 0.2f, // Simulate sky lighting
                    fmaxf(0.0f, ray_dir.y) * 0.9f + 0.1f,
                    fmaxf(0.0f, ray_dir.y) * 1.0f + 0.0f
                };
                
                total_irradiance.x += radiance.x;
                total_irradiance.y += radiance.y;
                total_irradiance.z += radiance.z;
                total_distance += gi->ddgi.max_ray_distance;
                valid_samples++;
            }
            
            if (valid_samples > 0) {
                // Average the samples
                total_irradiance.x /= valid_samples;
                total_irradiance.y /= valid_samples;
                total_irradiance.z /= valid_samples;
                total_distance /= valid_samples;
                
                // Update nearby voxels with probe data
                int probe_voxel_x = (int)((probe_pos->x - gi->ddgi.world_origin.x) / gi->ddgi.voxel_size);
                int probe_voxel_y = (int)((probe_pos->y - gi->ddgi.world_origin.y) / gi->ddgi.voxel_size);
                int probe_voxel_z = (int)((probe_pos->z - gi->ddgi.world_origin.z) / gi->ddgi.voxel_size);
                
                // Update voxels in a 3x3x3 neighborhood around the probe
                for (int dz = -1; dz <= 1; dz++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int vx = probe_voxel_x + dx;
                            int vy = probe_voxel_y + dy;
                            int vz = probe_voxel_z + dz;
                            
                            if (vx >= 0 && vx < gi->ddgi.voxel_count_x &&
                                vy >= 0 && vy < gi->ddgi.voxel_count_y &&
                                vz >= 0 && vz < gi->ddgi.voxel_count_z) {
                                
                                uint32_t voxel_index = (vz * gi->ddgi.voxel_count_y + vy) * gi->ddgi.voxel_count_x + vx;
                                GIVoxel *voxel = &gi->ddgi.voxels[voxel_index];
                                
                                // Calculate falloff based on distance to probe
                                float3 voxel_pos = {
                                    gi->ddgi.world_origin.x + vx * gi->ddgi.voxel_size,
                                    gi->ddgi.world_origin.y + vy * gi->ddgi.voxel_size,
                                    gi->ddgi.world_origin.z + vz * gi->ddgi.voxel_size
                                };
                                
                                float3 to_voxel = {
                                    voxel_pos.x - probe_pos->x,
                                    voxel_pos.y - probe_pos->y,
                                    voxel_pos.z - probe_pos->z
                                };
                                float voxel_distance = sqrtf(to_voxel.x * to_voxel.x + to_voxel.y * to_voxel.y + to_voxel.z * to_voxel.z);
                                float falloff = fmaxf(0.0f, 1.0f - voxel_distance / (gi->ddgi.voxel_size * 2.0f));
                                
                                // Apply temporal accumulation
                                float alpha = gi->ddgi.temporal_alpha * falloff;
                                voxel->indirect_lighting[0] = voxel->indirect_lighting[0] * (1.0f - alpha) + total_irradiance.x * alpha;
                                voxel->indirect_lighting[1] = voxel->indirect_lighting[1] * (1.0f - alpha) + total_irradiance.y * alpha;
                                voxel->indirect_lighting[2] = voxel->indirect_lighting[2] * (1.0f - alpha) + total_irradiance.z * alpha;
                                voxel->distance = voxel->distance * (1.0f - alpha) + total_distance * alpha;
                                voxel->confidence = fminf(1.0f, voxel->confidence + alpha * 0.1f);
                            }
                        }
                    }
                }
            }
            
            probes_updated++;
            rays_traced += gi->ddgi.rays_per_probe;
        }
    }
    
    uint32_t probes_updated = 0;
    uint32_t rays_traced = 0;
    
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
            // Update this probe
            // TODO: Trace rays and update voxel data
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
    
    // Create importance buffer
    gi->restir.importance_buffer = create_buffer(reservoir_size * sizeof(float));
    if (!gi->restir.importance_buffer) {
        LOG_ERROR("Failed to create ReSTIR importance buffer");
        destroy_buffer(gi->restir.reservoir_buffer);
        gi->restir.reservoir_buffer = NULL;
        return;
    }
    
    // Create history buffer
    gi->restir.history_buffer = create_buffer(reservoir_size * temporal_frames * sizeof(float4));
    if (!gi->restir.history_buffer) {
        LOG_ERROR("Failed to create ReSTIR history buffer");
        destroy_buffer(gi->restir.reservoir_buffer);
        destroy_buffer(gi->restir.importance_buffer);
        gi->restir.reservoir_buffer = NULL;
        gi->restir.importance_buffer = NULL;
        return;
    }
    
    LOG_INFO("Initialized ReSTIR: %u reservoirs, %u temporal frames", reservoir_size, temporal_frames);
}

static void update_restir(GISystem *gi, const float *camera_pos, const float *view_matrix) {
    if (!gi->restir.reservoir_buffer) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Implement ReSTIR update algorithm
    // Generate importance samples and update reservoirs
    Reservoir *reservoirs = (Reservoir*)gi->restir.reservoir_buffer;
    float *importance = (float*)gi->restir.importance_buffer;
    float4 *history = (float4*)gi->restir.history_buffer;
    
    if (!reservoirs || !importance || !history) return;
    
    for (uint32_t i = 0; i < gi->restir.reservoir_size; i++) {
        // Generate new candidate sample
        float u1 = (float)rand() / RAND_MAX;
        float u2 = (float)rand() / RAND_MAX;
        float u3 = (float)rand() / RAND_MAX;
        
        // Create sample with position, direction, and radiance
        float3 sample_pos = {
            (u1 - 0.5f) * 100.0f,
            (u2 - 0.5f) * 100.0f,
            (u3 - 0.5f) * 100.0f
        };
        
        float3 sample_dir = {
            (float)rand() / RAND_MAX - 0.5f,
            (float)rand() / RAND_MAX - 0.5f,
            (float)rand() / RAND_MAX - 0.5f
        };
        
        // Normalize direction
        float dir_length = sqrtf(sample_dir.x * sample_dir.x + sample_dir.y * sample_dir.y + sample_dir.z * sample_dir.z);
        if (dir_length > 0.0f) {
            sample_dir.x /= dir_length;
            sample_dir.y /= dir_length;
            sample_dir.z /= dir_length;
        }
        
        float3 sample_radiance = {
            (float)rand() / RAND_MAX,
            (float)rand() / RAND_MAX,
            (float)rand() / RAND_MAX
        };
        
        // Calculate target function (simplified importance sampling)
        float target_function = sample_radiance.x + sample_radiance.y + sample_radiance.z;
        
        // Update reservoir with new sample
        float weight = target_function * gi->restir.reservoir_size;
        reservoirs[i].weight_sum += weight;
        
        if ((float)rand() / RAND_MAX < weight / reservoirs[i].weight_sum) {
            reservoirs[i].sample_pos = sample_pos;
            reservoirs[i].sample_dir = sample_dir;
            reservoirs[i].sample_radiance = sample_radiance;
            reservoirs[i].target_function = target_function;
        }
        
        // Update importance based on target function
        importance[i] = target_function;
        
        // Temporal reuse: blend with history
        uint32_t history_index = (gi->restir.temporal_frames * i + gi->restir.history_frames) % (gi->restir.reservoir_size * gi->restir.temporal_frames);
        float4 history_sample = history[history_index];
        
        // Blend current sample with history
        float temporal_blend = gi->restir.temporal_bias;
        reservoirs[i].sample_radiance.x = reservoirs[i].sample_radiance.x * (1.0f - temporal_blend) + history_sample.x * temporal_blend;
        reservoirs[i].sample_radiance.y = reservoirs[i].sample_radiance.y * (1.0f - temporal_blend) + history_sample.y * temporal_blend;
        reservoirs[i].sample_radiance.z = reservoirs[i].sample_radiance.z * (1.0f - temporal_blend) + history_sample.z * temporal_blend;
        
        // Update history
        history[history_index] = (float4){
            reservoirs[i].sample_radiance.x,
            reservoirs[i].sample_radiance.y,
            reservoirs[i].sample_radiance.z,
            reservoirs[i].target_function
        };
        
        // Spatial resampling (simplified)
        if (gi->restir.spatial_radius > 0.0f && i > 0) {
            // Check neighboring reservoirs
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int neighbor_index = i + dx + dy * (int)sqrtf(gi->restir.reservoir_size);
                    if (neighbor_index >= 0 && neighbor_index < gi->restir.reservoir_size && neighbor_index != i) {
                        // Calculate distance between samples
                        float3 neighbor_to_current = {
                            reservoirs[i].sample_pos.x - reservoirs[neighbor_index].sample_pos.x,
                            reservoirs[i].sample_pos.y - reservoirs[neighbor_index].sample_pos.y,
                            reservoirs[i].sample_pos.z - reservoirs[neighbor_index].sample_pos.z
                        };
                        float spatial_distance = sqrtf(neighbor_to_current.x * neighbor_to_current.x + 
                                                      neighbor_to_current.y * neighbor_to_current.y + 
                                                      neighbor_to_current.z * neighbor_to_current.z);
                        
                        if (spatial_distance < gi->restir.spatial_radius) {
                            // Spatial resampling with distance falloff
                            float spatial_weight = fmaxf(0.0f, 1.0f - spatial_distance / gi->restir.spatial_radius);
                            float neighbor_weight = reservoirs[neighbor_index].target_function * spatial_weight;
                            
                            if ((float)rand() / RAND_MAX < neighbor_weight / (reservoirs[i].weight_sum + neighbor_weight)) {
                                reservoirs[i].sample_pos = reservoirs[neighbor_index].sample_pos;
                                reservoirs[i].sample_dir = reservoirs[neighbor_index].sample_dir;
                                reservoirs[i].sample_radiance = reservoirs[neighbor_index].sample_radiance;
                                reservoirs[i].target_function = reservoirs[neighbor_index].target_function;
                                reservoirs[i].weight_sum += neighbor_weight;
                            }
                        }
                    }
                }
            }
        }
    }
    
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
    g_gi_system.gi_compute_shader = load_compute_shader("gi_compute");
    if (!g_gi_system.gi_compute_shader) {
        LOG_ERROR("Failed to load GI compute shader");
        // Continue without compute shader for now
    }
    
    // Create GI render target
    g_gi_system.gi_render_target = create_render_target(resolution, resolution, FORMAT_RGBA16F);
    if (!g_gi_system.gi_render_target) {
        LOG_ERROR("Failed to create GI render target");
        // Continue without render target for now
    }
    
    // Create GI buffer
    g_gi_system.gi_buffer = create_buffer(sizeof(GIData));
    if (!g_gi_system.gi_buffer) {
        LOG_ERROR("Failed to create GI buffer");
        // Continue without buffer for now
    }
    
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
    
    // Destroy buffers
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
    
    // Destroy GPU resources
    if (g_gi_system.gi_compute_shader) {
        destroy_shader(g_gi_system.gi_compute_shader);
        g_gi_system.gi_compute_shader = NULL;
    }
    if (g_gi_system.gi_render_target) {
        destroy_render_target(g_gi_system.gi_render_target);
        g_gi_system.gi_render_target = NULL;
    }
    if (g_gi_system.gi_buffer) {
        destroy_buffer(g_gi_system.gi_buffer);
        g_gi_system.gi_buffer = NULL;
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
            
        case GI_TYPE_RESTIR:
            // Sample ReSTIR
            if (g_gi_system.restir.reservoir_buffer) {
                Reservoir *reservoirs = (Reservoir*)g_gi_system.restir.reservoir_buffer;
                
                // Find nearest reservoirs to sample position
                float3 total_radiance = {0.0f, 0.0f, 0.0f};
                float total_weight = 0.0f;
                uint32_t samples_used = 0;
                
                for (uint32_t i = 0; i < g_gi_system.restir.reservoir_size && samples_used < 8; i++) {
                    if (reservoirs[i].weight_sum > 0.0f) {
                        // Calculate distance from sample position to reservoir sample
                        float3 to_sample = {
                            world_pos.x - reservoirs[i].sample_pos.x,
                            world_pos.y - reservoirs[i].sample_pos.y,
                            world_pos.z - reservoirs[i].sample_pos.z
                        };
                        float distance = sqrtf(to_sample.x * to_sample.x + to_sample.y * to_sample.y + to_sample.z * to_sample.z);
                        
                        // Only use samples within reasonable distance
                        if (distance < 50.0f) {
                            // Calculate visibility based on normal
                            float dot_product = normal.x * reservoirs[i].sample_dir.x + 
                                              normal.y * reservoirs[i].sample_dir.y + 
                                              normal.z * reservoirs[i].sample_dir.z;
                            float visibility = fmaxf(0.0f, dot_product);
                            
                            // Weight by distance and visibility
                            float weight = visibility * fmaxf(0.0f, 1.0f - distance / 50.0f);
                            
                            total_radiance.x += reservoirs[i].sample_radiance.x * weight;
                            total_radiance.y += reservoirs[i].sample_radiance.y * weight;
                            total_radiance.z += reservoirs[i].sample_radiance.z * weight;
                            total_weight += weight;
                            samples_used++;
                        }
                    }
                }
                
                if (total_weight > 0.0f) {
                    total_radiance.x /= total_weight;
                    total_radiance.y /= total_weight;
                    total_radiance.z /= total_weight;
                }
                
                return total_radiance * g_gi_system.gi_intensity;
            }
            return (float3){0.0f, 0.0f, 0.0f};
            
        case GI_TYPE_HYBRID:
            // Combine DDGI and ReSTIR
            float3 ddgi_contribution = sample_ddgi_irradiance(&g_gi_system, world_pos, normal);
            float3 restir_contribution = {0.0f, 0.0f, 0.0f};
            
            // Sample ReSTIR
            if (g_gi_system.restir.reservoir_buffer) {
                Reservoir *reservoirs = (Reservoir*)g_gi_system.restir.reservoir_buffer;
                
                float3 total_radiance = {0.0f, 0.0f, 0.0f};
                float total_weight = 0.0f;
                uint32_t samples_used = 0;
                
                for (uint32_t i = 0; i < g_gi_system.restir.reservoir_size && samples_used < 4; i++) {
                    if (reservoirs[i].weight_sum > 0.0f) {
                        float3 to_sample = {
                            world_pos.x - reservoirs[i].sample_pos.x,
                            world_pos.y - reservoirs[i].sample_pos.y,
                            world_pos.z - reservoirs[i].sample_pos.z
                        };
                        float distance = sqrtf(to_sample.x * to_sample.x + to_sample.y * to_sample.y + to_sample.z * to_sample.z);
                        
                        if (distance < 30.0f) {
                            float dot_product = normal.x * reservoirs[i].sample_dir.x + 
                                              normal.y * reservoirs[i].sample_dir.y + 
                                              normal.z * reservoirs[i].sample_dir.z;
                            float visibility = fmaxf(0.0f, dot_product);
                            float weight = visibility * fmaxf(0.0f, 1.0f - distance / 30.0f);
                            
                            total_radiance.x += reservoirs[i].sample_radiance.x * weight;
                            total_radiance.y += reservoirs[i].sample_radiance.y * weight;
                            total_radiance.z += reservoirs[i].sample_radiance.z * weight;
                            total_weight += weight;
                            samples_used++;
                        }
                    }
                }
                
                if (total_weight > 0.0f) {
                    restir_contribution.x = total_radiance.x / total_weight;
                    restir_contribution.y = total_radiance.y / total_weight;
                    restir_contribution.z = total_radiance.z / total_weight;
                }
            }
            
            // Blend DDGI and ReSTIR contributions
            float blend_factor = 0.6f; // Favor DDGI for stability
            float3 final_contribution = {
                ddgi_contribution.x * blend_factor + restir_contribution.x * (1.0f - blend_factor),
                ddgi_contribution.y * blend_factor + restir_contribution.y * (1.0f - blend_factor),
                ddgi_contribution.z * blend_factor + restir_contribution.z * (1.0f - blend_factor)
            };
            
            return final_contribution * g_gi_system.gi_intensity;
            
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
