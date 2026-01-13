#pragma once

#include "core/core.h"
#include "core/memory.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
#else
    #include <include/network/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#define UDP_MAX_PACKET_SIZE 1500
#define UDP_BUFFER_SIZE 2048

typedef struct {
    char address[64];
    u16 port;
    struct sockaddr_in addr;
} NetworkAddress;

typedef struct {
    socket_t socket;
    NetworkAddress local_address;
    NetworkAddress remote_address;
    bool is_bound;
    bool is_connected;
    u8 send_buffer[UDP_BUFFER_SIZE];
    u8 recv_buffer[UDP_BUFFER_SIZE];
    u32 send_buffer_size;
    u32 recv_buffer_size;
} UDPSocket;

typedef struct {
    u32 packets_sent;
    u32 packets_received;
    u32 bytes_sent;
    u32 bytes_received;
    u32 send_errors;
    u32 recv_errors;
    f32 avg_latency_ms;
} UDPStats;

// UDP Socket Management
UDPSocket* udp_socket_create(void);
void udp_socket_destroy(UDPSocket* socket);

// Connection Management
bool udp_socket_bind(UDPSocket* socket, const char* address, u16 port);
bool udp_socket_connect(UDPSocket* socket, const char* address, u16 port);
void udp_socket_disconnect(UDPSocket* socket);
bool udp_socket_set_blocking(UDPSocket* socket, bool blocking);
bool udp_socket_set_timeout(UDPSocket* socket, u32 timeout_ms);

// Data Transmission
bool udp_socket_send(UDPSocket* socket, const void* data, u32 size);
bool udp_socket_send_to(UDPSocket* socket, const NetworkAddress* address, const void* data, u32 size);
bool udp_socket_receive(UDPSocket* socket, void* data, u32* size, NetworkAddress* from_address);
bool udp_socket_receive_from(UDPSocket* socket, void* data, u32* size, NetworkAddress* from_address);

// Utility Functions
bool udp_address_from_string(NetworkAddress* address, const char* address_str);
bool udp_address_to_string(const NetworkAddress* address, char* buffer, u32 buffer_size);
bool udp_address_equals(const NetworkAddress* a, const NetworkAddress* b);
void udp_address_set_port(NetworkAddress* address, u16 port);

// Statistics
void udp_socket_get_stats(const UDPSocket* socket, UDPStats* stats);
void udp_socket_reset_stats(UDPSocket* socket);

// Error Handling
const char* udp_socket_get_last_error(void);
bool udp_socket_would_block(void);

// Network Utilities
bool network_initialize(void);
void network_shutdown(void);
bool network_is_initialized(void);
