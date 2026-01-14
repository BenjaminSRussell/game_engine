#pragma once

#include "core/core.h"
#include "engine/include/core/memory.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
#else
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#define TCP_BUFFER_SIZE 4096

typedef struct {
    char address[64];
    u16 port;
    struct sockaddr_in addr;
} TCPAddress;

typedef struct {
    socket_t socket;
    TCPAddress local_address;
    TCPAddress remote_address;
    bool is_bound;
    bool is_listening;
    bool is_connected;
    u32 bytes_sent;
    u32 bytes_received;
    u32 send_errors;
    u32 recv_errors;
    u32 connections_accepted;
    u32 connections_closed;
} TCPSocket;

typedef struct {
    u32 connections_accepted;
    u32 connections_closed;
    u32 bytes_sent;
    u32 bytes_received;
    u32 send_errors;
    u32 recv_errors;
} TCPStats;

/* TCP Socket Management */
TCPSocket* tcp_socket_create(void);
void tcp_socket_destroy(TCPSocket* socket);

/* Connection Management */
bool tcp_socket_bind(TCPSocket* socket, const char* address, u16 port);
bool tcp_socket_listen(TCPSocket* socket, u32 backlog);
TCPSocket* tcp_socket_accept(TCPSocket* socket);
bool tcp_socket_connect(TCPSocket* socket, const char* address, u16 port);
void tcp_socket_disconnect(TCPSocket* socket);
bool tcp_socket_set_blocking(TCPSocket* socket, bool blocking);
bool tcp_socket_set_timeout(TCPSocket* socket, u32 timeout_ms);

/* Data Transmission */
int tcp_socket_send(TCPSocket* socket, const void* data, u32 size);
int tcp_socket_receive(TCPSocket* socket, void* data, u32 size);

/* Utility Functions */
bool tcp_address_from_string(TCPAddress* address, const char* address_str);
bool tcp_address_to_string(const TCPAddress* address, char* buffer, u32 buffer_size);
bool tcp_address_equals(const TCPAddress* a, const TCPAddress* b);
void tcp_address_set_port(TCPAddress* address, u16 port);

/* Statistics */
void tcp_socket_get_stats(const TCPSocket* socket, TCPStats* stats);
void tcp_socket_reset_stats(TCPSocket* socket);

/* Error Handling */
const char* tcp_socket_get_last_error(void);
bool tcp_socket_would_block(void);
