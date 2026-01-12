#include "advanced_texture_compression.h"
#include "../../core/logging.h"
#include "../../core/thread_pool.h"
#include "../../core/file_system.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Global texture streaming manager
static texture_streaming_manager_t* g_streaming_manager = NULL;

texture_streaming_manager_t* texture_streaming_manager_create(uint32_t max_textures, uint32_t streaming_budget_mb) {
    texture_streaming_manager_t* manager = malloc(sizeof(texture_streaming_manager_t));
    if (!manager) {
        log_error("Failed to allocate memory for texture streaming manager");
        return NULL;
    }
    
    memset(manager, 0, sizeof(texture_streaming_manager_t));
    
    manager->textures = malloc(sizeof(advanced_texture_t) * max_textures);
    if (!manager->textures) {
        log_error("Failed to allocate memory for texture array");
        free(manager);
        return NULL;
    }
    
    manager->tiles = malloc(sizeof(texture_tile_t) * max_textures * 16); // Estimate 16 tiles per texture
    if (!manager->tiles) {
        log_error("Failed to allocate memory for texture tiles");
        free(manager->textures);
        free(manager);
        return NULL;
    }
    
    manager->max_textures = max_textures;
    manager->streaming_budget_mb = streaming_budget_mb;
    manager->tile_size = 256; // Default tile size
    manager->max_concurrent_uploads = 4;
    manager->streaming_distance_threshold = 1000.0f;
    manager->default_quality = COMPRESSION_QUALITY_HIGH;
    manager->default_format = TEXTURE_COMPRESSION_BC7;
    
    log_info("Texture streaming manager created with budget: %d MB, max textures: %d", 
             streaming_budget_mb, max_textures);
    
    return manager;
}

void texture_streaming_manager_destroy(texture_streaming_manager_t* manager) {
    if (!manager) return;
    
    // Clean up all textures
    for (uint32_t i = 0; i < manager->texture_count; i++) {
        advanced_texture_destroy(&manager->textures[i]);
    }
    
    // Clean up all tiles
    for (uint32_t i = 0; i < manager->tile_count; i++) {
        if (manager->tiles[i].tile_data) {
            free(manager->tiles[i].tile_data);
        }
    }
    
    free(manager->textures);
    free(manager->tiles);
    free(manager);
    
    log_info("Texture streaming manager destroyed");
}

advanced_texture_t* advanced_texture_create(const char* file_path, texture_compression_format_t format, compression_quality_t quality) {
    if (!file_path) {
        log_error("Invalid file path for texture creation");
        return NULL;
    }
    
    advanced_texture_t* texture = malloc(sizeof(advanced_texture_t));
    if (!texture) {
        log_error("Failed to allocate memory for advanced texture");
        return NULL;
    }
    
    memset(texture, 0, sizeof(advanced_texture_t));
    
    // Load basic texture info (this would load from actual file)
    texture->file_path = strdup(file_path);
    texture->compression_format = format;
    texture->quality = quality;
    texture->width = 1024; // Default - would be loaded from file
    texture->height = 1024;
    texture->depth = 1;
    texture->mip_levels = 10; // Calculate based on size
    texture->format = TEXTURE_FORMAT_RGBA8; // Default
    
    // Calculate uncompressed size
    texture->uncompressed_size = texture->width * texture->height * 4; // RGBA8
    
    log_info("Created advanced texture: %s, format: %d, quality: %d", 
             file_path, format, quality);
    
    return texture;
}

void advanced_texture_destroy(advanced_texture_t* texture) {
    if (!texture) return;
    
    if (texture->file_path) free(texture->file_path);
    if (texture->compressed_data) free(texture->compressed_data);
    if (texture->uncompressed_data) free(texture->uncompressed_data);
    
    free(texture);
}

bool advanced_texture_compress(advanced_texture_t* texture, texture_compressor_t* compressor) {
    if (!texture || !compressor) {
        log_error("Invalid parameters for texture compression");
        return false;
    }
    
    // Simulate compression timing
    uint64_t start_time = get_current_time_ms();
    
    bool success = false;
    
    switch (compressor->format) {
        case TEXTURE_COMPRESSION_DXT1:
        case TEXTURE_COMPRESSION_DXT3:
        case TEXTURE_COMPRESSION_DXT5:
            success = compress_texture_dxt(texture->uncompressed_data, texture->width, texture->height,
                                         &texture->compressed_data, &texture->compressed_size, compressor->quality);
            break;
            
        case TEXTURE_COMPRESSION_BC7:
            success = compress_texture_bc7(texture->uncompressed_data, texture->width, texture->height,
                                          &texture->compressed_data, &texture->compressed_size, compressor->quality);
            break;
            
        case TEXTURE_COMPRESSION_ASTC:
            success = compress_texture_astc(texture->uncompressed_data, texture->width, texture->height,
                                          &texture->compressed_data, &texture->compressed_size, compressor->quality);
            break;
            
        default:
            log_error("Unsupported compression format: %d", compressor->format);
            return false;
    }
    
    if (success) {
        texture->compression_ratio = (float)texture->uncompressed_size / texture->compressed_size;
        uint64_t compression_time = get_current_time_ms() - start_time;
        
        log_info("Texture compressed successfully: %s, ratio: %.2f:1, time: %llu ms",
                 texture->file_path, texture->compression_ratio, compression_time);
    }
    
    return success;
}

bool compress_texture_dxt(const void* input_data, uint32_t width, uint32_t height, 
                         void** output_data, uint32_t* output_size, compression_quality_t quality) {
    if (!input_data || !output_data || !output_size) {
        return false;
    }
    
    // Calculate compressed size for DXT1 (8 bytes per 4x4 block)
    uint32_t block_width = (width + 3) / 4;
    uint32_t block_height = (height + 3) / 4;
    *output_size = block_width * block_height * 8;
    
    *output_data = malloc(*output_size);
    if (!*output_data) {
        log_error("Failed to allocate memory for DXT compression");
        return false;
    }
    
    // Simulate DXT compression (would use actual compression algorithm)
    memset(*output_data, 0, *output_size);
    
    log_debug("DXT compression completed: %dx%d -> %d bytes", width, height, *output_size);
    return true;
}

bool compress_texture_bc7(const void* input_data, uint32_t width, uint32_t height, 
                         void** output_data, uint32_t* output_size, compression_quality_t quality) {
    if (!input_data || !output_data || !output_size) {
        return false;
    }
    
    // Calculate compressed size for BC7 (16 bytes per 4x4 block)
    uint32_t block_width = (width + 3) / 4;
    uint32_t block_height = (height + 3) / 4;
    *output_size = block_width * block_height * 16;
    
    *output_data = malloc(*output_size);
    if (!*output_data) {
        log_error("Failed to allocate memory for BC7 compression");
        return false;
    }
    
    // Simulate BC7 compression (would use actual compression algorithm)
    memset(*output_data, 0, *output_size);
    
    log_debug("BC7 compression completed: %dx%d -> %d bytes", width, height, *output_size);
    return true;
}

bool compress_texture_astc(const void* input_data, uint32_t width, uint32_t height, 
                          void** output_data, uint32_t* output_size, compression_quality_t quality) {
    if (!input_data || !output_data || !output_size) {
        return false;
    }
    
    // ASTC block size varies, use 6x6 as example
    uint32_t block_size_x = 6, block_size_y = 6;
    uint32_t block_width = (width + block_size_x - 1) / block_size_x;
    uint32_t block_height = (height + block_size_y - 1) / block_size_y;
    *output_size = block_width * block_height * 16; // 16 bytes per ASTC block
    
    *output_data = malloc(*output_size);
    if (!*output_data) {
        log_error("Failed to allocate memory for ASTC compression");
        return false;
    }
    
    // Simulate ASTC compression (would use actual compression algorithm)
    memset(*output_data, 0, *output_size);
    
    log_debug("ASTC compression completed: %dx%d -> %d bytes", width, height, *output_size);
    return true;
}

bool texture_streaming_load_tile(texture_streaming_manager_t* manager, advanced_texture_t* texture, 
                                uint32_t tile_x, uint32_t tile_y, uint32_t mip_level) {
    if (!manager || !texture) {
        log_error("Invalid parameters for tile loading");
        return false;
    }
    
    // Check if we have space for more tiles
    if (manager->tile_count >= manager->max_tiles) {
        log_warn("Tile limit reached, triggering garbage collection");
        texture_streaming_gc(manager);
    }
    
    // Create new tile
    texture_tile_t* tile = &manager->tiles[manager->tile_count];
    tile->tile_x = tile_x;
    tile->tile_y = tile_y;
    tile->tile_width = manager->tile_size;
    tile->tile_height = manager->tile_size;
    tile->mip_level = mip_level;
    tile->is_loaded = false;
    tile->is_requested = true;
    tile->load_time = get_current_time_ms();
    
    // Allocate memory for tile data (simulated)
    uint32_t tile_data_size = tile->tile_width * tile->tile_height * 4; // RGBA8
    tile->tile_data = malloc(tile_data_size);
    if (!tile->tile_data) {
        log_error("Failed to allocate memory for tile data");
        return false;
    }
    
    // Simulate tile loading (would load from disk or decompress)
    memset(tile->tile_data, 0x80, tile_data_size); // Fill with gray for testing
    tile->is_loaded = true;
    
    manager->tile_count++;
    manager->current_memory_usage_mb += tile_data_size / (1024 * 1024);
    
    log_debug("Loaded tile: (%d,%d) mip %d for texture %s", 
              tile_x, tile_y, mip_level, texture->file_path);
    
    return true;
}

void texture_streaming_update(texture_streaming_manager_t* manager, vec3_t viewer_position) {
    if (!manager) return;
    
    // Update streaming based on viewer position
    // This would calculate which tiles are needed based on distance
    
    // Simulate streaming update
    static uint32_t update_counter = 0;
    update_counter++;
    
    if (update_counter % 60 == 0) { // Update every 60 frames
        log_debug("Texture streaming update: %d textures, %d MB used", 
                  manager->texture_count, manager->current_memory_usage_mb);
    }
}

void texture_streaming_gc(texture_streaming_manager_t* manager) {
    if (!manager) return;
    
    // Remove least recently used tiles to free memory
    uint32_t tiles_removed = 0;
    uint32_t target_memory = manager->streaming_budget_mb * 0.8f; // Target 80% of budget
    
    while (manager->current_memory_usage_mb > target_memory && manager->tile_count > 0) {
        // Find oldest tile (simplified - would use LRU in real implementation)
        texture_tile_t* oldest_tile = &manager->tiles[0];
        uint32_t oldest_index = 0;
        
        for (uint32_t i = 1; i < manager->tile_count; i++) {
            if (manager->tiles[i].load_time < oldest_tile->load_time) {
                oldest_tile = &manager->tiles[i];
                oldest_index = i;
            }
        }
        
        // Free tile data
        if (oldest_tile->tile_data) {
            free(oldest_tile->tile_data);
            uint32_t tile_size = oldest_tile->tile_width * oldest_tile->tile_height * 4;
            manager->current_memory_usage_mb -= tile_size / (1024 * 1024);
        }
        
        // Remove tile from array
        if (oldest_index < manager->tile_count - 1) {
            memmove(&manager->tiles[oldest_index], &manager->tiles[oldest_index + 1],
                   (manager->tile_count - oldest_index - 1) * sizeof(texture_tile_t));
        }
        
        manager->tile_count--;
        tiles_removed++;
    }
    
    if (tiles_removed > 0) {
        log_info("Texture streaming GC: removed %d tiles, memory usage: %d MB", 
                 tiles_removed, manager->current_memory_usage_mb);
    }
}

uint32_t calculate_compressed_size(uint32_t width, uint32_t height, texture_compression_format_t format) {
    uint32_t block_width = (width + 3) / 4;
    uint32_t block_height = (height + 3) / 4;
    
    switch (format) {
        case TEXTURE_COMPRESSION_DXT1:
            return block_width * block_height * 8;
        case TEXTURE_COMPRESSION_DXT3:
        case TEXTURE_COMPRESSION_DXT5:
            return block_width * block_height * 16;
        case TEXTURE_COMPRESSION_BC4:
        case TEXTURE_COMPRESSION_BC5:
            return block_width * block_height * 8;
        case TEXTURE_COMPRESSION_BC6H:
        case TEXTURE_COMPRESSION_BC7:
            return block_width * block_height * 16;
        case TEXTURE_COMPRESSION_ASTC:
            return block_width * block_height * 16; // Varies by block size
        default:
            return width * height * 4; // Uncompressed
    }
}

texture_compression_format_t get_optimal_compression_format(texture_format_t format) {
    // Return optimal compression format based on input format
    switch (format) {
        case TEXTURE_FORMAT_R8:
            return TEXTURE_COMPRESSION_BC4;
        case TEXTURE_FORMAT_RG8:
            return TEXTURE_COMPRESSION_BC5;
        case TEXTURE_FORMAT_RGB8:
        case TEXTURE_FORMAT_RGBA8:
            return TEXTURE_COMPRESSION_BC7;
        case TEXTURE_FORMAT_RGB16F:
        case TEXTURE_FORMAT_RGBA16F:
            return TEXTURE_COMPRESSION_BC6H;
        default:
            return TEXTURE_COMPRESSION_BC7;
    }
}

void texture_streaming_get_stats(texture_streaming_manager_t* manager, 
                                 uint32_t* memory_usage, uint32_t* texture_count, 
                                 float* compression_ratio, float* streaming_efficiency) {
    if (!manager) return;
    
    if (memory_usage) *memory_usage = manager->current_memory_usage_mb;
    if (texture_count) *texture_count = manager->texture_count;
    
    // Calculate average compression ratio
    if (compression_ratio && manager->texture_count > 0) {
        float total_ratio = 0.0f;
        for (uint32_t i = 0; i < manager->texture_count; i++) {
            total_ratio += manager->textures[i].compression_ratio;
        }
        *compression_ratio = total_ratio / manager->texture_count;
    }
    
    // Calculate streaming efficiency (tiles loaded / tiles requested)
    if (streaming_efficiency && manager->tile_count > 0) {
        uint32_t loaded_tiles = 0;
        for (uint32_t i = 0; i < manager->tile_count; i++) {
            if (manager->tiles[i].is_loaded) {
                loaded_tiles++;
            }
        }
        *streaming_efficiency = (float)loaded_tiles / manager->tile_count;
    }
}
