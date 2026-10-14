// network_manager.c - High-level network management system
// TODO: MVP PATH - Implement connection migration and failover
// TODO: MVP PATH - Add network topology awareness (client-server, P2P hybrid)
// TODO: MVP PATH - Implement adaptive tick rate based on network conditions
// TODO: MVP PATH - Add network simulation mode for testing (latency, packet
// loss)
// TODO: MVP PATH - Implement bandwidth estimation and adaptive streaming
#include "include/network/network_manager.h"
#include "include/core/logger.h"
#include "include/network/packet.h"
#include "include/network/rpc_system.h"
#include "include/network/socket.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_CLIENTS 64
#define HEARTBEAT_INTERVAL 5.0f
#define CONNECTION_TIMEOUT 10.0f

typedef struct {
  NetAddress address;
  uint32_t client_id;
  char username[MAX_USERNAME_LENGTH];
  bool connected;
  float last_heartbeat;
  uint16_t next_sequence;
  uint16_t expected_sequence;
} NetworkClient;

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

  // Common state
  float last_heartbeat_sent;
  NetworkStats stats;
} NetworkManager;

static NetworkManager g_network = {0};

// Utility functions
static uint64_t get_timestamp_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static uint32_t generate_client_id(void) {
  static uint32_t counter = 1000;
  return ++counter;
}

// Internal packet handlers forward declarations
static void network_server_handle_packet(const NetAddress *from,
                                         const Packet *packet);
static void network_client_handle_packet(const NetAddress *from,
                                         const Packet *packet);

// Server functions
int network_server_start(const char *server_name, uint16_t port,
                         uint32_t max_players, const char *password) {
  if (g_network.is_running) {
    log_error("Network server already running");
    return -1;
  }

  g_network.socket = socket_create(port);
  if (!g_network.socket) {
    log_error("Failed to create server socket");
    return -1;
  }

  g_network.is_server = true;
  g_network.is_running = true;
  g_network.client_count = 0;
  g_network.next_client_id = 1;

  // Initialize client array
  memset(g_network.clients, 0, sizeof(g_network.clients));

  // Initialize RPC system
  rpc_init();

  log_info("Network server started on port %d (max players: %u)", port,
           max_players);
  return 0;
}

int network_server_stop(void) {
  if (!g_network.is_running || !g_network.is_server) {
    return -1;
  }

  // Send disconnect to all clients
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (g_network.clients[i].connected) {
      Packet packet;
      packet_init_write(&packet, PACKET_TYPE_DISCONNECT, 0);
      packet_write_u32(&packet, g_network.clients[i].client_id);
      uint16_t size = packet_finalize(&packet);
      socket_send(g_network.socket, &g_network.clients[i].address,
                  packet.buffer, size);
    }
  }

  socket_close(g_network.socket);
  g_network.socket = NULL;
  g_network.is_running = false;
  g_network.is_server = false;

  rpc_shutdown();

  log_info("Network server stopped");
  return 0;
}

int network_server_broadcast(PacketType type, const void *data,
                             size_t data_size) {
  if (!g_network.is_running || !g_network.is_server) {
    return -1;
  }

  int sent_count = 0;
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (g_network.clients[i].connected) {
      if (network_server_send_to_client(g_network.clients[i].client_id, type,
                                        data, data_size) == 0) {
        sent_count++;
      }
    }
  }

  return sent_count > 0 ? 0 : -1;
}

int network_server_send_to_client(uint32_t client_id, PacketType type,
                                  const void *data, size_t data_size) {
  if (!g_network.is_running || !g_network.is_server) {
    return -1;
  }

  // Find client
  NetworkClient *client = NULL;
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (g_network.clients[i].connected &&
        g_network.clients[i].client_id == client_id) {
      client = &g_network.clients[i];
      break;
    }
  }

  if (!client) {
    log_error("Client %u not found", client_id);
    return -1;
  }

  // Create packet
  Packet packet;
  packet_init_write(&packet, type, 0);

  // Set sequence number
  PacketHeader *header = (PacketHeader *)packet.buffer;
  header->sequence = client->next_sequence++;
  header->timestamp = get_timestamp_ms();

  // Write data if provided
  if (data && data_size > 0) {
    memcpy(&packet.buffer[packet.write_pos], data, data_size);
    packet.write_pos += data_size;
  }

  uint16_t packet_size = packet_finalize(&packet);

  if (socket_send(g_network.socket, &client->address, packet.buffer,
                  packet_size)) {
    g_network.stats.packets_sent++;
    g_network.stats.bytes_sent += packet_size;
    return 0;
  }

  return -1;
}

uint32_t network_server_get_client_count(void) {
  return g_network.client_count;
}

// Client functions
int network_client_connect(const char *server_address, uint16_t port,
                           const char *username, const char *password) {
  if (g_network.is_running) {
    log_error("Network already running");
    return -1;
  }

  // Create client socket (bind to any available port)
  g_network.socket = socket_create(0);
  if (!g_network.socket) {
    log_error("Failed to create client socket");
    return -1;
  }

  // Parse server address
  g_network.server_address.host = inet_addr(server_address);
  g_network.server_address.port = port;

  if (g_network.server_address.host == INADDR_NONE) {
    log_error("Invalid server address: %s", server_address);
    socket_close(g_network.socket);
    return -1;
  }

  g_network.is_server = false;
  g_network.is_running = true;
  g_network.connected_to_server = false;

  strncpy(g_network.clients[0].username, username, MAX_USERNAME_LENGTH - 1);
  g_network.clients[0].username[MAX_USERNAME_LENGTH - 1] = '\0';

  // Initialize RPC system
  rpc_init();

  // Send connection request
  Packet packet;
  packet_init_write(&packet, PACKET_TYPE_CONNECT, 0);
  packet_write_string(&packet, username);
  if (password) {
    packet_write_string(&packet, password);
  }

  uint16_t size = packet_finalize(&packet);
  if (socket_send(g_network.socket, &g_network.server_address, packet.buffer,
                  size)) {
    log_info("Sent connection request to %s:%d", server_address, port);
    return 0;
  }

  socket_close(g_network.socket);
  g_network.is_running = false;
  return -1;
}

int network_client_disconnect(void) {
  if (!g_network.is_running || g_network.is_server) {
    return -1;
  }

  if (g_network.connected_to_server) {
    // Send disconnect packet
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_DISCONNECT, 0);
    packet_write_u32(&packet, g_network.local_client_id);
    uint16_t size = packet_finalize(&packet);
    socket_send(g_network.socket, &g_network.server_address, packet.buffer,
                size);
  }

  socket_close(g_network.socket);
  g_network.socket = NULL;
  g_network.is_running = false;
  g_network.connected_to_server = false;

  rpc_shutdown();

  log_info("Disconnected from server");
  return 0;
}

int network_client_send(PacketType type, const void *data, size_t data_size) {
  if (!g_network.is_running || g_network.is_server ||
      !g_network.connected_to_server) {
    return -1;
  }

  // Create packet
  Packet packet;
  packet_init_write(&packet, type, 0);

  // Set sequence number
  PacketHeader *header = (PacketHeader *)packet.buffer;
  header->sequence = g_network.clients[0].next_sequence++;
  header->timestamp = get_timestamp_ms();

  // Write data if provided
  if (data && data_size > 0) {
    memcpy(&packet.buffer[packet.write_pos], data, data_size);
    packet.write_pos += data_size;
  }

  uint16_t packet_size = packet_finalize(&packet);

  if (socket_send(g_network.socket, &g_network.server_address, packet.buffer,
                  packet_size)) {
    g_network.stats.packets_sent++;
    g_network.stats.bytes_sent += packet_size;
    return 0;
  }

  return -1;
}

bool network_client_is_connected(void) {
  return g_network.is_running && !g_network.is_server &&
         g_network.connected_to_server;
}

uint32_t network_client_get_id(void) { return g_network.local_client_id; }

// Network update
int network_update(float delta_time) {
  if (!g_network.is_running || !g_network.socket) {
    return -1;
  }

  uint8_t buffer[MAX_PACKET_SIZE];
  NetAddress from;
  int bytes_received;

  // Process incoming packets
  while ((bytes_received = socket_receive(g_network.socket, &from, buffer,
                                          sizeof(buffer))) > 0) {
    g_network.stats.packets_received++;
    g_network.stats.bytes_received += bytes_received;

    // Read packet header
    Packet packet;
    if (!packet_init_read(&packet, buffer, bytes_received)) {
      log_warn("Received invalid packet");
      continue;
    }

    // PacketHeader header = packet_get_header(&packet); // Unused variable

    if (g_network.is_server) {
      // Server packet handling
      network_server_handle_packet(&from, &packet);
    } else {
      // Client packet handling
      network_client_handle_packet(&from, &packet);
    }
  }

  // Handle RPC updates
  rpc_update(delta_time);

  // Handle heartbeats
  g_network.last_heartbeat_sent += delta_time;
  if (g_network.last_heartbeat_sent >= HEARTBEAT_INTERVAL) {
    if (g_network.is_server) {
      // Server sends heartbeat to all clients
      network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0);
    } else if (g_network.connected_to_server) {
      // Client sends heartbeat to server
      network_client_send(PACKET_TYPE_HEARTBEAT, NULL, 0);
    }
    g_network.last_heartbeat_sent = 0.0f;
  }

  return 0;
}

// Utility functions
const char *network_get_error_string(int error_code) {
  return socket_get_error();
}

bool network_is_valid_address(const char *address) {
  return inet_addr(address) != INADDR_NONE;
}

bool network_is_valid_port(uint16_t port) { return port > 0 && port <= 65535; }

uint32_t network_get_local_ip(void) {
  // This is a simplified implementation
  // In a real scenario, you'd want to enumerate network interfaces
  return inet_addr("127.0.0.1");
}

// Internal packet handlers
static void network_server_handle_packet(const NetAddress *from,
                                         const Packet *packet) {
  PacketHeader header = packet_get_header(packet);

  switch (header.type) {
  case PACKET_TYPE_CONNECT: {
    // Handle new connection
    char username[MAX_USERNAME_LENGTH];
    if (packet_read_string((Packet *)packet, username, sizeof(username))) {
      // Find empty client slot
      for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
        if (!g_network.clients[i].connected) {
          g_network.clients[i].address = *from;
          g_network.clients[i].client_id = g_network.next_client_id++;
          strncpy(g_network.clients[i].username, username,
                  MAX_USERNAME_LENGTH - 1);
          g_network.clients[i].connected = true;
          g_network.clients[i].last_heartbeat = 0.0f;
          g_network.client_count++;

          // Send acceptance
          Packet response;
          packet_init_write(&response, PACKET_TYPE_AUTH_RESPONSE, 0);
          packet_write_u32(&response, g_network.clients[i].client_id);
          packet_write_u8(&response, 1); // success
          packet_write_string(&response, "Connected");
          uint16_t size = packet_finalize(&response);
          socket_send(g_network.socket, from, response.buffer, size);

          log_info("Client connected: %s (ID: %u)", username,
                   g_network.clients[i].client_id);
          break;
        }
      }
    }
    break;
  }

  case PACKET_TYPE_RPC: {
    // Find client by address
    uint32_t sender_id = 0;
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
      if (g_network.clients[i].connected &&
          g_network.clients[i].address.host == from->host &&
          g_network.clients[i].address.port == from->port) {
        sender_id = g_network.clients[i].client_id;
        break;
      }
    }

    if (sender_id > 0) {
      rpc_process_packet(sender_id, packet->buffer, packet->length);
    }
    break;
  }

  case PACKET_TYPE_DISCONNECT: {
    // Handle client disconnect
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
      if (g_network.clients[i].connected &&
          g_network.clients[i].address.host == from->host &&
          g_network.clients[i].address.port == from->port) {
        log_info("Client disconnected: %s (ID: %u)",
                 g_network.clients[i].username, g_network.clients[i].client_id);
        g_network.clients[i].connected = false;
        g_network.client_count--;
        break;
      }
    }
    break;
  }

  case PACKET_TYPE_HEARTBEAT: {
    // Update client heartbeat
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
      if (g_network.clients[i].connected &&
          g_network.clients[i].address.host == from->host &&
          g_network.clients[i].address.port == from->port) {
        g_network.clients[i].last_heartbeat = 0.0f;
        break;
      }
    }
    break;
  }
  }
}

static void network_client_handle_packet(const NetAddress *from,
                                         const Packet *packet) {
  PacketHeader header = packet_get_header(packet);

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
        LOG_INFO("Connected to server (ID: %u): %s", client_id, message);
      } else {
        LOG_ERROR("Connection failed: %s", message);
      }
    }
    break;
  }

  case PACKET_TYPE_RPC: {
    rpc_process_packet(0, packet->buffer, packet->length);
    break;
  }
  }
}

// TODO: MVP PATH - Add client-side connection quality monitoring
// TODO: MVP PATH - Implement automatic reconnection logic
// TODO: MVP PATH - Add client-side prediction and reconciliation integration
// TODO: MVP PATH - Implement client-side entity interpolation
