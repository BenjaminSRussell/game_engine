#ifndef CHUNK_FORMAT_H
#define CHUNK_FORMAT_H

#include "../game_common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Chunk file format (binary serialization)
// Magic number to identify chunk files
#define CHUNK_MAGIC 0x4D494E45  // "MINE" in hex

// Version for format compatibility
#define CHUNK_VERSION 1

// Compression types
#define CHUNK_COMPRESSION_NONE 0
#define CHUNK_COMPRESSION_ZLIB 1

// Chunk file header - written at start of every chunk file
typedef struct {
    uint32_t magic;           // 0x4D494E45 ("MINE") for validation
    uint8_t version;          // Version 1 (for format compatibility)
    uint8_t compression;      // 0=none, 1=zlib
    int32_t x, z;             // Chunk coordinates (2D, Y is implicit)
    uint32_t timestamp;       // Unix timestamp when saved
    uint32_t block_count;     // Total number of blocks in chunk
    uint32_t data_size;       // Size of compressed/uncompressed block data
} ChunkFileHeader;

// Chunk dimensions (from chunk.h)
#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 256
#define CHUNK_SIZE_Z 16
#define CHUNK_SIZE_TOTAL (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z)

// Serialization functions
int chunk_serialize(void *chunk, uint8_t *buffer, size_t buffer_size);
int chunk_deserialize(void *chunk, const uint8_t *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // CHUNK_FORMAT_H
