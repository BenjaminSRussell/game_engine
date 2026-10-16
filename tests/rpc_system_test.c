// Unit test for src/engine/network/rpc_system.c: handler registration, dispatch by
// name, dispatch of received packets, and response callbacks. rpc_system.c and
// packet.c are compiled directly. The logger and the network send functions are
// stubbed here, so the test does not link Engine.

// Keep assert() active in every build type so the checks cannot pass vacuously.
#undef NDEBUG
#include <assert.h>

#include "include/core/logger.h"
#include "include/network/network_manager.h"
#include "include/network/network_types.h"
#include "include/network/packet.h"
#include "include/network/rpc_system.h"

#include <stdio.h>
#include <string.h>

// rpc_system.c keeps at most MAX_RPC_FUNCTIONS (128) entries. rpc_init() uses one of
// them for "__RESPONSE__". RPC_NAME_LIMIT is the longest stored name (64-byte buffer).
#define RPC_TABLE_CAPACITY 128
#define RPC_NAME_LIMIT 63

static int g_sends;      // calls into the network send stubs
static int g_error_logs; // LOG_ERROR_CAT calls
static uint8_t g_last[MAX_PACKET_SIZE];
static size_t g_last_size;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
  if (level == LOG_LEVEL_ERROR) {
    g_error_logs++;
  }
}

static int capture_send(const void *data, size_t data_size) {
  g_sends++;
  g_last_size = 0;
  if (data && data_size > 0 && data_size <= sizeof(g_last)) {
    memcpy(g_last, data, data_size);
    g_last_size = data_size;
  }
  return 0;
}

int network_client_send(PacketType type, const void *data, size_t data_size) {
  (void)type;
  return capture_send(data, data_size);
}

int network_server_send_to_client(uint32_t client_id, PacketType type,
                                  const void *data, size_t data_size) {
  (void)client_id;
  (void)type;
  return capture_send(data, data_size);
}

int network_server_broadcast(PacketType type, const void *data,
                             size_t data_size) {
  (void)type;
  return capture_send(data, data_size);
}

// Records the arguments of the most recent call to record_handler.
typedef struct {
  int calls;
  uint32_t sender_id;
  uint32_t request_id;
  uint32_t size;
  void *data;
  uint8_t copy[MAX_PACKET_SIZE];
} Dispatch;

static Dispatch g_dispatch;
static int g_other_calls;

static void record_handler(uint32_t sender_id, uint32_t request_id, void *data,
                           uint32_t size) {
  g_dispatch.calls++;
  g_dispatch.sender_id = sender_id;
  g_dispatch.request_id = request_id;
  g_dispatch.size = size;
  g_dispatch.data = data;
  if (data && size <= sizeof(g_dispatch.copy)) {
    memcpy(g_dispatch.copy, data, size);
  }
}

static void other_handler(uint32_t sender_id, uint32_t request_id, void *data,
                          uint32_t size) {
  (void)sender_id;
  (void)request_id;
  (void)data;
  (void)size;
  g_other_calls++;
}

static void reset_state(void) {
  rpc_shutdown();
  rpc_init();
  memset(&g_dispatch, 0, sizeof(g_dispatch));
  g_other_calls = 0;
}

// Builds a body in the layout rpc_process_packet() reads: flags, name, request id,
// a u16 data size, then the data bytes. claimed_size may differ from payload_size
// to build malformed packets.
static size_t build_body(uint8_t *body, const char *name, uint32_t request_id,
                         uint16_t claimed_size, const void *payload,
                         size_t payload_size) {
  Packet packet;
  memset(&packet, 0, sizeof(packet));
  assert(packet_write_u8(&packet, 0));
  assert(packet_write_string(&packet, name));
  assert(packet_write_u32(&packet, request_id));
  assert(packet_write_u16(&packet, claimed_size));
  if (payload_size > 0) {
    assert(packet.write_pos + payload_size <= MAX_PACKET_SIZE);
    memcpy(&packet.buffer[packet.write_pos], payload, payload_size);
    packet.write_pos = (uint16_t)(packet.write_pos + payload_size);
  }
  memcpy(body, packet.buffer, packet.write_pos);
  return packet.write_pos;
}

// Prepends a PacketHeader to a body, as the network layer does before
// rpc_process_packet() sees the packet.
static uint16_t wire_from_body(uint8_t *wire, const uint8_t *body,
                               size_t body_size) {
  PacketHeader header;
  assert(sizeof(header) + body_size <= MAX_PACKET_SIZE);
  memset(&header, 0, sizeof(header));
  header.type = PACKET_TYPE_RPC;
  memcpy(wire, &header, sizeof(header));
  memcpy(wire + sizeof(header), body, body_size);
  return (uint16_t)(sizeof(header) + body_size);
}

static void deliver(uint32_t sender_id, const uint8_t *body, size_t body_size) {
  uint8_t wire[MAX_PACKET_SIZE];
  uint16_t wire_size = wire_from_body(wire, body, body_size);
  rpc_process_packet(sender_id, wire, wire_size);
}

static void test_register_and_dispatch_passes_arguments(void) {
  uint8_t payload[3] = {1, 2, 3};

  printf("Testing RPC registration and dispatch...\n");
  reset_state();
  rpc_register("ping", record_handler);

  assert(rpc_dispatch("ping", 7, 42, payload, (uint32_t)sizeof(payload)));
  assert(g_dispatch.calls == 1);
  assert(g_dispatch.sender_id == 7);
  assert(g_dispatch.request_id == 42);
  assert(g_dispatch.size == sizeof(payload));
  assert(g_dispatch.data == payload); // local dispatch passes the caller's buffer
  assert(memcmp(g_dispatch.copy, payload, sizeof(payload)) == 0);
}

static void test_unknown_and_null_names_are_not_dispatched(void) {
  printf("Testing dispatch of unknown RPC names...\n");
  reset_state();
  rpc_register("ping", record_handler);

  assert(!rpc_dispatch("pong", 0, 0, NULL, 0));
  assert(!rpc_dispatch("", 0, 0, NULL, 0));
  assert(!rpc_dispatch(NULL, 0, 0, NULL, 0));
  assert(g_dispatch.calls == 0);
}

static void test_register_rejects_invalid_arguments(void) {
  int errors;

  printf("Testing RPC registration argument checks...\n");
  reset_state();

  errors = g_error_logs;
  rpc_register(NULL, record_handler);
  assert(g_error_logs == errors + 1);

  errors = g_error_logs;
  rpc_register("no_handler", NULL);
  assert(g_error_logs == errors + 1);

  assert(!rpc_dispatch("no_handler", 0, 0, NULL, 0));
  assert(g_dispatch.calls == 0);
}

static void test_reregistration_replaces_handler(void) {
  printf("Testing RPC re-registration...\n");
  reset_state();
  rpc_register("ping", record_handler);
  rpc_register("ping", other_handler);

  assert(rpc_dispatch("ping", 1, 2, NULL, 0));
  assert(g_dispatch.calls == 0);
  assert(g_other_calls == 1);
}

static void test_name_length_limit(void) {
  char name[RPC_NAME_LIMIT + 1];

  printf("Testing RPC name length limit...\n");
  reset_state();
  memset(name, 'n', RPC_NAME_LIMIT);
  name[RPC_NAME_LIMIT] = '\0';

  rpc_register(name, record_handler);
  assert(rpc_dispatch(name, 0, 0, NULL, 0));
  assert(g_dispatch.calls == 1);
}

static void test_registration_capacity(void) {
  char name[16];
  int accepted = 0;

  printf("Testing RPC table capacity...\n");
  reset_state();
  rpc_register("ping", record_handler);
  rpc_register("ping", record_handler); // re-registration must not take a slot

  for (int i = 0; i < RPC_TABLE_CAPACITY; i++) {
    snprintf(name, sizeof(name), "fn_%03d", i);
    rpc_register(name, record_handler);
    if (!rpc_dispatch(name, 0, 0, NULL, 0)) {
      break;
    }
    accepted++;
  }

  // "__RESPONSE__" and "ping" hold two of the slots.
  assert(accepted == RPC_TABLE_CAPACITY - 2);
  assert(rpc_dispatch("ping", 0, 0, NULL, 0)); // earlier entries survive a full table
}

static void test_shutdown_clears_registrations(void) {
  printf("Testing RPC shutdown...\n");
  reset_state();
  rpc_register("ping", record_handler);

  rpc_shutdown();
  assert(!rpc_dispatch("ping", 0, 0, NULL, 0));

  rpc_init();
  assert(!rpc_dispatch("ping", 0, 0, NULL, 0)); // init does not restore user handlers
  assert(g_dispatch.calls == 0);
}

static void test_wire_dispatch_round_trip(void) {
  uint8_t payload[4] = {9, 8, 7, 6};

  printf("Testing dispatch of a sent RPC...\n");
  reset_state();
  rpc_register("ping", record_handler);

  g_sends = 0;
  rpc_send("ping", payload, (uint32_t)sizeof(payload));
  assert(g_sends == 1);
  deliver(5, g_last, g_last_size);

  assert(g_dispatch.calls == 1);
  assert(g_dispatch.sender_id == 5);
  assert(g_dispatch.request_id == 0);
  assert(g_dispatch.size == sizeof(payload));
  assert(memcmp(g_dispatch.copy, payload, sizeof(payload)) == 0);
}

static void test_wire_dispatch_empty_payload(void) {
  printf("Testing dispatch with no payload...\n");
  reset_state();
  rpc_register("ping", record_handler);

  g_sends = 0;
  rpc_send("ping", NULL, 0);
  assert(g_sends == 1);
  deliver(3, g_last, g_last_size);

  assert(g_dispatch.calls == 1);
  assert(g_dispatch.data == NULL);
  assert(g_dispatch.size == 0);
}

static void test_malformed_packets_are_dropped(void) {
  uint8_t payload[1025];
  uint8_t body[MAX_PACKET_SIZE];
  uint8_t wire[MAX_PACKET_SIZE];
  size_t body_size;

  printf("Testing RPC packet validation before dispatch...\n");
  reset_state();
  rpc_register("ping", record_handler);
  memset(payload, 0xAB, sizeof(payload));
  memset(wire, 0, sizeof(wire));

  // Shorter than a PacketHeader.
  rpc_process_packet(0, wire, (uint16_t)(sizeof(PacketHeader) - 1));
  assert(g_dispatch.calls == 0);

  // No packet data.
  rpc_process_packet(0, NULL, 0);
  assert(g_dispatch.calls == 0);

  // Claims 10 data bytes but carries 2.
  body_size = build_body(body, "ping", 0, 10, payload, 2);
  deliver(0, body, body_size);
  assert(g_dispatch.calls == 0);

  // Claims 1025 data bytes, one more than the 1024-byte receive buffer.
  body_size = build_body(body, "ping", 0, 1025, payload, 1025);
  deliver(0, body, body_size);
  assert(g_dispatch.calls == 0);

  // Unknown name.
  body_size = build_body(body, "nope", 0, 2, payload, 2);
  deliver(0, body, body_size);
  assert(g_dispatch.calls == 0);

  // 1024 bytes is the largest payload that is dispatched.
  body_size = build_body(body, "ping", 9, 1024, payload, 1024);
  deliver(4, body, body_size);
  assert(g_dispatch.calls == 1);
  assert(g_dispatch.sender_id == 4);
  assert(g_dispatch.request_id == 9);
  assert(g_dispatch.size == 1024);
  assert(memcmp(g_dispatch.copy, payload, 1024) == 0);
}

typedef struct {
  int calls;
  uint32_t request_id;
  uint32_t value;
} CallContext;

static void on_response(uint32_t request_id, Packet *response, void *user_data) {
  CallContext *ctx = (CallContext *)user_data;
  uint32_t value = 0;

  ctx->calls++;
  ctx->request_id = request_id;
  if (response && packet_read_u32(response, &value)) {
    ctx->value = value;
  }
}

static void test_response_callback_dispatch(void) {
  CallContext ctx;
  Packet params;
  Packet results;
  uint8_t body[MAX_PACKET_SIZE];
  size_t body_size;
  uint32_t request_id;

  printf("Testing RPC response dispatch...\n");
  reset_state();
  memset(&ctx, 0, sizeof(ctx));

  rpc_params_init(&params);
  request_id = rpc_call("remote_add", &params, on_response, &ctx);
  assert(request_id != 0);

  // The peer's reply arrives as a __RESPONSE__ packet with the same request id.
  rpc_params_init(&results);
  assert(packet_write_u32(&results, 1234));
  body_size = build_body(body, "__RESPONSE__", request_id, results.write_pos,
                         results.buffer, results.write_pos);
  deliver(0, body, body_size);
  assert(ctx.calls == 1);
  assert(ctx.request_id == request_id);
  assert(ctx.value == 1234);

  // The pending request is consumed, so a repeated reply does not call back again.
  deliver(0, body, body_size);
  assert(ctx.calls == 1);
}

int main(void) {
  test_register_and_dispatch_passes_arguments();
  test_unknown_and_null_names_are_not_dispatched();
  test_register_rejects_invalid_arguments();
  test_reregistration_replaces_handler();
  test_name_length_limit();
  test_registration_capacity();
  test_shutdown_clears_registrations();
  test_wire_dispatch_round_trip();
  test_wire_dispatch_empty_payload();
  test_malformed_packets_are_dropped();
  test_response_callback_dispatch();
  rpc_shutdown();
  printf("All RPC system tests passed.\n");
  return 0;
}
