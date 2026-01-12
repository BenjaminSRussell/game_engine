// socket.c - Cross-platform UDP socket implementation
#include "include/network/socket.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

struct NetSocket {
#ifdef _WIN32
    SOCKET handle;
#else
    int handle;
#endif
    uint16_t port;
    bool is_open;
};

static bool socket_initialized = false;

static bool init_socket_system(void) {
    if (socket_initialized) return true;
    
#ifdef _WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        log_error("WSAStartup failed: %d", result);
        return false;
    }
#endif
    
    socket_initialized = true;
    return true;
}

static void cleanup_socket_system(void) {
    if (!socket_initialized) return;
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    socket_initialized = false;
}

static void set_non_blocking(int sock) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

NetSocket *socket_create(uint16_t port) {
    if (!init_socket_system()) {
        return NULL;
    }
    
    NetSocket *sock = malloc(sizeof(NetSocket));
    if (!sock) {
        log_error("Failed to allocate socket");
        return NULL;
    }
    
    sock->handle = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    if (sock->handle == INVALID_SOCKET) {
#else
    if (sock->handle < 0) {
#endif
        log_error("Failed to create socket: %s", socket_get_error());
        free(sock);
        return NULL;
    }
    
    set_non_blocking(sock->handle);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sock->handle, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_error("Failed to bind socket to port %d: %s", port, socket_get_error());
#ifdef _WIN32
        closesocket(sock->handle);
#else
        close(sock->handle);
#endif
        free(sock);
        return NULL;
    }
    
    sock->port = port;
    sock->is_open = true;
    
    log_info("Created UDP socket on port %d", port);
    return sock;
}

void socket_close(NetSocket *sock) {
    if (!sock || !sock->is_open) {
        return;
    }
    
#ifdef _WIN32
    closesocket(sock->handle);
#else
    close(sock->handle);
#endif
    
    sock->is_open = false;
    free(sock);
    
    log_info("Closed socket on port %d", sock->port);
}

bool socket_send(NetSocket *sock, const NetAddress *addr, const void *data, uint32_t size) {
    if (!sock || !sock->is_open || !addr || !data || size == 0) {
        return false;
    }
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(addr->port);
    dest_addr.sin_addr.s_addr = addr->host;
    
    int bytes_sent = sendto(sock->handle, (const char*)data, size, 0,
                           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (bytes_sent < 0) {
        log_error("Socket send failed: %s", socket_get_error());
        return false;
    }
    
    return (uint32_t)bytes_sent == size;
}

int socket_receive(NetSocket *sock, NetAddress *from, void *buffer, uint32_t buffer_size) {
    if (!sock || !sock->is_open || !from || !buffer || buffer_size == 0) {
        return 0;
    }
    
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    
    int bytes_received = recvfrom(sock->handle, (char*)buffer, buffer_size, 0,
                                 (struct sockaddr*)&sender_addr, &addr_len);
    
    if (bytes_received < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            return 0; // No data available
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No data available
        }
#endif
        log_error("Socket receive failed: %s", socket_get_error());
        return 0;
    }
    
    from->host = sender_addr.sin_addr.s_addr;
    from->port = ntohs(sender_addr.sin_port);
    
    return bytes_received;
}

const char *socket_get_error(void) {
#ifdef _WIN32
    static char error_buffer[256];
    int error = WSAGetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   error_buffer, sizeof(error_buffer), NULL);
    return error_buffer;
#else
    return strerror(errno);
#endif
}
