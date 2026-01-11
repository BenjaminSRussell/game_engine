#include "procedural_city.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_building_generation() {
  printf("Testing building generation...\n");

  ProceduralBuilding building;
  memset(&building, 0, sizeof(ProceduralBuilding));
  building.id = 1;
  building.floors = 3;
  building.floor_height = 4.0f;

  // 1. Test footprint generation
  BuildingFootprint footprint;
  CityBlock block; // Dummy block
  bool footprint_ok = building_generate_footprint(
      &footprint, BUILDING_APARTMENT, &block, 100.0f);
  assert(footprint_ok);
  assert(footprint.vertex_count == 4);

  building.footprint = footprint;
  printf("  Footprint generated: %d vertices\n", footprint.vertex_count);

  // 2. Test extrusion
  bool extrusion_ok = building_extrude_floors(&building);
  assert(extrusion_ok);
  printf("  Extrusion completed: %d floors\n", building.floors);

  // 3. Test window placement
  bool windows_ok = building_place_windows(&building);
  assert(windows_ok);
  printf("  Window placement logic executed.\n");

  // 4. Test door placement
  bool doors_ok = building_place_doors(&building);
  assert(doors_ok);
  printf("  Door placement logic executed.\n");

  printf("Building generation tests passed!\n");
}

int main() {
  test_building_generation();
  return 0;
}
