// udp_socket.c - UDP Socket Implementation
#include <include/core/logger.h>
#include <include/network/transport/udp_socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

static bool g_network_initialized = false;

bool network_initialize(void) {
  if (g_network_initialized) {
    LOG_WARN("Network already initialized");
    return true;
  }

#ifdef _WIN32
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (result != 0) {
    LOG_ERROR("WSAStartup failed: %d", result);
    return false;
  }
#else
  // No initialization needed for Unix systems
#endif

  g_network_initialized = true;
  LOG_INFO("Network initialized");
  return true;
}

void network_shutdown(void) {
  if (!g_network_initialized)
    return;

#ifdef _WIN32
  WSACleanup();
#endif

  g_network_initialized = false;
  LOG_INFO("Network shutdown");
}

bool network_is_initialized(void) { return g_network_initialized; }

UDPSocket *udp_socket_create(void) {
  if (!network_is_initialized()) {
    LOG_ERROR("Network not initialized");
    return NULL;
  }

  UDPSocket *udp_socket = (UDPSocket *)calloc(1, sizeof(UDPSocket));
  if (!udp_socket) {
    LOG_ERROR("Failed to allocate UDP socket");
    return NULL;
  }

  // Create socket
  udp_socket->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udp_socket->socket == INVALID_SOCKET) {
    LOG_ERROR("Failed to create UDP socket: %s", udp_socket_get_last_error());
    free(udp_socket);
    return NULL;
  }

  // Set socket to non-blocking
#ifdef _WIN32
  u32 mode = 1; // FIONBIO
  ioctlsocket(udp_socket->socket, FIONBIO, &mode);
#else
  int flags = fcntl(udp_socket->socket, F_GETFL, 0);
  fcntl(udp_socket->socket, F_SETFL, flags | O_NONBLOCK);
#endif

  udp_socket->is_bound = false;
  udp_socket->is_connected = false;
  udp_socket->send_buffer_size = 0;
  udp_socket->recv_buffer_size = 0;

  // Initialize addresses
  memset(&udp_socket->local_address, 0, sizeof(NetworkAddress));
  memset(&udp_socket->remote_address, 0, sizeof(NetworkAddress));

  udp_socket_reset_stats(udp_socket);

  LOG_DEBUG("Created UDP socket");
  return udp_socket;
}

void udp_socket_destroy(UDPSocket *socket) {
  if (!socket)
    return;

  if (socket->socket != INVALID_SOCKET) {
#ifdef _WIN32
    closesocket(socket->socket);
#else
    close(socket->socket);
#endif
  }

  free(socket);
  LOG_DEBUG("Destroyed UDP socket");
}

bool udp_socket_bind(UDPSocket *socket, const char *address, u16 port) {
  if (!socket || !address)
    return false;

  if (!udp_address_from_string(&socket->local_address, address)) {
    LOG_ERROR("Invalid bind address: %s", address);
    return false;
  }

  udp_address_set_port(&socket->local_address, port);
  socket->local_address.addr.sin_family = AF_INET;

  int result =
      bind(socket->socket, (struct sockaddr *)&socket->local_address.addr,
           sizeof(socket->local_address.addr));
  if (result == SOCKET_ERROR) {
    LOG_ERROR("Failed to bind UDP socket to %s:%u: %s", address, port,
              udp_socket_get_last_error());
    return false;
  }

  socket->is_bound = true;
  LOG_INFO("UDP socket bound to %s:%u", address, port);
  return true;
}

bool udp_socket_connect(UDPSocket *socket, const char *address, u16 port) {
  if (!socket || !address)
    return false;

  if (!udp_address_from_string(&socket->remote_address, address)) {
    LOG_ERROR("Invalid connect address: %s", address);
    return false;
  }

  udp_address_set_port(&socket->remote_address, port);
  socket->remote_address.addr.sin_family = AF_INET;

  // UDP is connectionless, but we set the default remote address for send_to
  // calls
  socket->is_connected = true;

  LOG_INFO("UDP socket set remote address to %s:%u", address, port);
  return true;
}

void udp_socket_disconnect(UDPSocket *socket) {
  if (!socket)
    return;

  memset(&socket->remote_address, 0, sizeof(NetworkAddress));
  socket->is_connected = false;

  LOG_DEBUG("UDP socket disconnected");
}

bool udp_socket_set_blocking(UDPSocket *socket, bool blocking) {
  if (!socket)
    return false;

#ifdef _WIN32
  u32 mode = blocking ? 0 : 1; // FIONBIO
  int result = ioctlsocket(socket->socket, FIONBIO, &mode);
  return result == 0;
#else
  int flags = fcntl(socket->socket, F_GETFL, 0);
  return fcntl(socket->socket, F_SETFL,
               blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
#endif
}

bool udp_socket_set_timeout(UDPSocket *socket, u32 timeout_ms) {
  if (!socket)
    return false;

#ifdef _WIN32
  DWORD timeout = timeout_ms;
  return setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO,
                    (const char *)&timeout, sizeof(timeout)) == 0;
#else
  struct timeval timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;
  return setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                    sizeof(timeout)) == 0;
#endif
}

bool udp_socket_send(UDPSocket *socket, const void *data, u32 size) {
  if (!socket || !data || size == 0)
    return false;

  if (!socket->is_connected) {
    LOG_ERROR("UDP socket not connected");
    return false;
  }

  int bytes_sent = sendto(socket->socket, (const char *)data, size, 0,
                          (struct sockaddr *)&socket->remote_address.addr,
                          sizeof(socket->remote_address.addr));

  if (bytes_sent == SOCKET_ERROR) {
    socket->send_errors++;
    LOG_ERROR("UDP send failed: %s", udp_socket_get_last_error());
    return false;
  }

  socket->packets_sent++;
  socket->bytes_sent += bytes_sent;

  LOG_DEBUG("UDP sent %u bytes", bytes_sent);
  return true;
}

bool udp_socket_send_to(UDPSocket *socket, const NetworkAddress *address,
                        const void *data, u32 size) {
  if (!socket || !address || !data || size == 0)
    return false;

  int bytes_sent =
      sendto(socket->socket, (const char *)data, size, 0,
             (struct sockaddr *)&address->addr, sizeof(address->addr));

  if (bytes_sent == SOCKET_ERROR) {
    socket->send_errors++;
    LOG_ERROR("UDP send_to failed: %s", udp_socket_get_last_error());
    return false;
  }

  socket->packets_sent++;
  socket->bytes_sent += bytes_sent;

  char address_str[128] = {0};
  if (udp_address_to_string(address, address_str, sizeof(address_str))) {
    LOG_DEBUG("UDP sent %u bytes to %s", bytes_sent, address_str);
  } else {
    LOG_DEBUG("UDP sent %u bytes", bytes_sent);
  }
  return true;
}

bool udp_socket_receive(UDPSocket *socket, void *data, u32 *size,
                        NetworkAddress *from_address) {
  if (!socket || !data || !size || !from_address)
    return false;

  socklen_t addr_size = sizeof(from_address->addr);
  int bytes_received =
      recvfrom(socket->socket, (char *)data, *size, 0,
               (struct sockaddr *)&from_address->addr, &addr_size);

  if (bytes_received == SOCKET_ERROR) {
    socket->recv_errors++;
    if (udp_socket_would_block()) {
      return false; // No data available
    }

    LOG_ERROR("UDP receive failed: %s", udp_socket_get_last_error());
    return false;
  }

  *size = (u32)bytes_received;
  socket->packets_received++;
  socket->bytes_received += *size;

  from_address->addr.sin_family = AF_INET;
  from_address->port = ntohs(from_address->addr.sin_port);
  inet_ntop(AF_INET, &from_address->addr.sin_addr, from_address->address,
            sizeof(from_address->address));

  LOG_DEBUG("UDP received %u bytes from %s:%u", *size, from_address->address,
            from_address->port);
  return true;
}

bool udp_socket_receive_from(UDPSocket *socket, void *data, u32 *size,
                             NetworkAddress *from_address) {
  if (!socket || !data || !size || !from_address)
    return false;

  socklen_t addr_size = sizeof(from_address->addr);
  int bytes_received =
      recvfrom(socket->socket, (char *)data, *size, MSG_WAITALL,
               (struct sockaddr *)&from_address->addr, &addr_size);

  if (bytes_received == SOCKET_ERROR) {
    socket->recv_errors++;
    if (udp_socket_would_block()) {
      return false; // No data available
    }

    LOG_ERROR("UDP receive_from failed: %s", udp_socket_get_last_error());
    return false;
  }

  *size = (u32)bytes_received;
  socket->packets_received++;
  socket->bytes_received += *size;

  from_address->addr.sin_family = AF_INET;
  from_address->port = ntohs(from_address->addr.sin_port);
  inet_ntop(AF_INET, &from_address->addr.sin_addr, from_address->address,
            sizeof(from_address->address));

  LOG_DEBUG("UDP received %u bytes from %s:%u", *size, from_address->address,
            from_address->port);
  return true;
}

bool udp_address_from_string(NetworkAddress *address, const char *address_str) {
  if (!address || !address_str)
    return false;

  memset(address, 0, sizeof(NetworkAddress));

  // Parse IPv4 address (simplified)
  char *colon = strchr(address_str, ':');
  if (!colon) {
    LOG_ERROR("Invalid address format: %s", address_str);
    return false;
  }

  char host[256];
  u16 port;

  // Copy host part
  size_t host_len = colon - address_str;
  strncpy(host, address_str, host_len);
  host[host_len] = '\0';

  // Parse port part
  char *port_str = colon + 1;
  port = (u16)atoi(port_str);

  // Convert host to network byte order
  address->addr.sin_family = AF_INET;
  address->addr.sin_port = htons(port);

  struct hostent *host_entry = gethostbyname(host);
  if (!host_entry) {
    LOG_ERROR("Failed to resolve host: %s", host);
    return false;
  }

  memcpy(&address->addr.sin_addr, host_entry->h_addr_list[0],
         sizeof(struct in_addr));

  strncpy(address->address, host, sizeof(address->address) - 1);
  address->port = port;

  return true;
}

bool udp_address_to_string(const NetworkAddress *address, char *buffer,
                           u32 buffer_size) {
  if (!address || !buffer || buffer_size == 0)
    return false;

  char host[256];
  if (!inet_ntop(AF_INET, &address->addr.sin_addr, host, INET_ADDRSTRLEN)) {
    return false;
  }

  snprintf(buffer, buffer_size, "%s:%u", host, address->port);
  return true;
}

bool udp_address_equals(const NetworkAddress *a, const NetworkAddress *b) {
  if (!a || !b)
    return false;

  return a->addr.sin_addr.s_addr == b->addr.sin_addr.s_addr &&
         a->addr.sin_port == b->addr.sin_port;
}

void udp_address_set_port(NetworkAddress *address, u16 port) {
  if (!address)
    return;
  address->port = port;
  address->addr.sin_port = htons(port);
}

void udp_socket_get_stats(const UDPSocket *socket, UDPStats *stats) {
  if (!socket || !stats)
    return;

  stats->packets_sent = socket->packets_sent;
  stats->packets_received = socket->packets_received;
  stats->bytes_sent = socket->bytes_sent;
  stats->bytes_received = socket->bytes_received;
  stats->send_errors = socket->send_errors;
  stats->recv_errors = socket->recv_errors;

  // Calculate average latency (simplified)
  if (socket->packets_received > 0) {
    stats->avg_latency_ms =
        ((f32)socket->bytes_received / (f32)socket->packets_received) / 1024.0f;
  } else {
    stats->avg_latency_ms = 0.0f;
  }
}

void udp_socket_reset_stats(UDPSocket *socket) {
  if (!socket)
    return;

  socket->packets_sent = 0;
  socket->packets_received = 0;
  socket->bytes_sent = 0;
  socket->bytes_received = 0;
  socket->send_errors = 0;
  socket->recv_errors = 0;
}

const char *udp_socket_get_last_error(void) {
#ifdef _WIN32
  int error = WSAGetLastError();
  switch (error) {
  case WSAEWOULDBLOCK:
    return "Would block";
  case WSAECONNRESET:
    return "Connection reset by peer";
  case WSAETIMEDOUT:
    return "Connection timed out";
  case WSAECONNREFUSED:
    return "Connection refused";
  case WSAEHOSTUNREACH:
    return "Host unreachable";
  case WSAENOTCONN:
    return "Not connected";
  case WSAEADDRINUSE:
    return "Address already in use";
  case WSAEADDRNOTAVAIL:
    return "Address not available";
  case WSAECONNABORTED:
    return "Connection aborted";
  case WSAECONNRESET:
    return "Connection reset by peer";
  default:
    return "Unknown error";
  }
#else
  switch (errno) {
  case EAGAIN:
    return "Would block";
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
  case EWOULDBLOCK:
    return "Would block";
#endif
  case ECONNRESET:
    return "Connection reset by peer";
  case ETIMEDOUT:
    return "Connection timed out";
  case ECONNREFUSED:
    return "Connection refused";
  case EHOSTUNREACH:
    return "Host unreachable";
  case ENOTCONN:
    return "Not connected";
  case EADDRINUSE:
    return "Address already in use";
  case EADDRNOTAVAIL:
    return "Address not available";
  default:
    return "Unknown error";
  }
#endif
}

bool udp_socket_would_block(void) {
#ifdef _WIN32
  return WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}
