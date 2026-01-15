#include "networking_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

/* ============================================================================
 * SOCKET IMPLEMENTATION
 * ============================================================================
 */

static bool g_socket_system_initialized = false;

static bool socket_init_system(void) {
  if (g_socket_system_initialized)
    return true;

#ifdef _WIN32
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (result != 0) {
    LOG_ERROR(LOG_CAT_GENERAL, "WSAStartup failed: %d", result);
    return false;
  }
#endif

  g_socket_system_initialized = true;
  LOG_INFO(LOG_CAT_GENERAL, "Socket system initialized");
  return true;
}

static void socket_cleanup_system(void) __attribute__((unused));
static void socket_cleanup_system(void) {
  if (!g_socket_system_initialized)
    return;

#ifdef _WIN32
  WSACleanup();
#endif

  g_socket_system_initialized = false;
  LOG_INFO(LOG_CAT_GENERAL, "Socket system cleaned up");
}

static void socket_set_non_blocking(int sock) {
#ifdef _WIN32
  u_long mode = 1;
  ioctlsocket(sock, FIONBIO, &mode);
#else
  int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

NetSocket *socket_create(uint16_t port) {
  if (!socket_init_system()) {
    return NULL;
  }

  NetSocket *sock = UNIFIED_ALLOC(sizeof(NetSocket));
  if (!sock) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate socket");
    return NULL;
  }
  memset(sock, 0, sizeof(NetSocket));

#ifdef _WIN32
  sock->handle = (void *)socket(AF_INET, SOCK_DGRAM, 0);
  if (sock->handle == (void *)INVALID_SOCKET) {
#else
  sock->handle = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock->handle < 0) {
#endif
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to create socket");
    UNIFIED_FREE(sock);
    return NULL;
  }

  socket_set_non_blocking(sock->handle);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

#ifdef _WIN32
  if (bind((SOCKET)sock->handle, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#else
  if (bind(sock->handle, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#endif
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to bind socket to port %d", port);
#ifdef _WIN32
    closesocket((SOCKET)sock->handle);
#else
    close(sock->handle);
#endif
    UNIFIED_FREE(sock);
    return NULL;
  }

  sock->port = port;
  sock->is_open = true;

  LOG_INFO(LOG_CAT_GENERAL, "Created UDP socket on port %d", port);
  return sock;
}

void socket_close(NetSocket *sock) {
  if (!sock || !sock->is_open) {
    return;
  }

#ifdef _WIN32
  closesocket((SOCKET)sock->handle);
#else
  close(sock->handle);
#endif

  uint16_t port = sock->port;
  sock->is_open = false;
  UNIFIED_FREE(sock);

  LOG_INFO(LOG_CAT_GENERAL, "Closed socket on port %d", port);
}

bool socket_send(NetSocket *sock, const NetAddress *addr, const void *data,
                 uint32_t size) {
  if (!sock || !sock->is_open || !addr || !data || size == 0) {
    return false;
  }

  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(addr->port);
  dest_addr.sin_addr.s_addr = addr->host;

#ifdef _WIN32
  int bytes_sent = sendto((SOCKET)sock->handle, (const char *)data, size, 0,
                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));
#else
  int bytes_sent = sendto(sock->handle, (const char *)data, size, 0,
                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));
#endif

  if (bytes_sent < 0) {
    return false;
  }

  return (uint32_t)bytes_sent == size;
}

int socket_receive(NetSocket *sock, NetAddress *from, void *buffer,
                   uint32_t buffer_size) {
  if (!sock || !sock->is_open || !from || !buffer || buffer_size == 0) {
    return 0;
  }

  struct sockaddr_in sender_addr;
  socklen_t addr_len = sizeof(sender_addr);

#ifdef _WIN32
  int bytes_received =
      recvfrom((SOCKET)sock->handle, (char *)buffer, buffer_size, 0,
               (struct sockaddr *)&sender_addr, &addr_len);
#else
  int bytes_received = recvfrom(sock->handle, (char *)buffer, buffer_size, 0,
                                (struct sockaddr *)&sender_addr, &addr_len);
#endif

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
