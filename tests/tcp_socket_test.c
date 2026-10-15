/*
 * tcp_socket_test.c
 * Address parsing, loopback send/receive, and socket stats.
 * Compiles tcp_socket.c directly and does not link Engine.
 *
 * bind/connect parse a "host:port" string, then the port argument replaces
 * the port taken from that string.
 */

#include "include/network/transport/tcp_socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static void expect(int cond, const char *msg) {
  if (!cond) {
    fail(msg);
  }
}

static u16 bound_port(const TCPSocket *socket) {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);

  memset(&addr, 0, sizeof(addr));
  if (getsockname(socket->socket, (struct sockaddr *)&addr, &len) != 0) {
    fail("getsockname");
  }
  return ntohs(addr.sin_port);
}

static void test_address_helpers(void) {
  TCPAddress addr;
  TCPAddress other;
  char buffer[64];
  char long_host[280];

  printf("Testing TCP address helpers...\n");
  expect(!tcp_address_from_string(NULL, "127.0.0.1:80"), "null address");
  expect(!tcp_address_from_string(&addr, NULL), "null string");
  expect(!tcp_address_from_string(&addr, "127.0.0.1"), "missing port");

  memset(long_host, 'a', sizeof(long_host) - 1);
  long_host[sizeof(long_host) - 1] = '\0';
  memcpy(long_host + 256, ":1", 3);
  expect(!tcp_address_from_string(&addr, long_host), "host too long");

  expect(tcp_address_from_string(&addr, "127.0.0.1:8080"), "parse loopback");
  expect(addr.port == 8080, "parsed port");
  expect(strcmp(addr.address, "127.0.0.1") == 0, "parsed host");
  expect(ntohs(addr.addr.sin_port) == 8080, "sockaddr port");
  expect(addr.addr.sin_addr.s_addr == htonl(0x7f000001u), "sockaddr addr");

  expect(tcp_address_to_string(&addr, buffer, sizeof(buffer)), "format");
  expect(strcmp(buffer, "127.0.0.1:8080") == 0, "formatted text");
  expect(!tcp_address_to_string(NULL, buffer, sizeof(buffer)), "null format");
  expect(!tcp_address_to_string(&addr, NULL, 8), "null buffer");
  expect(!tcp_address_to_string(&addr, buffer, 0), "zero buffer");

  expect(tcp_address_from_string(&other, "127.0.0.1:8080"), "parse other");
  expect(tcp_address_equals(&addr, &other), "equal addresses");
  tcp_address_set_port(&other, 9);
  expect(other.port == 9, "set port field");
  expect(ntohs(other.addr.sin_port) == 9, "set sockaddr port");
  expect(!tcp_address_equals(&addr, &other), "port mismatch");
  expect(!tcp_address_equals(NULL, &other), "null equals");
  expect(!tcp_address_equals(&addr, NULL), "null equals rhs");
  tcp_address_set_port(NULL, 1);

  expect(tcp_address_from_string(&addr, "localhost:80"), "resolve localhost");
  expect(addr.port == 80, "localhost port");
  expect(strcmp(addr.address, "localhost") == 0, "localhost text");
  expect(addr.addr.sin_addr.s_addr != 0, "localhost resolved");
}

static void test_null_guards(void) {
  TCPStats stats;
  char buffer[8];

  printf("Testing TCP null guards...\n");
  tcp_socket_destroy(NULL);
  tcp_socket_disconnect(NULL);
  tcp_socket_get_stats(NULL, &stats);
  tcp_socket_reset_stats(NULL);
  expect(!tcp_socket_bind(NULL, "127.0.0.1:0", 0), "bind null");
  expect(!tcp_socket_listen(NULL, 1), "listen null");
  expect(tcp_socket_accept(NULL) == NULL, "accept null");
  expect(!tcp_socket_connect(NULL, "127.0.0.1:1", 1), "connect null socket");
  expect(!tcp_socket_set_blocking(NULL, true), "blocking null");
  expect(!tcp_socket_set_timeout(NULL, 10), "timeout null");
  expect(tcp_socket_send(NULL, "x", 1) == -1, "send null socket");
  expect(tcp_socket_receive(NULL, buffer, sizeof(buffer)) == -1,
         "receive null socket");
}

static void test_loopback_roundtrip(void) {
  TCPSocket *server;
  TCPSocket *client;
  TCPSocket *accepted;
  TCPStats stats;
  const char *msg = "ping-tcp";
  char received[32];
  u16 port;
  int sent;
  int got;

  printf("Testing TCP loopback send/receive...\n");
  server = tcp_socket_create();
  expect(server != NULL, "create server");
  expect(tcp_socket_set_timeout(server, 2000), "server timeout");
  expect(!tcp_socket_listen(server, 1), "listen before bind");
  expect(tcp_socket_bind(server, "127.0.0.1:9", 0), "bind ephemeral");
  port = bound_port(server);
  expect(port != 0 && port != 9, "ephemeral port override");
  expect(tcp_socket_listen(server, 0), "listen SOMAXCONN");
  expect(server->is_listening, "listening flag");

  client = tcp_socket_create();
  expect(client != NULL, "create client");
  expect(tcp_socket_set_timeout(client, 2000), "client timeout");
  expect(!tcp_socket_connect(client, NULL, port), "connect null address");
  expect(!tcp_socket_connect(client, "127.0.0.1", port), "connect missing colon");
  /* Port in the string is ignored; the port argument is the one used. */
  expect(tcp_socket_connect(client, "127.0.0.1:1", port), "connect override");
  expect(client->is_connected, "client connected");
  expect(client->is_bound, "client local name");

  accepted = tcp_socket_accept(server);
  expect(accepted != NULL, "accept");
  expect(accepted->is_connected, "accepted connected");
  expect(server->connections_accepted == 1, "accepted count");
  expect(tcp_socket_set_timeout(accepted, 2000), "accepted timeout");

  expect(tcp_socket_send(client, NULL, 4) == -1, "send null");
  expect(tcp_socket_send(client, msg, 0) == -1, "send zero");
  expect(tcp_socket_receive(accepted, NULL, 4) == -1, "receive null");
  expect(tcp_socket_receive(accepted, received, 0) == -1, "receive zero");

  sent = tcp_socket_send(client, msg, (u32)strlen(msg));
  expect(sent == (int)strlen(msg), "send payload");
  memset(received, 0, sizeof(received));
  got = tcp_socket_receive(accepted, received, sizeof(received) - 1);
  expect(got == sent, "receive payload");
  expect(strcmp(received, msg) == 0, "payload bytes");

  tcp_socket_get_stats(client, &stats);
  expect(stats.bytes_sent == (u32)sent, "bytes sent");
  expect(stats.send_errors == 0, "no send errors");
  tcp_socket_get_stats(accepted, &stats);
  expect(stats.bytes_received == (u32)got, "bytes received");
  tcp_socket_get_stats(server, &stats);
  expect(stats.connections_accepted == 1, "stats accepted");

  tcp_socket_reset_stats(client);
  tcp_socket_reset_stats(server);
  tcp_socket_get_stats(client, &stats);
  expect(stats.bytes_sent == 0, "reset sent");
  tcp_socket_get_stats(server, &stats);
  expect(stats.connections_accepted == 0, "reset accepted");
  expect(client->is_connected, "reset keeps connection");

  tcp_socket_disconnect(client);
  expect(!client->is_connected, "disconnected");
  expect(client->socket == INVALID_SOCKET, "socket closed");
  expect(client->connections_closed == 1, "closed count");
  got = tcp_socket_receive(accepted, received, sizeof(received));
  expect(got == 0, "peer close");
  expect(!accepted->is_connected, "accepted saw close");
  expect(tcp_socket_send(client, msg, (u32)strlen(msg)) == -1, "send after close");

  tcp_socket_destroy(accepted);
  tcp_socket_destroy(client);
  tcp_socket_destroy(server);
}

static void test_bind_conflict_and_nonblocking(void) {
  TCPSocket *first;
  TCPSocket *second;
  TCPStats stats;
  char spec[64];
  u16 port;

  printf("Testing TCP bind conflict and nonblocking accept...\n");
  first = tcp_socket_create();
  second = tcp_socket_create();
  expect(first && second, "create pair");
  expect(tcp_socket_bind(first, "127.0.0.1:0", 0), "bind first");
  port = bound_port(first);
  snprintf(spec, sizeof(spec), "127.0.0.1:%u", (unsigned)port);
  expect(!tcp_socket_bind(second, spec, port), "bind in use");
  tcp_socket_get_stats(second, &stats);
  expect(stats.send_errors == 1, "bind error counted");

  expect(tcp_socket_listen(first, 1), "listen first");
  expect(tcp_socket_set_blocking(first, false), "nonblocking");
  expect(tcp_socket_accept(first) == NULL, "accept empty");
  expect(tcp_socket_would_block(), "would block");
  expect(strcmp(tcp_socket_get_last_error(), "Would block") == 0, "block text");
  expect(tcp_socket_set_blocking(first, true), "blocking again");

  tcp_socket_destroy(first);
  tcp_socket_destroy(second);
}

static void test_connection_refused(void) {
  TCPSocket *socket;

  printf("Testing TCP connection refused...\n");
  socket = tcp_socket_create();
  expect(socket != NULL, "create");
  expect(tcp_socket_set_timeout(socket, 1000), "timeout");
  expect(!tcp_socket_connect(socket, "127.0.0.1:1", 1), "refused");
  expect(strcmp(tcp_socket_get_last_error(), "Connection refused") == 0,
         "refused text");
  expect(!tcp_socket_would_block(), "not a block");
  tcp_socket_destroy(socket);
}

int main(void) {
  test_address_helpers();
  test_null_guards();
  test_loopback_roundtrip();
  test_bind_conflict_and_nonblocking();
  test_connection_refused();
  printf("tcp_socket tests passed\n");
  return 0;
}
