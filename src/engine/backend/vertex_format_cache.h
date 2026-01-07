#pragma once

#include "backend/vertex_format.h"
#include <core/types.h>

/**
 * =================================================================================================
 *                          VERTEX FORMAT CACHE & STATISTICS
 * =================================================================================================
 */

#define MAX_CACHED_FORMATS 128

/* Format Cache Entry */
typedef struct {
    u32 hash;
    VertexFormat format;
    u32 usage_count;
    u64 vertex_count;        // Total vertices using this format
    u64 memory_uncompressed; // Hypothetical uncompressed size
    u64 memory_compressed;   // Actual compressed size
} VertexFormatCacheEntry;

/* Format Statistics */
typedef struct {
    u32 total_formats;
    u32 total_usage;
    u64 total_vertices;
    u64 total_memory_saved;
    f32 avg_compression_ratio;
} VertexFormatStats;

/* Cache API */
void vertex_format_cache_init(void);
void vertex_format_cache_shutdown(void);

/**
 * Register a format in the cache. Returns cached copy if already exists.
 */
const VertexFormat* vertex_format_cache_register(const VertexFormat* fmt);

/**
 * Find format by hash
 */
const VertexFormat* vertex_format_cache_find(u32 hash);

/**
 * Update usage statistics for a format
 */
void vertex_format_cache_update_stats(u32 format_hash, u32 vertex_count, u32 uncompressed_size, u32 compressed_size);

/**
 * Get global statistics
 */
VertexFormatStats vertex_format_cache_get_stats(void);

/**
 * Print cache contents (debug)
 */
void vertex_format_cache_print(void);
