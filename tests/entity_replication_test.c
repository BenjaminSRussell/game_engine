#include <network/replication/entity_replication.h>

#include <stdio.h>
#include <string.h>

/* Implemented in entity_replication.c. Not declared on the public header. */
uint32_t replication_get_entities_to_replicate(ReplicationSystem *system,
                                               uint32_t *entity_ids,
                                               uint32_t max_count,
                                               const Vec3 *reference_pos);
uint32_t replication_find_entities_by_owner(ReplicationSystem *system,
                                            uint32_t owner_client_id,
                                            uint32_t *entity_ids,
                                            uint32_t max_count);
void replication_reset_stats(ReplicationSystem *system);
void replication_update(ReplicationSystem *system, float delta_time);
ReplicationSystem *replication_get_system(void);

static int g_failures;

static void expect_true(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_failures++;
  }
}

static Vec3 vec3_xyz(float x, float y, float z) {
  Vec3 v;
  memset(&v, 0, sizeof(v));
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

typedef struct {
  uint32_t id;
  uint32_t data_size;
  uint8_t data[256];
} ParsedEntity;

/* Packet layout written by replication_generate_update_packet. */
static int parse_packet(const uint8_t *buf, uint32_t size, uint32_t *count_out,
                        ParsedEntity *out, int max_out) {
  uint32_t count;
  uint32_t offset;
  int n = 0;

  if (!buf || size < 4 || !out || max_out <= 0)
    return -1;
  memcpy(&count, buf, 4);
  if (count_out)
    *count_out = count;
  offset = 4;
  while (n < max_out && (uint32_t)n < count) {
    uint32_t id = 0;
    uint32_t data_size;
    if (offset + 8 > size)
      break;
    memcpy(&id, buf + offset, 3);
    offset += 3;
    offset += 1; /* flags */
    memcpy(&data_size, buf + offset, 4);
    offset += 4;
    if (data_size > sizeof(out->data) || offset + data_size > size)
      return -1;
    out[n].id = id;
    out[n].data_size = data_size;
    if (data_size > 0)
      memcpy(out[n].data, buf + offset, data_size);
    offset += data_size;
    n++;
  }
  return n;
}

static void test_create_and_lifecycle(void) {
  ReplicationSystem *system;
  ReplicationSystem *full;
  uint32_t id_a;
  uint32_t id_b;
  uint32_t id_c;
  uint32_t active = 99;
  uint32_t bytes = 99;
  uint32_t packets = 99;
  const char payload[] = "spawn";

  expect_true(replication_get_system() == NULL, "no global system yet");
  expect_true(replication_create(0) == NULL, "reject zero capacity");
  expect_true(replication_create(4097) == NULL, "reject capacity above 4096");
  expect_true(replication_get_system() == NULL, "failed create leaves global");

  system = replication_create(2);
  expect_true(system != NULL, "create capacity 2");
  expect_true(replication_get_system() == system, "global tracks latest create");

  replication_get_stats(system, &active, &bytes, &packets, NULL);
  expect_true(active == 0 && bytes == 0 && packets == 0, "fresh stats are zero");
  replication_get_stats(NULL, &active, &bytes, &packets, NULL);
  replication_update(NULL, 1.0f);
  replication_destroy(NULL);

  id_a = replication_spawn_entity(system, COMPONENT_TRANSFORM, 7);
  id_b = replication_spawn_entity(system, COMPONENT_PHYSICS, 7);
  id_c = replication_spawn_entity(system, COMPONENT_RENDER, 9);
  expect_true(id_a == 1 && id_b == 2, "ids are 1-based slots");
  expect_true(id_c == 0, "spawn fails when full");
  expect_true(replication_update_entity(system, id_a, payload, sizeof(payload)) ==
                  true,
              "update stores payload");
  expect_true(replication_update_entity(system, 0, payload, 4) == false,
              "reject id 0");
  expect_true(replication_update_entity(system, 3, payload, 4) == false,
              "reject id past capacity");
  expect_true(replication_despawn_entity(system, id_a) == true, "despawn id 1");
  expect_true(replication_despawn_entity(system, id_a) == false,
              "second despawn fails");
  expect_true(replication_update_entity(system, id_a, payload, 4) == false,
              "update after despawn fails");
  id_c = replication_spawn_entity(system, COMPONENT_RENDER, 9);
  expect_true(id_c == 1, "despawned slot is reused");

  replication_get_stats(system, &active, NULL, NULL, NULL);
  expect_true(active == 2, "active count after reuse");

  full = replication_create(1);
  expect_true(replication_get_system() == full, "second create replaces global");
  replication_destroy(full);
  expect_true(replication_get_system() == NULL, "destroy clears matching global");
  replication_destroy(system);
}

static void test_priority_and_relevancy(void) {
  ReplicationSystem *system;
  uint32_t ids[4];
  uint32_t n;
  uint32_t owners[4];
  Vec3 origin = vec3_xyz(0.0f, 0.0f, 0.0f);
  Vec3 near_edge = vec3_xyz(100.0f, 0.0f, 0.0f);
  Vec3 far_pos = vec3_xyz(50.0f, 0.0f, 0.0f);
  uint32_t id1;
  uint32_t id2;
  uint32_t id3;

  system = replication_create(4);
  expect_true(system != NULL, "relevancy system");
  expect_true(replication_set_relevancy_distance(NULL, 10.0f) == false,
              "null distance");
  expect_true(replication_set_reference_position(NULL, &origin) == false,
              "null system position");
  expect_true(replication_set_reference_position(system, NULL) == false,
              "null position");
  expect_true(replication_set_reference_position(system, &origin) == true,
              "set reference");
  expect_true(replication_set_relevancy_distance(system, 100.0f) == true,
              "set distance");

  id1 = replication_spawn_entity(system, COMPONENT_TRANSFORM, 7);
  id2 = replication_spawn_entity(system, COMPONENT_AI, 7);
  id3 = replication_spawn_entity(system, COMPONENT_AUDIO, 3);
  expect_true(id1 == 1 && id2 == 2 && id3 == 3, "three spawns");
  expect_true(replication_set_entity_priority(system, id1, 1) == true,
              "priority 1");
  expect_true(replication_set_entity_priority(system, id2, 10) == true,
              "priority 10");
  expect_true(replication_set_entity_priority(system, id3, 5) == true,
              "priority 5");
  expect_true(replication_set_entity_priority(system, 0, 1) == false,
              "bad priority id");

  n = replication_get_entities_to_replicate(system, ids, 4, &origin);
  expect_true(n == 3, "origin is inside default relevancy");
  expect_true(ids[0] == id2 && ids[1] == id3 && ids[2] == id1,
              "higher priority is replicated first");

  n = replication_get_entities_to_replicate(system, ids, 1, &origin);
  expect_true(n == 1 && ids[0] == id2, "max_count keeps the highest priority");

  n = replication_get_entities_to_replicate(system, ids, 4, &near_edge);
  expect_true(n == 3, "distance equal to the limit stays relevant");

  expect_true(replication_set_relevancy_distance(system, 10.0f) == true,
              "tight relevancy");
  n = replication_get_entities_to_replicate(system, ids, 4, &far_pos);
  expect_true(n == 0, "entities at the origin are culled past 10");
  n = replication_get_entities_to_replicate(system, ids, 4, NULL);
  expect_true(n == 3, "null reference skips distance culling");
  expect_true(replication_set_relevancy_distance(system, 0.0f) == true,
              "zero distance disables the range test");
  n = replication_get_entities_to_replicate(system, ids, 4, &far_pos);
  expect_true(n == 3, "non-positive distance keeps every active entity");
  expect_true(replication_set_relevancy_distance(system, 100.0f) == true,
              "restore distance");

  expect_true(replication_set_entity_dormant(system, id2, true) == true,
              "dormant");
  n = replication_get_entities_to_replicate(system, ids, 4, &origin);
  expect_true(n == 2 && ids[0] == id3 && ids[1] == id1,
              "dormant entity is not relevant");
  expect_true(replication_set_entity_dormant(system, id2, false) == true,
              "wake");
  n = replication_get_entities_to_replicate(system, ids, 4, &origin);
  expect_true(n == 3 && ids[0] == id2, "woken entity returns at its priority");

  n = replication_get_entities_to_replicate(NULL, ids, 4, &origin);
  expect_true(n == 0, "null system replicate query");
  n = replication_get_entities_to_replicate(system, NULL, 4, &origin);
  expect_true(n == 0, "null id buffer");
  n = replication_get_entities_to_replicate(system, ids, 0, &origin);
  expect_true(n == 0, "zero max_count");

  n = replication_find_entities_by_owner(system, 7, owners, 4);
  expect_true(n == 2 && owners[0] == id1 && owners[1] == id2, "owner 7");
  n = replication_find_entities_by_owner(system, 7, owners, 1);
  expect_true(n == 1 && owners[0] == id1, "owner query respects max_count");
  n = replication_find_entities_by_owner(system, 3, owners, 4);
  expect_true(n == 1 && owners[0] == id3, "owner 3");
  n = replication_find_entities_by_owner(system, 1, owners, 4);
  expect_true(n == 0, "unknown owner");
  n = replication_find_entities_by_owner(NULL, 7, owners, 4);
  expect_true(n == 0, "null owner query");

  replication_update(system, 0.016f);
  replication_destroy(system);
}

static void test_delta_packets(void) {
  ReplicationSystem *server;
  ReplicationSystem *client;
  uint8_t packet[128];
  uint8_t again[128];
  uint32_t size;
  uint32_t size2;
  uint32_t header_count = 0;
  uint32_t active = 0;
  uint32_t bytes = 0;
  uint32_t packets = 0;
  ParsedEntity parsed[4];
  int parsed_n;
  const char first[] = "DELTA-1";
  const char second[] = "DELTA-2-xyz";
  uint32_t id;
  uint32_t client_id;

  server = replication_create(4);
  client = replication_create(4);
  expect_true(server != NULL && client != NULL, "packet systems");

  size = replication_generate_update_packet(server, packet, sizeof(packet));
  expect_true(size == 4, "empty packet is just the count");
  replication_get_stats(server, &active, &bytes, &packets, NULL);
  expect_true(active == 0 && bytes == 4 && packets == 0, "empty send stats");
  expect_true(replication_generate_update_packet(server, packet, 7) == 0,
              "buffer under 8 bytes is rejected");
  expect_true(replication_generate_update_packet(NULL, packet, 32) == 0,
              "null generate");
  expect_true(replication_process_update_packet(server, packet, 3) == false,
              "short packet is rejected");
  expect_true(replication_process_update_packet(NULL, packet, 4) == false,
              "null process");

  replication_reset_stats(server);
  replication_get_stats(server, NULL, &bytes, &packets, NULL);
  expect_true(bytes == 0 && packets == 0, "reset stats");

  id = replication_spawn_entity(server, COMPONENT_TRANSFORM | COMPONENT_CUSTOM,
                                11);
  expect_true(replication_update_entity(server, id, first, sizeof(first)) == true,
              "first payload");
  size = replication_generate_update_packet(server, packet, sizeof(packet));
  parsed_n = parse_packet(packet, size, &header_count, parsed, 4);
  expect_true(parsed_n == 1 && header_count == 1, "one entity in the packet");
  expect_true(parsed[0].id == id && parsed[0].data_size == sizeof(first) &&
                  memcmp(parsed[0].data, first, sizeof(first)) == 0,
              "packet carries the entity payload");

  expect_true(replication_set_entity_dormant(server, id, true) == true,
              "dormant before send");
  size2 = replication_generate_update_packet(server, again, sizeof(again));
  parsed_n = parse_packet(again, size2, &header_count, parsed, 4);
  expect_true(header_count == 1 && parsed_n == 0,
              "dormant entity is omitted from the body");
  expect_true(replication_set_entity_dormant(server, id, false) == true,
              "wake before round trip");

  size = replication_generate_update_packet(server, packet, sizeof(packet));
  client_id = replication_spawn_entity(client, COMPONENT_TRANSFORM, 11);
  expect_true(client_id == id, "client slot matches server id");
  expect_true(replication_process_update_packet(client, packet, size) == true,
              "client accepts the packet");
  size2 = replication_generate_update_packet(client, again, sizeof(again));
  expect_true(size2 == size && memcmp(packet, again, size) == 0,
              "processed state regenerates the same packet");

  expect_true(
      replication_update_entity(server, id, second, sizeof(second)) == true,
      "second payload");
  size = replication_generate_update_packet(server, packet, sizeof(packet));
  expect_true(replication_process_update_packet(client, packet, size) == true,
              "client applies the newer payload");
  size2 = replication_generate_update_packet(client, again, sizeof(again));
  parsed_n = parse_packet(again, size2, &header_count, parsed, 4);
  expect_true(parsed_n == 1 && parsed[0].data_size == sizeof(second) &&
                  memcmp(parsed[0].data, second, sizeof(second)) == 0,
              "delta replaces the previous payload");

  replication_get_stats(server, &active, &bytes, &packets, NULL);
  expect_true(active == 1 && packets == 3 && bytes > 0,
              "each generated entity counts as a sent packet");

  expect_true(replication_despawn_entity(server, id) == true, "despawn sender");
  size = replication_generate_update_packet(server, packet, sizeof(packet));
  parsed_n = parse_packet(packet, size, &header_count, parsed, 4);
  expect_true(size == 4 && header_count == 0 && parsed_n == 0,
              "despawned entity is not packed");

  replication_destroy(client);
  replication_destroy(server);
  expect_true(replication_get_system() == NULL, "both systems destroyed");
}

int main(void) {
  test_create_and_lifecycle();
  test_priority_and_relevancy();
  test_delta_packets();
  if (g_failures != 0) {
    fprintf(stderr, "entity_replication: %d failure(s)\n", g_failures);
    return 1;
  }
  printf("ok\n");
  return 0;
}
