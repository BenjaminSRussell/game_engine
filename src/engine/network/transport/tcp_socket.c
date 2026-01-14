// tcp_socket.c - TCP Socket Implementation
#include <errno.h>
#include <include/core/logger.h>
#include <include/network/transport/tcp_socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#endif

static bool g_tcp_network_initialized = false;
static u32 g_tcp_socket_refcount = 0;

static bool tcp_network_initialize(void) {
  if (g_tcp_network_initialized) {
    return true;
  }

#ifdef _WIN32
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (result != 0) {
    LOG_ERROR("WSAStartup failed: %d", result);
    return false;
  }
#endif

  g_tcp_network_initialized = true;
  return true;
}

static void tcp_network_shutdown(void) {
  if (!g_tcp_network_initialized)
    return;

#ifdef _WIN32
  WSACleanup();
#endif

  g_tcp_network_initialized = false;
}

static void tcp_address_from_sockaddr(TCPAddress *out_address,
                                      const struct sockaddr_in *addr) {
  if (!out_address || !addr)
    return;
  memset(out_address, 0, sizeof(*out_address));
  out_address->addr = *addr;
  out_address->port = ntohs(addr->sin_port);
  inet_ntop(AF_INET, &addr->sin_addr, out_address->address,
            sizeof(out_address->address));
}

TCPSocket *tcp_socket_create(void) {
  if (!tcp_network_initialize()) {
    LOG_ERROR("TCP network initialization failed");
    return NULL;
  }

  TCPSocket *tcp_socket = (TCPSocket *)calloc(1, sizeof(TCPSocket));
  if (!tcp_socket) {
    LOG_ERROR("Failed to allocate TCP socket");
    return NULL;
  }

  tcp_socket->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (tcp_socket->socket == INVALID_SOCKET) {
    LOG_ERROR("Failed to create TCP socket: %s", tcp_socket_get_last_error());
    free(tcp_socket);
    return NULL;
  }

  g_tcp_socket_refcount++;
  return tcp_socket;
}

void tcp_socket_destroy(TCPSocket *socket) {
  if (!socket)
    return;

  if (socket->socket != INVALID_SOCKET) {
    closesocket(socket->socket);
    socket->socket = INVALID_SOCKET;
  }

  free(socket);

  if (g_tcp_socket_refcount > 0) {
    g_tcp_socket_refcount--;
    if (g_tcp_socket_refcount == 0) {
      tcp_network_shutdown();
    }
  }
}

bool tcp_socket_bind(TCPSocket *socket, const char *address, u16 port) {
  if (!socket)
    return false;

  const char *bind_address =
      address && address[0] != '\0' ? address : "0.0.0.0";
  if (!tcp_address_from_string(&socket->local_address, bind_address)) {
    return false;
  }

  tcp_address_set_port(&socket->local_address, port);
  socket->local_address.addr.sin_family = AF_INET;

  int result =
      bind(socket->socket, (struct sockaddr *)&socket->local_address.addr,
           sizeof(socket->local_address.addr));
  if (result == SOCKET_ERROR) {
    socket->send_errors++;
    LOG_ERROR("Failed to bind TCP socket to %s:%u: %s", bind_address, port,
              tcp_socket_get_last_error());
    return false;
  }

  socket->is_bound = true;
  LOG_INFO("TCP socket bound to %s:%u", bind_address, port);
  return true;
}

bool tcp_socket_listen(TCPSocket *socket, u32 backlog) {
  if (!socket || !socket->is_bound)
    return false;

  int listen_backlog = backlog > 0 ? (int)backlog : SOMAXCONN;
  if (listen(socket->socket, listen_backlog) == SOCKET_ERROR) {
    LOG_ERROR("TCP listen failed: %s", tcp_socket_get_last_error());
    return false;
  }

  socket->is_listening = true;
  return true;
}

TCPSocket *tcp_socket_accept(TCPSocket *socket) {
  if (!socket || !socket->is_listening)
    return NULL;

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  socket_t client_socket =
      accept(socket->socket, (struct sockaddr *)&client_addr, &addr_len);

  if (client_socket == INVALID_SOCKET) {
    if (!tcp_socket_would_block()) {
      LOG_ERROR("TCP accept failed: %s", tcp_socket_get_last_error());
      socket->recv_errors++;
    }
    return NULL;
  }

  TCPSocket *client = (TCPSocket *)calloc(1, sizeof(TCPSocket));
  if (!client) {
    closesocket(client_socket);
    return NULL;
  }

  client->socket = client_socket;
  client->is_connected = true;
  tcp_address_from_sockaddr(&client->remote_address, &client_addr);

  struct sockaddr_in local_addr;
  socklen_t local_len = sizeof(local_addr);
  if (getsockname(client_socket, (struct sockaddr *)&local_addr, &local_len) ==
      0) {
    tcp_address_from_sockaddr(&client->local_address, &local_addr);
    client->is_bound = true;
  }

  socket->connections_accepted++;
  g_tcp_socket_refcount++;
  return client;
}

bool tcp_socket_connect(TCPSocket *socket, const char *address, u16 port) {
  if (!socket || !address)
    return false;

  if (!tcp_address_from_string(&socket->remote_address, address)) {
    return false;
  }

  tcp_address_set_port(&socket->remote_address, port);
  socket->remote_address.addr.sin_family = AF_INET;

  int result =
      connect(socket->socket, (struct sockaddr *)&socket->remote_address.addr,
              sizeof(socket->remote_address.addr));
  if (result == SOCKET_ERROR) {
    if (!tcp_socket_would_block()) {
      LOG_ERROR("TCP connect failed: %s", tcp_socket_get_last_error());
    }
    return false;
  }

  struct sockaddr_in local_addr;
  socklen_t local_len = sizeof(local_addr);
  if (getsockname(socket->socket, (struct sockaddr *)&local_addr, &local_len) ==
      0) {
    tcp_address_from_sockaddr(&socket->local_address, &local_addr);
    socket->is_bound = true;
  }

  socket->is_connected = true;
  return true;
}

void tcp_socket_disconnect(TCPSocket *socket) {
  if (!socket || socket->socket == INVALID_SOCKET)
    return;

#ifdef _WIN32
  shutdown(socket->socket, SD_BOTH);
#else
  shutdown(socket->socket, SHUT_RDWR);
#endif
  closesocket(socket->socket);
  socket->socket = INVALID_SOCKET;
  socket->is_connected = false;
  socket->is_listening = false;
  socket->connections_closed++;
}

bool tcp_socket_set_blocking(TCPSocket *socket, bool blocking) {
  if (!socket)
    return false;

#ifdef _WIN32
  u_long mode = blocking ? 0 : 1;
  return ioctlsocket(socket->socket, FIONBIO, &mode) == 0;
#else
  int flags = fcntl(socket->socket, F_GETFL, 0);
  if (flags < 0)
    return false;
  if (blocking) {
    flags &= ~O_NONBLOCK;
  } else {
    flags |= O_NONBLOCK;
  }
  return fcntl(socket->socket, F_SETFL, flags) == 0;
#endif
}

bool tcp_socket_set_timeout(TCPSocket *socket, u32 timeout_ms) {
  if (!socket)
    return false;

#ifdef _WIN32
  DWORD timeout = (DWORD)timeout_ms;
  if (setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO,
                 (const char *)&timeout, sizeof(timeout)) != 0) {
    return false;
  }
  return setsockopt(socket->socket, SOL_SOCKET, SO_SNDTIMEO,
                    (const char *)&timeout, sizeof(timeout)) == 0;
#else
  struct timeval timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;
  if (setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout)) != 0) {
    return false;
  }
  return setsockopt(socket->socket, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                    sizeof(timeout)) == 0;
#endif
}

int tcp_socket_send(TCPSocket *socket, const void *data, u32 size) {
  if (!socket || !data || size == 0)
    return -1;

  int bytes_sent = send(socket->socket, (const char *)data, size, 0);
  if (bytes_sent == SOCKET_ERROR) {
    if (tcp_socket_would_block()) {
      return 0;
    }
    socket->send_errors++;
    LOG_ERROR("TCP send failed: %s", tcp_socket_get_last_error());
    return -1;
  }

  socket->bytes_sent += (u32)bytes_sent;
  return bytes_sent;
}

int tcp_socket_receive(TCPSocket *socket, void *data, u32 size) {
  if (!socket || !data || size == 0)
    return -1;

  int bytes_received = recv(socket->socket, (char *)data, size, 0);
  if (bytes_received == 0) {
    socket->is_connected = false;
    return 0;
  }

  if (bytes_received == SOCKET_ERROR) {
    if (tcp_socket_would_block()) {
      return 0;
    }
    socket->recv_errors++;
    LOG_ERROR("TCP receive failed: %s", tcp_socket_get_last_error());
    return -1;
  }

  socket->bytes_received += (u32)bytes_received;
  return bytes_received;
}

bool tcp_address_from_string(TCPAddress *address, const char *address_str) {
  if (!address || !address_str)
    return false;

  memset(address, 0, sizeof(TCPAddress));

  char *colon = strchr(address_str, ':');
  if (!colon) {
    LOG_ERROR("Invalid address format: %s", address_str);
    return false;
  }

  char host[256];
  size_t host_len = (size_t)(colon - address_str);
  if (host_len >= sizeof(host)) {
    return false;
  }
  memcpy(host, address_str, host_len);
  host[host_len] = '\0';

  const char *port_str = colon + 1;
  u16 port = (u16)atoi(port_str);

  address->addr.sin_family = AF_INET;
  address->addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host, &address->addr.sin_addr) != 1) {
    struct hostent *host_entry = gethostbyname(host);
    if (!host_entry || !host_entry->h_addr_list[0]) {
      LOG_ERROR("Failed to resolve host: %s", host);
      return false;
    }
    memcpy(&address->addr.sin_addr, host_entry->h_addr_list[0],
           sizeof(struct in_addr));
  }

  strncpy(address->address, host, sizeof(address->address) - 1);
  address->port = port;
  return true;
}

bool tcp_address_to_string(const TCPAddress *address, char *buffer,
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

bool tcp_address_equals(const TCPAddress *a, const TCPAddress *b) {
  if (!a || !b)
    return false;

  return a->addr.sin_addr.s_addr == b->addr.sin_addr.s_addr &&
         a->addr.sin_port == b->addr.sin_port;
}

void tcp_address_set_port(TCPAddress *address, u16 port) {
  if (!address)
    return;
  address->port = port;
  address->addr.sin_port = htons(port);
}

void tcp_socket_get_stats(const TCPSocket *socket, TCPStats *stats) {
  if (!socket || !stats)
    return;

  stats->connections_accepted = socket->connections_accepted;
  stats->connections_closed = socket->connections_closed;
  stats->bytes_sent = socket->bytes_sent;
  stats->bytes_received = socket->bytes_received;
  stats->send_errors = socket->send_errors;
  stats->recv_errors = socket->recv_errors;
}

void tcp_socket_reset_stats(TCPSocket *socket) {
  if (!socket)
    return;

  socket->bytes_sent = 0;
  socket->bytes_received = 0;
  socket->send_errors = 0;
  socket->recv_errors = 0;
  socket->connections_accepted = 0;
  socket->connections_closed = 0;
}

const char *tcp_socket_get_last_error(void) {
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

bool tcp_socket_would_block(void) {
#ifdef _WIN32
  return WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}
