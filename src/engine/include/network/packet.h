#ifndef PACKET_H
#define PACKET_H

#include "include/network/network_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_PACKET_SIZE 1400 // Typical MTU is 1500, leave room for headers
#define MAX_FRAGMENT_SIZE (MAX_PACKET_SIZE - sizeof(PacketHeader))
#define MAX_ORIGINAL_PACKET_SIZE (MAX_FRAGMENT_SIZE * MAX_FRAGMENTS_PER_PACKET)

typedef struct {
  uint8_t buffer[MAX_PACKET_SIZE];
  uint16_t write_pos;
  uint16_t read_pos;
  uint16_t length;
} Packet;

// Fragmentation and compression functions
typedef struct {
    Packet fragments[MAX_FRAGMENTS_PER_PACKET];
    uint16_t fragment_count;
    uint16_t expected_fragments;
    uint32_t original_size;
    uint8_t type;
    uint8_t flags;
    uint16_t sequence;
    uint32_t timestamp;
    bool is_complete;
} PacketAssembler;

// Compression types
typedef enum {
    COMPRESSION_NONE = 0,
    COMPRESSION_LZ4 = 1,
    COMPRESSION_ZSTD = 2
} CompressionType;

// Fragmentation functions
uint16_t packet_fragment_data(const void* data, uint32_t data_size, Packet* fragments, 
                           uint16_t sequence, uint32_t timestamp);
bool packet_assemble_fragment(PacketAssembler* assembler, const Packet* fragment);
void packet_init_assembler(PacketAssembler* assembler, uint16_t expected_fragments);
bool packet_is_assembler_complete(const PacketAssembler* assembler);

// Compression functions
uint32_t packet_compress_data(const void* input, uint32_t input_size, void* output, 
                           uint32_t output_size, CompressionType type);
uint32_t packet_decompress_data(const void* input, uint32_t input_size, void* output, 
                             uint32_t output_size, CompressionType type);
CompressionType packet_get_best_compression(uint32_t data_size);

// Initialize a packet for writing
void packet_init_write(Packet *packet, uint8_t type, uint8_t flags);

// Initialize a packet for reading from a raw buffer
// Returns false if buffer is too small for a header
bool packet_init_read(Packet *packet, const void *data, uint16_t size);

// Write functions
bool packet_write_u8(Packet *packet, uint8_t value);
bool packet_write_u16(Packet *packet, uint16_t value);
bool packet_write_u32(Packet *packet, uint32_t value);
bool packet_write_u64(Packet *packet, uint64_t value);
bool packet_write_i8(Packet *packet, int8_t value);
bool packet_write_i16(Packet *packet, int16_t value);
bool packet_write_i32(Packet *packet, int32_t value);
bool packet_write_float(Packet *packet, float value);
bool packet_write_string(Packet *packet, const char *str);
bool packet_write_bytes(Packet *packet, const void *data, uint16_t size);

// Read functions
bool packet_read_u8(Packet *packet, uint8_t *value);
bool packet_read_u16(Packet *packet, uint16_t *value);
bool packet_read_u32(Packet *packet, uint32_t *value);
bool packet_read_u64(Packet *packet, uint64_t *value);
bool packet_read_i8(Packet *packet, int8_t *value);
bool packet_read_i16(Packet *packet, int16_t *value);
bool packet_read_i32(Packet *packet, int32_t *value);
bool packet_read_float(Packet *packet, float *value);
bool packet_read_string(Packet *packet, char *buffer, uint16_t buffer_size);
bool packet_read_bytes(Packet *packet, void *buffer, uint16_t size);

// Header access
PacketHeader packet_get_header(const Packet *packet);

// Finalize packet for sending (updates length and data size)
uint16_t packet_finalize(Packet *packet);

#endif // PACKET_H
