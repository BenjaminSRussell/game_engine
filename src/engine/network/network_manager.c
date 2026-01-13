// network_manager.c - High-level network management system
// TODO: MVP PATH - Add network topology awareness (client-server, P2P hybrid)
// TODO: MVP PATH - Implement adaptive tick rate based on network conditions
// TODO: MVP PATH - Add network simulation mode for testing (latency, packet
// loss)
// TODO: MVP PATH - Implement bandwidth estimation and adaptive streaming
#include "include/network/network_manager.h"
#include "include/core/logger.h"
#include "include/network/network_replication.h"
#include "include/network/packet.h"
#include "include/network/rpc_system.h"
#include "include/network/socket.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_CLIENTS 64
#define HEARTBEAT_INTERVAL 5.0f
#define CONNECTION_TIMEOUT 10.0f
#define MAX_BACKUP_SERVERS 4

typedef struct {
  NetAddress address;
  uint32_t client_id;
  char username[MAX_USERNAME_LENGTH];
  bool connected;
  float last_heartbeat;
  uint16_t next_sequence;
  uint16_t expected_sequence;
  FragmentBuffer fragment_buffer;
} NetworkClient;

typedef struct {
  char address[256];
  uint16_t port;
} ServerEndpoint;

typedef struct {
  NetSocket *socket;
  bool is_server;
  bool is_running;

  // Server state
  NetworkClient clients[MAX_CLIENTS];
  uint32_t client_count;
  uint32_t next_client_id;

  // Client state
  uint32_t local_client_id;
  NetAddress server_address;
  bool connected_to_server;
  FragmentBuffer server_fragment_buffer;

  // Failover. Index -1 is the primary; 0..N-1 are backups.
  ServerEndpoint backups[MAX_BACKUP_SERVERS];
  uint32_t backup_count;
  int32_t current_server_index;
  char current_username[MAX_USERNAME_LENGTH];
  char current_password[MAX_PASSWORD_LENGTH];

  // Common state
  float last_heartbeat_sent;
  float time_since_last_packet;
  NetworkStats stats;
} NetworkManager;

static NetworkManager g_network = {.current_server_index = -1};

static uint64_t get_timestamp_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static bool address_equal(const NetAddress *a, const NetAddress *b) {
  if (!a || !b || a->port != b->port || a->type != b->type)
    return false;
  if (a->type == NET_ADDR_IPV6)
    return memcmp(a->ip6, b->ip6, 16) == 0;
  return a->host == b->host;
}

static NetworkClient *get_client_by_address(const NetAddress *from) {
  uint32_t i;
  if (!from)
    return NULL;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (g_network.clients[i].connected &&
        address_equal(&g_network.clients[i].address, from))
      return &g_network.clients[i];
  }
  return NULL;
}

static void network_server_handle_packet(const NetAddress *from,
                                         const Packet *packet);
static void network_client_handle_packet(const NetAddress *from,
                                         const Packet *packet);

// One transport header around the reassembled body. RPC bodies stay flags-first.
static void dispatch_reassembled(const NetAddress *from, uint8_t type,
                                 const uint8_t *payload, uint32_t payload_size) {
  uint32_t total = (uint32_t)sizeof(PacketHeader) + payload_size;
  uint8_t *buf = (uint8_t *)malloc(total > 0 ? total : 1);
  PacketHeader *header;

  if (!buf)
    return;
  memset(buf, 0, sizeof(PacketHeader));
  header = (PacketHeader *)buf;
  header->type = type;
  header->data_size =
      (uint16_t)(payload_size > 65535u ? 65535u : payload_size);
  header->timestamp = (uint32_t)get_timestamp_ms();
  if (payload && payload_size > 0)
    memcpy(buf + sizeof(PacketHeader), payload, payload_size);

  if (total <= MAX_PACKET_SIZE) {
    Packet packet;
    if (packet_init_read(&packet, buf, (uint16_t)total)) {
      if (g_network.is_server)
        network_server_handle_packet(from, &packet);
      else
        network_client_handle_packet(from, &packet);
    }
  } else if (!g_network.is_server && type == PACKET_TYPE_SNAPSHOT) {
    network_replication_process_snapshot(buf, total);
  } else if (!g_network.is_server && type == PACKET_TYPE_ENTITY_DESTROY) {
    network_replication_process_destroy(buf, total);
  } else {
    LOG_WARN_CAT(LOG_CAT_NETWORK,
                 "Dropping reassembled packet type %u of %u bytes", type,
                 payload_size);
  }
  free(buf);
}

static int send_payload(const NetAddress *addr, NetworkClient *seq_client,
                        PacketType type, const void *data, size_t data_size) {
  uint32_t max_payload = (uint32_t)(MAX_PACKET_SIZE - sizeof(PacketHeader));

  if (data_size > max_payload) {
    Packet *fragments = (Packet *)calloc(MAX_FRAGMENTS, sizeof(Packet));
    uint16_t count;
    uint16_t i;
    if (!fragments)
      return -1;
    count = packet_fragment_data(data, (uint32_t)data_size, (uint8_t)type, 0,
                                 fragments, MAX_FRAGMENTS);
    if (count == 0) {
      free(fragments);
      return -1;
    }
    for (i = 0; i < count; i++) {
      PacketHeader *header = (PacketHeader *)fragments[i].buffer;
      uint16_t size;
      if (seq_client)
        header->sequence = seq_client->next_sequence++;
      header->timestamp = (uint32_t)get_timestamp_ms();
      packet_compress(&fragments[i]);
      size = packet_finalize(&fragments[i]);
      if (!socket_send(g_network.socket, addr, fragments[i].buffer, size)) {
        free(fragments);
        return -1;
      }
      g_network.stats.packets_sent++;
      g_network.stats.bytes_sent += size;
    }
    free(fragments);
    return 0;
  }

  {
    Packet packet;
    PacketHeader *header;
    uint16_t packet_size;
    packet_init_write(&packet, (uint8_t)type, 0);
    header = (PacketHeader *)packet.buffer;
    if (seq_client)
      header->sequence = seq_client->next_sequence++;
    header->timestamp = (uint32_t)get_timestamp_ms();
    if (data && data_size > 0) {
      memcpy(&packet.buffer[packet.write_pos], data, data_size);
      packet.write_pos = (uint16_t)(packet.write_pos + data_size);
    }
    packet_finalize(&packet);
    packet_compress(&packet);
    packet_size = packet_finalize(&packet);
    if (socket_send(g_network.socket, addr, packet.buffer, packet_size)) {
      g_network.stats.packets_sent++;
      g_network.stats.bytes_sent += packet_size;
      return 0;
    }
  }
  return -1;
}

static void try_failover(void);

int network_server_start(const char *server_name, uint16_t port,
                         uint32_t max_players, const char *password) {
  (void)server_name;
  (void)password;
  if (g_network.is_running) {
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Network server already running");
    return -1;
  }

  g_network.socket = socket_create_typed(port, NET_ADDR_IPV6);
  if (!g_network.socket) {
    LOG_WARN_CAT(LOG_CAT_NETWORK,
                 "Failed to create IPv6 server socket, trying IPv4");
    g_network.socket = socket_create_typed(port, NET_ADDR_IPV4);
  }
  if (!g_network.socket) {
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Failed to create server socket");
    return -1;
  }

  g_network.is_server = true;
  g_network.is_running = true;
  g_network.client_count = 0;
  g_network.next_client_id = 1;
  memset(g_network.clients, 0, sizeof(g_network.clients));

  rpc_init();

  LOG_INFO_CAT(LOG_CAT_NETWORK,
               "Network server started on port %u (max players: %u)", port,
               max_players);
  return 0;
}

int network_server_stop(void) {
  uint32_t i;
  if (!g_network.is_running || !g_network.is_server)
    return -1;

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (!g_network.clients[i].connected)
      continue;
    {
      Packet packet;
      uint16_t size;
      packet_init_write(&packet, PACKET_TYPE_DISCONNECT, 0);
      packet_write_u32(&packet, g_network.clients[i].client_id);
      size = packet_finalize(&packet);
      socket_send(g_network.socket, &g_network.clients[i].address, packet.buffer,
                  size);
    }
  }

  socket_close(g_network.socket);
  g_network.socket = NULL;
  g_network.is_running = false;
  g_network.is_server = false;
  rpc_shutdown();
  LOG_INFO_CAT(LOG_CAT_NETWORK, "Network server stopped");
  return 0;
}

int network_server_send_to_client(uint32_t client_id, PacketType type,
                                  const void *data, size_t data_size) {
  NetworkClient *client = NULL;
  uint32_t i;
  if (!g_network.is_running || !g_network.is_server)
    return -1;

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (g_network.clients[i].connected &&
        g_network.clients[i].client_id == client_id) {
      client = &g_network.clients[i];
      break;
    }
  }
  if (!client) {
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Client %u not found", client_id);
    return -1;
  }
  return send_payload(&client->address, client, type, data, data_size);
}

int network_server_broadcast(PacketType type, const void *data,
                             size_t data_size) {
  int sent_count = 0;
  uint32_t i;
  if (!g_network.is_running || !g_network.is_server)
    return -1;

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (!g_network.clients[i].connected)
      continue;
    if (network_server_send_to_client(g_network.clients[i].client_id, type, data,
                                     data_size) == 0)
      sent_count++;
  }
  return sent_count > 0 ? 0 : -1;
}

uint32_t network_server_get_client_count(void) { return g_network.client_count; }

int network_server_get_clients(uint32_t *out_client_ids, size_t max_count) {
  uint32_t count = 0;
  uint32_t i;
  if (!out_client_ids || max_count == 0 || !g_network.is_running ||
      !g_network.is_server)
    return 0;

  for (i = 0; i < MAX_CLIENTS && count < max_count; i++) {
    if (g_network.clients[i].connected)
      out_client_ids[count++] = g_network.clients[i].client_id;
  }
  return (int)count;
}

int network_client_connect(const char *server_address, uint16_t port,
                           const char *username, const char *password) {
  NetAddressType addr_type = NET_ADDR_IPV4;
  struct in6_addr ipv6_addr;
  uint32_t ipv4_addr = 0;
  Packet packet;
  uint16_t size;

  if (g_network.is_running && g_network.connected_to_server) {
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Network already running/connected");
    return -1;
  }
  if (!server_address || !username)
    return -1;

  if (g_network.socket) {
    socket_close(g_network.socket);
    g_network.socket = NULL;
  }

  if (inet_pton(AF_INET6, server_address, &ipv6_addr) == 1) {
    addr_type = NET_ADDR_IPV6;
  } else {
    ipv4_addr = inet_addr(server_address);
    if (ipv4_addr == INADDR_NONE) {
      LOG_ERROR_CAT(LOG_CAT_NETWORK, "Invalid server address: %s",
                    server_address);
      return -1;
    }
  }

  g_network.socket = socket_create_typed(0, addr_type);
  if (!g_network.socket) {
    LOG_ERROR_CAT(LOG_CAT_NETWORK, "Failed to create client socket");
    return -1;
  }

  memset(&g_network.server_address, 0, sizeof(g_network.server_address));
  g_network.server_address.type = (uint8_t)addr_type;
  g_network.server_address.port = port;
  if (addr_type == NET_ADDR_IPV6)
    memcpy(g_network.server_address.ip6, &ipv6_addr, 16);
  else
    g_network.server_address.ip4 = ipv4_addr;

  g_network.is_server = false;
  g_network.is_running = true;
  g_network.connected_to_server = false;
  g_network.time_since_last_packet = 0;
  memset(&g_network.server_fragment_buffer, 0,
         sizeof(g_network.server_fragment_buffer));

  strncpy(g_network.current_username, username, MAX_USERNAME_LENGTH - 1);
  g_network.current_username[MAX_USERNAME_LENGTH - 1] = '\0';
  g_network.current_password[0] = '\0';
  if (password) {
    strncpy(g_network.current_password, password, MAX_PASSWORD_LENGTH - 1);
    g_network.current_password[MAX_PASSWORD_LENGTH - 1] = '\0';
  }

  strncpy(g_network.clients[0].username, username, MAX_USERNAME_LENGTH - 1);
  g_network.clients[0].username[MAX_USERNAME_LENGTH - 1] = '\0';

  rpc_init();

  packet_init_write(&packet, PACKET_TYPE_CONNECT, 0);
  packet_write_string(&packet, username);
  if (password && password[0] != '\0')
    packet_write_string(&packet, password);
  size = packet_finalize(&packet);
  if (socket_send(g_network.socket, &g_network.server_address, packet.buffer,
                  size)) {
    LOG_INFO_CAT(LOG_CAT_NETWORK, "Sent connection request to %s:%u",
                 server_address, port);
    return 0;
  }

  socket_close(g_network.socket);
  g_network.socket = NULL;
  g_network.is_running = false;
  return -1;
}

void network_client_add_backup(const char *address, uint16_t port) {
  ServerEndpoint *endpoint;
  if (!address || g_network.backup_count >= MAX_BACKUP_SERVERS)
    return;
  endpoint = &g_network.backups[g_network.backup_count++];
  memset(endpoint, 0, sizeof(*endpoint));
  strncpy(endpoint->address, address, sizeof(endpoint->address) - 1);
  endpoint->port = port;
  LOG_INFO_CAT(LOG_CAT_NETWORK, "Added backup server: %s:%u", endpoint->address,
               (unsigned)port);
}

static void try_failover(void) {
  const char *address;
  uint16_t port;
  if (g_network.is_server)
    return;

  g_network.current_server_index++;
  if (g_network.current_server_index < (int32_t)g_network.backup_count) {
    LOG_WARN_CAT(LOG_CAT_NETWORK,
                 "Connection lost. Attempting failover to backup server %d",
                 g_network.current_server_index + 1);
    address = g_network.backups[g_network.current_server_index].address;
    port = g_network.backups[g_network.current_server_index].port;
    network_client_connect(address, port, g_network.current_username,
                           g_network.current_password);
    return;
  }

  LOG_ERROR_CAT(LOG_CAT_NETWORK,
                "Connection lost. No more backup servers available.");
  g_network.is_running = false;
}

int network_client_disconnect(void) {
  if (!g_network.is_running || g_network.is_server)
    return -1;

  if (g_network.connected_to_server) {
    Packet packet;
    uint16_t size;
    packet_init_write(&packet, PACKET_TYPE_DISCONNECT, 0);
    packet_write_u32(&packet, g_network.local_client_id);
    size = packet_finalize(&packet);
    socket_send(g_network.socket, &g_network.server_address, packet.buffer,
                size);
  }

  socket_close(g_network.socket);
  g_network.socket = NULL;
  g_network.is_running = false;
  g_network.connected_to_server = false;
  rpc_shutdown();
  LOG_INFO_CAT(LOG_CAT_NETWORK, "Disconnected from server");
  return 0;
}

int network_client_send(PacketType type, const void *data, size_t data_size) {
  if (!g_network.is_running || g_network.is_server ||
      !g_network.connected_to_server)
    return -1;
  return send_payload(&g_network.server_address, &g_network.clients[0], type,
                      data, data_size);
}

bool network_client_is_connected(void) {
  return g_network.is_running && !g_network.is_server &&
         g_network.connected_to_server;
}

uint32_t network_client_get_id(void) { return g_network.local_client_id; }

int network_update(float delta_time) {
  uint8_t buffer[MAX_PACKET_SIZE];
  NetAddress from;
  int bytes_received;

  if (!g_network.is_running || !g_network.socket)
    return -1;

  while (1) {
    Packet packet;
    PacketHeader header;
    memset(&from, 0, sizeof(from));
    bytes_received =
        socket_receive(g_network.socket, &from, buffer, sizeof(buffer));
    if (bytes_received <= 0)
      break;

    g_network.stats.packets_received++;
    g_network.stats.bytes_received += (uint64_t)bytes_received;
    if (!g_network.is_server)
      g_network.time_since_last_packet = 0;

    if (!packet_init_read(&packet, buffer, (uint16_t)bytes_received)) {
      LOG_WARN_CAT(LOG_CAT_NETWORK, "Received invalid packet");
      continue;
    }

    if (packet_get_header(&packet).flags & NET_PACKET_FLAG_COMPRESSED) {
      if (!packet_decompress(&packet)) {
        LOG_ERROR_CAT(LOG_CAT_NETWORK, "Failed to decompress packet");
        continue;
      }
    }

    header = packet_get_header(&packet);
    if (header.flags & NET_PACKET_FLAG_FRAGMENT) {
      FragmentBuffer *frag_buf = NULL;
      if (g_network.is_server) {
        NetworkClient *client = get_client_by_address(&from);
        if (client)
          frag_buf = &client->fragment_buffer;
      } else {
        frag_buf = &g_network.server_fragment_buffer;
      }
      if (frag_buf && packet_reassemble_fragment(frag_buf, &packet)) {
        dispatch_reassembled(&from, header.type, frag_buf->buffer,
                             frag_buf->total_size);
        frag_buf->active = false;
      }
      continue;
    }

    if (g_network.is_server)
      network_server_handle_packet(&from, &packet);
    else
      network_client_handle_packet(&from, &packet);
  }

  rpc_update(delta_time);

  g_network.last_heartbeat_sent += delta_time;
  if (g_network.last_heartbeat_sent >= HEARTBEAT_INTERVAL) {
    if (g_network.is_server)
      network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0);
    else if (g_network.connected_to_server)
      network_client_send(PACKET_TYPE_HEARTBEAT, NULL, 0);
    g_network.last_heartbeat_sent = 0.0f;
  }

  if (!g_network.is_server && g_network.is_running &&
      g_network.time_since_last_packet > CONNECTION_TIMEOUT) {
    LOG_WARN_CAT(LOG_CAT_NETWORK, "Connection timed out");
    g_network.connected_to_server = false;
    g_network.time_since_last_packet = 0;
    try_failover();
  }

  return 0;
}

const char *network_get_error_string(int error_code) {
  (void)error_code;
  return socket_get_error();
}

bool network_is_valid_address(const char *address) {
  struct in6_addr ipv6;
  if (!address)
    return false;
  if (inet_pton(AF_INET6, address, &ipv6) == 1)
    return true;
  return inet_addr(address) != INADDR_NONE;
}

bool network_is_valid_port(uint16_t port) { return port > 0 && port <= 65535; }

uint32_t network_get_local_ip(void) { return inet_addr("127.0.0.1"); }

static void network_server_handle_packet(const NetAddress *from,
                                         const Packet *packet) {
  PacketHeader header = packet_get_header(packet);
  NetworkClient *client;

  switch (header.type) {
  case PACKET_TYPE_CONNECT: {
    char username[MAX_USERNAME_LENGTH];
    uint32_t i;
    if (!packet_read_string((Packet *)packet, username, sizeof(username)))
      break;
    for (i = 0; i < MAX_CLIENTS; i++) {
      Packet response;
      uint16_t size;
      if (g_network.clients[i].connected)
        continue;
      g_network.clients[i].address = *from;
      g_network.clients[i].client_id = g_network.next_client_id++;
      strncpy(g_network.clients[i].username, username, MAX_USERNAME_LENGTH - 1);
      g_network.clients[i].username[MAX_USERNAME_LENGTH - 1] = '\0';
      g_network.clients[i].connected = true;
      g_network.clients[i].last_heartbeat = 0.0f;
      memset(&g_network.clients[i].fragment_buffer, 0,
             sizeof(g_network.clients[i].fragment_buffer));
      g_network.client_count++;

      packet_init_write(&response, PACKET_TYPE_AUTH_RESPONSE, 0);
      packet_write_u32(&response, g_network.clients[i].client_id);
      packet_write_u8(&response, 1);
      packet_write_string(&response, "Connected");
      size = packet_finalize(&response);
      socket_send(g_network.socket, from, response.buffer, size);
      LOG_INFO_CAT(LOG_CAT_NETWORK, "Client connected: %s (ID: %u)", username,
                   g_network.clients[i].client_id);
      break;
    }
    break;
  }

  case PACKET_TYPE_RPC:
  case PACKET_TYPE_RPC_ACK: {
    client = get_client_by_address(from);
    if (client)
      rpc_process_packet(client->client_id, packet->buffer, packet->length);
    break;
  }

  case PACKET_TYPE_DISCONNECT: {
    client = get_client_by_address(from);
    if (client) {
      LOG_INFO_CAT(LOG_CAT_NETWORK, "Client disconnected: %s (ID: %u)",
                   client->username, client->client_id);
      client->connected = false;
      if (g_network.client_count > 0)
        g_network.client_count--;
    }
    break;
  }

  case PACKET_TYPE_SNAPSHOT_ACK: {
    uint16_t acked_sequence;
    client = get_client_by_address(from);
    if (client && packet_read_u16((Packet *)packet, &acked_sequence))
      network_replication_process_ack(client->client_id, acked_sequence);
    break;
  }

  case PACKET_TYPE_HEARTBEAT: {
    client = get_client_by_address(from);
    if (client)
      client->last_heartbeat = 0.0f;
    break;
  }

  default:
    break;
  }
}

static void network_client_handle_packet(const NetAddress *from,
                                         const Packet *packet) {
  PacketHeader header = packet_get_header(packet);
  (void)from;

  switch (header.type) {
  case PACKET_TYPE_AUTH_RESPONSE: {
    uint32_t client_id;
    uint8_t success;
    char message[256];
    if (packet_read_u32((Packet *)packet, &client_id) &&
        packet_read_u8((Packet *)packet, &success) &&
        packet_read_string((Packet *)packet, message, sizeof(message))) {
      if (success) {
        g_network.local_client_id = client_id;
        g_network.connected_to_server = true;
        g_network.current_server_index = -1;
        memset(&g_network.server_fragment_buffer, 0,
               sizeof(g_network.server_fragment_buffer));
        LOG_INFO_CAT(LOG_CAT_NETWORK, "Connected to server (ID: %u): %s",
                     client_id, message);
      } else {
        LOG_ERROR_CAT(LOG_CAT_NETWORK, "Connection failed: %s", message);
      }
    }
    break;
  }

  case PACKET_TYPE_RPC:
  case PACKET_TYPE_RPC_ACK:
    rpc_process_packet(0, packet->buffer, packet->length);
    break;

  case PACKET_TYPE_ENTITY_DESTROY:
    network_replication_process_destroy(packet->buffer, packet->length);
    break;

  case PACKET_TYPE_SNAPSHOT:
    network_replication_process_snapshot(packet->buffer, packet->length);
    break;

  default:
    break;
  }
}

// TODO: MVP PATH - Add client-side connection quality monitoring
// TODO: MVP PATH - Add client-side prediction and reconciliation integration
// TODO: MVP PATH - Implement client-side entity interpolation
