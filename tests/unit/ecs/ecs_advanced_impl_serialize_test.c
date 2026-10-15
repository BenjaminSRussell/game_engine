/* Proves ecs_serialize_entity writes the components stored for one entity in the
 * documented layout and honours its size contract. Issue #185. Compiles
 * ecs_advanced_impl.c directly because add_engine_test links Engine, which does
 * not build on origin/main.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool ecs_add_component_adv(uint32_t entity_id, uint32_t component_id,
                           const void *data, size_t size);
bool ecs_serialize_entity(uint32_t entity_id, void *buffer, size_t *size);

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

static uint32_t read_u32(const uint8_t *p) {
  uint32_t value;
  memcpy(&value, p, sizeof value);
  return value;
}

static void test_entity_without_components(void) {
  size_t size = 0;
  EXPECT(ecs_serialize_entity(42, NULL, &size), "size query should succeed");
  EXPECT(size == 2 * sizeof(uint32_t), "empty entity needs only its header");

  uint8_t buf[8];
  size = sizeof buf;
  EXPECT(ecs_serialize_entity(42, buf, &size), "serialize should succeed");
  EXPECT(size == 8, "empty entity writes 8 bytes");
  EXPECT(read_u32(buf) == 42, "header holds the entity id");
  EXPECT(read_u32(buf + 4) == 0, "empty entity has component_count 0");
}

static void test_components_serialize_in_layout(void) {
  const char payload[] = "abc";
  const uint8_t vector[] = {0x11, 0x22, 0x33, 0x44};
  const char other[] = "zz";
  EXPECT(ecs_add_component_adv(7, 3, payload, 3), "add component 3 to entity 7");
  EXPECT(ecs_add_component_adv(7, 4, vector, sizeof vector),
         "add component 4 to entity 7");
  EXPECT(ecs_add_component_adv(9, 3, other, 2), "add component 3 to entity 9");

  size_t size = 0;
  EXPECT(ecs_serialize_entity(7, NULL, &size), "size query should succeed");
  EXPECT(size == 31, "entity 7 needs 8 + (8 + 3) + (8 + 4) bytes");

  uint8_t buf[31];
  size_t capacity = 30;
  EXPECT(!ecs_serialize_entity(7, buf, &capacity), "short buffer is rejected");
  EXPECT(capacity == 31, "short buffer reports the required size");

  size = sizeof buf;
  EXPECT(ecs_serialize_entity(7, buf, &size), "serialize should succeed");
  EXPECT(size == 31, "serialize reports bytes written");
  EXPECT(read_u32(buf) == 7, "header holds the entity id");
  EXPECT(read_u32(buf + 4) == 2, "entity 7 has two components");

  EXPECT(read_u32(buf + 8) == 3, "first component id");
  EXPECT(read_u32(buf + 12) == 3, "first component data_size");
  EXPECT(memcmp(buf + 16, payload, 3) == 0, "first component data");
  EXPECT(read_u32(buf + 19) == 4, "second component id");
  EXPECT(read_u32(buf + 23) == 4, "second component data_size");
  EXPECT(memcmp(buf + 27, vector, 4) == 0, "second component data");
}

static void test_add_replaces_existing_component(void) {
  const char updated[] = "xy";
  EXPECT(ecs_add_component_adv(7, 3, updated, 2), "re-adding a component succeeds");

  size_t size = 0;
  EXPECT(ecs_serialize_entity(7, NULL, &size), "size query should succeed");
  EXPECT(size == 30, "replacing 3 bytes with 2 leaves two components");

  uint8_t buf[30];
  size = sizeof buf;
  EXPECT(ecs_serialize_entity(7, buf, &size), "serialize should succeed");
  EXPECT(size == 30, "serialize reports bytes written after replacement");
  EXPECT(read_u32(buf + 4) == 2, "replacement keeps the component count");
  EXPECT(read_u32(buf + 12) == 2, "replaced data_size is 2");
  EXPECT(memcmp(buf + 16, updated, 2) == 0, "replaced data is stored");
}

static void test_invalid_arguments(void) {
  EXPECT(!ecs_serialize_entity(7, NULL, NULL), "NULL size is rejected");
  EXPECT(!ecs_add_component_adv(8, 1, NULL, 4),
         "NULL data with a nonzero size is rejected");
}

int main(void) {
  test_entity_without_components();
  test_components_serialize_in_layout();
  test_add_replaces_existing_component();
  test_invalid_arguments();

  if (g_failed) {
    fprintf(stderr, "%d ecs serialize test(s) failed\n", g_failed);
    return 1;
  }
  printf("ecs advanced serialize tests passed\n");
  return 0;
}
