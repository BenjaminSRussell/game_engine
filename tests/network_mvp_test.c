#include "include/core/logger.h"
#include "include/network/network_manager.h"
#include "include/network/network_types.h"
#include "include/network/packet.h"
#include "include/network/rpc_system.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int g_sends;
static PacketType g_last_type;
static uint8_t g_last[MAX_PACKET_SIZE];
static size_t g_last_size;

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

int network_client_send(PacketType type, const void *data, size_t data_size) {
  g_sends++;
  g_last_type = type;
  g_last_size = 0;
  if (data && data_size > 0 && data_size <= sizeof(g_last)) {
    memcpy(g_last, data, data_size);
    g_last_size = data_size;
  }
  return 0;
}

int network_server_send_to_client(uint32_t client_id, PacketType type,
                                  const void *data, size_t data_size) {
  (void)client_id;
  return network_client_send(type, data, data_size);
}

int network_server_broadcast(PacketType type, const void *data,
                             size_t data_size) {
  return network_client_send(type, data, data_size);
}

static uint32_t read_be_u32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
         (uint32_t)p[3];
}

static void test_packet_type_ids(void) {
  printf("Testing packet type ids...\n");
  assert(PACKET_TYPE_ACK == 20);
  assert(PACKET_TYPE_FRAGMENT == 21);
  assert(PACKET_TYPE_SNAPSHOT_ACK == 22);
  assert(PACKET_TYPE_RPC_ACK == 23);
  assert(PACKET_TYPE_RPC_ACK != PACKET_TYPE_ACK);
  assert(PACKET_TYPE_RPC_ACK != PACKET_TYPE_FRAGMENT);
  assert(PACKET_TYPE_RPC_ACK != PACKET_TYPE_SNAPSHOT_ACK);
}

static void test_address_union(void) {
  NetAddress address;

  printf("Testing NetAddress IPv4/IPv6 union...\n");
  memset(&address, 0, sizeof(address));
  assert(address.type == NET_ADDR_IPV4);
  address.host = 0x7f000001u;
  assert(address.ip4 == 0x7f000001u);
  address.type = NET_ADDR_IPV6;
  memset(address.ip6, 0, sizeof(address.ip6));
  address.ip6[15] = 1;
  assert(address.type == NET_ADDR_IPV6);
  assert(address.ip6[15] == 1);
}

static void test_rle_roundtrip(void) {
  Packet packet;
  uint8_t original[200];

  printf("Testing RLE compress/decompress...\n");
  memset(original, 'A', sizeof(original));
  packet_init_write(&packet, PACKET_TYPE_WORLD_UPDATE, 0);
  memcpy(&packet.buffer[packet.write_pos], original, sizeof(original));
  packet.write_pos = (uint16_t)(packet.write_pos + sizeof(original));
  packet_finalize(&packet);
  assert(packet_compress(&packet));
  assert(packet_get_header(&packet).flags & NET_PACKET_FLAG_COMPRESSED);
  assert(packet.length < sizeof(PacketHeader) + sizeof(original));
  assert(packet_decompress(&packet));
  assert((packet_get_header(&packet).flags & NET_PACKET_FLAG_COMPRESSED) == 0);
  assert(packet.length == sizeof(PacketHeader) + sizeof(original));
  assert(memcmp(&packet.buffer[sizeof(PacketHeader)], original, sizeof(original)) ==
         0);

  for (int i = 0; i < (int)sizeof(original); i++)
    original[i] = (uint8_t)(i * 17u);
  packet_init_write(&packet, PACKET_TYPE_WORLD_UPDATE, 0);
  memcpy(&packet.buffer[packet.write_pos], original, sizeof(original));
  packet.write_pos = (uint16_t)(packet.write_pos + sizeof(original));
  packet_finalize(&packet);
  {
    uint16_t before = packet.length;
    assert(!packet_compress(&packet));
    assert(packet.length == before);
    assert((packet_get_header(&packet).flags & NET_PACKET_FLAG_COMPRESSED) == 0);
  }
}

static void test_fragment_roundtrip(void) {
  enum { PAYLOAD = 3000 };
  uint8_t data[PAYLOAD];
  Packet fragments[MAX_FRAGMENTS];
  FragmentBuffer buffer;
  uint16_t count;
  bool done = false;
  uint16_t i;

  printf("Testing fragmentation reassembly...\n");
  for (i = 0; i < PAYLOAD; i++)
    data[i] = (uint8_t)(i * 3u);

  count = packet_fragment_data(data, PAYLOAD, PACKET_TYPE_WORLD_UPDATE, 0,
                               fragments, MAX_FRAGMENTS);
  assert(count > 1);
  for (i = 0; i < count; i++) {
    PacketHeader header = packet_get_header(&fragments[i]);
    assert(header.type == PACKET_TYPE_WORLD_UPDATE);
    assert(header.type != PACKET_TYPE_FRAGMENT);
    assert(header.flags & NET_PACKET_FLAG_FRAGMENT);
  }

  memset(&buffer, 0, sizeof(buffer));
  for (i = 0; i < count; i++) {
    Packet view;
    uint16_t index = (uint16_t)(count - 1u - i);
    assert(packet_init_read(&view, fragments[index].buffer, fragments[index].length));
    done = packet_reassemble_fragment(&buffer, &view);
  }
  assert(done);
  assert(buffer.total_size == PAYLOAD);
  assert(memcmp(buffer.buffer, data, PAYLOAD) == 0);
}

static void test_rpc_body_and_ack(void) {
  Packet ack_body;
  uint8_t wire[MAX_PACKET_SIZE];
  PacketHeader header;
  uint32_t request_id;
  size_t id_off;
  uint16_t wire_size;
  int sends_after_first;

  printf("Testing RPC body flags and RPC_ACK...\n");
  rpc_init();
  g_sends = 0;
  rpc_send_reliable("hello", "xyz", 3);
  assert(g_sends == 1);
  assert(g_last_type == PACKET_TYPE_RPC);
  assert(g_last_size > 8);
  assert(g_last[0] == RPC_FLAG_RELIABLE);
  assert(g_last[1] == 5);
  assert(memcmp(g_last + 2, "hello", 5) == 0);
  id_off = 2u + g_last[1];
  request_id = read_be_u32(g_last + id_off);
  assert(request_id != 0);

  sends_after_first = g_sends;
  rpc_update(0.6f);
  assert(g_sends == sends_after_first + 1);
  assert(g_last_type == PACKET_TYPE_RPC);
  assert(g_last[0] == RPC_FLAG_RELIABLE);

  rpc_shutdown();
  rpc_init();
  g_sends = 0;
  rpc_send_reliable("hello", "xyz", 3);
  assert(g_sends == 1);
  assert(g_last[0] == RPC_FLAG_RELIABLE);
  id_off = 2u + g_last[1];
  request_id = read_be_u32(g_last + id_off);

  memset(&ack_body, 0, sizeof(ack_body));
  assert(packet_write_u8(&ack_body, 0));
  assert(packet_write_string(&ack_body, "__ACK__"));
  assert(packet_write_u32(&ack_body, request_id));
  assert(packet_write_u16(&ack_body, 0));
  ack_body.length = ack_body.write_pos;
  assert(ack_body.buffer[0] == 0);

  memset(&header, 0, sizeof(header));
  header.type = PACKET_TYPE_RPC_ACK;
  assert(sizeof(header) + ack_body.length <= sizeof(wire));
  memcpy(wire, &header, sizeof(header));
  memcpy(wire + sizeof(header), ack_body.buffer, ack_body.length);
  wire_size = (uint16_t)(sizeof(header) + ack_body.length);
  rpc_process_packet(0, wire, wire_size);

  rpc_update(0.6f);
  assert(g_sends == 1);
  rpc_shutdown();
}

int main(void) {
  test_packet_type_ids();
  test_address_union();
  test_rle_roundtrip();
  test_fragment_roundtrip();
  test_rpc_body_and_ack();
  printf("All network MVP tests passed.\n");
  return 0;
}
