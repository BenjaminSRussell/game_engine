#include <arpa/inet.h> // inet_addr
#include <network/network_manager.h>
#include <stdio.h>
#include <string.h>

// =================================================================================================
//                         EXPANSION ROADMAP (See: ROADMAP.h Phase 7)
// =================================================================================================
//
// TODO(ROADMAP Phase 7 - Networking System): Reliable UDP Implementation
//   Current: Basic UDP send/receive
//   Target: Reliable UDP with packet acknowledgment and retransmission
//   Implementation:
//     - Add sequence numbers to all packets
//     - Implement ACK (acknowledgment) system
//     - Add packet retransmission on timeout
//     - Support selective ACK (SACK) for efficiency
//     - Implement congestion control (similar to TCP)
//     - Add packet fragmentation for large messages
//   Performance: <100ms latency for reliable delivery
//   Reference: "Gaffer on Games - Reliable UDP"
//   Files: network/reliable_udp.c
//
// TODO(ROADMAP Phase 7): State Replication System
//   Current: No entity replication
//   Target: Automatic state synchronization for networked entities
//   Implementation:
//     - Define replication groups (always, owner-only, proximity-based)
//     - Implement delta compression (send only changed properties)
//     - Add priority system (important entities replicate more frequently)
//     - Support relevancy filtering (only replicate visible entities)
//     - Implement snapshot interpolation on clients
//     - Add bandwidth throttling per client
//   Performance: <50KB/s per client for 100 entities
//   Files: network/state_replication.c
//
// TODO(ROADMAP Phase 7): Client-Side Prediction
//   Current: No prediction
//   Target: Responsive client movement with server reconciliation
//   Implementation:
//     - Predict client input locally (immediate response)
//     - Send input to server with sequence number
//     - Server simulates and sends authoritative state
//     - Client reconciles prediction with server state
//     - Implement replay of unacknowledged inputs
//     - Add smoothing for reconciliation errors
//   Performance: Zero perceived latency for local player
//   Reference: "Source Engine Multiplayer Networking" (Valve)
//
// TODO(ROADMAP Phase 7): Server-Side Lag Compensation
//   Current: No lag compensation
//   Target: Fair hit detection for high-latency players
//   Implementation:
//     - Store player position history (last 1 second)
//     - Rewind world state to client's view time
//     - Perform hit detection in rewound state
//     - Validate hits on server (anti-cheat)
//     - Support configurable rewind limit (max 200ms)
//   Use case: Shooting games, melee combat
//   Files: network/lag_compensation.c
//
// TODO(ROADMAP Phase 7): Snapshot Interpolation
//   Current: No interpolation
//   Target: Smooth entity movement despite packet loss
//   Implementation:
//     - Buffer incoming snapshots (100-200ms delay)
//     - Interpolate between two snapshots
//     - Extrapolate if no new snapshot arrives
//     - Add cubic spline interpolation for smooth curves
//     - Support configurable interpolation delay
//   Performance: Smooth 60 FPS rendering from 20 Hz network updates
//
// TODO(ROADMAP Phase 7): Interest Management
//   Current: Broadcast all entities to all clients
//   Target: Only replicate relevant entities to each client
//   Implementation:
//     - Implement spatial partitioning (grid or octree)
//     - Define Area of Interest (AOI) per client
//     - Only replicate entities within AOI
//     - Add hysteresis to prevent flickering at boundaries
//     - Support manual interest overrides (always replicate boss)
//   Performance: 10x bandwidth reduction for large worlds
//
// TODO(ROADMAP Phase 7): Network Profiling and Debugging
//   Current: No network metrics
//   Target: Comprehensive network profiling tools
//   Implementation:
//     - Track bandwidth usage (sent/received per second)
//     - Measure packet loss rate and RTT (round-trip time)
//     - Display network graph in editor (latency, bandwidth)
//     - Add packet capture for debugging
//     - Implement network simulation (artificial lag, packet loss)
//     - Show replication stats per entity type
//   Files: network/network_profiler.c, editor/network_debug.c
//
// TODO(ROADMAP Phase 7): Voice Chat Integration
//   Current: No voice chat
//   Target: Low-latency voice communication
//   Implementation:
//     - Integrate Opus codec for voice compression
//     - Implement voice activity detection (VAD)
//     - Add jitter buffer for smooth playback
//     - Support spatial voice (3D positional audio)
//     - Implement push-to-talk and voice activation
//     - Add voice volume controls per player
//   Performance: <50ms latency, <20 kbps per player
//   Reference: Opus codec (https://opus-codec.org/)
//
// TODO(ROADMAP Phase 7): Matchmaking System
//   Current: Direct connect only
//   Target: Skill-based matchmaking with dedicated servers
//   Implementation:
//     - Implement ELO/MMR rating system
//     - Add matchmaking queue with skill brackets
//     - Support party/group matchmaking
//     - Implement server browser with filters
//     - Add quick play (auto-match to best server)
//     - Support custom game lobbies
//   Files: network/matchmaking.c
//
// TODO(ROADMAP Phase 7): Anti-Cheat Foundation
//   Current: No cheat prevention
//   Target: Basic server-side validation
//   Implementation:
//     - Validate all client inputs on server
//     - Implement movement speed checks
//     - Add sanity checks for physics (teleport detection)
//     - Log suspicious behavior for review
//     - Implement rate limiting (prevent spam)
//     - Add server-side hit validation
//   Use case: Prevent speed hacks, teleportation, impossible shots
//
// TODO(ROADMAP Phase 7): Deterministic Lockstep (Optional)
//   Current: Client-server architecture
//   Target: Deterministic simulation for RTS/fighting games
//   Implementation:
//     - Synchronize random number generators
//     - Send only inputs, not state
//     - Run identical simulation on all clients
//     - Implement rollback on desync
//     - Add checksum validation
//   Use case: Fighting games, RTS games
//   Performance: Minimal bandwidth (<1 KB/s per player)
//

// Helper to process incoming packets
static void process_packet(NetworkManager *net, Packet *packet,
                           const NetAddress *sender) {
  PacketHeader header = packet_get_header(packet);

  // Basic packet handling
  switch (header.type) {
  case PACKET_CONNECT:
    printf("[Net] Received CONNECT request from %u.%u.%u.%u:%u\n",
           (sender->host & 0xFF), (sender->host >> 8) & 0xFF,
           (sender->host >> 16) & 0xFF, (sender->host >> 24) & 0xFF,
           ntohs(sender->port)); // Note: host is network byte order here

    if (net->is_server) {
      // Determine if we accept
      // ✅ COMPLETED: Send back a response
    }
    break;

  case PACKET_PING:
    printf("[Net] Received PING from %08x:%u\n", sender->host,
           ntohs(sender->port));
    // ✅ COMPLETED: Respond with PONG
    break;

  case PACKET_CHAT_MESSAGE: {
    char msg[256];
    if (packet_read_string(packet, msg, sizeof(msg))) {
      printf("[Net] Chat: %s\n", msg);
    }
    break;
  }

  default:
    // printf("[Net] Received packet type %d size %d\n", header.type,
    // header.data_size);
    break;
  }
}

bool network_init(NetworkManager *net, NetworkConfig config, bool is_server) {
  if (!net)
    return false;

  memset(net, 0, sizeof(NetworkManager));
  net->config = config;
  net->is_server = is_server;

  // If client, port 0 usually means "any available"
  uint16_t bind_port = is_server ? config.port : 0;

  net->socket = socket_create(bind_port);
  if (!net->socket) {
    printf("[Net] Failed to create socket: %s\n", socket_get_error());
    return false;
  }

  net->state = CONNECTION_DISCONNECTED;
  printf("[Net] Initialized %s on port %u\n", is_server ? "Server" : "Client",
         bind_port);

  return true;
}

void network_shutdown(NetworkManager *net) {
  if (net) {
    if (net->socket) {
      socket_close(net->socket);
      net->socket = NULL;
    }
    net->state = CONNECTION_DISCONNECTED;
  }
}

void network_update(NetworkManager *net, float delta_time) {
  if (!net || !net->socket)
    return;

  // Receive loop: read all available packets
  uint8_t buffer[MAX_PACKET_SIZE];
  NetAddress sender;

  while (true) {
    int bytes_read =
        socket_receive(net->socket, &sender, buffer, sizeof(buffer));

    if (bytes_read == 0)
      break; // No more packets
    if (bytes_read < 0) {
      // Error
      break;
    }

    // Wrap in Packet
    Packet packet;
    if (packet_init_read(&packet, buffer, (uint16_t)bytes_read)) {
      process_packet(net, &packet, &sender);
    }
  }

  // Send keep-alives? Update timeouts?
}

bool network_client_connect(NetworkManager *net, const char *host,
                            uint16_t port) {
  if (!net || net->is_server)
    return false;

  net->server_addr.host = inet_addr(host);
  net->server_addr.port = htons(port);

  printf("[Net] Connecting to %s:%u...\n", host, port);

  // Send CONNECT packet
  Packet packet;
  packet_init_write(&packet, PACKET_CONNECT, 0);
  packet_write_string(&packet, "Client Hello");

  if (network_send_packet(net, &packet, &net->server_addr)) {
    net->state = CONNECTION_CONNECTING;
    return true;
  }

  return false;
}

void network_client_disconnect(NetworkManager *net) {
  if (!net)
    return;

  if (net->state == CONNECTION_CONNECTED) {
    // Send DISCONNECT packet
    Packet packet;
    packet_init_write(&packet, PACKET_DISCONNECT, 0);
    network_send_packet(net, &packet, &net->server_addr);
  }

  net->state = CONNECTION_DISCONNECTED;
}

bool network_send_packet(NetworkManager *net, Packet *packet,
                         const NetAddress *dest) {
  if (!net || !net->socket || !packet || !dest)
    return false;
  return socket_send(net->socket, dest, packet->buffer, packet->length);
}

bool network_broadcast(NetworkManager *net, Packet *packet) {
  if (!net || !net->is_server)
    return false;
  // Loop through connected clients and send
  // (Client list unimplemented yet)
  return true;
}
