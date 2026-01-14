#include "network/protocol/network_protocol.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define PROTOCOL_MAGIC_NUMBER 0x4E4554  // "NET"
#define MAX_FRAGMENTS 16
#define FRAGMENT_TIMEOUT 5.0f

// Fragment reassembly context
typedef struct {
    bool active;
    float start_time;
    uint32_t total_size;
    uint32_t received_size;
    uint16_t fragment_count;
    uint16_t fragments_received;
    uint8_t* fragments[MAX_FRAGMENTS];
    uint16_t fragment_sizes[MAX_FRAGMENTS];
    bool fragment_received[MAX_FRAGMENTS];
} FragmentContext;

static FragmentContext g_fragment_contexts[32];

NetworkProtocol* protocol_create(const ProtocolConfig* config, bool is_server) {
    NetworkProtocol* protocol = malloc(sizeof(NetworkProtocol));
    if (!protocol) {
        log_error("Failed to allocate network protocol");
        return NULL;
    }
    
    memset(protocol, 0, sizeof(NetworkProtocol));
    
    if (config) {
        protocol->config = *config;
    } else {
        // Default configuration
        protocol->config.port = is_server ? 7777 : 0;
        protocol->config.max_connections = is_server ? 64 : 1;
        protocol->config.heartbeat_interval = 1.0f;
        protocol->config.connection_timeout = 10.0f;
        protocol->config.rtt_smoothing_factor = 0.1f;
        protocol->config.enable_compression = false;
        protocol->config.enable_encryption = false;
    }
    
    protocol->max_connections = protocol->config.max_connections;
    protocol->connections = malloc(sizeof(NetworkConnection) * protocol->max_connections);
    if (!protocol->connections) {
        log_error("Failed to allocate connection array");
        free(protocol);
        return NULL;
    }
    
    memset(protocol->connections, 0, sizeof(NetworkConnection) * protocol->max_connections);
    protocol->is_server = is_server;
    protocol->next_sequence = 1;
    
    // Initialize fragment contexts
    memset(g_fragment_contexts, 0, sizeof(g_fragment_contexts));
    
    log_info("Network protocol created (server: %s, max_connections: %u)", 
             is_server ? "true" : "false", protocol->max_connections);
    return protocol;
}

void protocol_destroy(NetworkProtocol* protocol) {
    if (protocol) {
        // Free fragment contexts
        for (int i = 0; i < 32; i++) {
            FragmentContext* ctx = &g_fragment_contexts[i];
            if (ctx->active) {
                for (int j = 0; j < MAX_FRAGMENTS; j++) {
                    if (ctx->fragments[j]) {
                        free(ctx->fragments[j]);
                    }
                }
            }
        }
        
        free(protocol->connections);
        free(protocol);
        log_info("Network protocol destroyed");
    }
}

bool protocol_initialize(NetworkProtocol* protocol) {
    if (!protocol) return false;
    
    // Initialize socket
    // TODO: Implement actual socket initialization
    protocol->is_initialized = true;
    
    log_info("Network protocol initialized on port %u", protocol->config.port);
    return true;
}

void protocol_shutdown(NetworkProtocol* protocol) {
    if (!protocol) return;
    
    // Disconnect all clients
    for (uint32_t i = 0; i < protocol->max_connections; i++) {
        if (protocol->connections[i].state != CONN_DISCONNECTED) {
            protocol_remove_connection(protocol, i);
        }
    }
    
    protocol->is_initialized = false;
    log_info("Network protocol shutdown");
}

void protocol_update(NetworkProtocol* protocol, float current_time) {
    if (!protocol || !protocol->is_initialized) return;
    
    // Update connections and check timeouts
    for (uint32_t i = 0; i < protocol->max_connections; i++) {
        NetworkConnection* conn = &protocol->connections[i];
        if (conn->state == CONN_CONNECTED) {
            // Check heartbeat timeout
            if (current_time - conn->last_heartbeat_time > protocol->config.connection_timeout) {
                log_warn("Connection %u timed out", i);
                protocol_remove_connection(protocol, i);
            }
            // Send heartbeat if needed
            else if (current_time - conn->last_heartbeat_time > protocol->config.heartbeat_interval) {
                protocol_send_packet(protocol, i, MSG_HEARTBEAT, NULL, 0, PACKET_FLAG_RELIABLE);
                conn->last_heartbeat_time = current_time;
            }
        }
    }
    
    // Clean up old fragment contexts
    for (int i = 0; i < 32; i++) {
        FragmentContext* ctx = &g_fragment_contexts[i];
        if (ctx->active && (current_time - ctx->start_time) > FRAGMENT_TIMEOUT) {
            // Free fragments
            for (int j = 0; j < MAX_FRAGMENTS; j++) {
                if (ctx->fragments[j]) {
                    free(ctx->fragments[j]);
                    ctx->fragments[j] = NULL;
                }
            }
            ctx->active = false;
        }
    }
}

uint32_t protocol_add_connection(NetworkProtocol* protocol, const NetworkAddress* address) {
    if (!protocol || !address) return UINT32_MAX;
    
    // Find empty slot
    for (uint32_t i = 0; i < protocol->max_connections; i++) {
        NetworkConnection* conn = &protocol->connections[i];
        if (conn->state == CONN_DISCONNECTED) {
            conn->address = *address;
            conn->state = CONN_CONNECTED;
            conn->client_id = i;
            conn->last_heartbeat_time = 0.0f;
            conn->rtt = 0.0f;
            conn->packet_loss = 0.0f;
            conn->packets_sent = 0;
            conn->packets_received = 0;
            conn->packets_lost = 0;
            
            protocol->connection_count++;
            protocol->stats.connections_active++;
            
            log_info("Connection %u added from %u.%u.%u.%u:%u", i,
                     (address->address >> 24) & 0xFF,
                     (address->address >> 16) & 0xFF,
                     (address->address >> 8) & 0xFF,
                     address->address & 0xFF,
                     address->port);
            return i;
        }
    }
    
    log_warn("Failed to add connection - server full");
    return UINT32_MAX;
}

bool protocol_remove_connection(NetworkProtocol* protocol, uint32_t connection_id) {
    if (!protocol || connection_id >= protocol->max_connections) return false;
    
    NetworkConnection* conn = &protocol->connections[connection_id];
    if (conn->state == CONN_DISCONNECTED) return false;
    
    log_info("Connection %u removed", connection_id);
    
    conn->state = CONN_DISCONNECTED;
    protocol->connection_count--;
    protocol->stats.connections_active--;
    
    return true;
}

NetworkConnection* protocol_get_connection(NetworkProtocol* protocol, uint32_t connection_id) {
    if (!protocol || connection_id >= protocol->max_connections) return NULL;
    
    NetworkConnection* conn = &protocol->connections[connection_id];
    return (conn->state != CONN_DISCONNECTED) ? conn : NULL;
}

NetworkConnection* protocol_find_connection_by_address(NetworkProtocol* protocol, const NetworkAddress* address) {
    if (!protocol || !address) return NULL;
    
    for (uint32_t i = 0; i < protocol->max_connections; i++) {
        NetworkConnection* conn = &protocol->connections[i];
        if (conn->state != CONN_DISCONNECTED && address_equals(&conn->address, address)) {
            return conn;
        }
    }
    
    return NULL;
}

bool protocol_send_packet(NetworkProtocol* protocol, uint32_t connection_id, MessageType type, const void* data, uint16_t data_size, uint8_t flags) {
    if (!protocol || connection_id >= protocol->max_connections || data_size > MAX_PACKET_SIZE - sizeof(PacketHeader)) {
        return false;
    }
    
    NetworkConnection* conn = &protocol->connections[connection_id];
    if (conn->state != CONN_CONNECTED) return false;
    
    NetworkPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    // Fill header
    packet.header.protocol_version = NETWORK_PROTOCOL_VERSION;
    packet.header.message_type = type;
    packet.header.sequence_number = protocol->next_sequence++;
    packet.header.ack_number = conn->remote_sequence;
    packet.header.timestamp = (uint32_t)(time(NULL) * 1000);
    packet.header.flags = flags;
    packet.header.data_size = data_size;
    
    // Copy data
    if (data && data_size > 0) {
        memcpy(packet.data, data, data_size);
    }
    
    // Calculate checksum
    packet.header.checksum = calculate_checksum(&packet.header, sizeof(PacketHeader) - sizeof(uint16_t));
    packet.header.checksum = calculate_checksum(packet.data, data_size) ^ packet.header.checksum;
    
    packet.to_address = conn->address;
    
    // TODO: Actually send packet over socket
    conn->packets_sent++;
    protocol->stats.packets_sent++;
    protocol->stats.bytes_sent += sizeof(PacketHeader) + data_size;
    
    return true;
}

bool protocol_broadcast_packet(NetworkProtocol* protocol, MessageType type, const void* data, uint16_t data_size, uint8_t flags) {
    if (!protocol) return false;
    
    bool success = true;
    for (uint32_t i = 0; i < protocol->max_connections; i++) {
        NetworkConnection* conn = &protocol->connections[i];
        if (conn->state == CONN_CONNECTED) {
            if (!protocol_send_packet(protocol, i, type, data, data_size, flags)) {
                success = false;
            }
        }
    }
    
    return success;
}

bool protocol_receive_packet(NetworkProtocol* protocol, NetworkPacket* out_packet) {
    if (!protocol || !out_packet) return false;
    
    // TODO: Actually receive packet from socket
    // For now, return false (no packet received)
    return false;
}

void protocol_process_packet(NetworkProtocol* protocol, const NetworkPacket* packet) {
    if (!protocol || !packet) return;
    
    // Validate packet
    if (!validate_packet(packet)) {
        log_warn("Invalid packet received");
        return;
    }
    
    // Find connection
    NetworkConnection* conn = protocol_find_connection_by_address(protocol, &packet->from_address);
    if (!conn) {
        // New connection attempt
        if (packet->header.message_type == MSG_CONNECT && protocol->is_server) {
            uint32_t conn_id = protocol_add_connection(protocol, &packet->from_address);
            if (conn_id != UINT32_MAX) {
                conn = &protocol->connections[conn_id];
                protocol_send_packet(protocol, conn_id, MSG_CONNECT, NULL, 0, PACKET_FLAG_RELIABLE);
            }
        }
        return;
    }
    
    // Update connection state
    conn->packets_received++;
    conn->remote_sequence = packet->header.sequence_number;
    conn->last_heartbeat_time = packet->receive_time;
    
    // Update RTT
    if (packet->header.ack_number > 0) {
        // Simple RTT calculation
        // TODO: Implement proper RTT calculation with timestamps
        conn->rtt = conn->rtt * (1.0f - protocol->config.rtt_smoothing_factor) + 
                   0.1f * protocol->config.rtt_smoothing_factor;
    }
    
    protocol->stats.packets_received++;
    protocol->stats.bytes_received += sizeof(PacketHeader) + packet->header.data_size;
    
    // Handle fragmentation
    if (packet->header.flags & PACKET_FLAG_FRAGMENTED) {
        // TODO: Handle packet fragmentation
        return;
    }
    
    // Call message handler
    MessageType type = packet->header.message_type;
    if (type < MAX_MESSAGE_TYPES && protocol->message_handlers[type]) {
        protocol->message_handlers[type](packet, protocol->handler_user_data[type]);
    }
}

void protocol_register_handler(NetworkProtocol* protocol, MessageType type, MessageHandler handler, void* user_data) {
    if (!protocol || type >= MAX_MESSAGE_TYPES) return;
    
    protocol->message_handlers[type] = handler;
    protocol->handler_user_data[type] = user_data;
    
    log_info("Registered handler for message type %u", type);
}

void protocol_unregister_handler(NetworkProtocol* protocol, MessageType type) {
    if (!protocol || type >= MAX_MESSAGE_TYPES) return;
    
    protocol->message_handlers[type] = NULL;
    protocol->handler_user_data[type] = NULL;
    
    log_info("Unregistered handler for message type %u", type);
}

// Utility functions
uint16_t calculate_checksum(const void* data, size_t size) {
    if (!data || size == 0) return 0;
    
    const uint16_t* words = (const uint16_t*)data;
    uint32_t sum = 0;
    
    for (size_t i = 0; i < size / 2; i++) {
        sum += words[i];
    }
    
    // Handle odd byte
    if (size % 2 == 1) {
        sum += ((const uint8_t*)data)[size - 1];
    }
    
    // Add carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)(~sum);
}

bool validate_packet(const NetworkPacket* packet) {
    if (!packet) return false;
    
    // Check protocol version
    if (packet->header.protocol_version != NETWORK_PROTOCOL_VERSION) {
        return false;
    }
    
    // Check data size
    if (packet->header.data_size > MAX_PACKET_SIZE - sizeof(PacketHeader)) {
        return false;
    }
    
    // Validate checksum
    uint16_t expected_checksum = packet->header.checksum;
    uint16_t calculated_checksum = calculate_checksum(&packet->header, sizeof(PacketHeader) - sizeof(uint16_t));
    calculated_checksum = calculate_checksum(packet->data, packet->header.data_size) ^ calculated_checksum;
    
    return expected_checksum == calculated_checksum;
}

NetworkAddress parse_address(const char* address_string) {
    NetworkAddress addr = {0};
    if (!address_string) return addr;
    
    // Parse "x.x.x.x:port" format
    char addr_copy[64];
    strncpy(addr_copy, address_string, sizeof(addr_copy) - 1);
    addr_copy[sizeof(addr_copy) - 1] = '\0';
    
    char* colon = strchr(addr_copy, ':');
    if (colon) {
        *colon = '\0';
        addr.port = (uint16_t)atoi(colon + 1);
    }
    
    // Parse IP address
    uint32_t a, b, c, d;
    if (sscanf(addr_copy, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
        addr.address = (a << 24) | (b << 16) | (c << 8) | d;
    }
    
    return addr;
}

bool address_equals(const NetworkAddress* a, const NetworkAddress* b) {
    if (!a || !b) return false;
    return a->address == b->address && a->port == b->port;
}

uint32_t address_hash(const NetworkAddress* address) {
    if (!address) return 0;
    return address->address ^ ((uint32_t)address->port << 16);
}

void protocol_get_stats(const NetworkProtocol* protocol, ProtocolStats* out_stats) {
    if (!protocol || !out_stats) return;
    
    *out_stats = protocol->stats;
    
    // Calculate packet loss rate
    uint32_t total_packets = protocol->stats.packets_sent;
    if (total_packets > 0) {
        out_stats->packet_loss_rate = (float)protocol->stats.packets_lost / total_packets;
    }
    
    // Calculate average RTT
    uint32_t active_connections = 0;
    float total_rtt = 0.0f;
    for (uint32_t i = 0; i < protocol->max_connections; i++) {
        const NetworkConnection* conn = &protocol->connections[i];
        if (conn->state == CONN_CONNECTED) {
            total_rtt += conn->rtt;
            active_connections++;
        }
    }
    
    if (active_connections > 0) {
        out_stats->average_rtt = total_rtt / active_connections;
    }
}

void protocol_reset_stats(NetworkProtocol* protocol) {
    if (!protocol) return;
    
    memset(&protocol->stats, 0, sizeof(protocol->stats));
    
    for (uint32_t i = 0; i < protocol->max_connections; i++) {
        NetworkConnection* conn = &protocol->connections[i];
        conn->packets_sent = 0;
        conn->packets_received = 0;
        conn->packets_lost = 0;
    }
    
    log_info("Protocol statistics reset");
}

bool protocol_send_reliable(NetworkProtocol* protocol, uint32_t connection_id, MessageType type, const void* data, uint16_t data_size) {
    return protocol_send_packet(protocol, connection_id, type, data, data_size, PACKET_FLAG_RELIABLE);
}

bool protocol_send_unreliable(NetworkProtocol* protocol, uint32_t connection_id, MessageType type, const void* data, uint16_t data_size) {
    return protocol_send_packet(protocol, connection_id, type, data, data_size, 0);
}
