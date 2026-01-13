/**
 * @file city_test.c
 * @brief Test Implementation
 * @description Test system implementation
 * @date 2026-01-13
 */

#include "city_generator.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void test_config() {
  CityConfig config;
  bool success = city_config_from_preset(&config, CITY_STYLE_MEDIEVAL, 500.0f);
  assert(success);
  assert(config.style == CITY_STYLE_MEDIEVAL);
  assert(config.size_x == 500.0f);

  char buffer[1024];
  success = city_config_serialize(&config, buffer, sizeof(buffer));
  assert(success);
  printf("Serialized Config: %s\n", buffer);
}

void test_road_network() {
  RoadNetwork network;
  bool success = road_network_init(&network, 100);
  assert(success);

  CityConfig config;
  city_config_from_preset(&config, CITY_STYLE_MODERN, 200.0f);
  config.main_road_width = 10.0f;
  config.random_seed = 42;

  road_add_main_roads(&network, &config);
  printf("Nodes added: %u\n", network.node_count);
  printf("Segments added: %u\n", network.segment_count);

  assert(network.node_count > 0);
  assert(network.segment_count > 0);

  // Test pathfinding
  uint32_t path[100];
  uint32_t count = 0;
  success =
      road_find_path(&network, 0, network.node_count - 1, path, &count, 100);
  if (success) {
    printf("Path found! Length: %u\n", count);
    for (uint32_t i = 0; i < count; i++) {
      printf("%u ", path[i]);
    }
    printf("\n");
  } else {
    printf("No path found between 0 and %u\n", network.node_count - 1);
  }
}

int main() {
  printf("Starting City Generator Tests...\n");
  test_config();
  test_road_network();
  printf("All tests passed!\n");
  return 0;
}
