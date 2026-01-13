// rpc_system.c - Remote Procedure Call system with timeout and retry mechanisms
// TODO: MVP PATH - Implement RPC authentication and authorization
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

// Static state
static RPCFunction rpc_functions[MAX_RPC_FUNCTIONS];
static uint32_t rpc_count = 0;
static RPCTrackedRequest tracked_requests[MAX_TRACKED_REQUESTS];
static uint32_t next_request_id = 1;
static RPCTimeoutConfig global_timeout_config = {
    .timeout_seconds = DEFAULT_RPC_TIMEOUT,
    .max_retries = DEFAULT_MAX_RETRIES,
    .retry_delay_seconds = DEFAULT_RETRY_DELAY,
    .backoff_multiplier = DEFAULT_BACKOFF_MULTIPLIER
};

// Helper functions
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static uint32_t generate_request_id(void) {
    return next_request_id++;
}

static RPCTrackedRequest* find_tracked_request(uint32_t request_id) {
    for (uint32_t i = 0; i < MAX_TRACKED_REQUESTS; i++) {
        if (tracked_requests[i].request_id == request_id && !tracked_requests[i].completed) {
            return &tracked_requests[i];
        }
    }
    return NULL;
}

static RPCTrackedRequest* allocate_tracked_request(void) {
    for (uint32_t i = 0; i < MAX_TRACKED_REQUESTS; i++) {
        if (tracked_requests[i].request_id == 0) {
            memset(&tracked_requests[i], 0, sizeof(RPCTrackedRequest));
            return &tracked_requests[i];
        }
    }
    return NULL;
}

static void free_tracked_request(RPCTrackedRequest* request) {
    if (request) {
        request->request_id = 0;
    }
}

static void retry_request(RPCTrackedRequest* request) {
    if (!request || request->retry_count >= request->config.max_retries) {
        return;
    }
    
    request->retry_count++;
    request->send_time = get_timestamp_ms();
    
    // Calculate delay with exponential backoff
    float delay = request->config.retry_delay_seconds;
    for (uint32_t i = 1; i < request->retry_count; i++) {
        delay *= request->config.backoff_multiplier;
    }
    
    log_info("Retrying RPC '%s' (attempt %u/%u) after %.2fs delay", 
             request->name, request->retry_count, request->config.max_retries, delay);
    
    // TODO: Schedule retry after delay (would need timer system)
    // For now, we'll retry immediately in the update function
}

static void complete_request(RPCTrackedRequest* request) {
    if (request) {
        request->completed = true;
        request->timed_out = false;
        log_debug("RPC request '%s' completed successfully", request->name);
    }
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

uint32_t rpc_send_with_timeout(const char *name, void *data, uint32_t size, 
                                 const RPCTimeoutConfig* config) {
    if (!name) {
        log_error("RPC send with timeout: invalid name");
        return 0;
    }
    
    // Allocate tracked request
    RPCTrackedRequest* request = allocate_tracked_request();
    if (!request) {
        log_error("RPC send with timeout: no free request slots");
        return 0;
    }
    
    // Initialize request tracking
    request->request_id = generate_request_id();
    strncpy(request->name, name, RPC_NAME_MAX_LENGTH - 1);
    request->send_time = get_timestamp_ms();
    request->retry_count = 0;
    request->completed = false;
    request->timed_out = false;
    request->timeout_ms = (uint32_t)(config->timeout_seconds * 1000.0f);
    request->config = *config;
    
    // Send the RPC normally
    rpc_send(name, data, size);
    
    log_debug("RPC '%s' sent with request ID %u", name, request->request_id);
    return request->request_id;
}

void rpc_set_timeout_config(const RPCTimeoutConfig* config) {
    if (config) {
        global_timeout_config = *config;
        log_info("RPC timeout config updated: timeout=%.1fs, retries=%u, delay=%.1fs, backoff=%.1f",
                 config->timeout_seconds, config->max_retries, 
                 config->retry_delay_seconds, config->backoff_multiplier);
    }
}

void rpc_get_timeout_config(RPCTimeoutConfig* config) {
    if (config) {
        *config = global_timeout_config;
    }
}

void rpc_update_timeouts(float delta_time) {
    uint64_t current_time = get_timestamp_ms();
    
    for (uint32_t i = 0; i < MAX_TRACKED_REQUESTS; i++) {
        RPCTrackedRequest* request = &tracked_requests[i];
        
        // Skip inactive requests
        if (request->request_id == 0 || request->completed) {
            continue;
        }
        
        // Check for timeout
        uint64_t elapsed = current_time - request->send_time;
        if (elapsed >= request->timeout_ms) {
            if (request->retry_count < request->config.max_retries) {
                log_warn("RPC '%s' (ID %u) timed out, retrying...", 
                         request->name, request->request_id);
                retry_request(request);
            } else {
                log_error("RPC '%s' (ID %u) failed after %u retries", 
                          request->name, request->request_id, request->retry_count);
                request->timed_out = true;
                free_tracked_request(request);
            }
        }
    }
}

uint32_t rpc_get_pending_request_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_TRACKED_REQUESTS; i++) {
        if (tracked_requests[i].request_id != 0 && !tracked_requests[i].completed) {
            count++;
        }
    }
    return count;
}

bool rpc_is_request_pending(uint32_t request_id) {
    RPCTrackedRequest* request = find_tracked_request(request_id);
    return request != NULL && !request->completed;
}

void rpc_send(const char *name, void *data, uint32_t size) {
    rpc_send_with_timeout(name, data, size, &global_timeout_config);
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
        log_error("RPC send to client: invalid name");
        return;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_RPC, 0);
    
    // Write RPC name and data
    if (!packet_write_string(&packet, name) || 
        !packet_write_bytes(&packet, data, size)) {
        log_error("RPC send to client: failed to serialize packet");
        return;
    }
    
    // Finalize and send
    uint16_t packet_size = packet_finalize(&packet);
    if (network_server_send_to_client(client_id, PACKET_TYPE_RPC, packet.buffer, packet_size) != 0) {
        log_error("RPC send to client: failed to send packet to client %u", client_id);
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
    memset(tracked_requests, 0, sizeof(tracked_requests));
    rpc_count = 0;
    next_request_id = 1;
    
    // Initialize default timeout config
    global_timeout_config.timeout_seconds = DEFAULT_RPC_TIMEOUT;
    global_timeout_config.max_retries = DEFAULT_MAX_RETRIES;
    global_timeout_config.retry_delay_seconds = DEFAULT_RETRY_DELAY;
    global_timeout_config.backoff_multiplier = DEFAULT_BACKOFF_MULTIPLIER;
    
    log_info("RPC system initialized with timeout and retry support");
}

// Shutdown RPC system
void rpc_shutdown(void) {
    // Complete all pending requests
    for (uint32_t i = 0; i < MAX_TRACKED_REQUESTS; i++) {
        if (tracked_requests[i].request_id != 0 && !tracked_requests[i].completed) {
            log_warn("Completing pending RPC '%s' on shutdown", tracked_requests[i].name);
            tracked_requests[i].completed = true;
        }
    }
    
    memset(rpc_functions, 0, sizeof(rpc_functions));
    memset(tracked_requests, 0, sizeof(tracked_requests));
    rpc_count = 0;
    next_request_id = 1;
    
    log_info("RPC system shutdown");
}

// TODO: MVP PATH - Add RPC statistics and monitoring
// TODO: MVP PATH - Implement RPC debugging and logging tools
// TODO: MVP PATH - Add RPC dependency injection for testing
// TODO: MVP PATH - Implement RPC middleware for cross-cutting concerns
