/**
 * Issue #273: city_generator.h must name ProceduralBuilding before
 * block_fill_with_buildings. Including the header is the regression check:
 * an unknown type at that prototype is a compile error.
 */
#include "city_generator.h"

#include <stdio.h>

int main(void) {
  ProceduralBuilding building = {0};
  CityBlock block = {0};

  building.id = 7;
  building.type = BUILDING_HOUSE;
  block.id = 3;

  /* Complete type: a forward declaration would not allow sizeof of the object
   * layout used by the city generator. Pointer prototypes still require the
   * name to be declared first, which this translation unit just parsed. */
  if (sizeof(building) == 0 || building.id != 7 || block.id != 3) {
    fprintf(stderr, "FAIL ProceduralBuilding order\n");
    return 1;
  }

  printf("PASS ProceduralBuilding declared before use id=%u sizeof=%zu\n",
         building.id, sizeof(building));
  return 0;
}
