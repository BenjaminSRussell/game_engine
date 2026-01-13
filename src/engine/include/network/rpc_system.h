/**
 * =================================================================================================
 *                          RPC SYSTEM HEADER
 *                          Phase 5: Networking
 * =================================================================================================
 */

#ifndef RPC_SYSTEM_H
#define RPC_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*RPCHandler)(uint32_t sender_id, void *data, uint32_t size);

// RPC timeout and retry configuration
typedef struct {
    float timeout_seconds;       // Timeout for RPC response
    uint32_t max_retries;        // Maximum retry attempts
    float retry_delay_seconds;    // Delay between retries
    float backoff_multiplier;     // Exponential backoff multiplier
} RPCTimeoutConfig;

// RPC request tracking
typedef struct {
    uint32_t request_id;          // Unique request identifier
    char name[64];                // RPC name
    uint64_t send_time;            // When request was sent
    uint32_t retry_count;          // Current retry attempt
    uint32_t timeout_ms;           // Timeout in milliseconds
    bool completed;                 // Request completed successfully
    bool timed_out;                 // Request timed out
    RPCTimeoutConfig config;       // Timeout configuration
} RPCTrackedRequest;

#define MAX_TRACKED_REQUESTS 64
#define DEFAULT_RPC_TIMEOUT 5.0f
#define DEFAULT_MAX_RETRIES 3
#define DEFAULT_RETRY_DELAY 1.0f
#define DEFAULT_BACKOFF_MULTIPLIER 2.0f

// Core RPC functions
void rpc_init(void);
void rpc_shutdown(void);
void rpc_register(const char *name, RPCHandler handler);
bool rpc_dispatch(const char *name, uint32_t sender_id, void *data, uint32_t size);

// Timeout and retry configuration
void rpc_set_timeout_config(const RPCTimeoutConfig* config);
void rpc_get_timeout_config(RPCTimeoutConfig* config);

// Client-side functions
uint32_t rpc_send_with_timeout(const char *name, void *data, uint32_t size, 
                                 const RPCTimeoutConfig* config);
void rpc_send(const char *name, void *data, uint32_t size);

// Server-side functions
void rpc_broadcast(const char *name, void *data, uint32_t size);
void rpc_send_to_client(uint32_t client_id, const char *name, void *data, uint32_t size);

// Packet processing
void rpc_process_packet(uint32_t sender_id, const void *packet_data, uint16_t packet_size);

// Request tracking
void rpc_update_timeouts(float delta_time);
uint32_t rpc_get_pending_request_count(void);
bool rpc_is_request_pending(uint32_t request_id);

#endif // RPC_SYSTEM_H
