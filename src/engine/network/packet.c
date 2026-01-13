// packet.c - Reliable packet serialization system
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
    
    if (size > MAX_PACKET_SIZE) {
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

bool packet_read_fragment_header(Packet *packet, FragmentHeader *header) {
    if (!packet || !header) return false;

    PacketHeader ph = packet_get_header(packet);
    if (!(ph.flags & NET_PACKET_FLAG_FRAGMENT)) return false;

    uint16_t saved_pos = packet->read_pos;
    bool result = true;

    if (!packet_read_u16(packet, &header->packet_id) ||
        !packet_read_u16(packet, &header->fragment_index) ||
        !packet_read_u16(packet, &header->fragment_count) ||
        !packet_read_u32(packet, &header->total_size)) {
        result = false;
    }

    packet->read_pos = saved_pos;
    return result;
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

uint16_t packet_fragment_data(const void *data, uint32_t size, uint8_t type, uint8_t flags, Packet *fragments, uint16_t max_fragments) {
    if (!data || size == 0 || !fragments || max_fragments == 0) {
        return 0;
    }

    uint32_t header_size = sizeof(PacketHeader);
    uint32_t fragment_header_size = sizeof(FragmentHeader);
    uint32_t max_payload_per_fragment = MAX_PACKET_SIZE - header_size - fragment_header_size;

    uint32_t needed_fragments = (size + max_payload_per_fragment - 1) / max_payload_per_fragment;

    if (needed_fragments > max_fragments) {
        LOG_ERROR(LOG_CAT_NETWORK, "packet_fragment: buffer too small, needed %u fragments, got %u", needed_fragments, max_fragments);
        return 0;
    }

    static uint16_t next_packet_id = 1;
    uint16_t packet_id = next_packet_id++;

    uint32_t offset = 0;
    for (uint16_t i = 0; i < needed_fragments; i++) {
        Packet *p = &fragments[i];

        // Init packet with FRAGMENT flag
        packet_init_write(p, type, flags | NET_PACKET_FLAG_FRAGMENT);

        uint32_t chunk_size = size - offset;
        if (chunk_size > max_payload_per_fragment) {
            chunk_size = max_payload_per_fragment;
        }

        // Write fragment header
        FragmentHeader fh;
        fh.packet_id = packet_id;
        fh.fragment_index = i;
        fh.fragment_count = (uint16_t)needed_fragments;
        fh.total_size = size;

        // Manual write of struct members to ensure endianness/packing
        packet_write_u16(p, fh.packet_id);
        packet_write_u16(p, fh.fragment_index);
        packet_write_u16(p, fh.fragment_count);
        packet_write_u32(p, fh.total_size);

        // Write data
        memcpy(&p->buffer[p->write_pos], (const uint8_t*)data + offset, chunk_size);
        p->write_pos += chunk_size;

        packet_finalize(p);

        offset += chunk_size;
    }

    return (uint16_t)needed_fragments;
}

bool packet_reassemble_fragment(FragmentBuffer *buffer, const Packet *fragment) {
    if (!buffer || !fragment) return false;

    FragmentHeader fh;
    if (!packet_read_fragment_header((Packet*)fragment, &fh)) return false;

    // Check if new packet ID
    if (!buffer->active || buffer->packet_id != fh.packet_id) {
        // Reset buffer
        buffer->active = true;
        buffer->packet_id = fh.packet_id;
        buffer->total_fragments = fh.fragment_count;
        buffer->total_size = fh.total_size;
        buffer->received_mask = 0;
        buffer->fragments_received = 0;
        buffer->timeout = 0.0f; // Caller manages timeout

        if (buffer->total_size > MAX_LARGE_PACKET_SIZE) {
            LOG_ERROR(LOG_CAT_NETWORK, "Reassembly: Packet too large (%u)", buffer->total_size);
            buffer->active = false;
            return false;
        }
    }

    uint32_t header_size = sizeof(PacketHeader) + sizeof(FragmentHeader);
    uint32_t payload_size = fragment->length - header_size;

    // Offset calculation
    uint32_t max_payload = MAX_PACKET_SIZE - sizeof(PacketHeader) - sizeof(FragmentHeader);
    uint32_t offset = fh.fragment_index * max_payload;

    if (offset + payload_size > buffer->total_size || offset + payload_size > MAX_LARGE_PACKET_SIZE) {
        LOG_ERROR(LOG_CAT_NETWORK, "Reassembly: Buffer overflow");
        return false;
    }

    memcpy(&buffer->buffer[offset], &fragment->buffer[header_size], payload_size);

    if (!(buffer->received_mask & (1 << fh.fragment_index))) {
        buffer->received_mask |= (1 << fh.fragment_index);
        buffer->fragments_received++;
    }

    return (buffer->fragments_received == buffer->total_fragments);
}

// Simple RLE compression
// Format:
// Control Byte (C):
// If C < 128: Literal run of length C+1. Followed by C+1 bytes.
// If C >= 128: Repeat run of length (C-128)+2. Followed by 1 byte.
bool packet_compress(Packet *packet) {
    if (!packet || (packet_get_header(packet).flags & NET_PACKET_FLAG_COMPRESSED)) {
        return false;
    }

    uint16_t header_size = sizeof(PacketHeader);
    uint8_t *payload = &packet->buffer[header_size];
    uint16_t payload_size = packet->length - header_size;

    if (payload_size == 0) return true;

    uint8_t *dest_buffer = malloc(MAX_PACKET_SIZE);
    if (!dest_buffer) return false;

    uint16_t read_idx = 0;
    uint16_t write_idx = 0;

    while (read_idx < payload_size) {
        if (write_idx >= MAX_PACKET_SIZE) {
             free(dest_buffer);
             return false; // Expanded instead of compressed
        }

        // Look for repeat run
        uint16_t run_len = 1;
        while (read_idx + run_len < payload_size && run_len < 129 &&
               payload[read_idx + run_len] == payload[read_idx]) {
            run_len++;
        }

        if (run_len >= 2) {
             // Repeat run
             // (len - 2) + 128
             dest_buffer[write_idx++] = (uint8_t)((run_len - 2) + 128);
             dest_buffer[write_idx++] = payload[read_idx];
             read_idx += run_len;
        } else {
             // Literal run
             uint16_t lit_len = 0;
             while (read_idx + lit_len < payload_size && lit_len < 128) {
                  // Check if a repeat run starts here (greedy)
                  if (read_idx + lit_len + 1 < payload_size &&
                      payload[read_idx + lit_len] == payload[read_idx + lit_len + 1]) {
                      break;
                  }
                  lit_len++;
             }

             dest_buffer[write_idx++] = (uint8_t)(lit_len - 1);
             memcpy(&dest_buffer[write_idx], &payload[read_idx], lit_len);
             write_idx += lit_len;
             read_idx += lit_len;
        }
    }

    // Check if compression actually saved space
    if (write_idx < payload_size) {
        memcpy(payload, dest_buffer, write_idx);
        packet->write_pos = header_size + write_idx;

        // Update header flag
        PacketHeader *header = (PacketHeader*)packet->buffer;
        header->flags |= NET_PACKET_FLAG_COMPRESSED;

        packet_finalize(packet);
        free(dest_buffer);
        return true;
    }

    free(dest_buffer);
    return false;
}

bool packet_decompress(Packet *packet) {
    if (!packet || !(packet_get_header(packet).flags & NET_PACKET_FLAG_COMPRESSED)) {
        return false;
    }

    uint16_t header_size = sizeof(PacketHeader);
    uint8_t *src = &packet->buffer[header_size];
    uint16_t src_size = packet->length - header_size;

    uint8_t *dest_buffer = malloc(MAX_PACKET_SIZE * 2); // Allow expansion (safe upper bound? usually MAX_PACKET_SIZE is limit)
    // Actually we should just decompress to max packet size.
    if (!dest_buffer) return false;

    uint16_t read_idx = 0;
    uint16_t write_idx = 0;

    while (read_idx < src_size) {
        uint8_t ctrl = src[read_idx++];

        if (ctrl >= 128) {
            // Repeat run
            uint16_t len = (ctrl - 128) + 2;
            uint8_t val = src[read_idx++];

            if (write_idx + len > MAX_PACKET_SIZE * 2) {
                 free(dest_buffer);
                 return false; // Overflow
            }
            memset(&dest_buffer[write_idx], val, len);
            write_idx += len;
        } else {
            // Literal run
            uint16_t len = ctrl + 1;
            if (read_idx + len > src_size) {
                 free(dest_buffer);
                 return false; // Buffer overrun
            }
            if (write_idx + len > MAX_PACKET_SIZE * 2) {
                 free(dest_buffer);
                 return false; // Overflow
            }
            memcpy(&dest_buffer[write_idx], &src[read_idx], len);
            read_idx += len;
            write_idx += len;
        }
    }

    if (write_idx + header_size > MAX_PACKET_SIZE) {
        // Decompressed data too large for packet
        // This is tricky. If we just modify packet, it must fit in MAX_PACKET_SIZE.
        // If it was originally fragmented then compressed, decompressed fragments fit.
        // If it was compressed then fragmented, we decompress fragments? No, usually fragment then compress.
        // Or compress then fragment.
        // If we compressed a single packet, it should decompress to <= MAX_PACKET_SIZE if valid.
        // But RLE can act weird. Assuming valid input.
        // We will cap at MAX_PACKET_SIZE for now.
    }

    if (write_idx + header_size <= MAX_PACKET_SIZE) {
        memcpy(&packet->buffer[header_size], dest_buffer, write_idx);
        packet->write_pos = header_size + write_idx;

        PacketHeader *header = (PacketHeader*)packet->buffer;
        header->flags &= ~NET_PACKET_FLAG_COMPRESSED;

        packet_finalize(packet);
        free(dest_buffer);
        return true;
    }

    free(dest_buffer);
    return false; // Decompressed size too big
}
