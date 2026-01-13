// packet.c - Reliable packet serialization system with fragmentation and compression
// TODO: MVP PATH - Implement packet prioritization (critical vs non-critical data)
// TODO: MVP PATH - Add packet encryption and authentication (AES-GCM, HMAC)
// TODO: MVP PATH - Implement adaptive MTU discovery for optimal packet sizes
#include "include/network/packet.h"
#include "include/core/logger.h"
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_LZ4
#include <lz4.h>
#endif

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

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

// Fragmentation implementation
uint16_t packet_fragment_data(const void* data, uint32_t data_size, Packet* fragments, 
                           uint16_t sequence, uint32_t timestamp) {
    if (!data || data_size == 0 || !fragments) {
        return 0;
    }
    
    // Don't fragment if data is small enough
    if (data_size <= MAX_FRAGMENT_SIZE) {
        return 0;
    }
    
    uint16_t fragment_count = (data_size + MAX_FRAGMENT_SIZE - 1) / MAX_FRAGMENT_SIZE;
    if (fragment_count > MAX_FRAGMENTS_PER_PACKET) {
        log_error("Data too large, would require %u fragments (max %u)", 
                 fragment_count, MAX_FRAGMENTS_PER_PACKET);
        return 0;
    }
    
    const uint8_t* src_data = (const uint8_t*)data;
    uint32_t offset = 0;
    
    for (uint16_t i = 0; i < fragment_count; i++) {
        // Initialize fragment packet
        packet_init_write(&fragments[i], 0, PACKET_FLAG_FRAGMENTED);
        
        // Calculate fragment size
        uint32_t fragment_size = (data_size - offset > MAX_FRAGMENT_SIZE) ? 
                                  MAX_FRAGMENT_SIZE : (data_size - offset);
        
        // Write fragment data
        if (!packet_write_bytes(&fragments[i], src_data + offset, fragment_size)) {
            log_error("Failed to write fragment %u data", i);
            return 0;
        }
        
        // Update header with fragmentation info
        PacketHeader* header = (PacketHeader*)fragments[i].buffer;
        header->type = 0; // Will be set by caller
        header->sequence = sequence;
        header->timestamp = timestamp;
        header->fragment_id = i + 1; // 1-based fragment ID
        header->fragment_offset = offset;
        header->total_fragments = fragment_count;
        header->original_size = data_size;
        
        // Finalize fragment
        fragments[i].length = packet_finalize(&fragments[i]);
        
        offset += fragment_size;
    }
    
    return fragment_count;
}

bool packet_assemble_fragment(PacketAssembler* assembler, const Packet* fragment) {
    if (!assembler || !fragment) {
        return false;
    }
    
    PacketHeader header = packet_get_header(fragment);
    
    // Check if this is a fragment
    if (!(header.flags & PACKET_FLAG_FRAGMENTED)) {
        return false;
    }
    
    // Validate fragment ID
    if (header.fragment_id == 0 || header.fragment_id > assembler->expected_fragments) {
        log_warn("Invalid fragment ID: %u (expected 1-%u)", 
                  header.fragment_id, assembler->expected_fragments);
        return false;
    }
    
    // Check if we already have this fragment
    if (assembler->fragments[header.fragment_id - 1].length > 0) {
        log_warn("Duplicate fragment %u", header.fragment_id);
        return false;
    }
    
    // Store fragment
    assembler->fragments[header.fragment_id - 1] = *fragment;
    assembler->fragment_count++;
    
    // Check if assembly is complete
    if (assembler->fragment_count == assembler->expected_fragments) {
        assembler->is_complete = true;
        return true;
    }
    
    return false;
}

void packet_init_assembler(PacketAssembler* assembler, uint16_t expected_fragments) {
    if (!assembler) return;
    
    memset(assembler, 0, sizeof(PacketAssembler));
    assembler->expected_fragments = expected_fragments;
}

bool packet_is_assembler_complete(const PacketAssembler* assembler) {
    return assembler ? assembler->is_complete : false;
}

// Compression implementation
CompressionType packet_get_best_compression(uint32_t data_size) {
    // For small data, compression might not be worth it
    if (data_size < 128) {
        return COMPRESSION_NONE;
    }
    
#ifdef HAVE_LZ4
    return COMPRESSION_LZ4;
#elif defined(HAVE_ZSTD)
    return COMPRESSION_ZSTD;
#else
    return COMPRESSION_NONE;
#endif
}

uint32_t packet_compress_data(const void* input, uint32_t input_size, void* output, 
                           uint32_t output_size, CompressionType type) {
    if (!input || input_size == 0 || !output || output_size == 0) {
        return 0;
    }
    
    switch (type) {
        case COMPRESSION_LZ4:
#ifdef HAVE_LZ4
        {
            int compressed_size = LZ4_compress_default((const char*)input, (int)input_size, 
                                                    (char*)output, (int)output_size);
            if (compressed_size < 0) {
                log_error("LZ4 compression failed");
                return 0;
            }
            return (uint32_t)compressed_size;
        }
#else
        {
            log_error("LZ4 compression not available");
            return 0;
        }
        
        case COMPRESSION_ZSTD:
#ifdef HAVE_ZSTD
        {
            size_t compressed_size = ZSTD_compress(output, output_size, input, input_size, 1);
            if (ZSTD_isError(compressed_size)) {
                log_error("ZSTD compression failed: %s", ZSTD_getErrorName(compressed_size));
                return 0;
            }
            return (uint32_t)compressed_size;
        }
#else
        {
            log_error("ZSTD compression not available");
            return 0;
        }
        
        case COMPRESSION_NONE:
        default:
            // No compression, just copy
            if (input_size > output_size) {
                return 0;
            }
            memcpy(output, input, input_size);
            return input_size;
    }
}

uint32_t packet_decompress_data(const void* input, uint32_t input_size, void* output, 
                             uint32_t output_size, CompressionType type) {
    if (!input || input_size == 0 || !output || output_size == 0) {
        return 0;
    }
    
    switch (type) {
        case COMPRESSION_LZ4:
#ifdef HAVE_LZ4
        {
            int decompressed_size = LZ4_decompress_safe((const char*)input, (int)input_size, 
                                                      (char*)output, (int)output_size);
            if (decompressed_size < 0) {
                log_error("LZ4 decompression failed");
                return 0;
            }
            return (uint32_t)decompressed_size;
        }
#else
        {
            log_error("LZ4 decompression not available");
            return 0;
        }
        
        case COMPRESSION_ZSTD:
#ifdef HAVE_ZSTD
        {
            size_t decompressed_size = ZSTD_decompress(output, output_size, input, input_size);
            if (ZSTD_isError(decompressed_size)) {
                log_error("ZSTD decompression failed: %s", ZSTD_getErrorName(decompressed_size));
                return 0;
            }
            return (uint32_t)decompressed_size;
        }
#else
        {
            log_error("ZSTD decompression not available");
            return 0;
        }
        
        case COMPRESSION_NONE:
        default:
            // No compression, just copy
            if (input_size > output_size) {
                return 0;
            }
            memcpy(output, input, input_size);
            return input_size;
    }
}

// TODO: MVP PATH - Add packet validation and checksum verification
// TODO: MVP PATH - Implement packet versioning for backward compatibility
// TODO: MVP PATH - Add packet pooling for memory efficiency
// TODO: MVP PATH - Implement packet batching for multiple small messages
