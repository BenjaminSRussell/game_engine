#ifndef TEXTURE_COMPRESSOR_H
#define TEXTURE_COMPRESSOR_H

#include "core/core.h"
#include "renderer/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

// Texture compression formats
typedef enum {
    TEXTURE_FORMAT_BC1 = 0,     // DXT1, RGB, 8:1 compression
    TEXTURE_FORMAT_BC3,         // DXT5, RGBA, 4:1 compression
    TEXTURE_FORMAT_BC5,         // Normal maps, RG, 4:1 compression
    TEXTURE_FORMAT_BC7,         // High quality RGBA, 4:1 compression
    TEXTURE_FORMAT_ETC2,        // Mobile RGB, 6:1 compression
    TEXTURE_FORMAT_ETC2_ALPHA,  // Mobile RGBA, 4:1 compression
    TEXTURE_FORMAT_ASTC_4x4,    // Mobile high quality, 3.56:1 compression
    TEXTURE_FORMAT_ASTC_6x6,    // Mobile balanced, 9:1 compression
    TEXTURE_FORMAT_ASTC_8x8,    // Mobile efficient, 16:1 compression
    TEXTURE_FORMAT_UNCOMPRESSED  // No compression
} TextureCompressionFormat;

// Quality presets
typedef enum {
    QUALITY_FAST = 0,      // Fastest compression, lower quality
    QUALITY_NORMAL,         // Balanced speed/quality
    QUALITY_HIGH,           // High quality, slower
    QUALITY_MAXIMUM         // Best quality, slowest
} TextureQualityPreset;

// Texture types
typedef enum {
    TEXTURE_TYPE_COLOR = 0,     // Diffuse/albedo textures
    TEXTURE_TYPE_NORMAL,         // Normal maps
    TEXTURE_TYPE_ROUGHNESS,      // Roughness/metallic maps
    TEXTURE_TYPE_EMISSIVE,       // Emissive/glow maps
    TEXTURE_TYPE_HDR,           // HDR textures
    TEXTURE_TYPE_LUT            // Lookup tables
} TextureType;

// Compression settings
typedef struct {
    TextureCompressionFormat format;
    TextureQualityPreset quality;
    TextureType type;
    bool generate_mipmaps;
    bool srgb;
    bool normal_map_renormalize;
    u32 max_mip_levels;
    f32 alpha_threshold;
    bool pack_channels;
} TextureCompressionSettings;

// Default settings for common texture types
extern const TextureCompressionSettings DEFAULT_COLOR_SETTINGS;
extern const TextureCompressionSettings DEFAULT_NORMAL_SETTINGS;
extern const TextureCompressionSettings DEFAULT_ROUGHNESS_SETTINGS;
extern const TextureCompressionSettings DEFAULT_MOBILE_SETTINGS;

// Progress callback
typedef void (*TextureProgressCallback)(f32 progress, const char* current_task, void* user_data);

// Texture compressor instance
typedef struct TextureCompressor TextureCompressor;

// Core API
TextureCompressor* texture_compressor_create(void);
void texture_compressor_destroy(TextureCompressor* compressor);

// Compression operations
bool texture_compressor_compress_file(TextureCompressor* compressor,
                                      const char* input_path,
                                      const char* output_path,
                                      const TextureCompressionSettings* settings);

bool texture_compressor_compress_memory(TextureCompressor* compressor,
                                        const u8* input_data,
                                        size_t input_size,
                                        u32 width, u32 height, u32 channels,
                                        u8** output_data,
                                        size_t* output_size,
                                        const TextureCompressionSettings* settings);

// Batch processing
bool texture_compressor_compress_batch(TextureCompressor* compressor,
                                       const char** input_paths,
                                       const char** output_paths,
                                       u32 count,
                                       const TextureCompressionSettings* settings);

// Multi-threading
void texture_compressor_set_thread_count(TextureCompressor* compressor, u32 thread_count);
u32 texture_compressor_get_thread_count(TextureCompressor* compressor);

// Progress reporting
void texture_compressor_set_progress_callback(TextureCompressor* compressor,
                                             TextureProgressCallback callback,
                                             void* user_data);

// Incremental compilation
bool texture_compressor_needs_update(TextureCompressor* compressor,
                                     const char* input_path,
                                     const char* output_path);

// Utility functions
const char* texture_compressor_format_to_string(TextureCompressionFormat format);
const char* texture_compressor_quality_to_string(TextureQualityPreset quality);
size_t texture_compressor_estimate_compressed_size(u32 width, u32 height,
                                                   TextureCompressionFormat format,
                                                   u32 mip_levels);

// Statistics
typedef struct {
    u64 total_input_size;
    u64 total_output_size;
    u32 textures_processed;
    f64 compression_ratio;
    f64 total_time;
} TextureCompressionStats;

void texture_compressor_get_stats(TextureCompressor* compressor, TextureCompressionStats* stats);
void texture_compressor_reset_stats(TextureCompressor* compressor);

#ifdef __cplusplus
}
#endif

#endif // TEXTURE_COMPRESSOR_H
