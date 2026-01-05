#include "networking/transport/udp_socket.h"
#include "core/logger.h"
#include <string.h>

#ifdef _WIN32
static bool winsock_initialized = false;
#endif

// ✅ COMPLETED: UDP socket abstraction with cross-platform support
// Implements reliable UDP socket management with error handling and statistics

UDPSocket* udp_socket_create(void) {
    if (!network_is_initialized()) {
        LOG_ERROR("Network not initialized");
        return NULL;
    }
    
    UDPSocket* socket = malloc(sizeof(UDPSocket));
    if (!socket) {
        LOG_ERROR("Failed to allocate UDP socket");
        return NULL;
    }
    
    memset(socket, 0, sizeof(UDPSocket));
    socket->socket = INVALID_SOCKET;
    socket->is_bound = false;
    socket->is_connected = false;
    
    // Create UDP socket
    socket->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket->socket == INVALID_SOCKET) {
        LOG_ERROR("Failed to create UDP socket: %s", udp_socket_get_last_error());
        free(socket);
        return NULL;
    }
    
    // Set socket options
    int broadcast = 1;
    if (setsockopt(socket->socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) != 0) {
        LOG_WARN("Failed to enable broadcast: %s", udp_socket_get_last_error());
    }
    
    // Set non-blocking by default
    udp_socket_set_blocking(socket, false);
    
    LOG_INFO("UDP socket created successfully");
    return socket;
}

void udp_socket_destroy(UDPSocket* socket) {
    if (!socket) return;
    
    if (socket->socket != INVALID_SOCKET) {
        closesocket(socket->socket);
    }
    
    free(socket);
    LOG_INFO("UDP socket destroyed");
}

bool udp_socket_bind(UDPSocket* socket, const char* address, u16 port) {
    if (!socket || !address) return false;
    
    NetworkAddress addr;
    if (!udp_address_from_string(&addr, address)) {
        LOG_ERROR("Invalid bind address: %s", address);
        return false;
    }
    addr.port = port;
    
    if (bind(socket->socket, (struct sockaddr*)&addr.addr, sizeof(addr.addr)) != 0) {
        LOG_ERROR("Failed to bind to %s:%u - %s", address, port, udp_socket_get_last_error());
        return false;
    }
    
    socket->local_address = addr;
    socket->is_bound = true;
    
    LOG_INFO("UDP socket bound to %s:%u", address, port);
    return true;
}

bool udp_socket_connect(UDPSocket* socket, const char* address, u16 port) {
    if (!socket || !address) return false;
    
    NetworkAddress addr;
    if (!udp_address_from_string(&addr, address)) {
        LOG_ERROR("Invalid connect address: %s", address);
        return false;
    }
    addr.port = port;
    
    if (connect(socket->socket, (struct sockaddr*)&addr.addr, sizeof(addr.addr)) != 0) {
        LOG_ERROR("Failed to connect to %s:%u - %s", address, port, udp_socket_get_last_error());
        return false;
    }
    
    socket->remote_address = addr;
    socket->is_connected = true;
    
    LOG_INFO("UDP socket connected to %s:%u", address, port);
    return true;
}

void udp_socket_disconnect(UDPSocket* socket) {
    if (!socket) return;
    
    socket->is_connected = false;
    memset(&socket->remote_address, 0, sizeof(socket->remote_address));
    
    LOG_INFO("UDP socket disconnected");
}

bool udp_socket_set_blocking(UDPSocket* socket, bool blocking) {
    if (!socket) return false;
    
#ifdef _WIN32
    u32 mode = blocking ? 0 : 1;
    if (ioctlsocket(socket->socket, FIONBIO, &mode) != 0) {
        LOG_ERROR("Failed to set blocking mode: %s", udp_socket_get_last_error());
        return false;
    }
#else
    int flags = fcntl(socket->socket, F_GETFL, 0);
    if (flags == -1) return false;
    
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    
    if (fcntl(socket->socket, F_SETFL, flags) != 0) {
        LOG_ERROR("Failed to set blocking mode: %s", udp_socket_get_last_error());
        return false;
    }
#endif
    
    return true;
}

bool udp_socket_set_timeout(UDPSocket* socket, u32 timeout_ms) {
    if (!socket) return false;
    
#ifdef _WIN32
    DWORD timeout = timeout_ms;
    if (setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) != 0) {
        LOG_ERROR("Failed to set receive timeout: %s", udp_socket_get_last_error());
        return false;
    }
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    if (setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
        LOG_ERROR("Failed to set receive timeout: %s", udp_socket_get_last_error());
        return false;
    }
#endif
    
    return true;
}

bool udp_socket_send(UDPSocket* socket, const void* data, u32 size) {
    if (!socket || !data || size == 0 || size > UDP_MAX_PACKET_SIZE) return false;
    
    if (!socket->is_connected) {
        LOG_ERROR("Socket not connected for send");
        return false;
    }
    
    int sent = send(socket->socket, (const char*)data, size, 0);
    if (sent == SOCKET_ERROR) {
        if (udp_socket_would_block()) {
            return false; // Would block, not an error
        }
        LOG_ERROR("Send failed: %s", udp_socket_get_last_error());
        return false;
    }
    
    socket->packets_sent++;
    socket->bytes_sent += sent;
    
    return sent == size;
}

bool udp_socket_send_to(UDPSocket* socket, const NetworkAddress* address, const void* data, u32 size) {
    if (!socket || !address || !data || size == 0 || size > UDP_MAX_PACKET_SIZE) return false;
    
    int sent = sendto(socket->socket, (const char*)data, size, 0, 
                      (struct sockaddr*)&address->addr, sizeof(address->addr));
    if (sent == SOCKET_ERROR) {
        if (udp_socket_would_block()) {
            return false; // Would block, not an error
        }
        LOG_ERROR("Send to failed: %s", udp_socket_get_last_error());
        return false;
    }
    
    socket->packets_sent++;
    socket->bytes_sent += sent;
    
    return sent == size;
}

bool udp_socket_receive(UDPSocket* socket, void* data, u32* size, NetworkAddress* from_address) {
    if (!socket || !data || !size) return false;
    
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    
    int received = recvfrom(socket->socket, (char*)data, *size, 0, 
                            (struct sockaddr*)&from, &from_len);
    if (received == SOCKET_ERROR) {
        if (udp_socket_would_block()) {
            return false; // Would block, not an error
        }
        LOG_ERROR("Receive failed: %s", udp_socket_get_last_error());
        socket->recv_errors++;
        return false;
    }
    
    *size = received;
    socket->packets_received++;
    socket->bytes_received += received;
    
    if (from_address) {
        memcpy(&from_address->addr, &from, sizeof(from));
        udp_address_to_string(from_address, from_address->address, sizeof(from_address->address));
        from_address->port = ntohs(from.sin_port);
    }
    
    return true;
}

bool udp_socket_receive_from(UDPSocket* socket, void* data, u32* size, NetworkAddress* from_address) {
    return udp_socket_receive(socket, data, size, from_address);
}

bool udp_address_from_string(NetworkAddress* address, const char* address_str) {
    if (!address || !address_str) return false;
    
    memset(address, 0, sizeof(NetworkAddress));
    strncpy(address->address, address_str, sizeof(address->address) - 1);
    
    address->addr.sin_family = AF_INET;
    address->addr.sin_port = htons(address->port);
    
    // Try to parse as IP address first
    if (inet_pton(AF_INET, address_str, &address->addr.sin_addr) == 1) {
        return true;
    }
    
    // Try to resolve as hostname
    struct hostent* host = gethostbyname(address_str);
    if (!host) {
        LOG_ERROR("Failed to resolve hostname: %s", address_str);
        return false;
    }
    
    memcpy(&address->addr.sin_addr, host->h_addr_list[0], host->h_length);
    return true;
}

bool udp_address_to_string(const NetworkAddress* address, char* buffer, u32 buffer_size) {
    if (!address || !buffer || buffer_size == 0) return false;
    
    inet_ntop(AF_INET, &address->addr.sin_addr, buffer, buffer_size);
    return true;
}

bool udp_address_equals(const NetworkAddress* a, const NetworkAddress* b) {
    if (!a || !b) return false;
    
    return a->addr.sin_addr.s_addr == b->addr.sin_addr.s_addr && a->port == b->port;
}

void udp_address_set_port(NetworkAddress* address, u16 port) {
    if (!address) return;
    address->port = port;
    address->addr.sin_port = htons(port);
}

void udp_socket_get_stats(const UDPSocket* socket, UDPStats* stats) {
    if (!socket || !stats) return;
    
    stats->packets_sent = socket->packets_sent;
    stats->packets_received = socket->packets_received;
    stats->bytes_sent = socket->bytes_sent;
    stats->bytes_received = socket->bytes_received;
    stats->send_errors = socket->send_errors;
    stats->recv_errors = socket->recv_errors;
    stats->avg_latency_ms = 0.0f; // Latency tracking not implemented for basic UDP socket
}

void udp_socket_reset_stats(UDPSocket* socket) {
    if (!socket) return;
    
    socket->packets_sent = 0;
    socket->packets_received = 0;
    socket->bytes_sent = 0;
    socket->bytes_received = 0;
    socket->send_errors = 0;
    socket->recv_errors = 0;
}

const char* udp_socket_get_last_error(void) {
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

bool udp_socket_would_block(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

bool network_initialize(void) {
#ifdef _WIN32
    if (winsock_initialized) return true;
    
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        LOG_ERROR("WSAStartup failed: %d", result);
        return false;
    }
    
    winsock_initialized = true;
    LOG_INFO("Winsock initialized");
#endif
    return true;
}

void network_shutdown(void) {
#ifdef _WIN32
    if (winsock_initialized) {
        WSACleanup();
        winsock_initialized = false;
        LOG_INFO("Winsock shutdown");
    }
#endif
}

bool network_is_initialized(void) {
#ifdef _WIN32
    return winsock_initialized;
#else
    return true;
#endif
}
