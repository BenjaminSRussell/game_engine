// packet.c - Reliable packet serialization system
// TODO: MVP PATH - Implement packet fragmentation for large payloads
// TODO: MVP PATH - Add packet compression (LZ4, Zstd) for bandwidth optimization
// TODO: MVP PATH - Implement packet prioritization (critical vs non-critical data)
// TODO: MVP PATH - Add packet encryption and authentication (AES-GCM, HMAC)
// TODO: MVP PATH - Implement adaptive MTU discovery for optimal packet sizes
#include "include/network/packet.h"
#include "include/core/logger.h"
#include <string.h>
#include <stdlib.h>

static bool packet_has_space(const Packet *packet, uint16_t size) {
    return packet->write_pos + size <= MAX_PACKET_SIZE;
}

static bool packet_has_data(const Packet *packet, uint16_t size) {
    return packet->read_pos + size <= packet->length;
}

void packet_init_write(Packet *packet, uint8_t type, uint8_t flags) {
    if (!packet) return;
    
    memset(packet, 0, sizeof(Packet));
    packet->write_pos = sizeof(PacketHeader);
    
    // Write header
    PacketHeader header = {0};
    header.type = type;
    header.flags = flags;
    header.sequence = 0; // Will be set by network layer
    header.timestamp = 0; // Will be set by network layer
    
    memcpy(packet->buffer, &header, sizeof(PacketHeader));
}

bool packet_init_read(Packet *packet, const void *data, uint16_t size) {
    if (!packet || !data || size < sizeof(PacketHeader)) {
        return false;
    }
    
    memset(packet, 0, sizeof(Packet));
    memcpy(packet->buffer, data, size);
    packet->length = size;
    packet->read_pos = sizeof(PacketHeader);
    
    return true;
}

bool packet_write_u8(Packet *packet, uint8_t value) {
    if (!packet || !packet_has_space(packet, 1)) {
        return false;
    }
    
    packet->buffer[packet->write_pos++] = value;
    return true;
}

bool packet_write_u16(Packet *packet, uint16_t value) {
    if (!packet || !packet_has_space(packet, 2)) {
        return false;
    }
    
    // Write in network byte order
    packet->buffer[packet->write_pos++] = (value >> 8) & 0xFF;
    packet->buffer[packet->write_pos++] = value & 0xFF;
    return true;
}

bool packet_write_u32(Packet *packet, uint32_t value) {
    if (!packet || !packet_has_space(packet, 4)) {
        return false;
    }
    
    // Write in network byte order
    packet->buffer[packet->write_pos++] = (value >> 24) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 16) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 8) & 0xFF;
    packet->buffer[packet->write_pos++] = value & 0xFF;
    return true;
}

bool packet_write_u64(Packet *packet, uint64_t value) {
    if (!packet || !packet_has_space(packet, 8)) {
        return false;
    }
    
    // Write in network byte order
    packet->buffer[packet->write_pos++] = (value >> 56) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 48) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 40) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 32) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 24) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 16) & 0xFF;
    packet->buffer[packet->write_pos++] = (value >> 8) & 0xFF;
    packet->buffer[packet->write_pos++] = value & 0xFF;
    return true;
}

bool packet_write_i8(Packet *packet, int8_t value) {
    return packet_write_u8(packet, (uint8_t)value);
}

bool packet_write_i16(Packet *packet, int16_t value) {
    return packet_write_u16(packet, (uint16_t)value);
}

bool packet_write_i32(Packet *packet, int32_t value) {
    return packet_write_u32(packet, (uint32_t)value);
}

bool packet_write_float(Packet *packet, float value) {
    union {
        float f;
        uint32_t u;
    } converter;
    converter.f = value;
    return packet_write_u32(packet, converter.u);
}

bool packet_write_string(Packet *packet, const char *str) {
    if (!packet || !str) {
        return false;
    }
    
    uint16_t len = strlen(str);
    if (len > 255) { // Limit string length
        len = 255;
    }
    
    if (!packet_has_space(packet, 1 + len)) {
        return false;
    }
    
    packet_write_u8(packet, (uint8_t)len);
    memcpy(&packet->buffer[packet->write_pos], str, len);
    packet->write_pos += len;
    
    return true;
}

bool packet_write_bytes(Packet *packet, const void *data, uint16_t size) {
    if (!packet || !data || size == 0) {
        return false;
    }
    
    if (!packet_has_space(packet, 2 + size)) {
        return false;
    }
    
    packet_write_u16(packet, size);
    memcpy(&packet->buffer[packet->write_pos], data, size);
    packet->write_pos += size;
    
    return true;
}

bool packet_read_u8(Packet *packet, uint8_t *value) {
    if (!packet || !value || !packet_has_data(packet, 1)) {
        return false;
    }
    
    *value = packet->buffer[packet->read_pos++];
    return true;
}

bool packet_read_u16(Packet *packet, uint16_t *value) {
    if (!packet || !value || !packet_has_data(packet, 2)) {
        return false;
    }
    
    // Read in network byte order
    *value = ((uint16_t)packet->buffer[packet->read_pos] << 8) |
             (uint16_t)packet->buffer[packet->read_pos + 1];
    packet->read_pos += 2;
    return true;
}

bool packet_read_u32(Packet *packet, uint32_t *value) {
    if (!packet || !value || !packet_has_data(packet, 4)) {
        return false;
    }
    
    // Read in network byte order
    *value = ((uint32_t)packet->buffer[packet->read_pos] << 24) |
             ((uint32_t)packet->buffer[packet->read_pos + 1] << 16) |
             ((uint32_t)packet->buffer[packet->read_pos + 2] << 8) |
             (uint32_t)packet->buffer[packet->read_pos + 3];
    packet->read_pos += 4;
    return true;
}

bool packet_read_u64(Packet *packet, uint64_t *value) {
    if (!packet || !value || !packet_has_data(packet, 8)) {
        return false;
    }
    
    // Read in network byte order
    *value = ((uint64_t)packet->buffer[packet->read_pos] << 56) |
             ((uint64_t)packet->buffer[packet->read_pos + 1] << 48) |
             ((uint64_t)packet->buffer[packet->read_pos + 2] << 40) |
             ((uint64_t)packet->buffer[packet->read_pos + 3] << 32) |
             ((uint64_t)packet->buffer[packet->read_pos + 4] << 24) |
             ((uint64_t)packet->buffer[packet->read_pos + 5] << 16) |
             ((uint64_t)packet->buffer[packet->read_pos + 6] << 8) |
             (uint64_t)packet->buffer[packet->read_pos + 7];
    packet->read_pos += 8;
    return true;
}

bool packet_read_i8(Packet *packet, int8_t *value) {
    uint8_t uval;
    if (!packet_read_u8(packet, &uval)) {
        return false;
    }
    *value = (int8_t)uval;
    return true;
}

bool packet_read_i16(Packet *packet, int16_t *value) {
    uint16_t uval;
    if (!packet_read_u16(packet, &uval)) {
        return false;
    }
    *value = (int16_t)uval;
    return true;
}

bool packet_read_i32(Packet *packet, int32_t *value) {
    uint32_t uval;
    if (!packet_read_u32(packet, &uval)) {
        return false;
    }
    *value = (int32_t)uval;
    return true;
}

bool packet_read_float(Packet *packet, float *value) {
    uint32_t uval;
    if (!packet_read_u32(packet, &uval)) {
        return false;
    }
    
    union {
        float f;
        uint32_t u;
    } converter;
    converter.u = uval;
    *value = converter.f;
    return true;
}

bool packet_read_string(Packet *packet, char *buffer, uint16_t buffer_size) {
    if (!packet || !buffer || buffer_size == 0) {
        return false;
    }
    
    uint8_t len;
    if (!packet_read_u8(packet, &len) || !packet_has_data(packet, len)) {
        return false;
    }
    
    uint16_t copy_len = (len < buffer_size - 1) ? len : buffer_size - 1;
    memcpy(buffer, &packet->buffer[packet->read_pos], copy_len);
    buffer[copy_len] = '\0';
    packet->read_pos += len;
    
    return true;
}

bool packet_read_bytes(Packet *packet, void *buffer, uint16_t size) {
    if (!packet || !buffer || size == 0) {
        return false;
    }
    
    uint16_t data_size;
    if (!packet_read_u16(packet, &data_size) || !packet_has_data(packet, data_size)) {
        return false;
    }
    
    uint16_t copy_len = (data_size < size) ? data_size : size;
    memcpy(buffer, &packet->buffer[packet->read_pos], copy_len);
    packet->read_pos += data_size;
    
    return true;
}

PacketHeader packet_get_header(const Packet *packet) {
    PacketHeader header = {0};
    if (!packet) {
        return header;
    }
    
    memcpy(&header, packet->buffer, sizeof(PacketHeader));
    return header;
}

uint16_t packet_finalize(Packet *packet) {
    if (!packet) {
        return 0;
    }
    
    // Update data size in header
    PacketHeader *header = (PacketHeader*)packet->buffer;
    header->data_size = packet->write_pos - sizeof(PacketHeader);
    
    packet->length = packet->write_pos;
    return packet->length;
}

// TODO: MVP PATH - Add packet validation and checksum verification
// TODO: MVP PATH - Implement packet versioning for backward compatibility
// TODO: MVP PATH - Add packet pooling for memory efficiency
// TODO: MVP PATH - Implement packet batching for multiple small messages
