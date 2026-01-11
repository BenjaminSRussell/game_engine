#ifndef SOCKET_H
#define SOCKET_H

#include "network_types.h"
#include <stdbool.h>
#include <stdint.h>

// Opaque socket handle
typedef struct NetSocket NetSocket;

// Create UDP socket (non-blocking)
NetSocket *socket_create(uint16_t port);

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

#endif // SOCKET_H
