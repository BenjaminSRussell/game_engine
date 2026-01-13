/*
 * compression_wrapper.h
 *
 * Unified compression interface for LZ4 and ZSTD
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * Provides a unified API for compressing/decompressing data using
 * LZ4 or ZSTD compression algorithms with automatic selection.
 */

#ifndef COMPRESSION_WRAPPER_H
#define COMPRESSION_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

typedef enum {
    COMPRESSION_ALGORITHM_AUTO = 0,   /* Automatically select best algorithm */
    COMPRESSION_ALGORITHM_LZ4,         /* Fast compression (LZ4) */
    COMPRESSION_ALGORITHM_ZSTD,        /* Balanced compression (ZSTD) */
} CompressionAlgorithm;

typedef enum {
    COMPRESSION_LEVEL_FAST = 1,
    COMPRESSION_LEVEL_DEFAULT = 3,
    COMPRESSION_LEVEL_BALANCED = 6,
    COMPRESSION_LEVEL_HIGH = 9,
    COMPRESSION_LEVEL_MAXIMUM = 22,
} CompressionLevel;

typedef struct {
    CompressionAlgorithm algorithm;
    CompressionLevel level;
    bool use_dictionary;
    const void* dictionary;
    size_t dictionary_size;
} CompressionOptions;

typedef struct {
    size_t original_size;
    size_t compressed_size;
    CompressionAlgorithm algorithm_used;
    double compression_ratio;
} CompressionStats;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/*
 * Compress data buffer
 * Returns: 0 on success, negative value on error
 */
int compression_compress(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size,
    const CompressionOptions* options
);

/*
 * Decompress data buffer
 * Returns: 0 on success, negative value on error
 */
int compression_decompress(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size
);

/*
 * Get compression statistics
 */
int compression_get_stats(
    const CompressionStats* stats
);

/*
 * Estimate compressed size before compression
 */
size_t compression_estimate_compressed_size(
    size_t input_size,
    CompressionAlgorithm algorithm
);

/*
 * Check if algorithm is available
 */
bool compression_is_algorithm_available(CompressionAlgorithm algorithm);

/*
 * Get compression algorithm name
 */
const char* compression_algorithm_to_string(CompressionAlgorithm algorithm);

/*
 * Compress file to file
 */
int compression_compress_file(
    const char* input_path,
    const char* output_path,
    const CompressionOptions* options
);

/*
 * Decompress file to file
 */
int compression_decompress_file(
    const char* input_path,
    const char* output_path
);

/*
 * Get compression ratio for buffer
 */
double compression_get_ratio(
    size_t original_size,
    size_t compressed_size
);

#ifdef __cplusplus
}
#endif

#endif // COMPRESSION_WRAPPER_H
