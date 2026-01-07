/**
 * =================================================================================================
 *                          RELIABLE UDP TRANSPORT
 *                          Phase 5: Networking & Multiplayer
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NET_MAX_PACKET_SIZE 1400
#define NET_MAX_CHANNELS 8
#define NET_MAX_PENDING 256
#define NET_SEQUENCE_BITS 16
#define NET_SEQUENCE_MAX ((1 << NET_SEQUENCE_BITS) - 1)
#define NET_ACK_BITS 32

// Packet flags
typedef enum {
  PACKET_UNRELIABLE = 0,
  PACKET_RELIABLE = 1 << 0,
  PACKET_ORDERED = 1 << 1,
  PACKET_FRAGMENTED = 1 << 2
} PacketFlags;

// Channel types
typedef enum {
  CHANNEL_UNRELIABLE,
  CHANNEL_RELIABLE_UNORDERED,
  CHANNEL_RELIABLE_ORDERED,
  CHANNEL_RELIABLE_SEQUENCED
} ChannelType;

// Packet header
typedef struct {
  uint16_t protocol_id;
  uint16_t sequence;
  uint16_t ack;
  uint32_t ack_bits;
  uint8_t channel;
  uint8_t flags;
  uint16_t payload_size;
} PacketHeader;

// Pending packet (awaiting ACK)
typedef struct {
  uint16_t sequence;
  float time_sent;
  float last_resend;
  uint8_t resend_count;
  uint8_t *data;
  uint16_t size;
  bool acked;
} PendingPacket;

// Channel state
typedef struct {
  ChannelType type;
  uint16_t local_sequence;
  uint16_t remote_sequence;
  uint16_t oldest_unacked;

  // Reliable tracking
  PendingPacket pending[NET_MAX_PENDING];
  uint32_t pending_count;

  // Ordered delivery
  uint16_t next_receive_sequence;
  uint8_t *ordered_buffer[NET_MAX_PENDING];
  uint16_t ordered_sizes[NET_MAX_PENDING];
} NetChannel;

// Connection state
typedef enum {
  CONN_DISCONNECTED,
  CONN_CONNECTING,
  CONN_CONNECTED,
  CONN_DISCONNECTING
} ConnectionState;

// Network connection
typedef struct {
  uint32_t id;
  uint32_t address; // IPv4
  uint16_t port;
  ConnectionState state;

  NetChannel channels[NET_MAX_CHANNELS];

  // RTT estimation
  float rtt;
  float rtt_variance;
  float timeout;

  // Statistics
  uint32_t packets_sent;
  uint32_t packets_received;
  uint32_t packets_lost;
  uint32_t bytes_sent;
  uint32_t bytes_received;

  float last_receive_time;
  float last_send_time;
} NetConnection;

// Network manager
typedef struct {
  int socket_handle;
  uint16_t local_port;
  uint16_t protocol_id;

  NetConnection *connections;
  uint32_t connection_count;
  uint32_t connection_capacity;

  // Callbacks
  void (*on_connect)(uint32_t conn_id);
  void (*on_disconnect)(uint32_t conn_id);
  void (*on_receive)(uint32_t conn_id, uint8_t channel, uint8_t *data,
                     uint16_t size);

  float time;
} NetworkManager;

// -----------------------------------------------------------------------------
// Sequence Number Utilities
// -----------------------------------------------------------------------------

static bool sequence_greater(uint16_t a, uint16_t b) {
  return ((a > b) && (a - b <= NET_SEQUENCE_MAX / 2)) ||
         ((a < b) && (b - a > NET_SEQUENCE_MAX / 2));
}

static uint16_t sequence_diff(uint16_t a, uint16_t b) {
  if (a >= b) {
    return a - b;
  } else {
    return (NET_SEQUENCE_MAX - b) + a + 1;
  }
}

// -----------------------------------------------------------------------------
// Network Manager
// -----------------------------------------------------------------------------

NetworkManager *net_create(uint16_t port, uint16_t protocol_id) {
  NetworkManager *net = (NetworkManager *)calloc(1, sizeof(NetworkManager));
  if (!net)
    return NULL;

  net->local_port = port;
  net->protocol_id = protocol_id;
  net->connection_capacity = 32;
  net->connections = (NetConnection *)calloc(32, sizeof(NetConnection));

  // Initialize channels with default types
  // Socket creation would happen here (platform-specific)

  return net;
}

void net_destroy(NetworkManager *net) {
  if (!net)
    return;
  free(net->connections);
  free(net);
}

// -----------------------------------------------------------------------------
// Connection Management
// -----------------------------------------------------------------------------

uint32_t net_connect(NetworkManager *net, uint32_t address, uint16_t port) {
  if (!net || net->connection_count >= net->connection_capacity)
    return UINT32_MAX;

  NetConnection *conn = &net->connections[net->connection_count];
  memset(conn, 0, sizeof(NetConnection));

  conn->id = net->connection_count;
  conn->address = address;
  conn->port = port;
  conn->state = CONN_CONNECTING;
  conn->rtt = 0.1f;
  conn->timeout = 10.0f;

  // Initialize channels
  conn->channels[0].type = CHANNEL_UNRELIABLE;
  conn->channels[1].type = CHANNEL_RELIABLE_ORDERED;
  conn->channels[2].type = CHANNEL_RELIABLE_UNORDERED;

  net->connection_count++;
  return conn->id;
}

void net_disconnect(NetworkManager *net, uint32_t conn_id) {
  if (!net || conn_id >= net->connection_count)
    return;
  net->connections[conn_id].state = CONN_DISCONNECTING;
}

// -----------------------------------------------------------------------------
// Packet Sending
// -----------------------------------------------------------------------------

static void write_header(uint8_t *buffer, PacketHeader *header) {
  buffer[0] = header->protocol_id >> 8;
  buffer[1] = header->protocol_id & 0xFF;
  buffer[2] = header->sequence >> 8;
  buffer[3] = header->sequence & 0xFF;
  buffer[4] = header->ack >> 8;
  buffer[5] = header->ack & 0xFF;
  buffer[6] = (header->ack_bits >> 24) & 0xFF;
  buffer[7] = (header->ack_bits >> 16) & 0xFF;
  buffer[8] = (header->ack_bits >> 8) & 0xFF;
  buffer[9] = header->ack_bits & 0xFF;
  buffer[10] = header->channel;
  buffer[11] = header->flags;
  buffer[12] = header->payload_size >> 8;
  buffer[13] = header->payload_size & 0xFF;
}

bool net_send(NetworkManager *net, uint32_t conn_id, uint8_t channel,
              uint8_t *data, uint16_t size, bool reliable) {
  if (!net || conn_id >= net->connection_count)
    return false;
  if (size > NET_MAX_PACKET_SIZE - 14)
    return false;

  NetConnection *conn = &net->connections[conn_id];
  NetChannel *ch = &conn->channels[channel];

  // Build packet
  uint8_t packet[NET_MAX_PACKET_SIZE];
  PacketHeader header = {0};

  header.protocol_id = net->protocol_id;
  header.sequence = ch->local_sequence++;
  header.ack = ch->remote_sequence;
  header.ack_bits = 0; // Would compute from received history
  header.channel = channel;
  header.flags = reliable ? PACKET_RELIABLE : PACKET_UNRELIABLE;
  header.payload_size = size;

  write_header(packet, &header);
  memcpy(packet + 14, data, size);

  // Track reliable packets
  if (reliable && ch->pending_count < NET_MAX_PENDING) {
    PendingPacket *pending = &ch->pending[ch->pending_count++];
    pending->sequence = header.sequence;
    pending->time_sent = net->time;
    pending->last_resend = net->time;
    pending->resend_count = 0;
    pending->data = (uint8_t *)malloc(size);
    memcpy(pending->data, data, size);
    pending->size = size;
    pending->acked = false;
  }

  // Send would happen here (platform-specific socket send)
  conn->packets_sent++;
  conn->bytes_sent += size + 14;
  conn->last_send_time = net->time;

  return true;
}

// -----------------------------------------------------------------------------
// Packet Receiving
// -----------------------------------------------------------------------------

static PacketHeader read_header(uint8_t *buffer) {
  PacketHeader h;
  h.protocol_id = (buffer[0] << 8) | buffer[1];
  h.sequence = (buffer[2] << 8) | buffer[3];
  h.ack = (buffer[4] << 8) | buffer[5];
  h.ack_bits =
      (buffer[6] << 24) | (buffer[7] << 16) | (buffer[8] << 8) | buffer[9];
  h.channel = buffer[10];
  h.flags = buffer[11];
  h.payload_size = (buffer[12] << 8) | buffer[13];
  return h;
}

static void process_acks(NetChannel *ch, uint16_t ack, uint32_t ack_bits) {
  for (uint32_t i = 0; i < ch->pending_count; i++) {
    PendingPacket *p = &ch->pending[i];
    if (p->acked)
      continue;

    uint16_t diff = sequence_diff(ack, p->sequence);
    if (diff == 0) {
      p->acked = true;
      free(p->data);
      p->data = NULL;
    } else if (diff < 32 && (ack_bits & (1 << (diff - 1)))) {
      p->acked = true;
      free(p->data);
      p->data = NULL;
    }
  }

  // Compact pending array
  uint32_t write = 0;
  for (uint32_t read = 0; read < ch->pending_count; read++) {
    if (!ch->pending[read].acked) {
      if (write != read) {
        ch->pending[write] = ch->pending[read];
      }
      write++;
    }
  }
  ch->pending_count = write;
}

void net_receive_packet(NetworkManager *net, uint32_t conn_id, uint8_t *packet,
                        uint16_t packet_size) {
  if (!net || conn_id >= net->connection_count)
    return;
  if (packet_size < 14)
    return;

  NetConnection *conn = &net->connections[conn_id];
  PacketHeader header = read_header(packet);

  if (header.protocol_id != net->protocol_id)
    return;
  if (header.channel >= NET_MAX_CHANNELS)
    return;

  NetChannel *ch = &conn->channels[header.channel];

  // Process ACKs
  process_acks(ch, header.ack, header.ack_bits);

  // Update remote sequence
  if (sequence_greater(header.sequence, ch->remote_sequence)) {
    ch->remote_sequence = header.sequence;
  }

  conn->packets_received++;
  conn->bytes_received += packet_size;
  conn->last_receive_time = net->time;

  // Deliver payload
  if (net->on_receive && header.payload_size > 0) {
    net->on_receive(conn_id, header.channel, packet + 14, header.payload_size);
  }
}

// -----------------------------------------------------------------------------
// Update & Resend
// -----------------------------------------------------------------------------

void net_update(NetworkManager *net, float dt) {
  if (!net)
    return;

  net->time += dt;

  for (uint32_t c = 0; c < net->connection_count; c++) {
    NetConnection *conn = &net->connections[c];
    if (conn->state != CONN_CONNECTED)
      continue;

    // Check timeout
    if (net->time - conn->last_receive_time > conn->timeout) {
      conn->state = CONN_DISCONNECTED;
      if (net->on_disconnect)
        net->on_disconnect(c);
      continue;
    }

    // Resend unacked packets
    for (uint8_t ch_idx = 0; ch_idx < NET_MAX_CHANNELS; ch_idx++) {
      NetChannel *ch = &conn->channels[ch_idx];

      for (uint32_t i = 0; i < ch->pending_count; i++) {
        PendingPacket *p = &ch->pending[i];
        if (p->acked)
          continue;

        float resend_time = conn->rtt * 1.5f;
        if (net->time - p->last_resend > resend_time) {
          // Resend packet
          net_send(net, c, ch_idx, p->data, p->size, false);
          p->last_resend = net->time;
          p->resend_count++;
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Delta Compression
// -----------------------------------------------------------------------------

typedef struct {
  uint32_t entity_id;
  uint32_t component_mask;
  uint8_t data[256];
  uint16_t data_size;
} EntityState;

uint16_t net_delta_encode(EntityState *baseline, EntityState *current,
                          uint8_t *out) {
  if (!baseline || !current || !out)
    return 0;

  uint16_t offset = 0;

  // Write entity ID
  out[offset++] = (current->entity_id >> 24) & 0xFF;
  out[offset++] = (current->entity_id >> 16) & 0xFF;
  out[offset++] = (current->entity_id >> 8) & 0xFF;
  out[offset++] = current->entity_id & 0xFF;

  // Write changed component mask
  uint32_t changed = baseline->component_mask ^ current->component_mask;
  out[offset++] = (changed >> 24) & 0xFF;
  out[offset++] = (changed >> 16) & 0xFF;
  out[offset++] = (changed >> 8) & 0xFF;
  out[offset++] = changed & 0xFF;

  // XOR delta encode data
  uint16_t min_size = baseline->data_size < current->data_size
                          ? baseline->data_size
                          : current->data_size;

  out[offset++] = (current->data_size >> 8) & 0xFF;
  out[offset++] = current->data_size & 0xFF;

  for (uint16_t i = 0; i < min_size; i++) {
    out[offset++] = baseline->data[i] ^ current->data[i];
  }
  for (uint16_t i = min_size; i < current->data_size; i++) {
    out[offset++] = current->data[i];
  }

  return offset;
}

void net_delta_decode(EntityState *baseline, uint8_t *delta, EntityState *out) {
  if (!baseline || !delta || !out)
    return;

  uint16_t offset = 0;

  out->entity_id = (delta[offset] << 24) | (delta[offset + 1] << 16) |
                   (delta[offset + 2] << 8) | delta[offset + 3];
  offset += 4;

  uint32_t changed = (delta[offset] << 24) | (delta[offset + 1] << 16) |
                     (delta[offset + 2] << 8) | delta[offset + 3];
  offset += 4;

  out->component_mask = baseline->component_mask ^ changed;

  out->data_size = (delta[offset] << 8) | delta[offset + 1];
  offset += 2;

  uint16_t min_size = baseline->data_size < out->data_size ? baseline->data_size
                                                           : out->data_size;

  for (uint16_t i = 0; i < min_size; i++) {
    out->data[i] = baseline->data[i] ^ delta[offset++];
  }
  for (uint16_t i = min_size; i < out->data_size; i++) {
    out->data[i] = delta[offset++];
  }
}
