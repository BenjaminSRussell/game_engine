/*
 * compression_wrapper.c
 *
 * Unified compression interface implementation for LZ4 and ZSTD
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 */

#include "compression_wrapper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * CONDITIONAL COMPRESSION LIBRARY INCLUDES
 * ============================================================================ */

#ifdef ENABLE_LZ4
#include <lz4.h>
#include <lz4hc.h>
#endif

#ifdef ENABLE_ZSTD
#include <zstd.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define COMPRESSION_BUFFER_SIZE (1024 * 1024)  /* 1MB buffer */
#define COMPRESSION_FILE_CHUNK_SIZE (256 * 1024)  /* 256KB chunks */

/* ============================================================================
 * COMPRESSION STATISTICS
 * ============================================================================ */

static CompressionStats s_last_compression_stats = {0};

/* ============================================================================
 * COMPRESSION UTILITY FUNCTIONS
 * ============================================================================ */

static int map_compression_level_to_zstd(CompressionLevel level) {
    switch (level) {
        case COMPRESSION_LEVEL_FAST: return 1;
        case COMPRESSION_LEVEL_DEFAULT: return 3;
        case COMPRESSION_LEVEL_BALANCED: return 6;
        case COMPRESSION_LEVEL_HIGH: return 9;
        case COMPRESSION_LEVEL_MAXIMUM: return 22;
        default: return 3;
    }
}

static int map_compression_level_to_lz4(CompressionLevel level) {
    switch (level) {
        case COMPRESSION_LEVEL_FAST: return 1;
        case COMPRESSION_LEVEL_DEFAULT: return 3;
        case COMPRESSION_LEVEL_BALANCED: return 6;
        case COMPRESSION_LEVEL_HIGH: return 9;
        case COMPRESSION_LEVEL_MAXIMUM: return 12;
        default: return 3;
    }
}

static bool should_use_zstd_for_size(size_t input_size) {
    /* Use ZSTD for larger files (better compression ratio) */
    return input_size > (256 * 1024);  /* > 256KB */
}

/* ============================================================================
 * LZ4 COMPRESSION FUNCTIONS
 * ============================================================================ */

#ifdef ENABLE_LZ4

static int compress_lz4(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size,
    CompressionLevel level) {

    if (!input || !output || !output_size) {
        return -1;
    }

    int compression_level = map_compression_level_to_lz4(level);

    /* Allocate output buffer with header */
    size_t max_compressed_size = LZ4_compressBound(input_size) + 8;
    *output = malloc(max_compressed_size);
    if (!*output) {
        return -2;
    }

    /* Write original size to output (for decompression) */
    uint32_t* header = (uint32_t*)(*output);
    header[0] = input_size;

    /* Compress data */
    int compressed_size = 0;
    if (compression_level <= 3) {
        compressed_size = LZ4_compress_default(
            (const char*)input,
            (char*)(*output) + 8,
            input_size,
            max_compressed_size - 8
        );
    } else {
        compressed_size = LZ4_compress_HC(
            (const char*)input,
            (char*)(*output) + 8,
            input_size,
            max_compressed_size - 8,
            compression_level
        );
    }

    if (compressed_size <= 0) {
        free(*output);
        *output = NULL;
        return -3;
    }

    *output_size = compressed_size + 8;

    /* Update statistics */
    s_last_compression_stats.original_size = input_size;
    s_last_compression_stats.compressed_size = *output_size;
    s_last_compression_stats.algorithm_used = COMPRESSION_ALGORITHM_LZ4;
    s_last_compression_stats.compression_ratio =
        (double)input_size / *output_size;

    return 0;
}

static int decompress_lz4(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size) {

    if (!input || !output || !output_size || input_size < 8) {
        return -1;
    }

    /* Read original size from header */
    const uint32_t* header = (const uint32_t*)input;
    uint32_t decompressed_size = header[0];

    *output = malloc(decompressed_size);
    if (!*output) {
        return -2;
    }

    /* Decompress */
    int result = LZ4_decompress_safe(
        (const char*)input + 8,
        (char*)(*output),
        input_size - 8,
        decompressed_size
    );

    if (result < 0) {
        free(*output);
        *output = NULL;
        return -3;
    }

    *output_size = decompressed_size;
    return 0;
}

#endif  /* ENABLE_LZ4 */

/* ============================================================================
 * ZSTD COMPRESSION FUNCTIONS
 * ============================================================================ */

#ifdef ENABLE_ZSTD

static int compress_zstd(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size,
    CompressionLevel level) {

    if (!input || !output || !output_size) {
        return -1;
    }

    int compression_level = map_compression_level_to_zstd(level);

    /* Allocate output buffer */
    size_t max_compressed_size = ZSTD_compressBound(input_size);
    *output = malloc(max_compressed_size);
    if (!*output) {
        return -2;
    }

    /* Compress */
    size_t compressed_size = ZSTD_compress(
        *output,
        max_compressed_size,
        input,
        input_size,
        compression_level
    );

    if (ZSTD_isError(compressed_size)) {
        free(*output);
        *output = NULL;
        return -3;
    }

    *output_size = compressed_size;

    /* Update statistics */
    s_last_compression_stats.original_size = input_size;
    s_last_compression_stats.compressed_size = compressed_size;
    s_last_compression_stats.algorithm_used = COMPRESSION_ALGORITHM_ZSTD;
    s_last_compression_stats.compression_ratio =
        (double)input_size / compressed_size;

    return 0;
}

static int decompress_zstd(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size) {

    if (!input || !output || !output_size) {
        return -1;
    }

    /* Get decompressed size */
    unsigned long long decompressed_size = ZSTD_getFrameContentSize(input, input_size);
    if (ZSTD_isError(decompressed_size)) {
        return -2;
    }

    *output = malloc(decompressed_size);
    if (!*output) {
        return -3;
    }

    /* Decompress */
    size_t result = ZSTD_decompress(
        *output,
        decompressed_size,
        input,
        input_size
    );

    if (ZSTD_isError(result)) {
        free(*output);
        *output = NULL;
        return -4;
    }

    *output_size = decompressed_size;
    return 0;
}

#endif  /* ENABLE_ZSTD */

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

int compression_compress(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size,
    const CompressionOptions* options) {

    if (!input || !output || !output_size || input_size == 0) {
        return -1;
    }

    CompressionAlgorithm algorithm = COMPRESSION_ALGORITHM_ZSTD;
    CompressionLevel level = COMPRESSION_LEVEL_DEFAULT;

    if (options) {
        algorithm = options->algorithm;
        level = options->level;
    }

    /* Auto-select algorithm based on input size */
    if (algorithm == COMPRESSION_ALGORITHM_AUTO) {
        if (should_use_zstd_for_size(input_size)) {
            algorithm = COMPRESSION_ALGORITHM_ZSTD;
        } else {
            algorithm = COMPRESSION_ALGORITHM_LZ4;
        }
    }

    switch (algorithm) {
#ifdef ENABLE_ZSTD
        case COMPRESSION_ALGORITHM_ZSTD:
            return compress_zstd(input, input_size, output, output_size, level);
#endif
#ifdef ENABLE_LZ4
        case COMPRESSION_ALGORITHM_LZ4:
            return compress_lz4(input, input_size, output, output_size, level);
#endif
        case COMPRESSION_ALGORITHM_AUTO:
            /* Fallback: use available algorithm */
#ifdef ENABLE_ZSTD
            return compress_zstd(input, input_size, output, output_size, level);
#elif defined(ENABLE_LZ4)
            return compress_lz4(input, input_size, output, output_size, level);
#else
            return -2;  /* No compression available */
#endif
        default:
            return -3;
    }
}

int compression_decompress(
    const void* input,
    size_t input_size,
    void** output,
    size_t* output_size) {

    if (!input || !output || !output_size || input_size == 0) {
        return -1;
    }

    /* Try ZSTD first (it has better error handling) */
#ifdef ENABLE_ZSTD
    int result = decompress_zstd(input, input_size, output, output_size);
    if (result == 0) {
        return 0;
    }
#endif

    /* Fall back to LZ4 */
#ifdef ENABLE_LZ4
    return decompress_lz4(input, input_size, output, output_size);
#endif

    return -2;  /* No decompression available */
}

int compression_get_stats(const CompressionStats* stats) {
    if (!stats) {
        return -1;
    }

    memcpy((void*)stats, &s_last_compression_stats, sizeof(CompressionStats));
    return 0;
}

size_t compression_estimate_compressed_size(
    size_t input_size,
    CompressionAlgorithm algorithm) {

    if (input_size == 0) {
        return 0;
    }

    switch (algorithm) {
#ifdef ENABLE_ZSTD
        case COMPRESSION_ALGORITHM_ZSTD:
            return ZSTD_compressBound(input_size);
#endif
#ifdef ENABLE_LZ4
        case COMPRESSION_ALGORITHM_LZ4:
            return LZ4_compressBound(input_size) + 8;
#endif
        case COMPRESSION_ALGORITHM_AUTO:
            /* Estimate based on input size */
            return (size_t)(input_size * 0.5) + 256;
        default:
            return input_size;
    }
}

bool compression_is_algorithm_available(CompressionAlgorithm algorithm) {
    switch (algorithm) {
#ifdef ENABLE_ZSTD
        case COMPRESSION_ALGORITHM_ZSTD:
            return true;
#endif
#ifdef ENABLE_LZ4
        case COMPRESSION_ALGORITHM_LZ4:
            return true;
#endif
        case COMPRESSION_ALGORITHM_AUTO:
            return true;
        default:
            return false;
    }
}

const char* compression_algorithm_to_string(CompressionAlgorithm algorithm) {
    switch (algorithm) {
        case COMPRESSION_ALGORITHM_AUTO: return "Auto";
        case COMPRESSION_ALGORITHM_LZ4: return "LZ4";
        case COMPRESSION_ALGORITHM_ZSTD: return "ZSTD";
        default: return "Unknown";
    }
}

int compression_compress_file(
    const char* input_path,
    const char* output_path,
    const CompressionOptions* options) {

    if (!input_path || !output_path) {
        return -1;
    }

    /* Open input file */
    FILE* input_file = fopen(input_path, "rb");
    if (!input_file) {
        return -2;
    }

    /* Get file size */
    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(input_file);
        return -3;
    }

    /* Read entire file into memory */
    void* file_data = malloc(file_size);
    if (!file_data) {
        fclose(input_file);
        return -4;
    }

    if (fread(file_data, 1, file_size, input_file) != (size_t)file_size) {
        free(file_data);
        fclose(input_file);
        return -5;
    }

    fclose(input_file);

    /* Compress */
    void* compressed_data = NULL;
    size_t compressed_size = 0;

    int result = compression_compress(
        file_data,
        file_size,
        &compressed_data,
        &compressed_size,
        options
    );

    free(file_data);

    if (result != 0) {
        return result;
    }

    /* Write compressed file */
    FILE* output_file = fopen(output_path, "wb");
    if (!output_file) {
        free(compressed_data);
        return -6;
    }

    if (fwrite(compressed_data, 1, compressed_size, output_file) != compressed_size) {
        fclose(output_file);
        free(compressed_data);
        return -7;
    }

    fclose(output_file);
    free(compressed_data);

    return 0;
}

int compression_decompress_file(
    const char* input_path,
    const char* output_path) {

    if (!input_path || !output_path) {
        return -1;
    }

    /* Open input file */
    FILE* input_file = fopen(input_path, "rb");
    if (!input_file) {
        return -2;
    }

    /* Get file size */
    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(input_file);
        return -3;
    }

    /* Read entire file into memory */
    void* file_data = malloc(file_size);
    if (!file_data) {
        fclose(input_file);
        return -4;
    }

    if (fread(file_data, 1, file_size, input_file) != (size_t)file_size) {
        free(file_data);
        fclose(input_file);
        return -5;
    }

    fclose(input_file);

    /* Decompress */
    void* decompressed_data = NULL;
    size_t decompressed_size = 0;

    int result = compression_decompress(
        file_data,
        file_size,
        &decompressed_data,
        &decompressed_size
    );

    free(file_data);

    if (result != 0) {
        return result;
    }

    /* Write decompressed file */
    FILE* output_file = fopen(output_path, "wb");
    if (!output_file) {
        free(decompressed_data);
        return -6;
    }

    if (fwrite(decompressed_data, 1, decompressed_size, output_file) != decompressed_size) {
        fclose(output_file);
        free(decompressed_data);
        return -7;
    }

    fclose(output_file);
    free(decompressed_data);

    return 0;
}

double compression_get_ratio(
    size_t original_size,
    size_t compressed_size) {

    if (compressed_size == 0 || original_size == 0) {
        return 0.0;
    }

    return (double)original_size / compressed_size;
}

/* End of compression_wrapper.c */
