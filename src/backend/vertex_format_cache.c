#include "backend/vertex_format_cache.h"
#include <core/logger.h>
#include <string.h>

static VertexFormatCacheEntry s_cache[MAX_CACHED_FORMATS];
static u32 s_cache_count = 0;

void vertex_format_cache_init(void) {
    memset(s_cache, 0, sizeof(s_cache));
    s_cache_count = 0;
    LOG_INFO("Vertex format cache initialized");
}

void vertex_format_cache_shutdown(void) {
    LOG_INFO("Vertex format cache shutdown. Total formats: %u", s_cache_count);
    s_cache_count = 0;
}

const VertexFormat* vertex_format_cache_register(const VertexFormat* fmt) {
    if (!fmt) return NULL;
    
    // Check if already cached
    for (u32 i = 0; i < s_cache_count; i++) {
        if (s_cache[i].hash == fmt->hash) {
            // Found existing entry
            s_cache[i].usage_count++;
            return &s_cache[i].format;
        }
    }
    
    // Add new entry
    if (s_cache_count >= MAX_CACHED_FORMATS) {
        LOG_WARN("Vertex format cache full!");
        return fmt; // Return original instead of caching
    }
    
    VertexFormatCacheEntry* entry = &s_cache[s_cache_count++];
    memcpy(&entry->format, fmt, sizeof(VertexFormat));
    entry->hash = fmt->hash;
    entry->usage_count = 1;
    entry->vertex_count = 0;
    entry->memory_uncompressed = 0;
    entry->memory_compressed = 0;
    
    LOG_DEBUG("Cached new vertex format (hash: 0x%08X, stride: %u)", fmt->hash, fmt->stride);
    
    return &entry->format;
}

const VertexFormat* vertex_format_cache_find(u32 hash) {
    for (u32 i = 0; i < s_cache_count; i++) {
        if (s_cache[i].hash == hash) {
            return &s_cache[i].format;
        }
    }
    return NULL;
}

void vertex_format_cache_update_stats(u32 format_hash, u32 vertex_count, u32 uncompressed_size, u32 compressed_size) {
    for (u32 i = 0; i < s_cache_count; i++) {
        if (s_cache[i].hash == format_hash) {
            s_cache[i].vertex_count += vertex_count;
            s_cache[i].memory_uncompressed += (u64)vertex_count * uncompressed_size;
            s_cache[i].memory_compressed += (u64)vertex_count * compressed_size;
            return;
        }
    }
}

VertexFormatStats vertex_format_cache_get_stats(void) {
    VertexFormatStats stats = {0};
    stats.total_formats = s_cache_count;
    
    for (u32 i = 0; i < s_cache_count; i++) {
        stats.total_usage += s_cache[i].usage_count;
        stats.total_vertices += s_cache[i].vertex_count;
        stats.total_memory_saved += (s_cache[i].memory_uncompressed - s_cache[i].memory_compressed);
    }
    
    u64 total_uncompressed = 0;
    u64 total_compressed = 0;
    for (u32 i = 0; i < s_cache_count; i++) {
        total_uncompressed += s_cache[i].memory_uncompressed;
        total_compressed += s_cache[i].memory_compressed;
    }
    
    if (total_uncompressed > 0) {
        stats.avg_compression_ratio = (f32)total_compressed / (f32)total_uncompressed;
    }
    
    return stats;
}

void vertex_format_cache_print(void) {
    LOG_INFO("=== Vertex Format Cache ===");
    LOG_INFO("Total formats: %u", s_cache_count);
    
    for (u32 i = 0; i < s_cache_count; i++) {
        VertexFormatCacheEntry* entry = &s_cache[i];
        LOG_INFO("  [%u] Hash: 0x%08X, Stride: %u, Usage: %u, Vertices: %llu",
                 i, entry->hash, entry->format.stride, entry->usage_count, entry->vertex_count);
        
        if (entry->memory_uncompressed > 0) {
            f32 ratio = (f32)entry->memory_compressed / (f32)entry->memory_uncompressed;
            LOG_INFO("      Memory: %llu KB (%.1f%% of uncompressed)",
                     entry->memory_compressed / 1024, ratio * 100.0f);
        }
    }
    
    VertexFormatStats stats = vertex_format_cache_get_stats();
    LOG_INFO("Total vertices: %llu", stats.total_vertices);
    LOG_INFO("Memory saved: %llu KB", stats.total_memory_saved / 1024);
    LOG_INFO("Avg compression: %.1f%%", stats.avg_compression_ratio * 100.0f);
}
