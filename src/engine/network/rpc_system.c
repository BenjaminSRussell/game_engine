// rpc_system.c - Remote Procedure Call system for network communication
// TODO: MVP PATH - Add RPC authentication and authorization
// TODO: MVP PATH - Implement RPC rate limiting and throttling
// TODO: MVP PATH - Add RPC response handling (async callbacks)
// TODO: MVP PATH - Implement RPC versioning and compatibility checking
#include "include/network/rpc_system.h"
#include "include/core/logger.h"
#include "include/network/packet.h"
#include "include/network/network_manager.h"
#include <stdlib.h>
#include <string.h>

#define MAX_RPC_FUNCTIONS 128
#define RPC_NAME_MAX_LENGTH 64
#define MAX_PENDING_RPCS 256
#define RPC_TIMEOUT 1.0f // seconds
#define RPC_MAX_RETRIES 5

typedef struct {
    char name[RPC_NAME_MAX_LENGTH];
    RPCHandler handler;
    bool active;
} RPCFunction;

typedef struct {
    uint16_t rpc_id;
    uint32_t target_client_id; // 0 usually implies server if we are client, but can be specific
    // Note: network_client_send doesn't take ID, it sends to server.
    // network_server_send_to_client takes ID.
    // We need to know if we are server or client to resend correctly?
    // Or just store the packet data and the function to call to resend?
    // Simpler: Store the packet buffer.
    uint8_t packet_buffer[MAX_PACKET_SIZE];
    uint16_t packet_size;
    float time_since_sent;
    int retries;
    bool active;
    bool is_broadcast; // If broadcast, we might not expect ACKs or expect multiple?
                       // Reliability for broadcast is hard. We'll skip reliability for broadcast for now
                       // or just assume best effort.
} PendingRPC;

static RPCFunction rpc_functions[MAX_RPC_FUNCTIONS];
static uint32_t rpc_count = 0;
static PendingRPC pending_rpcs[MAX_PENDING_RPCS];
static uint16_t next_rpc_id = 1;

void rpc_init(void) {
    memset(rpc_functions, 0, sizeof(rpc_functions));
    rpc_count = 0;
    memset(pending_rpcs, 0, sizeof(pending_rpcs));
    next_rpc_id = 1;
    LOG_INFO(LOG_CAT_NETWORK, "RPC system initialized");
}

void rpc_shutdown(void) {
    memset(rpc_functions, 0, sizeof(rpc_functions));
    rpc_count = 0;
    memset(pending_rpcs, 0, sizeof(pending_rpcs));
    LOG_INFO(LOG_CAT_NETWORK, "RPC system shutdown");
}

void rpc_register(const char *name, RPCHandler handler) {
    if (!name || !handler) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC register: invalid parameters");
        return;
    }
    
    if (rpc_count >= MAX_RPC_FUNCTIONS) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC register: maximum RPC functions reached");
        return;
    }
    
    // Check if already registered
    for (uint32_t i = 0; i < rpc_count; i++) {
        if (strcmp(rpc_functions[i].name, name) == 0) {
            LOG_WARN(LOG_CAT_NETWORK, "RPC register: function '%s' already registered, updating handler", name);
            rpc_functions[i].handler = handler;
            return;
        }
    }
    
    // Register new function
    strncpy(rpc_functions[rpc_count].name, name, RPC_NAME_MAX_LENGTH - 1);
    rpc_functions[rpc_count].name[RPC_NAME_MAX_LENGTH - 1] = '\0';
    rpc_functions[rpc_count].handler = handler;
    rpc_functions[rpc_count].active = true;
    rpc_count++;
    
    LOG_INFO(LOG_CAT_NETWORK, "RPC registered: %s", name);
}

bool rpc_dispatch(const char *name, uint32_t sender_id, void *data, uint32_t size) {
    if (!name) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC dispatch: invalid name");
        return false;
    }
    
    // Find the RPC function
    RPCHandler handler = NULL;
    for (uint32_t i = 0; i < rpc_count; i++) {
        if (strcmp(rpc_functions[i].name, name) == 0 && rpc_functions[i].active) {
            handler = rpc_functions[i].handler;
            break;
        }
    }
    
    if (!handler) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC dispatch: function '%s' not found", name);
        return false;
    }
    
    // Call the handler
    handler(sender_id, data, size);
    
    LOG_DEBUG(LOG_CAT_NETWORK, "RPC dispatched: %s from client %u", name, sender_id);
    return true;
}

static void add_pending_rpc(uint16_t rpc_id, uint32_t target_client_id, const void *packet_data, uint16_t packet_size, bool is_broadcast) {
    if (is_broadcast) return; // Don't track broadcasts for retry

    for (int i = 0; i < MAX_PENDING_RPCS; i++) {
        if (!pending_rpcs[i].active) {
            pending_rpcs[i].active = true;
            pending_rpcs[i].rpc_id = rpc_id;
            pending_rpcs[i].target_client_id = target_client_id;
            if (packet_size <= MAX_PACKET_SIZE) {
                memcpy(pending_rpcs[i].packet_buffer, packet_data, packet_size);
                pending_rpcs[i].packet_size = packet_size;
            }
            pending_rpcs[i].time_since_sent = 0;
            pending_rpcs[i].retries = 0;
            pending_rpcs[i].is_broadcast = is_broadcast;
            return;
        }
    }
    LOG_WARN(LOG_CAT_NETWORK, "RPC Pending queue full, reliability not guaranteed for RPC %u", rpc_id);
}

static void remove_pending_rpc(uint16_t rpc_id) {
    for (int i = 0; i < MAX_PENDING_RPCS; i++) {
        if (pending_rpcs[i].active && pending_rpcs[i].rpc_id == rpc_id) {
            pending_rpcs[i].active = false;
            return;
        }
    }
}

void rpc_send(const char *name, void *data, uint32_t size) {
    if (!name) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC send: invalid name");
        return;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC, 0);
    
    uint16_t rpc_id = next_rpc_id++;

    // Write RPC ID, Name, Data
    if (!packet_write_u16(&packet, rpc_id) ||
        !packet_write_string(&packet, name) ||
        !packet_write_bytes(&packet, data, size)) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC send: failed to serialize packet");
        return;
    }
    
    // Finalize and send
    uint16_t packet_size = packet_finalize(&packet);
    if (network_client_send(PACKET_TYPE_RPC, packet.buffer, packet_size) != 0) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC send: failed to send packet");
        return;
    }
    
    add_pending_rpc(rpc_id, 0, packet.buffer, packet_size, false);

    LOG_DEBUG(LOG_CAT_NETWORK, "RPC sent: %s (%u bytes, ID: %u)", name, size, rpc_id);
}

// Server-side RPC broadcast to all clients
void rpc_broadcast(const char *name, void *data, uint32_t size) {
    if (!name) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC broadcast: invalid name");
        return;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC, 0);
    
    uint16_t rpc_id = next_rpc_id++; // Helper ID for tracking if needed, but broadcasts are not tracked

    // Write RPC ID, Name, Data
    if (!packet_write_u16(&packet, rpc_id) ||
        !packet_write_string(&packet, name) ||
        !packet_write_bytes(&packet, data, size)) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC broadcast: failed to serialize packet");
        return;
    }
    
    // Finalize and broadcast
    uint16_t packet_size = packet_finalize(&packet);
    if (network_server_broadcast(PACKET_TYPE_RPC, packet.buffer, packet_size) != 0) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC broadcast: failed to broadcast packet");
        return;
    }
    
    // No tracking for broadcast
    LOG_DEBUG(LOG_CAT_NETWORK, "RPC broadcast: %s (%u bytes, ID: %u)", name, size, rpc_id);
}

// Server-side RPC send to specific client
void rpc_send_to_client(uint32_t client_id, const char *name, void *data, uint32_t size) {
    if (!name) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC send_to_client: invalid name");
        return;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC, 0);
    
    uint16_t rpc_id = next_rpc_id++;

    // Write RPC ID, Name, Data
    if (!packet_write_u16(&packet, rpc_id) ||
        !packet_write_string(&packet, name) ||
        !packet_write_bytes(&packet, data, size)) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC send_to_client: failed to serialize packet");
        return;
    }
    
    // Finalize and send
    uint16_t packet_size = packet_finalize(&packet);
    if (network_server_send_to_client(client_id, PACKET_TYPE_RPC, packet.buffer, packet_size) != 0) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC send_to_client: failed to send packet to client %u", client_id);
        return;
    }
    
    add_pending_rpc(rpc_id, client_id, packet.buffer, packet_size, false);

    LOG_DEBUG(LOG_CAT_NETWORK, "RPC sent to client %u: %s (%u bytes, ID: %u)", client_id, name, size, rpc_id);
}

static void send_ack(uint32_t recipient_id, uint16_t rpc_id) {
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC_ACK, 0);
    packet_write_u16(&packet, rpc_id);
    uint16_t packet_size = packet_finalize(&packet);

    // If recipient_id is 0, we are Client receiving from Server. We should reply to Server.
    // If recipient_id > 0, we are Server receiving from Client. We should reply to Client.

    if (recipient_id == 0) {
        // Reply to server
        network_client_send(PACKET_TYPE_RPC_ACK, packet.buffer, packet_size);
    } else {
        // Reply to client
        network_server_send_to_client(recipient_id, PACKET_TYPE_RPC_ACK, packet.buffer, packet_size);
    }
}

// Process incoming RPC packet
void rpc_process_packet(uint32_t sender_id, const void *packet_data, uint16_t packet_size, uint16_t *last_processed_id) {
    if (!packet_data || packet_size < sizeof(PacketHeader)) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC process: invalid packet data");
        return;
    }
    
    // Read packet
    Packet packet;
    if (!packet_init_read(&packet, packet_data, packet_size)) {
        LOG_ERROR(LOG_CAT_NETWORK, "RPC process: failed to read packet");
        return;
    }
    
    PacketHeader header = packet_get_header(&packet);
    
    if (header.type == PACKET_TYPE_RPC) {
        uint16_t rpc_id;
        if (!packet_read_u16(&packet, &rpc_id)) {
            LOG_ERROR(LOG_CAT_NETWORK, "RPC process: failed to read RPC ID");
            return;
        }

        // Duplicate detection
        if (last_processed_id) {
            // Simple duplicate check (handles retransmission of same ID)
            // TODO: Better handling of out-of-order and wrap-around
            if (*last_processed_id == rpc_id) {
                LOG_DEBUG(LOG_CAT_NETWORK, "Ignoring duplicate RPC %u from %u", rpc_id, sender_id);
                send_ack(sender_id, rpc_id);
                return;
            }
        }

        // Read RPC name
        char rpc_name[RPC_NAME_MAX_LENGTH];
        if (!packet_read_string(&packet, rpc_name, sizeof(rpc_name))) {
            LOG_ERROR(LOG_CAT_NETWORK, "RPC process: failed to read RPC name");
            return;
        }

        // Read RPC data
        uint8_t rpc_data[1024];
        uint16_t rpc_data_size;
        // The data is the rest of the packet or prefixed?
        // In rpc_send: packet_write_bytes which writes [size][data].

        // Let's check packet_write_bytes implementation.
        // It writes u16 size, then data.
        // So packet_read_bytes should work.

        // Wait, packet_read_bytes takes a size argument for buffer size, but implementation reads size from packet.
        // packet_read_bytes implementation:
        // packet_read_u16(packet, &data_size)

        // So we don't need to read size manually first.

        // However, I used packet_read_bytes which reads into buffer.

        if (!packet_read_bytes(&packet, rpc_data, sizeof(rpc_data))) {
            LOG_ERROR(LOG_CAT_NETWORK, "RPC process: failed to read RPC data");
            return;
        }

        // Send ACK
        send_ack(sender_id, rpc_id);

        if (last_processed_id) *last_processed_id = rpc_id;

        // Dispatch the RPC
        // The read data size is not exposed by packet_read_bytes directly (it returns bool).
        // But we need the actual data size for dispatch.
        // I need to peek the size or modify packet_read_bytes or rely on packet structure.
        // packet_read_bytes reads a u16 length first.
        // I should probably manually read u16 size then read bytes.
        // packet_read_bytes does exactly that. But doesn't tell us how many bytes read (it copies up to buffer_size).

        // Re-implement reading here to get size:
        // Actually packet_read_bytes advances read_pos.
        // If I use a helper:

        // Let's adjust reading:
        // I can't easily get the size from packet_read_bytes.
        // I'll manually read:
        // rewind? No.

        // Better: use packet_read_u16 for size, then packet_read_raw_bytes?
        // packet.c doesn't expose packet_read_raw_bytes (no size prefix).
        // packet_read_bytes expects the size prefix in the stream.

        // I'll use the fact that I know the format.
        // [ID] [Name] [Size] [Data]
        // I already read ID, Name.
        // Now read Size.
        // But wait, packet_read_bytes reads Size then Data.
        // So I can't use it if I want to know Size.
        // Or I can modify packet.h/c to return size.

        // Alternative: Use a temporary packet or peek.
        // Or just modify rpc_send to NOT use packet_write_bytes but write size then data manually?
        // packet_write_bytes does exactly that.

        // I will implement manual read here.
        // Access packet internal buffer? It's exposed in struct.
        // packet->read_pos points to [Size].

        uint16_t data_len;
        // Peek size
        uint16_t saved_pos = packet.read_pos;
        if (!packet_read_u16(&packet, &data_len)) return;
        packet.read_pos = saved_pos; // Rewind

        if (data_len > sizeof(rpc_data)) {
            LOG_ERROR(LOG_CAT_NETWORK, "RPC data too large");
            return;
        }

        if (!packet_read_bytes(&packet, rpc_data, sizeof(rpc_data))) {
             return;
        }

        rpc_dispatch(rpc_name, sender_id, rpc_data, data_len);

    } else if (header.type == PACKET_TYPE_RPC_ACK) {
        uint16_t rpc_id;
        if (!packet_read_u16(&packet, &rpc_id)) {
            return;
        }
        remove_pending_rpc(rpc_id);
        LOG_DEBUG(LOG_CAT_NETWORK, "RPC ACK received for ID: %u", rpc_id);
    }
}

void rpc_update(float delta_time) {
    for (int i = 0; i < MAX_PENDING_RPCS; i++) {
        if (pending_rpcs[i].active) {
            pending_rpcs[i].time_since_sent += delta_time;
            if (pending_rpcs[i].time_since_sent >= RPC_TIMEOUT) {
                if (pending_rpcs[i].retries < RPC_MAX_RETRIES) {
                    pending_rpcs[i].retries++;
                    pending_rpcs[i].time_since_sent = 0;

                    // Resend
                    if (pending_rpcs[i].target_client_id == 0) {
                        // Resend to server
                        network_client_send(PACKET_TYPE_RPC, pending_rpcs[i].packet_buffer, pending_rpcs[i].packet_size);
                    } else {
                        // Resend to client
                        network_server_send_to_client(pending_rpcs[i].target_client_id, PACKET_TYPE_RPC, pending_rpcs[i].packet_buffer, pending_rpcs[i].packet_size);
                    }
                    LOG_WARN(LOG_CAT_NETWORK, "Resending RPC %u (Attempt %d)", pending_rpcs[i].rpc_id, pending_rpcs[i].retries + 1);
                } else {
                    LOG_ERROR(LOG_CAT_NETWORK, "RPC %u timed out after %d retries", pending_rpcs[i].rpc_id, RPC_MAX_RETRIES);
                    pending_rpcs[i].active = false;
                }
            }
        }
    }
}
