// rpc_system.c - Remote Procedure Call system for network communication
// TODO: MVP PATH - Implement RPC timeout and retry mechanisms
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

typedef struct {
    char name[RPC_NAME_MAX_LENGTH];
    RPCHandler handler;
    bool active;
} RPCFunction;

static RPCFunction rpc_functions[MAX_RPC_FUNCTIONS];
static uint32_t rpc_count = 0;

void rpc_register(const char *name, RPCHandler handler) {
    if (!name || !handler) {
        log_error("RPC register: invalid parameters");
        return;
    }
    
    if (rpc_count >= MAX_RPC_FUNCTIONS) {
        log_error("RPC register: maximum RPC functions reached");
        return;
    }
    
    // Check if already registered
    for (uint32_t i = 0; i < rpc_count; i++) {
        if (strcmp(rpc_functions[i].name, name) == 0) {
            log_warn("RPC register: function '%s' already registered, updating handler", name);
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
    
    log_info("RPC registered: %s", name);
}

bool rpc_dispatch(const char *name, uint32_t sender_id, void *data, uint32_t size) {
    if (!name) {
        log_error("RPC dispatch: invalid name");
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
        log_error("RPC dispatch: function '%s' not found", name);
        return false;
    }
    
    // Call the handler
    handler(sender_id, data, size);
    
    log_debug("RPC dispatched: %s from client %u", name, sender_id);
    return true;
}

void rpc_send(const char *name, void *data, uint32_t size) {
    if (!name) {
        log_error("RPC send: invalid name");
        return;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC, 0);
    
    // Write RPC name and data
    if (!packet_write_string(&packet, name) || 
        !packet_write_bytes(&packet, data, size)) {
        log_error("RPC send: failed to serialize packet");
        return;
    }
    
    // Finalize and send
    uint16_t packet_size = packet_finalize(&packet);
    if (network_client_send(PACKET_TYPE_RPC, packet.buffer, packet_size) != 0) {
        log_error("RPC send: failed to send packet");
        return;
    }
    
    log_debug("RPC sent: %s (%u bytes)", name, size);
}

// Server-side RPC broadcast to all clients
void rpc_broadcast(const char *name, void *data, uint32_t size) {
    if (!name) {
        log_error("RPC broadcast: invalid name");
        return;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC, 0);
    
    // Write RPC name and data
    if (!packet_write_string(&packet, name) || 
        !packet_write_bytes(&packet, data, size)) {
        log_error("RPC broadcast: failed to serialize packet");
        return;
    }
    
    // Finalize and broadcast
    uint16_t packet_size = packet_finalize(&packet);
    if (network_server_broadcast(PACKET_TYPE_RPC, packet.buffer, packet_size) != 0) {
        log_error("RPC broadcast: failed to broadcast packet");
        return;
    }
    
    log_debug("RPC broadcast: %s (%u bytes)", name, size);
}

// Server-side RPC send to specific client
void rpc_send_to_client(uint32_t client_id, const char *name, void *data, uint32_t size) {
    if (!name) {
        log_error("RPC send_to_client: invalid name");
        return;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC, 0);
    
    // Write RPC name and data
    if (!packet_write_string(&packet, name) || 
        !packet_write_bytes(&packet, data, size)) {
        log_error("RPC send_to_client: failed to serialize packet");
        return;
    }
    
    // Finalize and send
    uint16_t packet_size = packet_finalize(&packet);
    if (network_server_send_to_client(client_id, PACKET_TYPE_RPC, packet.buffer, packet_size) != 0) {
        log_error("RPC send_to_client: failed to send packet to client %u", client_id);
        return;
    }
    
    log_debug("RPC sent to client %u: %s (%u bytes)", client_id, name, size);
}

// Process incoming RPC packet
void rpc_process_packet(uint32_t sender_id, const void *packet_data, uint16_t packet_size) {
    if (!packet_data || packet_size < sizeof(PacketHeader)) {
        log_error("RPC process: invalid packet data");
        return;
    }
    
    // Read packet
    Packet packet;
    if (!packet_init_read(&packet, packet_data, packet_size)) {
        log_error("RPC process: failed to read packet");
        return;
    }
    
    // Read RPC name
    char rpc_name[RPC_NAME_MAX_LENGTH];
    if (!packet_read_string(&packet, rpc_name, sizeof(rpc_name))) {
        log_error("RPC process: failed to read RPC name");
        return;
    }
    
    // Read RPC data
    uint8_t rpc_data[1024];
    uint16_t rpc_data_size;
    if (!packet_read_u16(&packet, &rpc_data_size)) {
        log_error("RPC process: failed to read RPC data size");
        return;
    }
    
    if (rpc_data_size > sizeof(rpc_data)) {
        log_error("RPC process: RPC data too large (%u bytes)", rpc_data_size);
        return;
    }
    
    if (!packet_read_bytes(&packet, rpc_data, rpc_data_size)) {
        log_error("RPC process: failed to read RPC data");
        return;
    }
    
    // Dispatch the RPC
    rpc_dispatch(rpc_name, sender_id, rpc_data, rpc_data_size);
}

// Initialize RPC system
void rpc_init(void) {
    memset(rpc_functions, 0, sizeof(rpc_functions));
    rpc_count = 0;
    log_info("RPC system initialized");
}

// Shutdown RPC system
void rpc_shutdown(void) {
    memset(rpc_functions, 0, sizeof(rpc_functions));
    rpc_count = 0;
    log_info("RPC system shutdown");
}

// TODO: MVP PATH - Add RPC statistics and monitoring
// TODO: MVP PATH - Implement RPC debugging and logging tools
// TODO: MVP PATH - Add RPC dependency injection for testing
// TODO: MVP PATH - Implement RPC middleware for cross-cutting concerns
