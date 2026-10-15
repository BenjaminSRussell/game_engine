/*
 * net_replication_test.c
 * Property registration, relevancy, bandwidth, and delta snapshots.
 * Compiles net_replication.c directly and does not link Engine.
 */

#include "include/network/replication/net_replication.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct {
  f32 health;
  int ammo;
  Vec3 position;
} Actor;

typedef struct {
  u32 client_id;
  u32 size;
  u8 data[128];
} SentPacket;

typedef struct {
  u8 index;
  const void *bytes;
  u32 size;
} ExpectProp;

static SentPacket g_sent[NET_MAX_CLIENTS];
static int g_sent_count;
static u8 g_packet[8192] __attribute__((aligned(16)));

static void reset_capture(void) {
  g_sent_count = 0;
  memset(g_sent, 0, sizeof(g_sent));
}

static void *get_packet_buffer(u32 size) {
  (void)size;
  memset(g_packet, 0, sizeof(g_packet));
  return g_packet;
}

static void send_packet(u32 client_id, void *data, u32 size) {
  SentPacket *pkt;
  if (g_sent_count >= NET_MAX_CLIENTS || size > sizeof(g_sent[0].data)) {
    fprintf(stderr, "FAIL: unexpected send client=%u size=%u\n", client_id,
            size);
    exit(1);
  }
  pkt = &g_sent[g_sent_count++];
  pkt->client_id = client_id;
  pkt->size = size;
  memcpy(pkt->data, data, size);
}

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static SentPacket *find_sent(u32 client_id) {
  for (int i = 0; i < g_sent_count; i++) {
    if (g_sent[i].client_id == client_id)
      return &g_sent[i];
  }
  return NULL;
}

static void expect_packet(const SentPacket *pkt, u32 net_id,
                          const ExpectProp *props, int prop_count) {
  u32 off = 0;
  u32 id = 0;
  if (!pkt)
    fail("missing packet");
  if (pkt->size < sizeof(u32))
    fail("short packet");
  memcpy(&id, pkt->data, sizeof(id));
  if (id != net_id)
    fail("net id");
  off = sizeof(u32);
  for (int i = 0; i < prop_count; i++) {
    if (off >= pkt->size || pkt->data[off] != props[i].index)
      fail("property index");
    off++;
    if (off + props[i].size > pkt->size)
      fail("property truncated");
    if (memcmp(pkt->data + off, props[i].bytes, props[i].size) != 0)
      fail("property bytes");
    off += props[i].size;
  }
  if (off != pkt->size)
    fail("trailing packet bytes");
}

static void expect_remote_packets(u32 local_client, u32 net_id,
                                  const ExpectProp *props, int prop_count) {
  if (g_sent_count != NET_MAX_CLIENTS - 1)
    fail("remote client count");
  if (find_sent(local_client))
    fail("replicated to self");
  for (u32 client = 0; client < NET_MAX_CLIENTS; client++) {
    if (client == local_client)
      continue;
    expect_packet(find_sent(client), net_id, props, prop_count);
  }
}

static void on_rpc(void *object, void *data) {
  (void)object;
  (void)data;
}

static void test_registration(void) {
  NetReplicationSystem *net;
  Actor actor;
  u32 id;
  u32 second;
  int i;

  memset(&actor, 0, sizeof(actor));
  net = net_replication_create(NET_AUTHORITY_SERVER, 3);
  if (!net)
    fail("create");
  if (net->authority != NET_AUTHORITY_SERVER || net->local_client_id != 3)
    fail("create fields");
  if (net->bytes_per_second_limit != 100000 || !net->enable_delta_compression)
    fail("create defaults");
  if (net->object_count != 0)
    fail("create object count");

  net_replication_destroy(NULL);
  if (net_register_object(NULL, &actor, 1) != 0)
    fail("register null");
  net_add_property(NULL, 0, "health", NET_PROP_FLOAT, 0, REPLICATE_ALWAYS);
  net_register_rpc(NULL, 0, "ping", on_rpc);
  net_update_relevancy(NULL, 0, 0, false);
  net_set_priority(NULL, 0, 0, 4.0f);
  net_replicate_objects(NULL, 0.016f, get_packet_buffer, send_packet);

  id = net_register_object(net, &actor, 9);
  if (id != 0 || net->object_count != 1)
    fail("first id");
  if (net->objects[id].object_ptr != &actor ||
      net->objects[id].owner_client_id != 9 ||
      net->objects[id].property_count != 0 || net->objects[id].rpc_count != 0)
    fail("registered object");
  for (i = 0; i < NET_MAX_CLIENTS; i++) {
    if (!net->objects[id].is_relevant[i] || net->objects[id].priority[i] != 1.0f)
      fail("default relevancy");
  }

  second = net_register_object(net, &actor, 4);
  if (second != 1 || net->object_count != 2)
    fail("second id");

  net_add_property(net, id, NULL, NET_PROP_FLOAT, offsetof(Actor, health),
                   REPLICATE_ALWAYS);
  if (net->objects[id].property_count != 0)
    fail("null property name");

  net_add_property(net, id, "health", NET_PROP_FLOAT, offsetof(Actor, health),
                   REPLICATE_OWNER_ONLY);
  net_add_property(net, id, "ammo", NET_PROP_INT, offsetof(Actor, ammo),
                   REPLICATE_ALWAYS);
  net_add_property(net, id, "position", NET_PROP_VEC3, offsetof(Actor, position),
                   REPLICATE_SIMULATED);
  if (net->objects[id].property_count != 3)
    fail("property count");
  if (strcmp(net->objects[id].properties[0].name, "health") != 0 ||
      net->objects[id].properties[0].type != NET_PROP_FLOAT ||
      net->objects[id].properties[0].offset != offsetof(Actor, health) ||
      net->objects[id].properties[0].flags != REPLICATE_OWNER_ONLY ||
      !net->objects[id].properties[0].dirty ||
      !net->objects[id].properties[0].last_value)
    fail("health property");
  if (*(unsigned char *)net->objects[id].properties[1].last_value != 0)
    fail("ammo baseline");

  net_add_property(net, 99, "nope", NET_PROP_BOOL, 0, REPLICATE_ALWAYS);
  if (net->objects[id].property_count != 3)
    fail("invalid net id property");

  net_register_rpc(net, id, "ping", on_rpc);
  if (net->objects[id].rpc_count != 1 ||
      net->objects[id].rpc_handlers[0] != on_rpc)
    fail("rpc");
  for (i = 0; i < 16; i++)
    net_register_rpc(net, id, "ping", on_rpc);
  if (net->objects[id].rpc_count != 16)
    fail("rpc limit");

  net_update_relevancy(net, id, 4, false);
  net_update_relevancy(net, id, NET_MAX_CLIENTS, false);
  net_set_priority(net, id, 6, 2.5f);
  net_set_priority(net, id, NET_MAX_CLIENTS, 9.0f);
  if (!net->objects[id].is_relevant[3] || net->objects[id].is_relevant[4] ||
      net->objects[id].priority[6] != 2.5f || net->objects[id].priority[5] != 1.0f)
    fail("relevancy update");

  net_replication_destroy(net);

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &actor, 1);
  for (i = 0; i < NET_MAX_PROPERTIES_PER_OBJECT; i++) {
    char name[16];
    snprintf(name, sizeof(name), "p%d", i);
    net_add_property(net, id, name, NET_PROP_BOOL, 0, REPLICATE_INITIAL_ONLY);
  }
  if (net->objects[id].property_count != NET_MAX_PROPERTIES_PER_OBJECT)
    fail("property fill");
  net_add_property(net, id, "overflow", NET_PROP_BOOL, 0, REPLICATE_ALWAYS);
  if (net->objects[id].property_count != NET_MAX_PROPERTIES_PER_OBJECT)
    fail("property limit");
  net_replication_destroy(net);
}

static void test_replication(void) {
  NetReplicationSystem *net;
  Actor actor;
  u32 id;
  ExpectProp initial[3];
  ExpectProp health_only[1];
  f32 health_zero = 0.0f;
  f32 health_next = 4.0f;
  f32 health_later = 9.0f;
  int ammo_zero = 0;

  memset(&actor, 0, sizeof(actor));
  actor.position.x = 1.0f;
  actor.position.y = 2.0f;
  actor.position.z = 3.0f;

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &actor, 8);
  net_add_property(net, id, "health", NET_PROP_FLOAT, offsetof(Actor, health),
                   REPLICATE_ALWAYS);
  net_add_property(net, id, "ammo", NET_PROP_INT, offsetof(Actor, ammo),
                   REPLICATE_ALWAYS);
  net_add_property(net, id, "position", NET_PROP_VEC3, offsetof(Actor, position),
                   REPLICATE_ALWAYS);

  initial[0].index = 0;
  initial[0].bytes = &health_zero;
  initial[0].size = sizeof(health_zero);
  initial[1].index = 1;
  initial[1].bytes = &ammo_zero;
  initial[1].size = sizeof(ammo_zero);
  initial[2].index = 2;
  initial[2].bytes = &actor.position;
  initial[2].size = sizeof(actor.position);

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  expect_remote_packets(0, id, initial, 3);
  if (net->objects[id].properties[0].dirty ||
      memcmp(net->objects[id].properties[0].last_value, &health_zero,
             sizeof(health_zero)) != 0)
    fail("baseline after initial send");

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != 0)
    fail("delta sent unchanged state");

  actor.health = health_next;
  health_only[0].index = 0;
  health_only[0].bytes = &health_next;
  health_only[0].size = sizeof(health_next);
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  expect_remote_packets(0, id, health_only, 1);

  net_update_relevancy(net, id, 4, false);
  actor.health = health_later;
  health_only[0].bytes = &health_later;
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (find_sent(4) || !find_sent(1) || g_sent_count != NET_MAX_CLIENTS - 2)
    fail("relevancy filter");
  expect_packet(find_sent(1), id, health_only, 1);
  expect_packet(find_sent(63), id, health_only, 1);

  net->enable_delta_compression = false;
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (find_sent(4) || g_sent_count != NET_MAX_CLIENTS - 2)
    fail("delta disabled count");
  initial[0].bytes = &health_later;
  expect_packet(find_sent(2), id, initial, 3);

  net_replication_destroy(net);

  net = net_replication_create(NET_AUTHORITY_CLIENT_PREDICTED, 0);
  net_register_object(net, &actor, 1);
  net_add_property(net, 0, "health", NET_PROP_FLOAT, offsetof(Actor, health),
                   REPLICATE_ALWAYS);
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != 0)
    fail("client authority sent");
  net_replication_destroy(net);

  net = net_replication_create(NET_AUTHORITY_SERVER, 5);
  net_register_object(net, NULL, 1);
  net_add_property(net, 0, "health", NET_PROP_FLOAT, 0, REPLICATE_ALWAYS);
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != 0)
    fail("null object sent");
  net_replicate_objects(net, 0.016f, NULL, send_packet);
  net_replicate_objects(net, 0.016f, get_packet_buffer, NULL);
  net_replication_destroy(net);
}

static void test_bandwidth(void) {
  NetReplicationSystem *net;
  f32 health = 0.0f;
  u32 id;
  ExpectProp prop;
  u32 one_packet = (u32)(sizeof(u32) + 1 + sizeof(health));

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &health, 1);
  net_add_property(net, id, "health", NET_PROP_FLOAT, 0, REPLICATE_ALWAYS);

  net->bytes_per_second_limit = one_packet;
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  prop.index = 0;
  prop.bytes = &health;
  prop.size = sizeof(health);
  if (g_sent_count != 1 || !find_sent(1) || find_sent(2))
    fail("budget allows one client");
  expect_packet(find_sent(1), id, &prop, 1);
  if (net->bytes_sent_this_second != one_packet)
    fail("bytes sent");

  net_replication_destroy(net);

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &health, 1);
  net_add_property(net, id, "health", NET_PROP_FLOAT, 0, REPLICATE_ALWAYS);
  net->bytes_sent_this_second = net->bytes_per_second_limit;
  net->second_accumulator = 0.0f;
  reset_capture();
  net_replicate_objects(net, 0.1f, get_packet_buffer, send_packet);
  if (g_sent_count != 0 || !net->objects[id].properties[0].dirty)
    fail("over budget dropped the delta");

  reset_capture();
  net_replicate_objects(net, 1.0f, get_packet_buffer, send_packet);
  if (g_sent_count != NET_MAX_CLIENTS - 1 || !find_sent(1))
    fail("budget reset did not resend");
  expect_packet(find_sent(1), id, &prop, 1);
  net_replication_destroy(net);
}

typedef struct {
  char name[8];
  u32 guard;
} Named;

static bool g_fail_next_buffer;

static void *get_packet_buffer_fail_once(u32 size) {
  if (g_fail_next_buffer) {
    g_fail_next_buffer = false;
    return NULL;
  }
  return get_packet_buffer(size);
}

static bool packet_has_byte(const SentPacket *pkt, u8 value) {
  for (u32 i = 0; i < pkt->size; i++) {
    if (pkt->data[i] == value)
      return true;
  }
  return false;
}

/* String fields send their characters only, and never bytes past the field. */
static void test_string_property(void) {
  NetReplicationSystem *net;
  Named named;
  Named full;
  SentPacket *pkt;
  u16 wire_len = 0;
  u32 id;

  memset(&named, 0, sizeof(named));
  memset(&full, 0, sizeof(full));
  strcpy(named.name, "abc");
  named.guard = 0xA5A5A5A5u;
  memcpy(full.name, "abcdefgh", sizeof(full.name));
  full.guard = 0xA5A5A5A5u;

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &named, 1);
  net_add_string_property(net, id, "name", offsetof(Named, name),
                          sizeof(named.name), REPLICATE_ALWAYS);
  if (net->objects[id].property_count != 1 ||
      net->objects[id].properties[0].type != NET_PROP_STRING)
    fail("string property registered");

  /* A plain add of the string type, and out-of-range capacities, are refused. */
  net_add_property(net, id, "plain", NET_PROP_STRING, 0, REPLICATE_ALWAYS);
  net_add_string_property(net, id, "zero", 0, 0, REPLICATE_ALWAYS);
  net_add_string_property(net, id, "huge", 0, NET_MAX_STRING_PROPERTY_BYTES + 1,
                          REPLICATE_ALWAYS);
  if (net->objects[id].property_count != 1)
    fail("string refusals");

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != NET_MAX_CLIENTS - 1)
    fail("string initial send");
  pkt = find_sent(1);
  if (!pkt || pkt->size != sizeof(u32) + 1 + sizeof(u16) + 3)
    fail("string packet size");
  if (pkt->data[sizeof(u32)] != 0)
    fail("string property index");
  memcpy(&wire_len, pkt->data + sizeof(u32) + 1, sizeof(wire_len));
  if (wire_len != 3 || memcmp(pkt->data + sizeof(u32) + 1 + sizeof(u16), "abc", 3) != 0)
    fail("string bytes");
  if (packet_has_byte(pkt, 0xA5))
    fail("string sent bytes past its NUL");

  /* Changing the text resends it; an unchanged field sends nothing. */
  strcpy(named.name, "xy");
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  pkt = find_sent(1);
  if (!pkt || pkt->size != sizeof(u32) + 1 + sizeof(u16) + 2)
    fail("string change size");
  memcpy(&wire_len, pkt->data + sizeof(u32) + 1, sizeof(wire_len));
  if (wire_len != 2 || memcmp(pkt->data + sizeof(u32) + 3, "xy", 2) != 0)
    fail("string change bytes");
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != 0)
    fail("string resent unchanged");
  net_replication_destroy(net);

  /* A full field with no NUL sends exactly its capacity. */
  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &full, 1);
  net_add_string_property(net, id, "name", offsetof(Named, name),
                          sizeof(full.name), REPLICATE_ALWAYS);
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  pkt = find_sent(1);
  if (!pkt || pkt->size != sizeof(u32) + 1 + sizeof(u16) + sizeof(full.name))
    fail("full string size");
  if (memcmp(pkt->data + sizeof(u32) + 3, "abcdefgh", sizeof(full.name)) != 0 ||
      packet_has_byte(pkt, 0xA5))
    fail("full string bytes");
  net_replication_destroy(net);
}

/* A client cut off by the byte budget still receives the change later. */
static void test_throttled_delta(void) {
  NetReplicationSystem *net;
  f32 health = 0.0f;
  ExpectProp prop;
  u32 id;
  u32 one_packet = (u32)(sizeof(u32) + 1 + sizeof(health));

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &health, 1);
  net_add_property(net, id, "health", NET_PROP_FLOAT, 0, REPLICATE_ALWAYS);

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != NET_MAX_CLIENTS - 1)
    fail("throttle initial send");

  health = 5.0f;
  prop.index = 0;
  prop.bytes = &health;
  prop.size = sizeof(health);
  net->bytes_per_second_limit = one_packet;
  net->bytes_sent_this_second = 0;
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != 1 || !find_sent(1) || find_sent(2))
    fail("throttle cut client 2");

  net->bytes_per_second_limit = 100000;
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  expect_packet(find_sent(2), id, &prop, 1);

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != 0)
    fail("throttle delta resent after every client got it");
  net_replication_destroy(net);
}

/* A client whose packet buffer was unavailable gets the change on the next tick. */
static void test_null_buffer_delta(void) {
  NetReplicationSystem *net;
  f32 health = 0.0f;
  ExpectProp prop;
  u32 id;

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &health, 1);
  net_add_property(net, id, "health", NET_PROP_FLOAT, 0, REPLICATE_ALWAYS);

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);

  health = 7.0f;
  prop.index = 0;
  prop.bytes = &health;
  prop.size = sizeof(health);
  g_fail_next_buffer = true;
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer_fail_once, send_packet);
  if (find_sent(1) || !find_sent(2))
    fail("null buffer client");

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  expect_packet(find_sent(1), id, &prop, 1);

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (g_sent_count != 0)
    fail("null buffer delta resent");
  net_replication_destroy(net);
}

/* A value that changed while a client was irrelevant reaches it once relevant. */
static void test_relevancy_return(void) {
  NetReplicationSystem *net;
  f32 health = 0.0f;
  ExpectProp prop;
  u32 id;

  net = net_replication_create(NET_AUTHORITY_SERVER, 0);
  id = net_register_object(net, &health, 1);
  net_add_property(net, id, "health", NET_PROP_FLOAT, 0, REPLICATE_ALWAYS);

  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);

  net_update_relevancy(net, id, 4, false);
  health = 6.0f;
  prop.index = 0;
  prop.bytes = &health;
  prop.size = sizeof(health);
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  if (find_sent(4) || !find_sent(1))
    fail("irrelevant client sent");

  net_update_relevancy(net, id, 4, true);
  reset_capture();
  net_replicate_objects(net, 0.016f, get_packet_buffer, send_packet);
  expect_packet(find_sent(4), id, &prop, 1);
  net_replication_destroy(net);
}

int main(void) {
  test_registration();
  test_replication();
  test_bandwidth();
  test_string_property();
  test_throttled_delta();
  test_null_buffer_delta();
  test_relevancy_return();
  printf("net_replication tests passed\n");
  return 0;
}
