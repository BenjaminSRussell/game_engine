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
#define MAX_PENDING_REQUESTS 64
#define MAX_RELIABLE_RPC 64
#define MAX_RETRIES 5
#define RETRY_INTERVAL 0.5f

typedef struct {
    char name[RPC_NAME_MAX_LENGTH];
    RPCHandler handler;
    bool active;
} RPCFunction;

typedef struct {
    uint32_t request_id;
    RpcCallback callback;
    void *user_data;
    float timestamp;
    bool active;
} PendingRequest;

typedef struct {
    Packet packet;
    uint32_t request_id;
    float time_since_sent;
    int retries;
    bool active;
    uint32_t target_client_id;
} PendingReliable;

static RPCFunction rpc_functions[MAX_RPC_FUNCTIONS];
static uint32_t rpc_count = 0;
static PendingRequest pending_requests[MAX_PENDING_REQUESTS];
static PendingReliable pending_reliable[MAX_RELIABLE_RPC];
static uint32_t next_request_id = 1;

// Internal handler for responses
static void rpc_response_handler(uint32_t sender_id, uint32_t request_id, void *data, uint32_t size) {
    (void)sender_id;
    for (int i = 0; i < MAX_PENDING_REQUESTS; ++i) {
        if (pending_requests[i].active && pending_requests[i].request_id == request_id) {
            if (pending_requests[i].callback) {
                Packet response;
                // Initialize packet for reading from the raw data buffer
                rpc_params_from_data(&response, data, size);
                pending_requests[i].callback(request_id, &response, pending_requests[i].user_data);
            }
            pending_requests[i].active = false;
            return;
        }
    }
}

// network_*_send wraps this buffer in a PacketHeader and forces flags to 0.
// The body therefore starts at byte 0: flags, name, request id, then bytes.
static bool write_rpc_packet(Packet *packet, const char *name, uint32_t req_id, const void *data, uint32_t size, uint8_t flags) {
    if (!packet || !name || size > 65535u || (size > 0 && !data)) {
        return false;
    }

    memset(packet, 0, sizeof(*packet));
    if (!packet_write_u8(packet, flags) ||
        !packet_write_string(packet, name) ||
        !packet_write_u32(packet, req_id)) {
        return false;
    }

    if (size > 0) {
        if (!packet_write_bytes(packet, data, (uint16_t)size)) {
            return false;
        }
    } else if (!packet_write_u16(packet, 0)) {
        return false;
    }

    packet->length = packet->write_pos;
    return true;
}

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

bool rpc_dispatch(const char *name, uint32_t sender_id, uint32_t request_id, void *data, uint32_t size) {
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
    handler(sender_id, request_id, data, size);
    
    log_debug("RPC dispatched: %s from client %u (req: %u)", name, sender_id, request_id);
    return true;
}

void rpc_params_init(Packet *params) {
    if (params) {
        memset(params, 0, sizeof(Packet));
        params->write_pos = 0;
        params->read_pos = 0;
        params->length = 0;
    }
}

void rpc_params_from_data(Packet *params, const void *data, uint32_t size) {
    if (params && data) {
        memset(params, 0, sizeof(Packet));
        if (size > MAX_PACKET_SIZE) size = MAX_PACKET_SIZE;
        memcpy(params->buffer, data, size);
        params->length = (uint16_t)size;
        params->read_pos = 0;
        params->write_pos = (uint16_t)size;
    }
}

void rpc_send(const char *name, void *data, uint32_t size) {
    if (!name) {
        log_error("RPC send: invalid name");
        return;
    }
    
    Packet packet;
    if (!write_rpc_packet(&packet, name, 0, data, size, 0)) {
        log_error("RPC send: failed to serialize packet");
        return;
    }

    if (network_client_send(PACKET_TYPE_RPC, packet.buffer, packet.length) != 0) {
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
    
    Packet packet;
    if (!write_rpc_packet(&packet, name, 0, data, size, 0)) {
        log_error("RPC broadcast: failed to serialize packet");
        return;
    }

    if (network_server_broadcast(PACKET_TYPE_RPC, packet.buffer, packet.length) != 0) {
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
    
    Packet packet;
    if (!write_rpc_packet(&packet, name, 0, data, size, 0)) {
        log_error("RPC send_to_client: failed to serialize packet");
        return;
    }

    if (network_server_send_to_client(client_id, PACKET_TYPE_RPC, packet.buffer, packet.length) != 0) {
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
    
    uint8_t flags = 0;
    if (!packet_read_u8(&packet, &flags)) {
        log_error("RPC process: failed to read flags");
        return;
    }

    // Read RPC name
    char rpc_name[RPC_NAME_MAX_LENGTH];
    if (!packet_read_string(&packet, rpc_name, sizeof(rpc_name))) {
        log_error("RPC process: failed to read RPC name");
        return;
    }

    // Read Request ID
    uint32_t request_id = 0;
    if (!packet_read_u32(&packet, &request_id)) {
        log_error("RPC process: failed to read Request ID");
        return;
    }

    // Handle ACKs
    if (strcmp(rpc_name, "__ACK__") == 0) {
        for (int i=0; i<MAX_RELIABLE_RPC; ++i) {
            if (pending_reliable[i].active && pending_reliable[i].request_id == request_id) {
                pending_reliable[i].active = false;
                log_debug("RPC reliable: ACK received for ID %u", request_id);
                break;
            }
        }
        return;
    }

    // Client ids start at 1, so sender 0 is the server and the reply goes out
    // through the client socket. Any other sender is a client of this server.
    if (flags & RPC_FLAG_RELIABLE) {
        Packet ack;
        if (write_rpc_packet(&ack, "__ACK__", request_id, NULL, 0, 0)) {
            if (sender_id == 0) {
                network_client_send(PACKET_TYPE_RPC, ack.buffer, ack.length);
            } else {
                network_server_send_to_client(sender_id, PACKET_TYPE_RPC, ack.buffer, ack.length);
            }
        }
    }
    
    // Read RPC data
    uint8_t rpc_data[1024];
    uint16_t rpc_data_size = 0;
    if (!packet_read_u16(&packet, &rpc_data_size)) {
         log_error("RPC process: failed to read data size");
         return;
    }
    
    if (rpc_data_size > sizeof(rpc_data)) {
        log_error("RPC process: data too large");
        return;
    }
    
    if (rpc_data_size > 0) {
        if (packet.read_pos + rpc_data_size > packet.length) {
            log_error("RPC process: incomplete data");
            return;
        }
        memcpy(rpc_data, &packet.buffer[packet.read_pos], rpc_data_size);
        packet.read_pos += rpc_data_size;
    }

    // Dispatch the RPC
    rpc_dispatch(rpc_name, sender_id, request_id, rpc_data_size ? rpc_data : NULL, rpc_data_size);
}

// Stubs for new features
void rpc_update(float delta_time) {
    for (int i=0; i<MAX_RELIABLE_RPC; ++i) {
        if (pending_reliable[i].active) {
            pending_reliable[i].time_since_sent += delta_time;
            if (pending_reliable[i].time_since_sent >= RETRY_INTERVAL) {
                pending_reliable[i].time_since_sent = 0;
                pending_reliable[i].retries++;
                if (pending_reliable[i].retries > MAX_RETRIES) {
                    log_warn("RPC reliable: failed to send after retries (ID: %u)", pending_reliable[i].request_id);
                    pending_reliable[i].active = false;
                } else {
                    // Resend
                    uint16_t len = pending_reliable[i].packet.length;
                    if (pending_reliable[i].target_client_id == 0) {
                        network_client_send(PACKET_TYPE_RPC, pending_reliable[i].packet.buffer, len);
                    } else {
                        network_server_send_to_client(pending_reliable[i].target_client_id, PACKET_TYPE_RPC, pending_reliable[i].packet.buffer, len);
                    }
                    log_debug("RPC reliable: resending ID %u", pending_reliable[i].request_id);
                }
            }
        }
    }
}

static void enqueue_reliable_packet(Packet *packet, uint32_t req_id, uint32_t target_client_id) {
    int slot = -1;
    for (int i=0; i<MAX_RELIABLE_RPC; ++i) {
        if (!pending_reliable[i].active) {
            slot = i;
            break;
        }
    }

    if (slot != -1) {
        pending_reliable[slot].active = true;
        pending_reliable[slot].request_id = req_id;
        pending_reliable[slot].retries = 0;
        pending_reliable[slot].time_since_sent = 0;
        pending_reliable[slot].target_client_id = target_client_id;
        memcpy(&pending_reliable[slot].packet, packet, sizeof(Packet));
    } else {
        log_error("RPC reliable: max pending reached");
    }
}

void rpc_send_reliable(const char *name, void *data, uint32_t size) {
    Packet packet;
    uint32_t req_id = next_request_id++;
    if (req_id == 0) req_id = next_request_id++;

    if (!write_rpc_packet(&packet, name, req_id, data, size, RPC_FLAG_RELIABLE)) {
        log_error("RPC reliable: failed to serialize packet");
        return;
    }

    enqueue_reliable_packet(&packet, req_id, 0); // 0 = Server

    if (network_client_send(PACKET_TYPE_RPC, packet.buffer, packet.length) != 0) {
        log_error("RPC reliable: failed to send packet");
    }
}

void rpc_broadcast_reliable(const char *name, void *data, uint32_t size) {
    log_warn("RPC reliable broadcast not fully implemented (unreliable fallback)");
    rpc_broadcast(name, data, size);
}

void rpc_send_to_client_reliable(uint32_t client_id, const char *name, void *data, uint32_t size) {
    Packet packet;
    uint32_t req_id = next_request_id++;
    if (req_id == 0) req_id = next_request_id++;

    if (!write_rpc_packet(&packet, name, req_id, data, size, RPC_FLAG_RELIABLE)) {
        log_error("RPC reliable: failed to serialize packet");
        return;
    }

    enqueue_reliable_packet(&packet, req_id, client_id);

    if (network_server_send_to_client(client_id, PACKET_TYPE_RPC, packet.buffer, packet.length) != 0) {
        log_error("RPC reliable: failed to send packet to client %u", client_id);
    }
}

uint32_t rpc_call(const char *name, Packet *params, RpcCallback callback, void *user_data) {
    uint32_t req_id = next_request_id++;
    if (req_id == 0) req_id = next_request_id++;

    if (callback) {
        int slot = -1;
        for (int i = 0; i < MAX_PENDING_REQUESTS; ++i) {
            if (!pending_requests[i].active) {
                slot = i;
                break;
            }
        }

        if (slot == -1) {
            log_error("RPC call: max pending requests reached");
            return 0;
        }

        pending_requests[slot].active = true;
        pending_requests[slot].request_id = req_id;
        pending_requests[slot].callback = callback;
        pending_requests[slot].user_data = user_data;
        pending_requests[slot].timestamp = 0;
    }

    Packet packet;
    // Use params buffer as data payload. Use write_pos as length since params might not be finalized.
    if (!write_rpc_packet(&packet, name, req_id, params ? params->buffer : NULL, params ? params->write_pos : 0, RPC_FLAG_RELIABLE)) {
        log_error("RPC call: failed to serialize packet");
        if (callback) {
            for (int i = 0; i < MAX_PENDING_REQUESTS; ++i) {
                if (pending_requests[i].active && pending_requests[i].request_id == req_id) {
                    pending_requests[i].active = false;
                    break;
                }
            }
        }
        return 0;
    }

    // Enqueue for reliability
    enqueue_reliable_packet(&packet, req_id, 0); // Assume server target for now

    if (network_client_send(PACKET_TYPE_RPC, packet.buffer, packet.length) != 0) {
        log_error("RPC call: failed to send packet");
        // Pending request remains active for retry via rpc_update
    }

    return req_id;
}

void rpc_reply(uint32_t target_id, uint32_t request_id, Packet *results) {
    Packet packet;
    if (!write_rpc_packet(&packet, "__RESPONSE__", request_id, results ? results->buffer : NULL, results ? results->write_pos : 0, 0)) {
        log_error("RPC reply: failed to serialize packet");
        return;
    }

    if (target_id == 0) {
        network_client_send(PACKET_TYPE_RPC, packet.buffer, packet.length);
    } else {
        network_server_send_to_client(target_id, PACKET_TYPE_RPC, packet.buffer, packet.length);
    }
}

// Initialize RPC system
void rpc_init(void) {
    memset(rpc_functions, 0, sizeof(rpc_functions));
    rpc_count = 0;

    memset(pending_requests, 0, sizeof(pending_requests));
    memset(pending_reliable, 0, sizeof(pending_reliable));

    // Register internal response handler
    rpc_register("__RESPONSE__", rpc_response_handler);

    log_info("RPC system initialized");
}

// Shutdown RPC system
void rpc_shutdown(void) {
    memset(rpc_functions, 0, sizeof(rpc_functions));
    rpc_count = 0;
    memset(pending_requests, 0, sizeof(pending_requests));
    memset(pending_reliable, 0, sizeof(pending_reliable));
    log_info("RPC system shutdown");
}

// TODO: MVP PATH - Add RPC statistics and monitoring
// TODO: MVP PATH - Implement RPC debugging and logging tools
// TODO: MVP PATH - Add RPC dependency injection for testing
// TODO: MVP PATH - Implement RPC middleware for cross-cutting concerns
