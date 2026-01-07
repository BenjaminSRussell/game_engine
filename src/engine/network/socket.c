#include "network/socket.h"
#include <stddef.h>

// Opaque socket definition
struct NetSocket {
    int handle;
};

NetSocket *socket_create(uint16_t port) {
    return NULL;
}

void socket_close(NetSocket *sock) {
}

bool socket_send(NetSocket *sock, const NetAddress *addr, const void *data, uint32_t size) {
    return false;
}

int socket_receive(NetSocket *sock, NetAddress *from, void *buffer, uint32_t buffer_size) {
    return 0;
}

const char *socket_get_error(void) {
    return "Not implemented";
}
