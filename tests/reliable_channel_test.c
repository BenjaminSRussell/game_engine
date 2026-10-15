#include "include/network/transport/reliable_channel.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Matches ReliableHeader / RELIABLE_CHANNEL_MAX_PACKET_SIZE in reliable_channel.c. */
#define RC_HEADER_SIZE 12
#define RC_MAX_PAYLOAD 1200

void reliable_channel_process_packet(ReliableChannel *channel, const u8 *data,
                                     u32 size);
void reliable_channel_get_statistics(const ReliableChannel *channel,
                                     u32 *out_packets_sent,
                                     u32 *out_packets_received,
                                     u32 *out_packets_lost, f32 *out_rtt_ms);
void reliable_channel_reset_statistics(ReliableChannel *channel);
bool reliable_channel_is_connected(const ReliableChannel *channel);

static u64 g_now_ms = 1000;
static int g_send_calls = 0;
static bool g_allow_send = true;

#define CAPTURE_MAX 8
typedef struct {
  u8 data[1600];
  u32 size;
  NetworkAddress addr;
} CapturedDatagram;

static CapturedDatagram g_sent[CAPTURE_MAX];
static int g_sent_count = 0;

u64 get_current_time_ms(void) { return g_now_ms; }

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

bool udp_socket_send_to(UDPSocket *socket, const NetworkAddress *address,
                        const void *data, u32 size) {
  (void)socket;
  g_send_calls++;
  if (!g_allow_send)
    return false;
  if (data && size <= sizeof(g_sent[0].data) && g_sent_count < CAPTURE_MAX) {
    CapturedDatagram *slot = &g_sent[g_sent_count++];
    memset(slot, 0, sizeof(*slot));
    if (address)
      slot->addr = *address;
    memcpy(slot->data, data, size);
    slot->size = size;
  }
  return true;
}

static void reset_net(void) {
  g_now_ms = 1000;
  g_send_calls = 0;
  g_allow_send = true;
  g_sent_count = 0;
  memset(g_sent, 0, sizeof(g_sent));
}

static u32 read_u32(const u8 *p) {
  u32 value;
  memcpy(&value, p, sizeof(value));
  return value;
}

static ReliableChannel *make_channel(UDPSocket *socket, u16 port,
                                     ReliableMode mode) {
  NetworkAddress address;
  memset(&address, 0, sizeof(address));
  memcpy(address.address, "127.0.0.1", 10);
  address.port = port;
  return reliable_channel_create(mode, socket, &address);
}

static void test_create_rejects_missing_endpoint(void) {
  UDPSocket socket;
  NetworkAddress address;

  printf("Testing reliable channel create guards...\n");
  memset(&socket, 0, sizeof(socket));
  memset(&address, 0, sizeof(address));
  address.port = 9;
  assert(reliable_channel_create(RELIABLE_MODE_RELIABLE, NULL, &address) ==
         NULL);
  assert(reliable_channel_create(RELIABLE_MODE_RELIABLE, &socket, NULL) ==
         NULL);
  reliable_channel_destroy(NULL);
  reliable_channel_update(NULL, 0.016f);
  reliable_channel_send_ack(NULL);
  reliable_channel_process_ack(NULL, 0, 0);
  reliable_channel_process_packet(NULL, (const u8 *)"xxxx", 4);
  reliable_channel_get_statistics(NULL, NULL, NULL, NULL, NULL);
  reliable_channel_reset_statistics(NULL);
  assert(!reliable_channel_is_connected(NULL));
  assert(!reliable_channel_send(NULL, "a", 1));
  assert(!reliable_channel_receive(NULL, &socket, NULL));
}

static void test_send_rejects_bad_payload(void) {
  UDPSocket socket;
  ReliableChannel *channel;
  u8 big[RC_MAX_PAYLOAD + 1];
  u8 exact[RC_MAX_PAYLOAD];
  u8 out[8];
  u32 out_size;

  printf("Testing reliable channel payload limits...\n");
  reset_net();
  memset(&socket, 0, sizeof(socket));
  channel = make_channel(&socket, 27015, RELIABLE_MODE_RELIABLE);
  assert(channel != NULL);
  assert(channel->mode == RELIABLE_MODE_RELIABLE);
  assert(channel->socket == &socket);
  assert(channel->remote_address.port == 27015);
  assert(strcmp(channel->remote_address.address, "127.0.0.1") == 0);
  assert(channel->send_sequence == 0);
  assert(channel->send_count == 0);
  assert(!reliable_channel_is_connected(channel));

  assert(!reliable_channel_send(channel, NULL, 4));
  assert(!reliable_channel_send(channel, "hi", 0));
  memset(big, 0xAB, sizeof(big));
  assert(!reliable_channel_send(channel, big, RC_MAX_PAYLOAD + 1));
  assert(channel->send_count == 0);
  assert(g_send_calls == 0);

  memset(exact, 0xCD, sizeof(exact));
  assert(reliable_channel_send(channel, exact, RC_MAX_PAYLOAD));
  assert(g_sent_count == 1);
  assert(g_sent[0].size == RC_HEADER_SIZE + RC_MAX_PAYLOAD);
  assert(read_u32(g_sent[0].data) == 0);
  assert(memcmp(g_sent[0].data + RC_HEADER_SIZE, exact, RC_MAX_PAYLOAD) == 0);

  out_size = sizeof(out);
  assert(!reliable_channel_receive(channel, NULL, &out_size));
  assert(!reliable_channel_receive(channel, out, NULL));

  reliable_channel_destroy(channel);
}

static void test_in_order_delivery_and_ack(void) {
  UDPSocket socket_a;
  UDPSocket socket_b;
  ReliableChannel *a;
  ReliableChannel *b;
  u8 out[16];
  u32 out_size;
  u32 sent = 9;
  u32 received = 9;
  u32 lost = 9;
  f32 rtt = 9.0f;
  u8 ack_only[RC_HEADER_SIZE];

  printf("Testing in-order delivery and acks...\n");
  reset_net();
  memset(&socket_a, 0, sizeof(socket_a));
  memset(&socket_b, 0, sizeof(socket_b));
  a = make_channel(&socket_a, 27015, RELIABLE_MODE_RELIABLE_ORDERED);
  b = make_channel(&socket_b, 27016, RELIABLE_MODE_RELIABLE_SEQUENCED);
  assert(a != NULL && b != NULL);
  assert(b->mode == RELIABLE_MODE_RELIABLE_SEQUENCED);

  assert(reliable_channel_send(a, "one", 3));
  assert(g_sent_count == 1);
  assert(g_sent[0].size == RC_HEADER_SIZE + 3);
  assert(g_sent[0].addr.port == 27015);
  assert(read_u32(g_sent[0].data) == 0);
  assert(read_u32(g_sent[0].data + 4) == 0);
  assert(read_u32(g_sent[0].data + 8) == 0);
  assert(memcmp(g_sent[0].data + RC_HEADER_SIZE, "one", 3) == 0);
  assert(a->send_count == 1);
  assert(a->send_sequence == 1);
  assert(a->send_buffer[0].in_flight);
  assert(!a->send_buffer[0].acked);

  reliable_channel_process_packet(b, g_sent[0].data, g_sent[0].size);
  reliable_channel_process_packet(b, g_sent[0].data, g_sent[0].size);

  out_size = 1;
  assert(!reliable_channel_receive(b, out, &out_size));
  assert(b->recv_buffer[0].received);
  out_size = sizeof(out);
  assert(reliable_channel_receive(b, out, &out_size));
  assert(out_size == 3);
  assert(memcmp(out, "one", 3) == 0);
  assert(b->recv_sequence == 1);
  assert(reliable_channel_is_connected(b));
  out_size = sizeof(out);
  assert(!reliable_channel_receive(b, out, &out_size));
  assert(out_size == 0);

  assert(reliable_channel_send(a, "two", 3));
  assert(g_sent_count == 2);
  assert(read_u32(g_sent[1].data) == 1);
  reliable_channel_process_packet(b, g_sent[1].data, g_sent[1].size);
  out_size = sizeof(out);
  assert(reliable_channel_receive(b, out, &out_size));
  assert(out_size == 3);
  assert(memcmp(out, "two", 3) == 0);
  assert(b->recv_sequence == 2);

  reliable_channel_get_statistics(a, &sent, &received, &lost, &rtt);
  assert(sent == 2);
  assert(received == 0);
  assert(lost == 0);
  assert(rtt == 0.0f);
  reliable_channel_get_statistics(a, NULL, NULL, NULL, NULL);

  reliable_channel_process_ack(a, 0, 0);
  assert(a->last_ack == 0);
  assert(a->send_buffer[0].data == NULL);
  assert(a->send_count == 1);
  reliable_channel_process_ack(a, 1, 0);
  assert(a->send_buffer[1].data == NULL);
  assert(a->send_count == 0);

  assert(reliable_channel_send(a, "z", 1));
  assert(a->send_buffer[2].sequence == 2);
  memset(ack_only, 0, sizeof(ack_only));
  {
    u32 ack_seq = 2;
    memcpy(ack_only + 4, &ack_seq, sizeof(ack_seq));
  }
  reliable_channel_process_packet(a, ack_only, RC_HEADER_SIZE);
  assert(a->send_count == 0);
  out_size = sizeof(out);
  assert(!reliable_channel_receive(a, out, &out_size));

  g_sent_count = 0;
  reliable_channel_send_ack(b);
  assert(g_sent_count == 1);
  assert(g_sent[0].size == RC_HEADER_SIZE);
  assert(g_sent[0].addr.port == 27016);
  assert(read_u32(g_sent[0].data + 4) == b->recv_sequence);

  a->last_ack = 7;
  a->last_ack_bits = 3;
  reliable_channel_reset_statistics(a);
  assert(a->last_ack == 0);
  assert(a->last_ack_bits == 0);

  reliable_channel_destroy(a);
  reliable_channel_destroy(b);
}

static void test_retransmit_then_drop(void) {
  UDPSocket socket;
  ReliableChannel *channel;
  u32 sent = 0;
  u32 received = 0;
  u32 lost = 0;
  f32 rtt = 1.0f;
  int calls;
  int i;

  printf("Testing retransmission and retry limit...\n");
  reset_net();
  g_now_ms = 1000;
  memset(&socket, 0, sizeof(socket));
  channel = make_channel(&socket, 4000, RELIABLE_MODE_UNRELIABLE);
  assert(channel != NULL);
  assert(channel->mode == RELIABLE_MODE_UNRELIABLE);
  assert(reliable_channel_send(channel, "abc", 3));
  assert(g_send_calls == 1);
  assert(channel->send_buffer[0].retry_count == 0);
  assert(channel->send_buffer[0].timestamp == 1000);

  reliable_channel_update(channel, 0.016f);
  assert(g_send_calls == 1);

  g_now_ms = 1100;
  reliable_channel_update(channel, 0.1f);
  assert(g_send_calls == 1);

  g_now_ms = 1101;
  reliable_channel_update(channel, 0.101f);
  assert(g_send_calls == 2);
  assert(channel->send_buffer[0].retry_count == 1);
  assert(channel->send_buffer[0].in_flight);
  assert(channel->send_buffer[0].timestamp == 1101);

  for (i = 0; i < 31; i++) {
    g_now_ms += 101;
    reliable_channel_update(channel, 0.101f);
  }
  assert(channel->send_buffer[0].retry_count == 32);
  assert(channel->send_buffer[0].in_flight);

  calls = g_send_calls;
  g_now_ms += 101;
  reliable_channel_update(channel, 0.101f);
  assert(g_send_calls == calls);
  assert(!channel->send_buffer[0].in_flight);
  assert(channel->send_buffer[0].data != NULL);

  reliable_channel_get_statistics(channel, &sent, &received, &lost, &rtt);
  assert(sent == 1);
  assert(received == 0);
  assert(lost == 1);
  assert(rtt == 0.0f);

  reliable_channel_destroy(channel);
}

static void test_send_failure_still_queues(void) {
  UDPSocket socket;
  ReliableChannel *channel;

  printf("Testing failed datagram still queues...\n");
  reset_net();
  memset(&socket, 0, sizeof(socket));
  channel = make_channel(&socket, 7, RELIABLE_MODE_RELIABLE);
  assert(channel != NULL);
  g_allow_send = false;
  assert(reliable_channel_send(channel, "x", 1));
  assert(g_send_calls == 1);
  assert(g_sent_count == 0);
  assert(channel->send_count == 1);
  assert(channel->send_buffer[0].in_flight);
  assert(channel->send_buffer[0].data != NULL);
  reliable_channel_destroy(channel);
}

static void test_send_buffer_full(void) {
  UDPSocket socket;
  ReliableChannel *channel;
  int accepted = 0;
  int i;
  u8 extra = 1;

  printf("Testing send buffer capacity...\n");
  reset_net();
  memset(&socket, 0, sizeof(socket));
  channel = make_channel(&socket, 8, RELIABLE_MODE_RELIABLE);
  assert(channel != NULL);
  for (i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    u8 byte = (u8)(i & 0xFF);
    if (!reliable_channel_send(channel, &byte, 1))
      break;
    accepted++;
  }
  assert(accepted == RELIABLE_CHANNEL_BUFFER_SIZE);
  assert(channel->send_count == RELIABLE_CHANNEL_BUFFER_SIZE);
  assert(!reliable_channel_send(channel, &extra, 1));
  assert(channel->send_count == RELIABLE_CHANNEL_BUFFER_SIZE);
  reliable_channel_destroy(channel);
}

int main(void) {
  test_create_rejects_missing_endpoint();
  test_send_rejects_bad_payload();
  test_in_order_delivery_and_ack();
  test_retransmit_then_drop();
  test_send_failure_still_queues();
  test_send_buffer_full();
  printf("All reliable channel tests passed.\n");
  return 0;
}
