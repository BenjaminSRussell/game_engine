/*
 * network_impl_test.c
 * UDP connect, auth, broadcast, and disconnect for network_impl.c.
 * Compiles that file directly and does not link Engine.
 */

#include "core/logger.h"
#include "network/network_manager.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

static void pump(void) {
  if (network_update(0.016f) != 0) {
    fail("network update");
  }
}

/* A single nonblocking read can run before the localhost datagram is queued. */
static void pump_until(int (*done)(void), const char *msg) {
  int i;
  for (i = 0; i < 50; i++) {
    if (done()) {
      return;
    }
    pump();
    if (done()) {
      return;
    }
    usleep(2000);
  }
  fail(msg);
}

static int client_authed(void) {
  return network_client_is_connected() && network_client_get_id() == 42;
}

static int client_idle(void) {
  return !network_client_is_connected();
}

static uint32_t g_want_count;

static int server_count_is(void) {
  return network_server_get_client_count() == g_want_count;
}

static void pump_count(uint32_t count, const char *msg) {
  g_want_count = count;
  pump_until(server_count_is, msg);
}

static int bind_udp(uint16_t *port) {
  int fd;
  int reuse;
  struct sockaddr_in addr;
  socklen_t len;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    return -1;
  }
  reuse = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(*port);
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }
  len = sizeof(addr);
  if (getsockname(fd, (struct sockaddr *)&addr, &len) == 0) {
    *port = ntohs(addr.sin_port);
  }
  return fd;
}

static struct sockaddr_in loopback(uint16_t port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(port);
  return addr;
}

static int send_raw(int fd, const struct sockaddr_in *to, PacketType type,
                    const void *data, size_t data_size) {
  uint8_t buffer[2048];
  PacketHeader header;

  if (sizeof(header) + data_size > sizeof(buffer)) {
    return -1;
  }
  memset(&header, 0, sizeof(header));
  memset(buffer, 0, sizeof(header));
  header.packet_id = 7;
  header.type = (uint8_t)type;
  header.timestamp = 1;
  header.data_size = (uint16_t)data_size;
  memcpy(buffer, &header, sizeof(header));
  if (data && data_size > 0) {
    memcpy(buffer + sizeof(header), data, data_size);
  }
  if (sendto(fd, buffer, sizeof(header) + data_size, 0, (struct sockaddr *)to,
             sizeof(*to)) < 0) {
    return -1;
  }
  return 0;
}

static int recv_raw(int fd, struct sockaddr_in *from, PacketHeader *header,
                    void *data, size_t max_size, int timeout_ms) {
  uint8_t buffer[2048];
  struct pollfd pfd;
  struct sockaddr_in storage;
  socklen_t from_len;
  ssize_t n;
  size_t payload;

  pfd.fd = fd;
  pfd.events = POLLIN;
  if (poll(&pfd, 1, timeout_ms) <= 0) {
    return -1;
  }
  from_len = sizeof(storage);
  n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&storage,
               &from_len);
  if (n < (ssize_t)sizeof(PacketHeader)) {
    return -1;
  }
  if (from) {
    *from = storage;
  }
  memcpy(header, buffer, sizeof(*header));
  payload = (size_t)n - sizeof(PacketHeader);
  if (payload > max_size) {
    payload = max_size;
  }
  if (data && payload > 0) {
    memcpy(data, buffer + sizeof(PacketHeader), payload);
  }
  return (int)payload;
}

static void test_idle_and_invalid_address(void) {
  printf("Testing idle guards and invalid address...\n");
  if (network_server_stop() != 0) {
    fail("stop while idle");
  }
  if (network_server_get_client_count() != 0) {
    fail("count while idle");
  }
  if (network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0) != -1) {
    fail("broadcast while idle");
  }
  if (network_server_send_to_client(1, PACKET_TYPE_HEARTBEAT, NULL, 0) != -1) {
    fail("send while idle");
  }
  if (network_client_is_connected()) {
    fail("client connected while idle");
  }
  if (network_client_get_id() != 0) {
    fail("client id while idle");
  }
  if (network_client_send(PACKET_TYPE_HEARTBEAT, NULL, 0) != -1) {
    fail("client send while idle");
  }
  if (network_client_disconnect() != 0) {
    fail("disconnect while idle");
  }
  if (network_update(0.016f) != 0) {
    fail("update while idle");
  }
  if (network_client_connect("not-an-ip", 1, "alice", NULL) != -1) {
    fail("invalid address was accepted");
  }
  if (network_client_is_connected() || network_server_get_client_count() != 0) {
    fail("invalid address changed state");
  }
  printf("Idle guards passed.\n");
}

static void test_client_handshake(void) {
  uint16_t port = 0;
  int peer;
  struct sockaddr_in from;
  struct sockaddr_in server;
  PacketHeader header;
  ClientInfo info;
  AuthResponse auth;
  ChatMessage chat;
  int payload;
  char big[8];

  printf("Testing client handshake...\n");
  peer = bind_udp(&port);
  if (peer < 0) {
    fail("bind client peer");
  }
  if (network_client_connect("127.0.0.1", port, "alice", NULL) != 0) {
    fail("client connect");
  }
  if (network_client_is_connected()) {
    fail("connected before auth");
  }
  if (network_client_send(PACKET_TYPE_CHAT, "x", 1) != -1) {
    fail("send before auth");
  }

  payload = recv_raw(peer, &from, &header, &info, sizeof(info), 1000);
  if (payload < (int)sizeof(ClientInfo) || header.type != PACKET_TYPE_CONNECT) {
    fail("missing client connect packet");
  }
  if (strcmp(info.username, "alice") != 0) {
    fail("connect username");
  }

  server = from;
  if (send_raw(peer, &from, PACKET_TYPE_CONNECT, NULL, 0) != 0) {
    fail("peer connect accept");
  }
  for (int i = 0; i < 20; i++) {
    pump();
    usleep(1000);
  }
  if (network_client_is_connected()) {
    fail("connected before auth response");
  }

  memset(&auth, 0, sizeof(auth));
  auth.success = true;
  auth.client_id = 42;
  strncpy(auth.message, "ok", sizeof(auth.message) - 1);
  if (send_raw(peer, &server, PACKET_TYPE_AUTH_RESPONSE, &auth, sizeof(auth)) !=
      0) {
    fail("peer auth response");
  }
  pump_until(client_authed, "client auth state");

  memset(big, 0, sizeof(big));
  if (network_client_send(PACKET_TYPE_CHAT, big, 100000) != -1) {
    fail("oversized client send");
  }

  memset(&chat, 0, sizeof(chat));
  strncpy(chat.username, "alice", sizeof(chat.username) - 1);
  strncpy(chat.message, "hi", sizeof(chat.message) - 1);
  if (network_client_send(PACKET_TYPE_CHAT, &chat, sizeof(chat)) != 0) {
    fail("client chat send");
  }
  payload = recv_raw(peer, NULL, &header, &chat, sizeof(chat), 1000);
  if (payload < (int)sizeof(ChatMessage) || header.type != PACKET_TYPE_CHAT ||
      strcmp(chat.message, "hi") != 0) {
    fail("peer did not receive chat");
  }

  if (send_raw(peer, &server, PACKET_TYPE_DISCONNECT, NULL, 0) != 0) {
    fail("peer disconnect");
  }
  pump_until(client_idle, "client stayed connected after disconnect packet");
  if (network_client_disconnect() != 0 || network_client_is_connected()) {
    fail("client disconnect cleanup");
  }
  close(peer);
  printf("Client handshake passed.\n");
}

static int start_on_free_port(const char *name, uint32_t max_players,
                              const char *password, uint16_t *port) {
  uint16_t base = (uint16_t)(25000 + (getpid() % 8000));
  int i;

  for (i = 0; i < 40; i++) {
    *port = (uint16_t)(base + (uint16_t)i);
    if (network_server_start(name, *port, max_players, password) == 0) {
      return 0;
    }
  }
  return -1;
}

static void expect_auth(int peer, bool ok, uint32_t *id_out) {
  PacketHeader header;
  AuthResponse auth;
  int payload;

  payload = recv_raw(peer, NULL, &header, &auth, sizeof(auth), 1000);
  if (payload < (int)sizeof(AuthResponse) ||
      header.type != PACKET_TYPE_AUTH_RESPONSE) {
    fail("missing auth response");
  }
  if (auth.success != ok) {
    fail(ok ? "auth was rejected" : "auth was accepted");
  }
  if (id_out) {
    *id_out = auth.client_id;
  }
}

static void test_server_session(void) {
  uint16_t port = 0;
  int peer;
  int second;
  struct sockaddr_in server;
  ClientInfo info;
  PacketHeader header;
  ChatMessage chat;
  uint32_t client_id = 0;
  uint16_t peer_port;

  printf("Testing server session...\n");
  if (start_on_free_port("TestServer", 8, NULL, &port) != 0) {
    fail("server start");
  }
  if (network_server_start("Other", port, 8, NULL) != -1) {
    fail("second start");
  }
  if (network_server_get_client_count() != 0) {
    fail("count after start");
  }
  if (network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0) != 0) {
    fail("broadcast with no clients");
  }
  if (network_server_send_to_client(1, PACKET_TYPE_HEARTBEAT, NULL, 0) != -1) {
    fail("send to missing client");
  }
  if (network_update(0.016f) != 0) {
    fail("server update with no packets");
  }

  peer_port = 0;
  peer = bind_udp(&peer_port);
  if (peer < 0) {
    fail("bind server peer");
  }
  server = loopback(port);
  memset(&info, 0, sizeof(info));
  strncpy(info.username, "bob", sizeof(info.username) - 1);
  if (send_raw(peer, &server, PACKET_TYPE_CONNECT, &info, sizeof(info)) != 0) {
    fail("peer connect");
  }
  pump_count(1, "count after connect");
  expect_auth(peer, true, &client_id);
  if (client_id == 0) {
    fail("assigned client id");
  }
  if (network_server_send_to_client(client_id + 50, PACKET_TYPE_HEARTBEAT, NULL,
                                    0) != -1) {
    fail("send to unknown id");
  }

  memset(&chat, 0, sizeof(chat));
  strncpy(chat.username, "server", sizeof(chat.username) - 1);
  strncpy(chat.message, "welcome", sizeof(chat.message) - 1);
  if (network_server_broadcast(PACKET_TYPE_CHAT, &chat, sizeof(chat)) != 1) {
    fail("broadcast count");
  }
  if (recv_raw(peer, NULL, &header, &chat, sizeof(chat), 1000) <
          (int)sizeof(ChatMessage) ||
      header.type != PACKET_TYPE_CHAT || strcmp(chat.message, "welcome") != 0) {
    fail("broadcast payload");
  }
  if (network_server_send_to_client(client_id, PACKET_TYPE_HEARTBEAT, NULL, 0) !=
      0) {
    fail("direct send");
  }
  if (recv_raw(peer, NULL, &header, NULL, 0, 1000) < 0 ||
      header.type != PACKET_TYPE_HEARTBEAT) {
    fail("direct heartbeat");
  }

  if (send_raw(peer, &server, PACKET_TYPE_DISCONNECT, NULL, 0) != 0) {
    fail("client disconnect packet");
  }
  pump_count(0, "count after disconnect");

  /* Same username is rejected for the second connected client. */
  memset(&info, 0, sizeof(info));
  strncpy(info.username, "sam", sizeof(info.username) - 1);
  if (send_raw(peer, &server, PACKET_TYPE_CONNECT, &info, sizeof(info)) != 0) {
    fail("reconnect sam");
  }
  pump_count(1, "count after sam");
  expect_auth(peer, true, NULL);

  peer_port = 0;
  second = bind_udp(&peer_port);
  if (second < 0) {
    fail("bind second peer");
  }
  if (send_raw(second, &server, PACKET_TYPE_CONNECT, &info, sizeof(info)) != 0) {
    fail("duplicate username connect");
  }
  pump_count(2, "duplicate still occupies a slot");
  expect_auth(second, false, NULL);
  if (network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0) != 1) {
    fail("broadcast skipped unauthenticated client");
  }
  if (recv_raw(peer, NULL, &header, NULL, 0, 1000) < 0 ||
      header.type != PACKET_TYPE_HEARTBEAT) {
    fail("authenticated client missed heartbeat");
  }
  if (recv_raw(second, NULL, &header, NULL, 0, 200) >= 0) {
    fail("unauthenticated client received broadcast");
  }

  if (network_server_stop() != 0) {
    fail("server stop");
  }
  if (network_server_get_client_count() != 0 ||
      network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0) != -1) {
    fail("state after stop");
  }
  if (network_server_stop() != 0) {
    fail("second stop");
  }
  close(peer);
  close(second);

  if (start_on_free_port("Locked", 4, "secret", &port) != 0) {
    fail("password server start");
  }
  peer_port = 0;
  peer = bind_udp(&peer_port);
  if (peer < 0) {
    fail("bind password peer");
  }
  server = loopback(port);
  memset(&info, 0, sizeof(info));
  strncpy(info.username, "eve", sizeof(info.username) - 1);
  if (send_raw(peer, &server, PACKET_TYPE_CONNECT, &info, sizeof(info)) != 0) {
    fail("password connect");
  }
  pump_count(1, "password client count");
  expect_auth(peer, false, NULL);
  if (network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0) != 0) {
    fail("password broadcast");
  }
  if (network_server_stop() != 0) {
    fail("password stop");
  }
  close(peer);

  if (start_on_free_port("Tiny", 1, NULL, &port) != 0) {
    fail("max player server start");
  }
  peer_port = 0;
  peer = bind_udp(&peer_port);
  second = -1;
  if (peer < 0) {
    fail("bind max peer");
  }
  server = loopback(port);
  memset(&info, 0, sizeof(info));
  strncpy(info.username, "one", sizeof(info.username) - 1);
  if (send_raw(peer, &server, PACKET_TYPE_CONNECT, &info, sizeof(info)) != 0) {
    fail("max first connect");
  }
  pump_count(1, "max first count");
  expect_auth(peer, true, NULL);
  peer_port = 0;
  second = bind_udp(&peer_port);
  if (second < 0) {
    fail("bind max second");
  }
  strncpy(info.username, "two", sizeof(info.username) - 1);
  if (send_raw(second, &server, PACKET_TYPE_CONNECT, &info, sizeof(info)) != 0) {
    fail("max second connect");
  }
  for (int i = 0; i < 20; i++) {
    pump();
    usleep(1000);
  }
  if (network_server_get_client_count() != 1) {
    fail("max players was not enforced");
  }
  if (recv_raw(second, NULL, &header, NULL, 0, 200) >= 0) {
    fail("rejected client got a packet");
  }
  if (network_server_stop() != 0) {
    fail("max stop");
  }
  close(peer);
  close(second);
  printf("Server session passed.\n");
}

int main(void) {
  test_idle_and_invalid_address();
  test_client_handshake();
  test_server_session();
  printf("All network_impl tests passed.\n");
  return 0;
}
