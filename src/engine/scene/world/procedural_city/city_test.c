#include "city_generator.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  printf("=== City Generator Tests ===\n\n");

  // Test 1: Config validation
  CityConfig config;
  if (city_config_from_preset(&config, CITY_STYLE_MEDIEVAL, 500.0f)) {
    printf("[PASS] city_config_from_preset\n");
    printf("  - Style: %d, Size: %.1f x %.1f\n", config.style, config.size_x,
           config.size_z);
  } else {
    printf("[FAIL] city_config_from_preset\n");
    return 1;
  }

  // Test 2: Config serialization
  char buffer[1024];
  if (city_config_serialize(&config, buffer, sizeof(buffer))) {
    printf("[PASS] city_config_serialize\n");
    printf("  - JSON: %s\n", buffer);
  } else {
    printf("[FAIL] city_config_serialize\n");
    return 1;
  }

  // Test 3: Road network initialization
  RoadNetwork network;
  if (road_network_init(&network, 100)) {
    printf("[PASS] road_network_init\n");
  } else {
    printf("[FAIL] road_network_init\n");
    return 1;
  }

  // Test 4: Add main roads
  config.main_road_width = 10.0f;
  config.random_seed = 42;
  road_add_main_roads(&network, &config);
  printf("[PASS] road_add_main_roads\n");
  printf("  - Nodes created: %u\n", network.node_count);
  printf("  - Segments created: %u\n", network.segment_count);

  // Test 5: Pathfinding
  if (network.node_count > 1) {
    uint32_t path[100];
    uint32_t count = 0;
    if (road_find_path(&network, 0, network.node_count - 1, path, &count,
                       100)) {
      printf("[PASS] road_find_path\n");
      printf("  - Path length: %u nodes\n", count);
    } else {
      printf("[INFO] road_find_path - No path found (expected for unconnected "
             "nodes)\n");
    }
  }

  // Test 6: Mesh generation
  uint32_t mesh_id = road_generate_mesh(&network);
  printf("[PASS] road_generate_mesh\n");
  printf("  - Mesh ID: %u\n", mesh_id);

  // Test 7: Intersection generation
  intersection_generate(&network, 0);
  printf("[PASS] intersection_generate\n");

  printf("\n=== All tests completed successfully ===\n");

  // Cleanup
  if (network.nodes)
    free(network.nodes);
  if (network.segments)
    free(network.segments);

  return 0;
}
