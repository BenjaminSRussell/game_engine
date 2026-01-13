// socket.c - Cross-platform UDP socket implementation
// TODO: MVP PATH - Add socket options for buffer sizes and timeout configuration
// TODO: MVP PATH - Implement NAT traversal helpers (UPnP, NAT-PMP)
// TODO: MVP PATH - Add bandwidth throttling and QoS support
#include "include/network/socket.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    NetAddressType type;
    NetSocketStats stats;
};

static bool socket_initialized = false;

static bool init_socket_system(void) {
    if (socket_initialized) return true;
    
#ifdef _WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        LOG_ERROR(LOG_CAT_NETWORK, "WSAStartup failed: %d", result);
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

NetSocket *socket_create_typed(uint16_t port, NetAddressType type) {
    if (!init_socket_system()) {
        return NULL;
    }
    
    NetSocket *sock = malloc(sizeof(NetSocket));
    if (!sock) {
        LOG_ERROR(LOG_CAT_NETWORK, "Failed to allocate socket");
        return NULL;
    }
    memset(sock, 0, sizeof(NetSocket));

    int domain = (type == NET_ADDR_IPV6) ? AF_INET6 : AF_INET;
    sock->handle = socket(domain, SOCK_DGRAM, 0);
    
#ifdef _WIN32
    if (sock->handle == INVALID_SOCKET) {
#else
    if (sock->handle < 0) {
#endif
        LOG_ERROR(LOG_CAT_NETWORK, "Failed to create socket: %s", socket_get_error());
        free(sock);
        return NULL;
    }
    
    // Enable dual-stack for IPv6 if supported (optional but good for compatibility)
    // For now we treat them strictly separate as per NetAddressType
    if (type == NET_ADDR_IPV6) {
        int no = 0;
#ifdef _WIN32
        setsockopt(sock->handle, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
#else
        setsockopt(sock->handle, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
#endif
    }
    
    set_non_blocking(sock->handle);
    
    if (type == NET_ADDR_IPV6) {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = in6addr_any;
        addr.sin6_port = htons(port);

        if (bind(sock->handle, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR(LOG_CAT_NETWORK, "Failed to bind socket to port %d: %s", port, socket_get_error());
#ifdef _WIN32
            closesocket(sock->handle);
#else
            close(sock->handle);
#endif
            free(sock);
            return NULL;
        }
    } else {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock->handle, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR(LOG_CAT_NETWORK, "Failed to bind socket to port %d: %s", port, socket_get_error());
#ifdef _WIN32
            closesocket(sock->handle);
#else
            close(sock->handle);
#endif
            free(sock);
            return NULL;
        }
    }
    
    sock->port = port;
    sock->is_open = true;
    sock->type = type;
    
    LOG_INFO(LOG_CAT_NETWORK, "Created UDP socket on port %d (%s)", port, (type == NET_ADDR_IPV6) ? "IPv6" : "IPv4");
    return sock;
}

NetSocket *socket_create(uint16_t port) {
    return socket_create_typed(port, NET_ADDR_IPV4);
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
    uint16_t port = sock->port;
    free(sock);
    
    LOG_INFO(LOG_CAT_NETWORK, "Closed socket on port %d", port);
}

bool socket_send(NetSocket *sock, const NetAddress *addr, const void *data, uint32_t size) {
    if (!sock || !sock->is_open || !addr || !data || size == 0) {
        return false;
    }
    
    int bytes_sent = -1;

    if (addr->type == NET_ADDR_IPV6) {
         struct sockaddr_in6 dest_addr;
         memset(&dest_addr, 0, sizeof(dest_addr));
         dest_addr.sin6_family = AF_INET6;
         dest_addr.sin6_port = htons(addr->port);
         memcpy(&dest_addr.sin6_addr, addr->ip6, 16);

         bytes_sent = sendto(sock->handle, (const char*)data, size, 0,
                            (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    } else {
         struct sockaddr_in dest_addr;
         memset(&dest_addr, 0, sizeof(dest_addr));
         dest_addr.sin_family = AF_INET;
         dest_addr.sin_port = htons(addr->port);
         // Use the union member appropriate for IPv4
         dest_addr.sin_addr.s_addr = addr->ip4; // or addr->host

         bytes_sent = sendto(sock->handle, (const char*)data, size, 0,
                            (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    }
    
    if (bytes_sent < 0) {
        sock->stats.send_errors++;
        LOG_ERROR(LOG_CAT_NETWORK, "Socket send failed: %s", socket_get_error());
        return false;
    }
    
    if ((uint32_t)bytes_sent == size) {
        sock->stats.packets_sent++;
        sock->stats.bytes_sent += size;
        return true;
    }

    return false;
}

int socket_receive(NetSocket *sock, NetAddress *from, void *buffer, uint32_t buffer_size) {
    if (!sock || !sock->is_open || !from || !buffer || buffer_size == 0) {
        return 0;
    }
    
    int bytes_received = -1;
    
    if (sock->type == NET_ADDR_IPV6) {
        struct sockaddr_in6 sender_addr;
        socklen_t addr_len = sizeof(sender_addr);

        bytes_received = recvfrom(sock->handle, (char*)buffer, buffer_size, 0,
                                     (struct sockaddr*)&sender_addr, &addr_len);

        if (bytes_received >= 0) {
            from->type = NET_ADDR_IPV6;
            from->port = ntohs(sender_addr.sin6_port);
            memcpy(from->ip6, &sender_addr.sin6_addr, 16);
        }
    } else {
        struct sockaddr_in sender_addr;
        socklen_t addr_len = sizeof(sender_addr);

        bytes_received = recvfrom(sock->handle, (char*)buffer, buffer_size, 0,
                                     (struct sockaddr*)&sender_addr, &addr_len);

        if (bytes_received >= 0) {
            from->type = NET_ADDR_IPV4;
            from->port = ntohs(sender_addr.sin_port);
            from->ip4 = sender_addr.sin_addr.s_addr;
        }
    }
    
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
        sock->stats.receive_errors++;
        LOG_ERROR(LOG_CAT_NETWORK, "Socket receive failed: %s", socket_get_error());
        return 0;
    }
    
    sock->stats.packets_received++;
    sock->stats.bytes_received += bytes_received;
    
    return bytes_received;
}

void socket_get_stats(const NetSocket *sock, NetSocketStats *stats) {
    if (sock && stats) {
        *stats = sock->stats;
    }
}
