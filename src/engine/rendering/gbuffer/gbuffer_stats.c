/**
 * =================================================================================================
 *                              G-BUFFER STATISTICS IMPLEMENTATION
 * =================================================================================================
 */

#include "gbuffer_stats.h"
#include <core/logger/logger.h>

GBufferStats gbuffer_calculate_stats(u32 width, u32 height) {
    GBufferStats stats = {0};
    
    u64 pixel_count = (u64)width * height;
    
    // Albedo: RGBA8 (4 bytes)
    stats.albedo_memory_bytes = pixel_count * 4;
    
    // Normal: RG16F (4 bytes - 2x 16-bit float)
    stats.normal_memory_bytes = pixel_count * 4;
    
    // Material: RGBA8 (4 bytes)
    stats.material_memory_bytes = pixel_count * 4;
    
    // Emissive: R11G11B10F (4 bytes)
    stats.emissive_memory_bytes = pixel_count * 4;
    
    // Velocity: RG16F (4 bytes)
    stats.velocity_memory_bytes = pixel_count * 4;
    
    // Depth: D32F (4 bytes)
    stats.depth_memory_bytes = pixel_count * 4;
    
    stats.total_memory_bytes = stats.albedo_memory_bytes + 
                              stats.normal_memory_bytes + 
                              stats.material_memory_bytes + 
                              stats.emissive_memory_bytes + 
                              stats.velocity_memory_bytes + 
                              stats.depth_memory_bytes;
                              
    // Bandwidth: Sum of all writes (MB)
    stats.estimated_write_bandwidth_mb = (f32)stats.total_memory_bytes / (1024.0f * 1024.0f);
    
    // Read bandwidth for lighting pass: 
    // Albedo, Normal, Material, Depth, Emissive (5 textures)
    stats.estimated_read_bandwidth_mb = stats.estimated_write_bandwidth_mb - ((f32)stats.velocity_memory_bytes / (1024.0f * 1024.0f));
    
    stats.early_z_enabled = true; // Assumed default
    stats.normal_encoding_active = true;
    
    return stats;
}

void gbuffer_log_stats(GBufferStats stats) {
    LOG_INFO("--- G-Buffer Performance Summary ---");
    LOG_INFO("Resolution: %llu pixels", stats.total_memory_bytes / (4 * 6)); // Rough estimate back to pixel count
    LOG_INFO("Total VRAM: %.2f MB", (f32)stats.total_memory_bytes / (1024.0f * 1024.0f));
    LOG_INFO("Estimated Write BW: %.2f MB/frame", stats.estimated_write_bandwidth_mb);
    LOG_INFO("Estimated Read BW: %.2f MB/frame", stats.estimated_read_bandwidth_mb);
    LOG_INFO("Normal Encoding: %s", stats.normal_encoding_active ? "ENABLED (Octahedral)" : "DISABLED");
    LOG_INFO("Early-Z: %s", stats.early_z_enabled ? "ENABLED" : "DISABLED");
    LOG_INFO("-------------------------------------");
}

void gbuffer_stats_reset_frame(GBufferStats *stats) {
    if (!stats) return;
    stats->draw_call_count = 0;
    stats->vertex_count = 0;
    stats->triangle_count = 0;
}
