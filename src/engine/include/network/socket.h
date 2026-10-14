#ifndef SOCKET_H
#define SOCKET_H

#include "include/network/network_types.h"
#include <stdbool.h>
#include <stdint.h>

// Opaque socket handle
typedef struct NetSocket NetSocket;

// Socket statistics
typedef struct {
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t packets_sent;
    uint64_t packets_received;
    uint32_t send_errors;
    uint32_t receive_errors;
} NetSocketStats;

// Create UDP socket (non-blocking, IPv4)
NetSocket *socket_create(uint16_t port);

// Create UDP socket with specific type (IPv4 or IPv6)
NetSocket *socket_create_typed(uint16_t port, NetAddressType type);

// Close and free socket
void socket_close(NetSocket *sock);

// Send data to address
bool socket_send(NetSocket *sock, const NetAddress *addr, const void *data,
                 uint32_t size);

// Receive data (non-blocking, returns number of bytes received or 0 if none
// available)
int socket_receive(NetSocket *sock, NetAddress *from, void *buffer,
                   uint32_t buffer_size);

// Get last error message
const char *socket_get_error(void);

// Get socket statistics
void socket_get_stats(const NetSocket *sock, NetSocketStats *stats);

#endif // SOCKET_H
