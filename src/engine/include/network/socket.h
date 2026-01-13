#ifndef SOCKET_H
#define SOCKET_H

#include "include/network/network_types.h"
#include <stdbool.h>
#include <stdint.h>

// Opaque socket handle
typedef struct NetSocket NetSocket;

// Address utility functions
bool address_parse_ipv4(const char* str, NetAddress* addr);
bool address_parse_ipv6(const char* str, NetAddress* addr);
bool address_parse(const char* str, NetAddress* addr); // Auto-detect format
void address_to_string(const NetAddress* addr, char* buffer, size_t buffer_size);
bool address_equals(const NetAddress* a, const NetAddress* b);

// Create UDP socket (non-blocking)
NetSocket *socket_create(uint16_t port);
NetSocket *socket_create_ipv6(uint16_t port);
NetSocket *socket_create_dual_stack(uint16_t port); // IPv4 + IPv6

// Close and free socket
void socket_close(NetSocket *sock);

// Send data to address
bool socket_send(NetSocket *sock, const NetAddress *addr, const void *data,
                 uint32_t size);
bool socket_send_ipv4(NetSocket *sock, const NetAddress *addr, const void *data,
                     uint32_t size);
bool socket_send_ipv6(NetSocket *sock, const NetAddress *addr, const void *data,
                     uint32_t size);

// Receive data (non-blocking, returns number of bytes received or 0 if none
// available)
int socket_receive(NetSocket *sock, NetAddress *from, void *buffer,
                   uint32_t buffer_size);

// Get last error message
const char *socket_get_error(void);

// Connection quality monitoring
typedef struct {
    float rtt_ms;           // Round-trip time in milliseconds
    float packet_loss;       // Packet loss percentage (0.0-1.0)
    float jitter_ms;         // Jitter in milliseconds
    uint32_t packets_sent;   // Total packets sent
    uint32_t packets_recv;   // Total packets received
    uint32_t bytes_sent;     // Total bytes sent
    uint32_t bytes_recv;     // Total bytes received
    float bandwidth_kbps;    // Current bandwidth in KB/s
    uint64_t last_update;    // Timestamp of last update
} ConnectionQuality;

void socket_get_quality_metrics(NetSocket *sock, ConnectionQuality *quality);
void socket_reset_quality_metrics(NetSocket *sock);

#endif // SOCKET_H
