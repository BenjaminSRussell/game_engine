#ifndef PACKET_H
#define PACKET_H

#include "network_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_PACKET_SIZE 1400 // Typical MTU is 1500, leave room for headers

typedef struct {
  uint8_t buffer[MAX_PACKET_SIZE];
  uint16_t write_pos;
  uint16_t read_pos;
  uint16_t length;
} Packet;

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

#endif // PACKET_H
