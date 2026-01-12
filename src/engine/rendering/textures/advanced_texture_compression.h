#ifndef ADVANCED_TEXTURE_COMPRESSION_H
#define ADVANCED_TEXTURE_COMPRESSION_H

#include "../texture.h"
#include "../../core/memory.h"
#include "../../math/vector.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Texture compression formats
typedef enum {
    TEXTURE_COMPRESSION_NONE = 0,
    TEXTURE_COMPRESSION_DXT1,
    TEXTURE_COMPRESSION_DXT3,
    TEXTURE_COMPRESSION_DXT5,
    TEXTURE_COMPRESSION_BC4,
    TEXTURE_COMPRESSION_BC5,
    TEXTURE_COMPRESSION_BC6H,
    TEXTURE_COMPRESSION_BC7,
    TEXTURE_COMPRESSION_ASTC,
    TEXTURE_COMPRESSION_ETC2,
    TEXTURE_COMPRESSION_PVRTC,
    TEXTURE_COMPRESSION_CUSTOM
} texture_compression_format_t;

// Compression quality levels
typedef enum {
    COMPRESSION_QUALITY_LOW = 0,
    COMPRESSION_QUALITY_MEDIUM,
    COMPRESSION_QUALITY_HIGH,
    COMPRESSION_QUALITY_ULTRA
} compression_quality_t;

// Texture streaming priority
typedef enum {
    STREAMING_PRIORITY_LOW = 0,
    STREAMING_PRIORITY_MEDIUM,
    STREAMING_PRIORITY_HIGH,
    STREAMING_PRIORITY_CRITICAL
} streaming_priority_t;

// Advanced texture descriptor
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_levels;
    texture_format_t format;
    texture_compression_format_t compression_format;
    compression_quality_t quality;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    float compression_ratio;
    bool is_streaming;
    streaming_priority_t priority;
    char* file_path;
    void* compressed_data;
    void* uncompressed_data;
} advanced_texture_t;

// Streaming texture tile
typedef struct {
    uint32_t tile_x;
    uint32_t tile_y;
    uint32_t tile_width;
    uint32_t tile_height;
    uint32_t mip_level;
    bool is_loaded;
    bool is_requested;
    uint64_t load_time;
    void* tile_data;
} texture_tile_t;

// Texture streaming manager
typedef struct {
    advanced_texture_t* textures;
    uint32_t max_textures;
    uint32_t texture_count;
    
    texture_tile_t* tiles;
    uint32_t max_tiles;
    uint32_t tile_count;
    
    uint32_t streaming_budget_mb;
    uint32_t current_memory_usage_mb;
    
    // Compression settings
    compression_quality_t default_quality;
    texture_compression_format_t default_format;
    
    // Streaming settings
    uint32_t tile_size;
    uint32_t max_concurrent_uploads;
    float streaming_distance_threshold;
    
    // Performance metrics
    uint32_t total_compressions;
    uint32_t total_decompressions;
    float average_compression_time;
    float average_decompression_time;
} texture_streaming_manager_t;

// Compression system
typedef struct {
    texture_compression_format_t format;
    compression_quality_t quality;
    bool use_hardware_compression;
    bool enable_multi_threading;
    uint32_t thread_count;
} texture_compressor_t;

// Function declarations
texture_streaming_manager_t* texture_streaming_manager_create(uint32_t max_textures, uint32_t streaming_budget_mb);
void texture_streaming_manager_destroy(texture_streaming_manager_t* manager);

advanced_texture_t* advanced_texture_create(const char* file_path, texture_compression_format_t format, compression_quality_t quality);
void advanced_texture_destroy(advanced_texture_t* texture);

bool advanced_texture_compress(advanced_texture_t* texture, texture_compressor_t* compressor);
bool advanced_texture_decompress(advanced_texture_t* texture, void** output_data, uint32_t* output_size);

bool texture_streaming_load_tile(texture_streaming_manager_t* manager, advanced_texture_t* texture, 
                                uint32_t tile_x, uint32_t tile_y, uint32_t mip_level);
bool texture_streaming_unload_tile(texture_streaming_manager_t* manager, texture_tile_t* tile);

void texture_streaming_update(texture_streaming_manager_t* manager, vec3_t viewer_position);
void texture_streaming_gc(texture_streaming_manager_t* manager);

// Compression functions
bool compress_texture_dxt(const void* input_data, uint32_t width, uint32_t height, 
                         void** output_data, uint32_t* output_size, compression_quality_t quality);
bool compress_texture_bc7(const void* input_data, uint32_t width, uint32_t height, 
                         void** output_data, uint32_t* output_size, compression_quality_t quality);
bool compress_texture_astc(const void* input_data, uint32_t width, uint32_t height, 
                          void** output_data, uint32_t* output_size, compression_quality_t quality);

// Decompression functions
bool decompress_texture_dxt(const void* compressed_data, uint32_t compressed_size, 
                          uint32_t width, uint32_t height, void** output_data, uint32_t* output_size);
bool decompress_texture_bc7(const void* compressed_data, uint32_t compressed_size, 
                          uint32_t width, uint32_t height, void** output_data, uint32_t* output_size);
bool decompress_texture_astc(const void* compressed_data, uint32_t compressed_size, 
                           uint32_t width, uint32_t height, void** output_data, uint32_t* output_size);

// Utility functions
uint32_t calculate_compressed_size(uint32_t width, uint32_t height, texture_compression_format_t format);
texture_compression_format_t get_optimal_compression_format(texture_format_t format);
compression_quality_t get_quality_from_settings(float quality_factor);

// Performance monitoring
void texture_streaming_get_stats(texture_streaming_manager_t* manager, 
                                 uint32_t* memory_usage, uint32_t* texture_count, 
                                 float* compression_ratio, float* streaming_efficiency);

#ifdef __cplusplus
}
#endif

#endif // ADVANCED_TEXTURE_COMPRESSION_H
