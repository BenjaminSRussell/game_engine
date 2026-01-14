#include "city_generator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void run_test(const char *name, bool result) {
  printf("[%s] %s\n", result ? "PASS" : "FAIL", name);
}

int main() {
  printf("Running City Generator Tests...\n");

  // 1. Test Configuration
  CityConfig config;
  bool config_ok = city_config_from_preset(&config, CITY_STYLE_MODERN, 1000.0f);
  run_test("Config Init", config_ok && config.size_x == 1000.0f);

  // 2. Test City Generator Init
  CityGenerator generator;
  bool gen_init = city_generator_init(&generator, &config);
  run_test("Generator Init", gen_init);

  // 3. Test Road Generation (Grid)
  bool road_gen = road_network_generate_grid(&generator.roads, &config);
  run_test("Road Grid Gen", road_gen && generator.roads.node_count > 0);

  // 4. Test Block Subdivision
  CityBlock parent_block = {0};
  parent_block.id = 1;
  parent_block.vertex_count = 4;
  parent_block.vertices[0][0] = 0;
  parent_block.vertices[0][1] = 0;
  parent_block.vertices[1][0] = 100;
  parent_block.vertices[1][1] = 0;
  parent_block.vertices[2][0] = 100;
  parent_block.vertices[2][1] = 100;
  parent_block.vertices[3][0] = 0;
  parent_block.vertices[3][1] = 100;
  parent_block.center[0] = 50;
  parent_block.center[1] = 50;
  parent_block.area = 10000;
  parent_block.zone = ZONE_RESIDENTIAL;

  CityBlock children[4];
  uint32_t child_count = 0;
  bool subdiv =
      block_subdivide(&parent_block, children, &child_count, 4, 1000.0f);
  run_test("Block Subdivide", subdiv && child_count == 2);

  if (child_count > 0) {
    printf("  Child 1 Area: %.2f\n", children[0].area);
    printf("  Child 2 Area: %.2f\n", children[1].area);
  }

  // 5. Test Building Generation
  ProceduralBuilding buildings[10];
  uint32_t building_count = 0;

  // Fill first child block or parent if subdiv failed
  CityBlock *target_block = (child_count > 0) ? &children[0] : &parent_block;
  bool building_fill = block_fill_with_buildings(target_block, buildings,
                                                 &building_count, 10, &config);
  run_test("Building Fill", building_fill && building_count > 0);

  if (building_count > 0) {
    printf("  Generated %u buildings\n", building_count);
    printf("  Building 0 Type: %d, Floors: %u, Height: %.2f\n",
           buildings[0].type, buildings[0].floors,
           buildings[0].footprint.max_height);
  }

  // 6. Test NPC Assignment
  if (building_count > 0) {
    uint32_t npc_ids[32];
    uint32_t npc_count = 0;
    bool npc_assign = building_assign_npcs(&buildings[0], npc_ids, &npc_count);
    run_test("NPC Assignment", npc_assign && npc_count > 0);
    printf("  Assigned %u NPCs to building 0\n", npc_count);
  }

  // 7. Cleanup
  city_generator_shutdown(&generator);
  run_test("Shutdown", true);

  return 0;
}
