// src/engine/core/profiling/gpu_profiler.c
//
// Purpose: GPU timeline profiling system
// Provides comprehensive GPU performance analysis with timeline visualization

#include "core/profiling/gpu_profiler.h"
#include "core/memory/unified_allocator.h"
#include "core/logging/unified_logger.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct GPUProfileEntry {
    char name[64];
    u64 timestamp_start;
    u64 timestamp_end;
    u32 frame_index;
    GPUProfileCategory category;
    f32 duration_ms;
} GPUProfileEntry;

typedef struct GPUProfilerContext {
    // Query pools for timestamp queries
    u32 timestamp_query_pool;
    u32 current_query_index;
    u32 max_queries_per_frame;
    
    // Profile entries storage
    GPUProfileEntry* entries;
    u32 entry_count;
    u32 entry_capacity;
    
    // Frame tracking
    u32 current_frame;
    u32 frame_delay;  // Number of frames to wait for query results
    
    // Statistics
    f32 total_frame_time_ms;
    f32 category_times_ms[GPU_PROFILE_CATEGORY_COUNT];
    u32 category_counts[GPU_PROFILE_CATEGORY_COUNT];
    
    // Buffer for reading back timestamps
    u64* timestamp_buffer;
    
    bool initialized;
    bool capturing;
} GPUProfilerContext;

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

static GPUProfilerContext* g_gpu_profiler = NULL;

// ============================================================================
// CATEGORY NAMES
// ============================================================================

static const char* category_names[GPU_PROFILE_CATEGORY_COUNT] = {
    "Rendering",
    "Compute",
    "Transfer",
    "PostProcess",
    "Shadows",
    "Geometry",
    "Animation",
    "Physics",
    "Audio",
    "Other"
};

// ============================================================================
// CREATION AND DESTRUCTION
// ============================================================================

GPUProfilerContext* gpu_profiler_create(u32 max_queries_per_frame, u32 frame_delay) {
    LOG_INFO(LOG_CAT_PROFILING, "Creating GPU profiler (queries: %u, delay: %u)", 
             max_queries_per_frame, frame_delay);
    
    GPUProfilerContext* ctx = MALLOC_PERSISTENT(sizeof(GPUProfilerContext));
    if (!ctx) {
        LOG_ERROR(LOG_CAT_PROFILING, "Failed to allocate GPU profiler context");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(GPUProfilerContext));
    
    ctx->max_queries_per_frame = max_queries_per_frame;
    ctx->frame_delay = frame_delay;
    ctx->entry_capacity = max_queries_per_frame * 10; // Store multiple frames
    
    // Allocate entries storage
    ctx->entries = MALLOC_PERSISTENT(ctx->entry_capacity * sizeof(GPUProfileEntry));
    if (!ctx->entries) {
        LOG_ERROR(LOG_CAT_PROFILING, "Failed to allocate GPU profiler entries");
        FREE(ctx);
        return NULL;
    }
    
    // Allocate timestamp buffer
    ctx->timestamp_buffer = MALLOC_TEMP(max_queries_per_frame * 2 * sizeof(u64));
    if (!ctx->timestamp_buffer) {
        LOG_ERROR(LOG_CAT_PROFILING, "Failed to allocate timestamp buffer");
        FREE(ctx->entries);
        FREE(ctx);
        return NULL;
    }
    
    // Create timestamp query pool
    ctx->timestamp_query_pool = gpu_create_timestamp_query_pool(max_queries_per_frame * 2);
    if (!ctx->timestamp_query_pool) {
        LOG_ERROR(LOG_CAT_PROFILING, "Failed to create timestamp query pool");
        FREE(ctx->entries);
        FREE(ctx->timestamp_buffer);
        FREE(ctx);
        return NULL;
    }
    
    ctx->initialized = true;
    ctx->capturing = false;
    
    LOG_INFO(LOG_CAT_PROFILING, "GPU profiler created successfully");
    return ctx;
}

void gpu_profiler_destroy(GPUProfilerContext* ctx) {
    if (!ctx) return;
    
    LOG_INFO(LOG_CAT_PROFILING, "Destroying GPU profiler");
    
    if (ctx->timestamp_query_pool) {
        gpu_destroy_timestamp_query_pool(ctx->timestamp_query_pool);
    }
    
    if (ctx->entries) {
        FREE(ctx->entries);
    }
    
    if (ctx->timestamp_buffer) {
        FREE(ctx->timestamp_buffer);
    }
    
    FREE(ctx);
}

// ============================================================================
// GLOBAL PROFILER MANAGEMENT
// ============================================================================

bool gpu_profiler_initialize(u32 max_queries_per_frame, u32 frame_delay) {
    if (g_gpu_profiler) {
        LOG_WARN(LOG_CAT_PROFILING, "GPU profiler already initialized");
        return true;
    }
    
    g_gpu_profiler = gpu_profiler_create(max_queries_per_frame, frame_delay);
    return g_gpu_profiler != NULL;
}

void gpu_profiler_shutdown(void) {
    if (g_gpu_profiler) {
        gpu_profiler_destroy(g_gpu_profiler);
        g_gpu_profiler = NULL;
    }
}

GPUProfilerContext* gpu_profiler_get_global(void) {
    return g_gpu_profiler;
}

// ============================================================================
// PROFILING CONTROL
// ============================================================================

void gpu_profiler_begin_capture(void) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized) {
        LOG_WARN(LOG_CAT_PROFILING, "GPU profiler not initialized");
        return;
    }
    
    g_gpu_profiler->capturing = true;
    g_gpu_profiler->current_query_index = 0;
    
    // Reset frame statistics
    g_gpu_profiler->total_frame_time_ms = 0.0f;
    memset(g_gpu_profiler->category_times_ms, 0, sizeof(g_gpu_profiler->category_times_ms));
    memset(g_gpu_profiler->category_counts, 0, sizeof(g_gpu_profiler->category_counts));
    
    LOG_INFO(LOG_CAT_PROFILING, "GPU profiler capture started");
}

void gpu_profiler_end_capture(void) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized) {
        LOG_WARN(LOG_CAT_PROFILING, "GPU profiler not initialized");
        return;
    }
    
    g_gpu_profiler->capturing = false;
    
    // Read back timestamp results
    if (g_gpu_profiler->current_query_index > 0) {
        gpu_get_timestamp_query_results(g_gpu_profiler->timestamp_query_pool,
                                       0, g_gpu_profiler->current_query_index,
                                       g_gpu_profiler->timestamp_buffer);
        
        // Process timestamp pairs
        for (u32 i = 0; i < g_gpu_profiler->current_query_index / 2; i++) {
            u64 start = g_gpu_profiler->timestamp_buffer[i * 2];
            u64 end = g_gpu_profiler->timestamp_buffer[i * 2 + 1];
            
            if (start < end) {
                // Find corresponding entry
                for (u32 j = 0; j < g_gpu_profiler->entry_count; j++) {
                    GPUProfileEntry* entry = &g_gpu_profiler->entries[j];
                    if (entry->frame_index == g_gpu_profiler->current_frame - g_gpu_profiler->frame_delay &&
                        entry->timestamp_start == 0) {
                        entry->timestamp_start = start;
                        entry->timestamp_end = end;
                        entry->duration_ms = (f32)(end - start) / 1000000.0f; // Convert to ms
                        
                        // Update statistics
                        g_gpu_profiler->category_times_ms[entry->category] += entry->duration_ms;
                        g_gpu_profiler->category_counts[entry->category]++;
                        g_gpu_profiler->total_frame_time_ms += entry->duration_ms;
                        break;
                    }
                }
            }
        }
    }
    
    LOG_INFO(LOG_CAT_PROFILING, "GPU profiler capture ended");
}

// ============================================================================
// TIMELINE PROFILING
// ============================================================================

GPUProfileHandle gpu_profiler_begin_event(const char* name, GPUProfileCategory category) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized || !g_gpu_profiler->capturing) {
        return 0;
    }
    
    if (g_gpu_profiler->current_query_index >= g_gpu_profiler->max_queries_per_frame * 2) {
        LOG_WARN(LOG_CAT_PROFILING, "GPU profiler query limit reached");
        return 0;
    }
    
    // Create profile entry
    if (g_gpu_profiler->entry_count < g_gpu_profiler->entry_capacity) {
        GPUProfileEntry* entry = &g_gpu_profiler->entries[g_gpu_profiler->entry_count];
        strncpy(entry->name, name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        entry->timestamp_start = 0; // Will be filled when results are available
        entry->timestamp_end = 0;
        entry->frame_index = g_gpu_profiler->current_frame;
        entry->category = category;
        entry->duration_ms = 0.0f;
        
        g_gpu_profiler->entry_count++;
    }
    
    // Issue timestamp query
    u32 query_index = g_gpu_profiler->current_query_index++;
    gpu_timestamp_query(g_gpu_profiler->timestamp_query_pool, query_index);
    
    return query_index + 1; // Return non-zero handle
}

void gpu_profiler_end_event(GPUProfileHandle handle) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized || !g_gpu_profiler->capturing || handle == 0) {
        return;
    }
    
    if (g_gpu_profiler->current_query_index >= g_gpu_profiler->max_queries_per_frame * 2) {
        return;
    }
    
    // Issue timestamp query
    u32 query_index = g_gpu_profiler->current_query_index++;
    gpu_timestamp_query(g_gpu_profiler->timestamp_query_pool, query_index);
}

// ============================================================================
// FRAME MANAGEMENT
// ============================================================================

void gpu_profiler_begin_frame(void) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized) {
        return;
    }
    
    g_gpu_profiler->current_frame++;
    g_gpu_profiler->current_query_index = 0;
    
    // Remove old entries (older than frame_delay)
    u32 min_frame = g_gpu_profiler->current_frame - g_gpu_profiler->frame_delay - 1;
    u32 write_index = 0;
    
    for (u32 i = 0; i < g_gpu_profiler->entry_count; i++) {
        if (g_gpu_profiler->entries[i].frame_index >= min_frame) {
            if (write_index != i) {
                g_gpu_profiler->entries[write_index] = g_gpu_profiler->entries[i];
            }
            write_index++;
        }
    }
    
    g_gpu_profiler->entry_count = write_index;
}

void gpu_profiler_end_frame(void) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized) {
        return;
    }
    
    // Process any pending query results
    if (g_gpu_profiler->current_frame > g_gpu_profiler->frame_delay) {
        gpu_profiler_end_capture();
        gpu_profiler_begin_capture();
    }
}

// ============================================================================
// STATISTICS AND REPORTING
// ============================================================================

void gpu_profiler_get_statistics(GPUProfilerStats* out_stats) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized || !out_stats) {
        return;
    }
    
    out_stats->total_frame_time_ms = g_gpu_profiler->total_frame_time_ms;
    out_stats->current_frame = g_gpu_profiler->current_frame;
    out_stats->entries_count = g_gpu_profiler->entry_count;
    
    for (int i = 0; i < GPU_PROFILE_CATEGORY_COUNT; i++) {
        out_stats->category_times_ms[i] = g_gpu_profiler->category_times_ms[i];
        out_stats->category_counts[i] = g_gpu_profiler->category_counts[i];
    }
}

void gpu_profiler_print_summary(void) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized) {
        LOG_WARN(LOG_CAT_PROFILING, "GPU profiler not initialized");
        return;
    }
    
    LOG_INFO(LOG_CAT_PROFILING, "=== GPU Profiler Summary ===");
    LOG_INFO(LOG_CAT_PROFILING, "Frame: %u, Total Time: %.2f ms", 
             g_gpu_profiler->current_frame, g_gpu_profiler->total_frame_time_ms);
    
    for (int i = 0; i < GPU_PROFILE_CATEGORY_COUNT; i++) {
        if (g_gpu_profiler->category_counts[i] > 0) {
            f32 avg_time = g_gpu_profiler->category_times_ms[i] / g_gpu_profiler->category_counts[i];
            LOG_INFO(LOG_CAT_PROFILING, "%s: %.2f ms total, %.2f ms avg (%u calls)",
                     category_names[i], g_gpu_profiler->category_times_ms[i], avg_time,
                     g_gpu_profiler->category_counts[i]);
        }
    }
    
    LOG_INFO(LOG_CAT_PROFILING, "=== End Summary ===");
}

void gpu_profiler_print_timeline(void) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized) {
        LOG_WARN(LOG_CAT_PROFILING, "GPU profiler not initialized");
        return;
    }
    
    LOG_INFO(LOG_CAT_PROFILING, "=== GPU Timeline ===");
    
    for (u32 i = 0; i < g_gpu_profiler->entry_count; i++) {
        GPUProfileEntry* entry = &g_gpu_profiler->entries[i];
        if (entry->duration_ms > 0.0f) {
            LOG_INFO(LOG_CAT_PROFILING, "[%s] %s: %.2f ms (frame %u)",
                     category_names[entry->category], entry->name, entry->duration_ms,
                     entry->frame_index);
        }
    }
    
    LOG_INFO(LOG_CAT_PROFILING, "=== End Timeline ===");
}

// ============================================================================
// EXPORT FUNCTIONS
// ============================================================================

void gpu_profiler_export_csv(const char* filename) {
    if (!g_gpu_profiler || !g_gpu_profiler->initialized || !filename) {
        return;
    }
    
    // In a real implementation, would write to file
    LOG_INFO(LOG_CAT_PROFILING, "Exporting GPU profiler data to CSV: %s", filename);
    
    // Export header
    // Frame,Category,Name,Duration_ms
    
    // Export entries
    for (u32 i = 0; i < g_gpu_profiler->entry_count; i++) {
        GPUProfileEntry* entry = &g_gpu_profiler->entries[i];
        if (entry->duration_ms > 0.0f) {
            // LOG_INFO(LOG_CAT_PROFILING, "%u,%s,%s,%.2f", 
            //          entry->frame_index, category_names[entry->category], 
            //          entry->name, entry->duration_ms);
        }
    }
}
