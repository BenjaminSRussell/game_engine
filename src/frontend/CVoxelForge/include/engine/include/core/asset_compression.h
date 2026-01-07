#ifndef ASSET_COMPRESSION_H
#define ASSET_COMPRESSION_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
  COMPRESSION_NONE = 0,
  COMPRESSION_RLE = 1,
  COMPRESSION_LZ4 = 2,
  COMPRESSION_ZSTD = 3
} CompressionMethod;

typedef enum {
  ASSET_TYPE_UNKNOWN = 0,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_AUDIO,
  ASSET_TYPE_MESH,
  ASSET_TYPE_TEXT,
  ASSET_TYPE_SHADER,
  ASSET_TYPE_SCRIPT
} AssetType;

/**
 * Compresses data using the specified compression method.
 * Returns the size of the compressed data, or 0 on failure.
 * output should be pre-allocated with at least asset_compress_bound(src_size).
 */
bool asset_compress(const void* input, size_t input_size, 
                    void* output, size_t output_capacity, 
                    size_t* output_size, CompressionMethod method);

/**
 * Decompresses data using the specified compression method.
 * Returns the size of the decompressed data, or 0 on failure.
 * output should be pre-allocated with at least original_size.
 */
bool asset_decompress(const void* input, size_t input_size, 
                      void* output, size_t output_capacity, 
                      size_t* output_size, CompressionMethod method);

/**
 * Returns the best compression method for a given asset type.
 */
CompressionMethod asset_get_best_compression(AssetType type);

#endif // ASSET_COMPRESSION_H
