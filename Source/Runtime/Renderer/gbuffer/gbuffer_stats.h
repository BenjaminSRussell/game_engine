/**
 * =================================================================================================
 *                              G-BUFFER PERFORMANCE STATISTICS
 * =================================================================================================
 *
 * Purpose: Provides metrics and utilities for monitoring G-Buffer performance.
 * Tracks memory usage, bandwidth, and rendering overhead.
 */

#pragma once

#include <core/types.h>

/**
 * Statistics structure for G-Buffer performance
 */
typedef struct {
    // Memory usage in bytes
    u64 total_memory_bytes;
    u64 albedo_memory_bytes;
    u64 normal_memory_bytes;
    u64 material_memory_bytes;
    u64 emissive_memory_bytes;
    u64 velocity_memory_bytes;
    u64 depth_memory_bytes;

    // Bandwidth estimation (MB/frame for a full-screen pass)
    f32 estimated_write_bandwidth_mb;
    f32 estimated_read_bandwidth_mb;

    // Rendering metrics
    u32 draw_call_count;
    u32 vertex_count;
    u32 triangle_count;

    // Optimization flags
    bool early_z_enabled;
    bool normal_encoding_active;
} GBufferStats;

/**
 * Calculates current G-Buffer statistics based on dimensions and formats.
 * 
 * @param width Viewport width
 * @param height Viewport height
 * @return Calculated statistics
 */
GBufferStats gbuffer_calculate_stats(u32 width, u32 height);

/**
 * Logs a summary of G-Buffer performance statistics.
 * 
 * @param stats Statistics to log
 */
void gbuffer_log_stats(GBufferStats stats);

/**
 * Resets the transient metrics (draw calls, counts) for a new frame.
 * 
 * @param stats Stats structure to reset
 */
void gbuffer_stats_reset_frame(GBufferStats *stats);
