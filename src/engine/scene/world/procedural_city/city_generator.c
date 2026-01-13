#include "city_generator.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal helper for random floats
static float random_float(uint32_t *seed) {
  *seed = *seed * 1103515245 + 12345;
  return (float)((*seed / 65536) % 32768) / 32767.0f;
}

static float random_range(uint32_t *seed, float min, float max) {
  return min + random_float(seed) * (max - min);
}

/* =================================================================================================
 *                                    CITY CONFIGURATION
 * =================================================================================================
 */

bool city_config_validate(const CityConfig *config) {
  if (!config)
    return false;
  if (config->size_x <= 0 || config->size_z <= 0)
    return false;
  if (config->density < 0 || config->density > 1.0f)
    return false;
  return true;
}

bool city_config_from_preset(CityConfig *config, CityStyle style, float size) {
  if (!config)
    return false;
  memset(config, 0, sizeof(CityConfig));
  config->style = style;
  config->size_x = size;
  config->size_z = size;
  config->population_target = (uint32_t)(size * size * 0.01f);
  config->density = 0.5f;
  config->random_seed = 12345;

  config->min_floors = 1;
  config->max_floors = 5;
  config->floor_height = 3.5f;

  return true;
}

bool city_config_serialize(const CityConfig *config, char *buffer,
                           uint32_t buffer_size) {
  if (!config || !buffer)
    return false;

  int written = snprintf(
      buffer, buffer_size,
      "{\"style\":%d,\"size_x\":%.2f,\"size_z\":%.2f,\"population_target\":%u,"
      "\"density\":%.2f,\"random_seed\":%u,\"has_walls\":%s,\"has_castle\":%s,"
      "\"has_harbor\":%s,\"has_river\":%s,\"river_width\":%.2f}",
      config->style, config->size_x, config->size_z, config->population_target,
      config->density, config->random_seed,
      config->has_walls ? "true" : "false",
      config->has_castle ? "true" : "false",
      config->has_harbor ? "true" : "false",
      config->has_river ? "true" : "false", config->river_width);

  return (written > 0 && (uint32_t)written < buffer_size);
}

bool city_config_deserialize(CityConfig *config, const char *json_buffer) {
  if (!config || !json_buffer)
    return false;
  return false;
}

/* =================================================================================================
 *                                    ROAD NETWORK
 * =================================================================================================
 */

bool road_network_init(RoadNetwork *network, uint32_t initial_capacity) {
  if (!network)
    return false;
  network->nodes = (RoadNode *)calloc(initial_capacity, sizeof(RoadNode));
  network->segments =
      (RoadSegment *)calloc(initial_capacity, sizeof(RoadSegment));
  network->node_count = 0;
  network->node_capacity = initial_capacity;
  network->segment_count = 0;
  network->segment_capacity = initial_capacity;
  return network->nodes != NULL && network->segments != NULL;
}

bool road_network_generate_grid(RoadNetwork *network,
                                const CityConfig *config) {
  if (!network || !config)
    return false;
  road_add_main_roads(network, config);
  return true;
}

bool road_network_generate_organic(RoadNetwork *network,
                                   const CityConfig *config) {
  return false;
}

bool road_network_generate_radial(RoadNetwork *network,
                                  const CityConfig *config) {
  return false;
}

static uint32_t add_node(RoadNetwork *network, float x, float y, float z,
                         RoadType type) {
  if (network->node_count >= network->node_capacity)
    return 0xFFFFFFFF;
  uint32_t id = network->node_count++;
  RoadNode *node = &network->nodes[id];
  node->id = id;
  node->position[0] = x;
  node->position[1] = y;
  node->position[2] = z;
  node->connection_count = 0;
  node->type = type;
  node->is_intersection = false;
  return id;
}

static void add_segment(RoadNetwork *network, uint32_t start, uint32_t end,
                        RoadType type, float width) {
  if (network->segment_count >= network->segment_capacity)
    return;
  uint32_t id = network->segment_count++;
  RoadSegment *seg = &network->segments[id];
  seg->id = id;
  seg->start_node = start;
  seg->end_node = end;
  seg->type = type;
  seg->width = width;

  // Update node connections
  if (network->nodes[start].connection_count < 6) {
    network->nodes[start]
        .connections[network->nodes[start].connection_count++] = end;
  }
  if (network->nodes[end].connection_count < 6) {
    network->nodes[end].connections[network->nodes[end].connection_count++] =
        start;
  }

  if (network->nodes[start].connection_count > 2)
    network->nodes[start].is_intersection = true;
  if (network->nodes[end].connection_count > 2)
    network->nodes[end].is_intersection = true;
}

void road_add_main_roads(RoadNetwork *network, const CityConfig *config) {
  if (!network || !config)
    return;
  uint32_t seed = config->random_seed;

  // Basic grid of main roads
  float step_x = config->size_x / 4.0f;
  float step_z = config->size_z / 4.0f;

  uint32_t grid[5][5];
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      float ox = random_range(&seed, -step_x * 0.1f, step_x * 0.1f);
      float oz = random_range(&seed, -step_z * 0.1f, step_z * 0.1f);
      grid[i][j] =
          add_node(network, i * step_x + ox, 0, j * step_z + oz, ROAD_MAIN);
    }
  }

  // Connect grid
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      if (i < 4)
        add_segment(network, grid[i][j], grid[i + 1][j], ROAD_MAIN,
                    config->main_road_width);
      if (j < 4)
        add_segment(network, grid[i][j], grid[i][j + 1], ROAD_MAIN,
                    config->main_road_width);
    }
  }
}

void road_add_secondary_roads(RoadNetwork *network, const CityConfig *config) {
  // Placeholder
}

void road_add_alleys(RoadNetwork *network, const CityConfig *config) {
  // Placeholder
}

void road_add_bridge(RoadNetwork *network, uint32_t start_node_id,
                     uint32_t end_node_id) {
  add_segment(network, start_node_id, end_node_id, ROAD_BRIDGE, 5.0f);
}

uint32_t road_generate_mesh(const RoadNetwork *network) {
  return network ? network->segment_count * 10 : 0;
}

uint32_t road_generate_navmesh(const RoadNetwork *network) {
  return network ? network->node_count * 5 : 0;
}

bool road_find_path(const RoadNetwork *network, uint32_t start_node_id,
                    uint32_t end_node_id, uint32_t *out_path,
                    uint32_t *out_count, uint32_t max_count) {
  if (!network || start_node_id >= network->node_count ||
      end_node_id >= network->node_count)
    return false;

  uint32_t *queue = (uint32_t *)malloc(network->node_count * sizeof(uint32_t));
  uint32_t *parent = (uint32_t *)malloc(network->node_count * sizeof(uint32_t));
  bool *visited = (bool *)calloc(network->node_count, sizeof(bool));

  uint32_t head = 0, tail = 0;
  queue[tail++] = start_node_id;
  visited[start_node_id] = true;
  parent[start_node_id] = 0xFFFFFFFF;

  bool found = false;
  while (head < tail) {
    uint32_t curr = queue[head++];
    if (curr == end_node_id) {
      found = true;
      break;
    }

    RoadNode *node = &network->nodes[curr];
    for (uint32_t i = 0; i < node->connection_count; i++) {
      uint32_t next = node->connections[i];
      if (!visited[next]) {
        visited[next] = true;
        parent[next] = curr;
        queue[tail++] = next;
      }
    }
  }

  if (found) {
    uint32_t count = 0;
    uint32_t curr = end_node_id;
    while (curr != 0xFFFFFFFF && count < max_count) {
      out_path[count++] = curr;
      curr = parent[curr];
    }
    for (uint32_t i = 0; i < count / 2; i++) {
      uint32_t tmp = out_path[i];
      out_path[i] = out_path[count - 1 - i];
      out_path[count - 1 - i] = tmp;
    }
    *out_count = count;
  }

  free(queue);
  free(parent);
  free(visited);
  return found;
}

void intersection_generate(RoadNetwork *network, uint32_t node_id) {
  if (network && node_id < network->node_count) {
    network->nodes[node_id].is_intersection = true;
  }
}

void road_decorate(RoadNetwork *network, const CityConfig *config) {
  // Placeholder
}

/* =================================================================================================
 *                                    CITY BLOCKS
 * =================================================================================================
 */

bool block_from_road_polygon(CityBlock *block, const float *road_vertices,
                             uint32_t vertex_count) {
  if (!block || !road_vertices || vertex_count < 3 || vertex_count > 16)
    return false;

  block->vertex_count = vertex_count;
  float min_x = 1e9, max_x = -1e9, min_z = 1e9, max_z = -1e9;

  for (uint32_t i = 0; i < vertex_count; i++) {
    block->vertices[i][0] = road_vertices[i * 2];
    block->vertices[i][1] = road_vertices[i * 2 + 1];

    // Bounds for center
    if (block->vertices[i][0] < min_x)
      min_x = block->vertices[i][0];
    if (block->vertices[i][0] > max_x)
      max_x = block->vertices[i][0];
    if (block->vertices[i][1] < min_z)
      min_z = block->vertices[i][1];
    if (block->vertices[i][1] > max_z)
      max_z = block->vertices[i][1];
  }

  block->center[0] = (min_x + max_x) * 0.5f;
  block->center[1] = (min_z + max_z) * 0.5f;
  block->area = (max_x - min_x) * (max_z - min_z); // Approx for rect
  block->building_count = 0;
  block->prop_count = 0;

  return true;
}

bool block_subdivide(CityBlock *parent, CityBlock *children,
                     uint32_t *child_count, uint32_t max_children,
                     float min_area) {
  if (!parent || !children || !child_count)
    return false;

  // Simple recursive splitting (simulated)
  // For now, just create 4 sub-blocks for testing
  if (parent->area < min_area * 4 || max_children < 4) {
    *child_count = 1;
    children[0] = *parent;
    return true;
  }

  // Split into quarters
  float min_x = 1e9, max_x = -1e9, min_z = 1e9, max_z = -1e9;
  for (uint32_t i = 0; i < parent->vertex_count; i++) {
    if (parent->vertices[i][0] < min_x)
      min_x = parent->vertices[i][0];
    if (parent->vertices[i][0] > max_x)
      max_x = parent->vertices[i][0];
    if (parent->vertices[i][1] < min_z)
      min_z = parent->vertices[i][1];
    if (parent->vertices[i][1] > max_z)
      max_z = parent->vertices[i][1];
  }

  float mid_x = (min_x + max_x) * 0.5f;
  float mid_z = (min_z + max_z) * 0.5f;

  *child_count = 0;

  // Create 4 children (simplified to be rects)
  float params[4][4] = {{min_x, mid_x, min_z, mid_z},
                        {mid_x, max_x, min_z, mid_z},
                        {min_x, mid_x, mid_z, max_z},
                        {mid_x, max_x, mid_z, max_z}};

  for (int i = 0; i < 4; i++) {
    CityBlock *child = &children[(*child_count)++];
    child->id = parent->id * 10 + i;
    child->zone = parent->zone;
    child->vertex_count = 4;
    child->vertices[0][0] = params[i][0];
    child->vertices[0][1] = params[i][2];
    child->vertices[1][0] = params[i][1];
    child->vertices[1][1] = params[i][2];
    child->vertices[2][0] = params[i][1];
    child->vertices[2][1] = params[i][3];
    child->vertices[3][0] = params[i][0];
    child->vertices[3][1] = params[i][3];

    child->center[0] = (params[i][0] + params[i][1]) * 0.5f;
    child->center[1] = (params[i][2] + params[i][3]) * 0.5f;
    child->area = (params[i][1] - params[i][0]) * (params[i][3] - params[i][2]);
    child->building_count = 0;
  }

  return true;
}

bool block_assign_zone(CityBlock *block, const CityConfig *config,
                       const CityBlock *all_blocks, uint32_t total_blocks) {
  if (!block || !config)
    return false;
  // Simple check for now
  block->zone = ZONE_RESIDENTIAL;
  return true;
}

bool block_add_courtyard(CityBlock *block, float courtyard_ratio) {
  // Placeholder
  return true;
}

bool block_add_plaza(CityBlock *block, float plaza_size) {
  // Placeholder
  return true;
}

bool block_optimize_layout(CityBlock *block) {
  // Placeholder
  return true;
}

/* =================================================================================================
 *                                    BUILDING GENERATION
 * =================================================================================================
 */

bool building_generate_footprint(ProceduralBuilding *building,
                                 const CityBlock *block,
                                 const CityConfig *config) {
  if (!building || !block || !config)
    return false;

  // Generate simple rectangular footprint inset from block
  float inset = 2.0f; // Margin

  building->footprint.vertex_count = 4;
  for (int i = 0; i < 4; i++) {
    building->footprint.vertices[i][0] = block->vertices[i][0];
    building->footprint.vertices[i][1] = block->vertices[i][1];

    // Simple inset (assumes rect order)
    if (i == 0) {
      building->footprint.vertices[i][0] += inset;
      building->footprint.vertices[i][1] += inset;
    }
    if (i == 1) {
      building->footprint.vertices[i][0] -= inset;
      building->footprint.vertices[i][1] += inset;
    }
    if (i == 2) {
      building->footprint.vertices[i][0] -= inset;
      building->footprint.vertices[i][1] -= inset;
    }
    if (i == 3) {
      building->footprint.vertices[i][0] += inset;
      building->footprint.vertices[i][1] -= inset;
    }
  }

  // Set position to center of footprint
  building->position[0] = block->center[0];
  building->position[1] = 0; // Ground level
  building->position[2] = block->center[1];
  building->rotation = 0;

  return true;
}

bool building_extrude_floors(ProceduralBuilding *building) {
  if (!building)
    return false;

  // Set random floors between 1 and 5 (or config min/max)
  uint32_t seed =
      (uint32_t)(building->position[0] * 100 + building->position[2]);
  building->floors = (uint32_t)random_range(&seed, 1, 6);
  building->floor_height = 3.5f;

  building->footprint.min_height = 0;
  building->footprint.max_height = building->floors * building->floor_height;

  return true;
}

bool building_generate_roof(ProceduralBuilding *building,
                            const CityConfig *config) {
  if (!building)
    return false;
  building->roof_material = 1; // Default
  return true;
}

bool building_place_windows(ProceduralBuilding *building,
                            const CityConfig *config) {
  if (!building)
    return false;
  building->window_style = 1; // Default
  return true;
}

bool building_place_doors(ProceduralBuilding *building,
                          const CityConfig *config) {
  if (!building)
    return false;
  building->door_style = 1; // Default
  return true;
}

bool building_place_balconies(ProceduralBuilding *building,
                              const CityConfig *config) {
  if (!building)
    return false;
  building->balcony_style = 0; // None
  return true;
}

bool building_add_details(ProceduralBuilding *building,
                          const CityConfig *config) {
  // Placeholder
  return true;
}

bool block_fill_with_buildings(CityBlock *block, ProceduralBuilding *buildings,
                               uint32_t *building_count, uint32_t max_buildings,
                               const CityConfig *config) {
  if (!block || !buildings || !building_count || !config)
    return false;

  // Basic implementation: 1 building per block
  if (*building_count >= max_buildings)
    return false;

  ProceduralBuilding *b = &buildings[(*building_count)++];
  b->id = *building_count;
  b->type = BUILDING_HOUSE;

  building_generate_footprint(b, block, config);
  building_extrude_floors(b);
  building_generate_roof(b, config);
  building_place_windows(b, config);
  building_place_doors(b, config);

  block->building_ids[0] = b->id;
  block->building_count = 1;

  return true;
}
