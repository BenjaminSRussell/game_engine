// socket.c - Cross-platform UDP socket implementation with IPv6 support
// TODO: MVP PATH - Add socket options for buffer sizes and timeout configuration
// TODO: MVP PATH - Implement NAT traversal helpers (UPnP, NAT-PMP)
// TODO: MVP PATH - Add bandwidth throttling and QoS support
#include "include/network/socket.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif

#define MAX_ADDRESSES 32
#define PING_HISTORY_SIZE 16
#define QUALITY_UPDATE_INTERVAL 1.0f // seconds

struct NetSocket {
#ifdef _WIN32
    SOCKET handle;
    SOCKET handle_ipv6;
#else
    int handle;
    int handle_ipv6;
#endif
    uint16_t port;
    bool is_open;
    bool supports_ipv6;
    
    // Connection quality monitoring
    ConnectionQuality quality;
    uint64_t ping_times[PING_HISTORY_SIZE];
    uint32_t ping_index;
    uint64_t last_ping_time;
    float quality_update_timer;
    
    // Statistics tracking
    uint32_t packets_sent_this_second;
    uint32_t bytes_sent_this_second;
    uint64_t last_second_time;
};

// Utility functions for address handling
static uint64_t get_timestamp_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000) / frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

bool address_parse_ipv4(const char* str, NetAddress* addr) {
    if (!str || !addr) return false;
    
    struct in_addr in_addr;
    if (inet_pton(AF_INET, str, &in_addr) != 1) {
        return false;
    }
    
    addr->type = ADDRESS_TYPE_IPV4;
    addr->address.ipv4 = ntohl(in_addr.s_addr);
    return true;
}

bool address_parse_ipv6(const char* str, NetAddress* addr) {
    if (!str || !addr) return false;
    
    struct in6_addr in6_addr;
    if (inet_pton(AF_INET6, str, &in6_addr) != 1) {
        return false;
    }
    
    addr->type = ADDRESS_TYPE_IPV6;
    memcpy(addr->address.ipv6, &in6_addr, 16);
    return true;
}

bool address_parse(const char* str, NetAddress* addr) {
    if (!str || !addr) return false;
    
    // Try IPv4 first
    if (address_parse_ipv4(str, addr)) {
        return true;
    }
    
    // Then try IPv6
    if (address_parse_ipv6(str, addr)) {
        return true;
    }
    
    return false;
}

void address_to_string(const NetAddress* addr, char* buffer, size_t buffer_size) {
    if (!addr || !buffer || buffer_size == 0) {
        if (buffer && buffer_size > 0) buffer[0] = '\0';
        return;
    }
    
    if (addr->type == ADDRESS_TYPE_IPV4) {
        struct in_addr in_addr;
        in_addr.s_addr = htonl(addr->address.ipv4);
        inet_ntop(AF_INET, &in_addr, buffer, buffer_size);
    } else if (addr->type == ADDRESS_TYPE_IPV6) {
        struct in6_addr in6_addr;
        memcpy(&in6_addr, addr->address.ipv6, 16);
        inet_ntop(AF_INET6, &in6_addr, buffer, buffer_size);
    } else {
        snprintf(buffer, buffer_size, "<invalid>");
    }
}

bool address_equals(const NetAddress* a, const NetAddress* b) {
    if (!a || !b) return false;
    if (a->type != b->type) return false;
    if (a->port != b->port) return false;
    
    if (a->type == ADDRESS_TYPE_IPV4) {
        return a->address.ipv4 == b->address.ipv4;
    } else if (a->type == ADDRESS_TYPE_IPV6) {
        return memcmp(a->address.ipv6, b->address.ipv6, 16) == 0;
    }
    
    return false;
}

static void update_quality_metrics(NetSocket* sock) {
    if (!sock) return;
    
    uint64_t current_time = get_timestamp_ms();
    float delta_time = (current_time - sock->last_second_time) / 1000.0f;
    
    if (delta_time >= 1.0f) {
        // Calculate bandwidth
        sock->quality.bandwidth_kbps = (sock->bytes_sent_this_second * 8.0f) / delta_time / 1024.0f;
        
        // Reset counters
        sock->packets_sent_this_second = 0;
        sock->bytes_sent_this_second = 0;
        sock->last_second_time = current_time;
    }
    
    // Update timestamp
    sock->quality.last_update = current_time;
}

static void record_ping_time(NetSocket* sock, uint64_t rtt_us) {
    if (!sock) return;
    
    sock->ping_times[sock->ping_index] = rtt_us;
    sock->ping_index = (sock->ping_index + 1) % PING_HISTORY_SIZE;
    
    // Calculate average RTT and jitter
    uint64_t sum = 0;
    uint64_t min_time = UINT64_MAX;
    uint64_t max_time = 0;
    
    for (int i = 0; i < PING_HISTORY_SIZE; i++) {
        if (sock->ping_times[i] > 0) {
            sum += sock->ping_times[i];
            if (sock->ping_times[i] < min_time) min_time = sock->ping_times[i];
            if (sock->ping_times[i] > max_time) max_time = sock->ping_times[i];
        }
    }
    
    if (sum > 0) {
        sock->quality.rtt_ms = (float)sum / PING_HISTORY_SIZE / 1000.0f;
        sock->quality.jitter_ms = (float)(max_time - min_time) / 1000.0f;
    }
}

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
    return socket_create_ipv4(port);
}

static NetSocket *socket_create_ipv4(uint16_t port) {
    if (!init_socket_system()) {
        return NULL;
    }
    
    NetSocket *sock = malloc(sizeof(NetSocket));
    if (!sock) {
        log_error("Failed to allocate socket");
        return NULL;
    }
    
    memset(sock, 0, sizeof(NetSocket));
    sock->handle = socket(AF_INET, SOCK_DGRAM, 0);
    sock->handle_ipv6 = -1;
    sock->supports_ipv6 = false;
    
#ifdef _WIN32
    if (sock->handle == INVALID_SOCKET) {
#else
    if (sock->handle < 0) {
#endif
        log_error("Failed to create IPv4 socket: %s", socket_get_error());
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
        log_error("Failed to bind IPv4 socket to port %d: %s", port, socket_get_error());
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
    sock->last_second_time = get_timestamp_ms();
    
    log_info("Created IPv4 UDP socket on port %d", port);
    return sock;
}

NetSocket *socket_create_ipv6(uint16_t port) {
    if (!init_socket_system()) {
        return NULL;
    }
    
    NetSocket *sock = malloc(sizeof(NetSocket));
    if (!sock) {
        log_error("Failed to allocate socket");
        return NULL;
    }
    
    memset(sock, 0, sizeof(NetSocket));
    sock->handle = -1;
    sock->handle_ipv6 = socket(AF_INET6, SOCK_DGRAM, 0);
    sock->supports_ipv6 = true;
    
#ifdef _WIN32
    if (sock->handle_ipv6 == INVALID_SOCKET) {
#else
    if (sock->handle_ipv6 < 0) {
#endif
        log_error("Failed to create IPv6 socket: %s", socket_get_error());
        free(sock);
        return NULL;
    }
    
    set_non_blocking(sock->handle_ipv6);
    
    // Set IPv6-only mode to avoid dual-stack complications
    int ipv6_only = 1;
    setsockopt(sock->handle_ipv6, IPPROTO_IPV6, IPV6_V6ONLY, 
               (const char*)&ipv6_only, sizeof(ipv6_only));
    
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);
    
    if (bind(sock->handle_ipv6, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_error("Failed to bind IPv6 socket to port %d: %s", port, socket_get_error());
#ifdef _WIN32
        closesocket(sock->handle_ipv6);
#else
        close(sock->handle_ipv6);
#endif
        free(sock);
        return NULL;
    }
    
    sock->port = port;
    sock->is_open = true;
    sock->last_second_time = get_timestamp_ms();
    
    log_info("Created IPv6 UDP socket on port %d", port);
    return sock;
}

NetSocket *socket_create_dual_stack(uint16_t port) {
    if (!init_socket_system()) {
        return NULL;
    }
    
    NetSocket *sock = malloc(sizeof(NetSocket));
    if (!sock) {
        log_error("Failed to allocate socket");
        return NULL;
    }
    
    memset(sock, 0, sizeof(NetSocket));
    sock->supports_ipv6 = true;
    
    // Create IPv4 socket
    sock->handle = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    if (sock->handle == INVALID_SOCKET) {
#else
    if (sock->handle < 0) {
#endif
        log_error("Failed to create IPv4 socket for dual-stack: %s", socket_get_error());
        free(sock);
        return NULL;
    }
    
    // Create IPv6 socket
    sock->handle_ipv6 = socket(AF_INET6, SOCK_DGRAM, 0);
#ifdef _WIN32
    if (sock->handle_ipv6 == INVALID_SOCKET) {
#else
    if (sock->handle_ipv6 < 0) {
#endif
        log_warn("Failed to create IPv6 socket for dual-stack, falling back to IPv4 only: %s", socket_get_error());
        closesocket(sock->handle);
        sock->handle_ipv6 = -1;
        sock->supports_ipv6 = false;
    }
    
    set_non_blocking(sock->handle);
    if (sock->handle_ipv6 != -1) {
        set_non_blocking(sock->handle_ipv6);
        
        // Set dual-stack mode
        int ipv6_only = 0;
        setsockopt(sock->handle_ipv6, IPPROTO_IPV6, IPV6_V6ONLY, 
                   (const char*)&ipv6_only, sizeof(ipv6_only));
    }
    
    // Bind IPv4 socket
    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_addr.s_addr = INADDR_ANY;
    addr4.sin_port = htons(port);
    
    if (bind(sock->handle, (struct sockaddr*)&addr4, sizeof(addr4)) < 0) {
        log_error("Failed to bind IPv4 socket to port %d: %s", port, socket_get_error());
        goto cleanup_fail;
    }
    
    // Bind IPv6 socket if available
    if (sock->handle_ipv6 != -1) {
        struct sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(port);
        
        if (bind(sock->handle_ipv6, (struct sockaddr*)&addr6, sizeof(addr6)) < 0) {
            log_warn("Failed to bind IPv6 socket to port %d, continuing with IPv4 only: %s", 
                     port, socket_get_error());
            closesocket(sock->handle_ipv6);
            sock->handle_ipv6 = -1;
            sock->supports_ipv6 = false;
        }
    }
    
    sock->port = port;
    sock->is_open = true;
    sock->last_second_time = get_timestamp_ms();
    
    log_info("Created dual-stack UDP socket on port %d (IPv4: %s, IPv6: %s)", 
             port, "yes", sock->supports_ipv6 ? "yes" : "no");
    return sock;
    
cleanup_fail:
#ifdef _WIN32
    if (sock->handle != INVALID_SOCKET) closesocket(sock->handle);
    if (sock->handle_ipv6 != INVALID_SOCKET) closesocket(sock->handle_ipv6);
#else
    if (sock->handle >= 0) close(sock->handle);
    if (sock->handle_ipv6 >= 0) close(sock->handle_ipv6);
#endif
    free(sock);
    return NULL;
}

void socket_close(NetSocket *sock) {
    if (!sock || !sock->is_open) {
        return;
    }
    
#ifdef _WIN32
    if (sock->handle != INVALID_SOCKET) closesocket(sock->handle);
    if (sock->handle_ipv6 != INVALID_SOCKET) closesocket(sock->handle_ipv6);
#else
    if (sock->handle >= 0) close(sock->handle);
    if (sock->handle_ipv6 >= 0) close(sock->handle_ipv6);
#endif
    
    sock->is_open = false;
    uint16_t port = sock->port;
    free(sock);
    
    log_info("Closed socket(s) on port %d", port);
}

bool socket_send(NetSocket *sock, const NetAddress *addr, const void *data, uint32_t size) {
    if (!sock || !sock->is_open || !addr || !data || size == 0) {
        return false;
    }
    
    if (addr->type == ADDRESS_TYPE_IPV4) {
        return socket_send_ipv4(sock, addr, data, size);
    } else if (addr->type == ADDRESS_TYPE_IPV6) {
        return socket_send_ipv6(sock, addr, data, size);
    }
    
    return false;
}

bool socket_send_ipv4(NetSocket *sock, const NetAddress *addr, const void *data, uint32_t size) {
    if (!sock || !sock->is_open || !addr || !data || size == 0 || addr->type != ADDRESS_TYPE_IPV4) {
        return false;
    }
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(addr->port);
    dest_addr.sin_addr.s_addr = htonl(addr->address.ipv4);
    
    int bytes_sent = sendto(sock->handle, (const char*)data, size, 0,
                           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (bytes_sent < 0) {
        log_error("IPv4 socket send failed: %s", socket_get_error());
        return false;
    }
    
    // Update statistics
    sock->quality.packets_sent++;
    sock->quality.bytes_sent += size;
    sock->packets_sent_this_second++;
    sock->bytes_sent_this_second += size;
    update_quality_metrics(sock);
    
    return (uint32_t)bytes_sent == size;
}

bool socket_send_ipv6(NetSocket *sock, const NetAddress *addr, const void *data, uint32_t size) {
    if (!sock || !sock->is_open || !addr || !data || size == 0 || addr->type != ADDRESS_TYPE_IPV6) {
        return false;
    }
    
    if (sock->handle_ipv6 == -1) {
        log_error("IPv6 socket not available");
        return false;
    }
    
    struct sockaddr_in6 dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(addr->port);
    memcpy(&dest_addr.sin6_addr, addr->address.ipv6, 16);
    
    int bytes_sent = sendto(sock->handle_ipv6, (const char*)data, size, 0,
                           (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (bytes_sent < 0) {
        log_error("IPv6 socket send failed: %s", socket_get_error());
        return false;
    }
    
    // Update statistics
    sock->quality.packets_sent++;
    sock->quality.bytes_sent += size;
    sock->packets_sent_this_second++;
    sock->bytes_sent_this_second += size;
    update_quality_metrics(sock);
    
    return (uint32_t)bytes_sent == size;
}

int socket_receive(NetSocket *sock, NetAddress *from, void *buffer, uint32_t buffer_size) {
    if (!sock || !sock->is_open || !from || !buffer || buffer_size == 0) {
        return 0;
    }
    
    int max_bytes = 0;
    
    // Try IPv4 first
    if (sock->handle != -1) {
        struct sockaddr_in sender_addr;
        socklen_t addr_len = sizeof(sender_addr);
        
        int bytes_received = recvfrom(sock->handle, (char*)buffer, buffer_size, 0,
                                     (struct sockaddr*)&sender_addr, &addr_len);
        
        if (bytes_received > 0) {
            from->type = ADDRESS_TYPE_IPV4;
            from->address.ipv4 = ntohl(sender_addr.sin_addr.s_addr);
            from->port = ntohs(sender_addr.sin_port);
            
            // Update statistics
            sock->quality.packets_recv++;
            sock->quality.bytes_recv += bytes_received;
            update_quality_metrics(sock);
            
            return bytes_received;
        } else if (bytes_received < 0) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                log_error("IPv4 socket receive failed: %s", socket_get_error());
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                log_error("IPv4 socket receive failed: %s", socket_get_error());
            }
#endif
        }
    }
    
    // Try IPv6
    if (sock->handle_ipv6 != -1) {
        struct sockaddr_in6 sender_addr6;
        socklen_t addr_len6 = sizeof(sender_addr6);
        
        int bytes_received = recvfrom(sock->handle_ipv6, (char*)buffer, buffer_size, 0,
                                     (struct sockaddr*)&sender_addr6, &addr_len6);
        
        if (bytes_received > 0) {
            from->type = ADDRESS_TYPE_IPV6;
            memcpy(from->address.ipv6, &sender_addr6.sin6_addr, 16);
            from->port = ntohs(sender_addr6.sin6_port);
            
            // Update statistics
            sock->quality.packets_recv++;
            sock->quality.bytes_recv += bytes_received;
            update_quality_metrics(sock);
            
            return bytes_received;
        } else if (bytes_received < 0) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                log_error("IPv6 socket receive failed: %s", socket_get_error());
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                log_error("IPv6 socket receive failed: %s", socket_get_error());
            }
#endif
        }
    }
    
    return 0; // No data available
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

void socket_get_quality_metrics(NetSocket *sock, ConnectionQuality *quality) {
    if (!sock || !quality) {
        return;
    }
    
    // Update metrics before returning
    update_quality_metrics(sock);
    
    // Calculate packet loss if we have sent packets
    if (sock->quality.packets_sent > 0) {
        uint32_t expected_packets = sock->quality.packets_sent;
        uint32_t received_packets = sock->quality.packets_recv;
        // This is a simplified calculation - in reality you'd need sequence numbers
        sock->quality.packet_loss = fmaxf(0.0f, 
            (float)(expected_packets - received_packets) / expected_packets);
    }
    
    *quality = sock->quality;
}

void socket_reset_quality_metrics(NetSocket *sock) {
    if (!sock) {
        return;
    }
    
    memset(&sock->quality, 0, sizeof(ConnectionQuality));
    memset(sock->ping_times, 0, sizeof(sock->ping_times));
    sock->ping_index = 0;
    sock->last_ping_time = 0;
    sock->quality_update_timer = 0.0f;
    sock->packets_sent_this_second = 0;
    sock->bytes_sent_this_second = 0;
    sock->last_second_time = get_timestamp_ms();
    
    log_info("Reset socket quality metrics");
}

// TODO: MVP PATH - Add socket statistics tracking (bytes sent/received, packet counts)
// TODO: MVP PATH - Implement socket-level congestion control
// TODO: MVP PATH - Add support for multicast sockets
// TODO: MVP PATH - Implement socket security (TLS/DTLS support)
