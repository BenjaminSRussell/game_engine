#include "city_generator.h"
#include <math.h>
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
  // Simplified manual parsing for demo purposes
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
 *                                    BUILDING NPC ASSIGNMENT
 * =================================================================================================
 */

bool building_assign_npcs(ProceduralBuilding *building, uint32_t *npc_ids, uint32_t *npc_count) {
  if (!building || !npc_ids || !npc_count)
    return false;
  
  uint32_t seed = building->id * 1337;
  uint32_t count = 0;
  
  // Assign NPCs based on building type
  switch (building->type) {
    case BUILDING_HOUSE:
      count = (uint32_t)random_range(&seed, 1, 4);
      break;
    case BUILDING_SHOP:
      count = (uint32_t)random_range(&seed, 2, 6);
      break;
    case BUILDING_TAVERN:
      count = (uint32_t)random_range(&seed, 3, 8);
      break;
    case BUILDING_TEMPLE:
      count = (uint32_t)random_range(&seed, 2, 5);
      break;
    case BUILDING_WAREHOUSE:
      count = (uint32_t)random_range(&seed, 1, 3);
      break;
    case BUILDING_FACTORY:
      count = (uint32_t)random_range(&seed, 5, 15);
      break;
    case BUILDING_OFFICE:
      count = (uint32_t)random_range(&seed, 3, 12);
      break;
    case BUILDING_APARTMENT:
      count = (uint32_t)random_range(&seed, 8, 20);
      break;
    case BUILDING_MANSION:
      count = (uint32_t)random_range(&seed, 4, 10);
      break;
    case BUILDING_CASTLE:
      count = (uint32_t)random_range(&seed, 10, 30);
      break;
    case BUILDING_TOWER:
      count = (uint32_t)random_range(&seed, 1, 3);
      break;
  }
  
  *npc_count = count;
  for (uint32_t i = 0; i < count; i++) {
    npc_ids[i] = building->id * 1000 + i;
  }
  
  return true;
}

/* =================================================================================================
 *                                    CITY BLOCKS
 * =================================================================================================
 */

// Helper function to calculate polygon area
static float calculate_polygon_area(const float *vertices, uint32_t vertex_count) {
  float area = 0.0f;
  for (uint32_t i = 0; i < vertex_count; i++) {
    uint32_t j = (i + 1) % vertex_count;
    area += vertices[i * 2] * vertices[j * 2 + 1];
    area -= vertices[j * 2] * vertices[i * 2 + 1];
  }
  return fabsf(area) * 0.5f;
}

// Helper function to calculate polygon centroid
static void calculate_polygon_center(const float *vertices, uint32_t vertex_count, float *center) {
  center[0] = 0.0f;
  center[1] = 0.0f;
  float area = calculate_polygon_area(vertices, vertex_count);
  
  for (uint32_t i = 0; i < vertex_count; i++) {
    uint32_t j = (i + 1) % vertex_count;
    float factor = (vertices[i * 2] * vertices[j * 2 + 1] - vertices[j * 2] * vertices[i * 2 + 1]);
    center[0] += (vertices[i * 2] + vertices[j * 2]) * factor;
    center[1] += (vertices[i * 2 + 1] + vertices[j * 2 + 1]) * factor;
  }
  
  float six_area = area * 6.0f;
  if (six_area != 0.0f) {
    center[0] /= six_area;
    center[1] /= six_area;
  }
}

bool block_from_road_polygon(CityBlock *block, const float *road_vertices, 
                           uint32_t vertex_count) {
  if (!block || !road_vertices || vertex_count < 3 || vertex_count > 16) {
    return false;
  }
  
  block->id = 0; // Will be assigned by caller
  block->vertex_count = vertex_count;
  memcpy(block->vertices, road_vertices, vertex_count * 2 * sizeof(float));
  
  calculate_polygon_center(road_vertices, vertex_count, block->center);
  block->area = calculate_polygon_area(road_vertices, vertex_count);
  
  block->zone = ZONE_RESIDENTIAL; // Default zone
  block->building_count = 0;
  block->prop_count = 0;
  
  return true;
}

bool block_subdivide(CityBlock *parent, CityBlock *children, uint32_t *child_count,
                     uint32_t max_children, float min_area) {
  if (!parent || !children || !child_count || parent->area < min_area * 2.0f) {
    return false;
  }
  
  *child_count = 0;
  
  // Simple subdivision: split along longest axis through center
  float longest_axis = 0.0f;
  int split_axis = 0; // 0 = X, 1 = Z
  
  for (uint32_t i = 0; i < parent->vertex_count; i++) {
    uint32_t j = (i + 1) % parent->vertex_count;
    float dx = parent->vertices[j][0] - parent->vertices[i][0];
    float dz = parent->vertices[j][1] - parent->vertices[i][1];
    float length = sqrtf(dx * dx + dz * dz);
    
    if (length > longest_axis) {
      longest_axis = length;
      split_axis = (fabsf(dx) > fabsf(dz)) ? 0 : 1;
    }
  }
  
  // Create two child blocks by splitting along center
  if (*child_count + 2 <= max_children) {
    float split_pos = split_axis == 0 ? parent->center[0] : parent->center[1];
    
    // Child 1 - vertices on one side of split
    children[*child_count].id = parent->id * 100 + *child_count;
    children[*child_count].vertex_count = 0;
    
    // Child 2 - vertices on other side of split
    children[*child_count + 1].id = parent->id * 100 + *child_count + 1;
    children[*child_count + 1].vertex_count = 0;
    
    // Distribute vertices based on split position
    for (uint32_t i = 0; i < parent->vertex_count; i++) {
      float vertex_pos = split_axis == 0 ? parent->vertices[i][0] : parent->vertices[i][1];
      
      if (vertex_pos <= split_pos) {
        if (children[*child_count].vertex_count < 16) {
          children[*child_count].vertices[children[*child_count].vertex_count][0] = parent->vertices[i][0];
          children[*child_count].vertices[children[*child_count].vertex_count][1] = parent->vertices[i][1];
          children[*child_count].vertex_count++;
        }
      } else {
        if (children[*child_count + 1].vertex_count < 16) {
          children[*child_count + 1].vertices[children[*child_count + 1].vertex_count][0] = parent->vertices[i][0];
          children[*child_count + 1].vertices[children[*child_count + 1].vertex_count][1] = parent->vertices[i][1];
          children[*child_count + 1].vertex_count++;
        }
      }
    }
    
    // Calculate centers and areas for children
    calculate_polygon_center(children[*child_count].vertices[0], 
                          children[*child_count].vertex_count, 
                          children[*child_count].center);
    children[*child_count].area = calculate_polygon_area(children[*child_count].vertices[0], 
                                                     children[*child_count].vertex_count);
    
    calculate_polygon_center(children[*child_count + 1].vertices[0], 
                          children[*child_count + 1].vertex_count, 
                          children[*child_count + 1].center);
    children[*child_count + 1].area = calculate_polygon_area(children[*child_count + 1].vertices[0], 
                                                          children[*child_count + 1].vertex_count);
    
    children[*child_count].zone = parent->zone;
    children[*child_count + 1].zone = parent->zone;
    
    children[*child_count].building_count = 0;
    children[*child_count + 1].building_count = 0;
    children[*child_count].prop_count = 0;
    children[*child_count + 1].prop_count = 0;
    
    *child_count += 2;
  }
  
  return *child_count > 0;
}

bool block_assign_zone(CityBlock *block, const CityConfig *config, 
                       const CityBlock *all_blocks, uint32_t total_blocks) {
  if (!block || !config) {
    return false;
  }
  
  // Zone assignment based on distance from center and neighboring zones
  float distance_from_center = sqrtf(
    block->center[0] * block->center[0] + 
    block->center[1] * block->center[1]
  );
  
  float max_distance = sqrtf(
    config->size_x * config->size_x + 
    config->size_z * config->size_z
  ) * 0.5f;
  
  float normalized_distance = distance_from_center / max_distance;
  
  // Assign zones based on distance and style
  if (normalized_distance < 0.2f) {
    block->zone = (config->style == CITY_STYLE_MEDIEVAL) ? ZONE_GOVERNMENT : ZONE_COMMERCIAL;
  } else if (normalized_distance < 0.5f) {
    block->zone = ZONE_COMMERCIAL;
  } else if (normalized_distance < 0.8f) {
    block->zone = ZONE_RESIDENTIAL;
  } else {
    block->zone = (config->style == CITY_STYLE_POST_APOCALYPTIC) ? ZONE_SLUM : ZONE_INDUSTRIAL;
  }
  
  // Check neighboring zones for consistency
  if (all_blocks && total_blocks > 0) {
    uint32_t zone_counts[10] = {0};
    
    for (uint32_t i = 0; i < total_blocks; i++) {
      if (all_blocks[i].id != block->id) {
        float dx = all_blocks[i].center[0] - block->center[0];
        float dz = all_blocks[i].center[1] - block->center[1];
        float distance = sqrtf(dx * dx + dz * dz);
        
        if (distance < 100.0f) { // Within influence radius
          zone_counts[all_blocks[i].zone]++;
        }
      }
    }
    
    // Find most common neighboring zone
    uint32_t max_count = 0;
    ZoneType most_common = block->zone;
    
    for (int i = 0; i < 10; i++) {
      if (zone_counts[i] > max_count) {
        max_count = zone_counts[i];
        most_common = (ZoneType)i;
      }
    }
    
    // 30% chance to adopt most common neighboring zone
    if (max_count > 2 && (rand() % 100) < 30) {
      block->zone = most_common;
    }
  }
  
  return true;
}

bool block_fill_with_buildings(CityBlock *block, ProceduralBuilding *buildings,
                                uint32_t *building_count, uint32_t max_buildings,
                                const CityConfig *config) {
  if (!block || !buildings || !building_count || !config) {
    return false;
  }
  
  uint32_t buildings_added = 0;
  float building_footprint_area = block->area * 0.6f; // 60% coverage
  
  // Calculate number of buildings based on zone and density
  uint32_t target_buildings = (uint32_t)(building_footprint_area / 100.0f); // 100m² per building
  target_buildings = (uint32_t)(target_buildings * config->density);
  
  if (target_buildings > max_buildings) {
    target_buildings = max_buildings;
  }
  
  if (target_buildings == 0) target_buildings = 1;
  
  for (uint32_t i = 0; i < target_buildings && buildings_added < max_buildings; i++) {
    ProceduralBuilding *building = &buildings[*building_count + buildings_added];
    
    building->id = block->id * 1000 + buildings_added;
    
    // Position building within block
    float offset_range = sqrtf(block->area) * 0.3f;
    building->position[0] = block->center[0] + (rand() % 100 - 50) * offset_range * 0.01f;
    building->position[1] = 0.0f; // Ground level
    building->position[2] = block->center[1] + (rand() % 100 - 50) * offset_range * 0.01f;
    building->rotation = (float)(rand() % 360) * M_PI / 180.0f;
    
    // Set building type based on zone
    switch (block->zone) {
      case ZONE_RESIDENTIAL:
        building->type = (rand() % 100) < 70 ? BUILDING_HOUSE : BUILDING_APARTMENT;
        break;
      case ZONE_COMMERCIAL:
        building->type = (rand() % 100) < 50 ? BUILDING_SHOP : BUILDING_OFFICE;
        break;
      case ZONE_INDUSTRIAL:
        building->type = (rand() % 100) < 60 ? BUILDING_WAREHOUSE : BUILDING_FACTORY;
        break;
      case ZONE_RELIGIOUS:
        building->type = BUILDING_TEMPLE;
        break;
      case ZONE_GOVERNMENT:
        building->type = BUILDING_OFFICE;
        break;
      case ZONE_ENTERTAINMENT:
        building->type = BUILDING_TAVERN;
        break;
      case ZONE_NOBLE:
        building->type = BUILDING_MANSION;
        break;
      default:
        building->type = BUILDING_HOUSE;
        break;
    }
    
    building->floors = config->min_floors + (rand() % (config->max_floors - config->min_floors + 1));
    building->floor_height = config->floor_height;
    
    // Generate footprint
    building->footprint.vertex_count = 4;
    float half_size = sqrtf(building_footprint_area / target_buildings) * 0.5f;
    
    for (int j = 0; j < 4; j++) {
      float angle = j * M_PI * 0.5f;
      building->footprint.vertices[j][0] = cosf(angle) * half_size;
      building->footprint.vertices[j][1] = sinf(angle) * half_size;
    }
    
    building->footprint.min_height = 0.0f;
    building->footprint.max_height = building->floors * building->floor_height;
    building->footprint.allow_irregular = false;
    
    // Set materials based on style
    building->wall_material = (uint32_t)config->style;
    building->roof_material = (uint32_t)config->style + 10;
    building->door_style = (uint32_t)building->type;
    building->window_style = (uint32_t)building->type + 10;
    building->balcony_style = (uint32_t)building->type + 20;
    
    building->mesh_id = 0; // Will be generated later
    building->interior_id = 0; // Will be generated later
    building->has_interior = false;
    
    buildings_added++;
  }
  
  *building_count += buildings_added;
  block->building_count = buildings_added;
  
  return buildings_added > 0;
}

bool block_add_courtyard(CityBlock *block, float courtyard_ratio) {
  if (!block || courtyard_ratio <= 0.0f || courtyard_ratio >= 1.0f) {
    return false;
  }
  
  // Add courtyard as a prop (simplified implementation)
  if (block->prop_count < 64) {
    block->prop_ids[block->prop_count++] = 1000 + block->id; // Courtyard prop ID
  }
  
  return true;
}

bool block_add_plaza(CityBlock *block, float plaza_size) {
  if (!block || plaza_size <= 0.0f || plaza_size > block->area) {
    return false;
  }
  
  // Add plaza as a prop (simplified implementation)
  if (block->prop_count < 64) {
    block->prop_ids[block->prop_count++] = 2000 + block->id; // Plaza prop ID
  }
  
  return true;
}

bool block_optimize_layout(CityBlock *block) {
  if (!block) {
    return false;
  }
  
  // Optimize building placement (simplified implementation)
  // In a full implementation, this would:
  // - Reorder buildings for better space utilization
  // - Adjust building footprints to fit better
  // - Ensure proper spacing between buildings
  
  return true;
}

/* =================================================================================================
 *                                    BUILDING GENERATION
 * =================================================================================================
 */

bool building_generate_footprint(ProceduralBuilding *building, const CityBlock *block,
                                  const CityConfig *config) {
  if (!building || !block || !config) {
    return false;
  }
  
  // Generate footprint based on building type and available space
  float base_size = 20.0f; // Base building size
  
  switch (building->type) {
    case BUILDING_HOUSE:
      building->footprint.vertex_count = 4;
      for (int i = 0; i < 4; i++) {
        float angle = i * M_PI * 0.5f;
        building->footprint.vertices[i][0] = cosf(angle) * base_size * 0.5f;
        building->footprint.vertices[i][1] = sinf(angle) * base_size * 0.5f;
      }
      break;
      
    case BUILDING_SHOP:
      building->footprint.vertex_count = 6; // Hexagonal
      for (int i = 0; i < 6; i++) {
        float angle = i * M_PI / 3.0f;
        building->footprint.vertices[i][0] = cosf(angle) * base_size * 0.4f;
        building->footprint.vertices[i][1] = sinf(angle) * base_size * 0.4f;
      }
      break;
      
    case BUILDING_TEMPLE:
      building->footprint.vertex_count = 8; // Octagonal
      for (int i = 0; i < 8; i++) {
        float angle = i * M_PI * 0.25f;
        building->footprint.vertices[i][0] = cosf(angle) * base_size * 0.8f;
        building->footprint.vertices[i][1] = sinf(angle) * base_size * 0.8f;
      }
      break;
      
    default:
      building->footprint.vertex_count = 4; // Default rectangular
      for (int i = 0; i < 4; i++) {
        float angle = i * M_PI * 0.5f;
        building->footprint.vertices[i][0] = cosf(angle) * base_size * 0.6f;
        building->footprint.vertices[i][1] = sinf(angle) * base_size * 0.6f;
      }
      break;
  }
  
  building->footprint.min_height = 0.0f;
  building->footprint.max_height = building->floors * building->floor_height;
  building->footprint.allow_irregular = (building->type == BUILDING_TEMPLE);
  
  return true;
}

bool building_extrude_floors(ProceduralBuilding *building) {
  if (!building || building->floors == 0) {
    return false;
  }
  
  // Calculate total building height
  building->footprint.max_height = building->floors * building->floor_height;
  
  return true;
}

bool building_generate_roof(ProceduralBuilding *building, const CityConfig *config) {
  if (!building || !config) {
    return false;
  }
  
  // Roof generation would create mesh geometry
  // For now, just set roof material based on style and building type
  switch (config->style) {
    case CITY_STYLE_MEDIEVAL:
      building->roof_material = 100; // Thatched roof
      break;
    case CITY_STYLE_MODERN:
      building->roof_material = 101; // Flat roof
      break;
    case CITY_STYLE_FUTURISTIC:
      building->roof_material = 102; // Solar panel roof
      break;
    default:
      building->roof_material = 103; // Generic roof
      break;
  }
  
  return true;
}

bool building_place_windows(ProceduralBuilding *building, const CityConfig *config) {
  if (!building || !config) {
    return false;
  }
  
  // Window placement would create window geometry on walls
  // For now, just set window style based on building type
  building->window_style = (uint32_t)building->type * 10;
  
  return true;
}

bool building_place_doors(ProceduralBuilding *building, const CityConfig *config) {
  if (!building || !config) {
    return false;
  }
  
  // Door placement would create door geometry
  // For now, just set door style based on building type
  building->door_style = (uint32_t)building->type * 5;
  
  return true;
}

bool building_place_balconies(ProceduralBuilding *building, const CityConfig *config) {
  if (!building || !config) {
    return false;
  }
  
  // Only place balconies on certain building types
  if (building->type == BUILDING_APARTMENT || building->type == BUILDING_MANSION) {
    building->balcony_style = (uint32_t)building->type * 15;
  }
  
  return true;
}

bool building_add_details(ProceduralBuilding *building, const CityConfig *config) {
  if (!building || !config) {
    return false;
  }
  
  // Add architectural details based on style
  // This would include cornices, trim, decorations, etc.
  
  return true;
}

bool building_generate_interior(ProceduralBuilding *building, const CityConfig *config) {
  if (!building || !config) {
    return false;
  }
  
  // Interior generation is complex - create room layouts
  building->has_interior = true;
  building->interior_id = building->id * 10000; // Interior mesh ID
  
  return true;
}

bool building_generate_furniture(ProceduralBuilding *building, const CityConfig *config) {
  if (!building || !config || !building->has_interior) {
    return false;
  }
  
  // Furniture placement based on building type and rooms
  // This would place furniture objects in the interior
  
  return true;
}

bool building_generate_lod(ProceduralBuilding *building, uint32_t lod_level) {
  if (!building) {
    return false;
  }
  
  // Generate different levels of detail
  // LOD 0: Highest detail
  // LOD 1: Medium detail  
  // LOD 2: Low detail
  // LOD 3: Billboard/placeholder
  
  return true;
}

bool building_generate_collision(ProceduralBuilding *building) {
  if (!building) {
    return false;
  }
  
  // Generate collision mesh for physics
  // This would create simplified geometry for collision detection
  
  return true;
}

bool building_bake_ao(ProceduralBuilding *building) {
  if (!building) {
    return false;
  }
  
  // Bake ambient occlusion into textures
  // This would pre-calculate shadows and lighting
  
  return true;
}

/* =================================================================================================
 *                                    SPECIAL STRUCTURES
 * =================================================================================================
 */

uint32_t generate_city_walls(const CityConfig *config, float center_x, float center_z) {
  if (!config || !config->has_walls)
    return 0;
  
  uint32_t seed = config->random_seed + 1001;
  float radius = fmaxf(config->size_x, config->size_z) * 0.5f;
  float wall_height = 8.0f + random_range(&seed, -2.0f, 4.0f);
  float wall_thickness = 2.0f;
  
  // Generate wall segments in a rough circle/rectangle
  uint32_t segments = 32;
  uint32_t mesh_id = 20000; // Placeholder mesh ID
  
  for (uint32_t i = 0; i < segments; i++) {
    float angle = (float)i * 2.0f * M_PI / (float)segments;
    float x = center_x + cosf(angle) * radius;
    float z = center_z + sinf(angle) * radius;
    
    // Add wall tower at regular intervals
    if (i % 4 == 0) {
      float tower_height = wall_height + random_range(&seed, 2.0f, 6.0f);
      // Generate tower mesh (placeholder)
    }
    
    // Generate wall segment (placeholder)
  }
  
  return mesh_id;
}

uint32_t generate_city_gates(uint32_t wall_id, const float *gate_positions, uint32_t gate_count) {
  if (!gate_positions || gate_count == 0)
    return 0;
  
  uint32_t gate_mesh_id = 21000; // Placeholder mesh ID
  
  for (uint32_t i = 0; i < gate_count; i++) {
    float x = gate_positions[i * 2];
    float z = gate_positions[i * 2 + 1];
    
    // Generate gate structure with archway and doors
    // Placeholder implementation
  }
  
  return gate_mesh_id;
}

uint32_t generate_castle(const CityConfig *config, float position_x, float position_z) {
  if (!config || !config->has_castle)
    return 0;
  
  uint32_t seed = config->random_seed + 2001;
  uint32_t castle_mesh_id = 30000; // Placeholder mesh ID
  
  // Generate main keep
  float keep_size = 20.0f + random_range(&seed, -5.0f, 10.0f);
  float keep_height = 30.0f + random_range(&seed, -5.0f, 15.0f);
  
  // Generate corner towers
  for (int i = 0; i < 4; i++) {
    float angle = (float)i * M_PI * 0.5f;
    float tower_x = position_x + cosf(angle) * keep_size;
    float tower_z = position_z + sinf(angle) * keep_size;
    float tower_height = keep_height + random_range(&seed, 5.0f, 10.0f);
    // Generate tower mesh (placeholder)
  }
  
  // Generate curtain walls
  // Generate gatehouse
  // Generate bailey buildings
  
  return castle_mesh_id;
}

uint32_t generate_temple(const CityConfig *config, float position_x, float position_z) {
  uint32_t seed = config->random_seed + 3001;
  uint32_t temple_mesh_id = 40000; // Placeholder mesh ID
  
  // Generate temple based on city style
  switch (config->style) {
    case CITY_STYLE_MEDIEVAL:
    case CITY_STYLE_FANTASY:
      // Gothic cathedral style
      break;
    case CITY_STYLE_ASIAN:
      // Pagoda style
      break;
    case CITY_STYLE_MIDDLE_EASTERN:
      // Mosque style
      break;
    default:
      // Classical temple style
      break;
  }
  
  // Generate main hall
  float hall_width = 15.0f + random_range(&seed, -3.0f, 6.0f);
  float hall_length = 25.0f + random_range(&seed, -5.0f, 10.0f);
  float hall_height = 20.0f + random_range(&seed, -3.0f, 8.0f);
  
  // Generate towers/spires
  // Generate entrance
  // Generate interior columns
  
  return temple_mesh_id;
}

uint32_t generate_market_plaza(const CityConfig *config, float center_x, float center_z, float radius) {
  uint32_t seed = config->random_seed + 4001;
  uint32_t plaza_mesh_id = 50000; // Placeholder mesh ID
  
  // Generate paved plaza area
  // Generate market stalls
  uint32_t stall_count = (uint32_t)(radius * 0.5f);
  for (uint32_t i = 0; i < stall_count; i++) {
    float angle = random_float(&seed) * 2.0f * M_PI;
    float distance = random_range(&seed, radius * 0.3f, radius * 0.8f);
    float stall_x = center_x + cosf(angle) * distance;
    float stall_z = center_z + sinf(angle) * distance;
    // Generate stall (placeholder)
  }
  
  // Generate central fountain
  generate_fountain(center_x, center_z, radius * 0.2f);
  
  return plaza_mesh_id;
}

uint32_t generate_harbor(const CityConfig *config, float shore_x, float shore_z) {
  if (!config || !config->has_harbor)
    return 0;
  
  uint32_t seed = config->random_seed + 5001;
  uint32_t harbor_mesh_id = 60000; // Placeholder mesh ID
  
  // Generate docks
  uint32_t dock_count = 3 + (uint32_t)random_range(&seed, 0, 4);
  for (uint32_t i = 0; i < dock_count; i++) {
    float dock_length = 20.0f + random_range(&seed, -5.0f, 15.0f);
    float dock_width = 5.0f + random_range(&seed, -1.0f, 3.0f);
    float dock_x = shore_x + (float)i * 15.0f;
    // Generate dock section (placeholder)
  }
  
  // Generate warehouse buildings
  // Generate harbor office
  // Generate ship berths
  
  return harbor_mesh_id;
}

uint32_t generate_bridge(float start_x, float start_z, float end_x, float end_z, float width) {
  uint32_t bridge_mesh_id = 70000; // Placeholder mesh ID
  
  float length = sqrtf((end_x - start_x) * (end_x - start_x) + 
                      (end_z - start_z) * (end_z - start_z));
  
  // Generate bridge supports
  uint32_t support_count = (uint32_t)(length / 20.0f) + 1;
  for (uint32_t i = 0; i <= support_count; i++) {
    float t = (float)i / (float)support_count;
    float support_x = start_x + t * (end_x - start_x);
    float support_z = start_z + t * (end_z - start_z);
    // Generate support pillar (placeholder)
  }
  
  // Generate bridge deck
  // Generate railings
  
  return bridge_mesh_id;
}

uint32_t generate_fountain(float center_x, float center_z, float radius) {
  uint32_t fountain_mesh_id = 80000; // Placeholder mesh ID
  
  // Generate basin
  float basin_depth = radius * 0.3f;
  
  // Generate central feature
  float central_height = radius * 1.5f;
  
  // Generate water jets
  uint32_t jet_count = 4 + (uint32_t)(radius * 2);
  for (uint32_t i = 0; i < jet_count; i++) {
    float angle = (float)i * 2.0f * M_PI / (float)jet_count;
    // Generate water jet (placeholder)
  }
  
  return fountain_mesh_id;
}

uint32_t generate_monument(float center_x, float center_z, float height) {
  uint32_t monument_mesh_id = 90000; // Placeholder mesh ID
  
  // Generate monument base
  float base_size = height * 0.3f;
  
  // Generate monument shaft
  float shaft_width = height * 0.1f;
  
  // Generate monument top/statue
  float top_size = height * 0.15f;
  
  return monument_mesh_id;
}

uint32_t generate_graveyard(float center_x, float center_z, float width, float depth) {
  uint32_t graveyard_mesh_id = 100000; // Placeholder mesh ID
  
  uint32_t seed = (uint32_t)(center_x + center_z) * 1337;
  
  // Generate perimeter wall/fence
  // Generate paths
  // Generate tombstones
  uint32_t tombstone_count = (uint32_t)(width * depth * 0.1f);
  for (uint32_t i = 0; i < tombstone_count; i++) {
    float x = center_x + random_range(&seed, -width * 0.4f, width * 0.4f);
    float z = center_z + random_range(&seed, -depth * 0.4f, depth * 0.4f);
    float tombstone_height = random_range(&seed, 0.8f, 2.5f);
    // Generate tombstone (placeholder)
  }
  
  // Generate mausoleum (if large enough)
  if (width > 20.0f && depth > 20.0f) {
    // Generate mausoleum building
  }
  
  return graveyard_mesh_id;
}

/* =================================================================================================
 *                                    CITY GENERATOR API
 * =================================================================================================
 */

bool city_generator_init(CityGenerator *generator, const CityConfig *config) {
  if (!generator || !config)
    return false;
  
  // Copy configuration
  memcpy(&generator->config, config, sizeof(CityConfig));
  
  // Initialize road network
  if (!road_network_init(&generator->roads, 1024)) {
    return false;
  }
  
  // Initialize arrays
  generator->blocks = NULL;
  generator->block_count = 0;
  generator->buildings = NULL;
  generator->building_count = 0;
  
  // Initialize generation state
  generator->generation_progress = 0.0f;
  generator->is_generating = false;
  generator->generation_stage = 0;
  
  return true;
}

void city_generator_shutdown(CityGenerator *generator) {
  if (!generator)
    return;
  
  // Cleanup road network
  if (generator->roads.nodes) {
    free(generator->roads.nodes);
    generator->roads.nodes = NULL;
  }
  if (generator->roads.segments) {
    free(generator->roads.segments);
    generator->roads.segments = NULL;
  }
  
  // Cleanup blocks
  if (generator->blocks) {
    free(generator->blocks);
    generator->blocks = NULL;
  }
  
  // Cleanup buildings
  if (generator->buildings) {
    free(generator->buildings);
    generator->buildings = NULL;
  }
  
  // Reset state
  generator->block_count = 0;
  generator->building_count = 0;
  generator->generation_progress = 0.0f;
  generator->is_generating = false;
  generator->generation_stage = 0;
}

bool city_generator_generate(CityGenerator *generator) {
  if (!generator)
    return false;
  
  generator->is_generating = true;
  generator->generation_progress = 0.0f;
  generator->generation_stage = 1;
  
  // Stage 1: Generate road network
  if (generator->config.grid_based) {
    road_network_generate_grid(&generator->roads, &generator->config);
  } else {
    road_network_generate_organic(&generator->roads, &generator->config);
  }
  generator->generation_progress = 0.3f;
  generator->generation_stage = 2;
  
  // Stage 2: Generate city blocks
  // Placeholder: Create basic blocks from road network
  generator->generation_progress = 0.5f;
  generator->generation_stage = 3;
  
  // Stage 3: Generate buildings
  // Placeholder: Create buildings in blocks
  generator->generation_progress = 0.8f;
  generator->generation_stage = 4;
  
  // Stage 4: Generate special structures
  if (generator->config.has_walls) {
    generate_city_walls(&generator->config, 0.0f, 0.0f);
  }
  if (generator->config.has_castle) {
    generate_castle(&generator->config, 0.0f, 0.0f);
  }
  if (generator->config.has_harbor) {
    generate_harbor(&generator->config, 0.0f, 0.0f);
  }
  
  generator->generation_progress = 1.0f;
  generator->generation_stage = 5;
  generator->is_generating = false;
  
  return true;
}

bool city_generator_generate_async(CityGenerator *generator) {
  // Placeholder: In a real implementation, this would spawn a thread
  return city_generator_generate(generator);
}

float city_generator_get_progress(const CityGenerator *generator) {
  return generator ? generator->generation_progress : 0.0f;
}

void city_generator_cancel(CityGenerator *generator) {
  if (generator) {
    generator->is_generating = false;
    generator->generation_progress = 0.0f;
    generator->generation_stage = 0;
  }
}

uint32_t city_generator_preview(const CityGenerator *generator, uint32_t detail_level) {
  if (!generator)
    return 0;
  
  // Generate low-detail preview mesh based on detail level
  uint32_t preview_mesh_id = 110000 + detail_level;
  
  // Simple preview: basic road network layout
  // Higher detail levels include buildings, structures, etc.
  
  return preview_mesh_id;
}

bool city_generator_regenerate_block(CityGenerator *generator, uint32_t block_id) {
  if (!generator || block_id >= generator->block_count)
    return false;
  
  // Regenerate specific block with new buildings
  CityBlock *block = &generator->blocks[block_id];
  
  // Clear existing buildings in block
  // Generate new buildings based on zone type
  
  return true;
}

bool city_generator_save(const CityGenerator *generator, const char *filename) {
  if (!generator || !filename)
    return false;
  
  FILE *file = fopen(filename, "wb");
  if (!file)
    return false;
  
  // Save configuration
  fwrite(&generator->config, sizeof(CityConfig), 1, file);
  
  // Save road network
  fwrite(&generator->roads.node_count, sizeof(uint32_t), 1, file);
  fwrite(&generator->roads.segment_count, sizeof(uint32_t), 1, file);
  fwrite(generator->roads.nodes, sizeof(RoadNode), generator->roads.node_count, file);
  fwrite(generator->roads.segments, sizeof(RoadSegment), generator->roads.segment_count, file);
  
  // Save blocks
  fwrite(&generator->block_count, sizeof(uint32_t), 1, file);
  fwrite(generator->blocks, sizeof(CityBlock), generator->block_count, file);
  
  // Save buildings
  fwrite(&generator->building_count, sizeof(uint32_t), 1, file);
  fwrite(generator->buildings, sizeof(ProceduralBuilding), generator->building_count, file);
  
  fclose(file);
  return true;
}

bool city_generator_load(CityGenerator *generator, const char *filename) {
  if (!generator || !filename)
    return false;
  
  FILE *file = fopen(filename, "rb");
  if (!file)
    return false;
  
  // Load configuration
  fread(&generator->config, sizeof(CityConfig), 1, file);
  
  // Load road network
  fread(&generator->roads.node_count, sizeof(uint32_t), 1, file);
  fread(&generator->roads.segment_count, sizeof(uint32_t), 1, file);
  
  generator->roads.nodes = (RoadNode*)malloc(generator->roads.node_count * sizeof(RoadNode));
  generator->roads.segments = (RoadSegment*)malloc(generator->roads.segment_count * sizeof(RoadSegment));
  
  fread(generator->roads.nodes, sizeof(RoadNode), generator->roads.node_count, file);
  fread(generator->roads.segments, sizeof(RoadSegment), generator->roads.segment_count, file);
  
  // Load blocks
  fread(&generator->block_count, sizeof(uint32_t), 1, file);
  generator->blocks = (CityBlock*)malloc(generator->block_count * sizeof(CityBlock));
  fread(generator->blocks, sizeof(CityBlock), generator->block_count, file);
  
  // Load buildings
  fread(&generator->building_count, sizeof(uint32_t), 1, file);
  generator->buildings = (ProceduralBuilding*)malloc(generator->building_count * sizeof(ProceduralBuilding));
  fread(generator->buildings, sizeof(ProceduralBuilding), generator->building_count, file);
  
  fclose(file);
  return true;
}

bool city_generator_export_mesh(const CityGenerator *generator, const char *filename) {
  if (!generator || !filename)
    return false;
  
  // Export all generated geometry to mesh file
  // This would combine road meshes, building meshes, and special structures
  
  FILE *file = fopen(filename, "wb");
  if (!file)
    return false;
  
  // Write mesh header
  uint32_t mesh_count = generator->roads.segment_count + generator->building_count;
  fwrite(&mesh_count, sizeof(uint32_t), 1, file);
  
  // Export road meshes
  for (uint32_t i = 0; i < generator->roads.segment_count; i++) {
    // Export road segment mesh
  }
  
  // Export building meshes
  for (uint32_t i = 0; i < generator->building_count; i++) {
    // Export building mesh
  }
  
  fclose(file);
  return true;
}

bool city_populate_npcs(CityGenerator *generator, uint32_t target_population) {
  if (!generator)
    return false;
  
  uint32_t current_population = 0;
  uint32_t seed = generator->config.random_seed + 9999;
  
  // Assign NPCs to buildings based on type and capacity
  for (uint32_t i = 0; i < generator->building_count && current_population < target_population; i++) {
    ProceduralBuilding *building = &generator->buildings[i];
    uint32_t npc_ids[32];
    uint32_t npc_count = 0;
    
    if (building_assign_npcs(building, npc_ids, &npc_count)) {
      current_population += npc_count;
    }
  }
  
  // Add wandering NPCs for streets and plazas
  uint32_t wandering_count = target_population / 10;
  for (uint32_t i = 0; i < wandering_count; i++) {
    float x = random_range(&seed, -generator->config.size_x * 0.4f, generator->config.size_x * 0.4f);
    float z = random_range(&seed, -generator->config.size_z * 0.4f, generator->config.size_z * 0.4f);
    // Create wandering NPC at position
  }
  
  return true;
}

bool city_add_ambient_life(CityGenerator *generator) {
  if (!generator)
    return false;
  
  uint32_t seed = generator->config.random_seed + 8888;
  
  // Add ambient animals based on city style
  switch (generator->config.style) {
    case CITY_STYLE_MEDIEVAL:
    case CITY_STYLE_FANTASY:
      // Add cats, dogs, birds, horses
      break;
    case CITY_STYLE_MODERN:
    case CITY_STYLE_FUTURISTIC:
      // Add pigeons, rats, pets
      break;
    case CITY_STYLE_POST_APOCALYPTIC:
      // Add mutated creatures, scavengers
      break;
    default:
      // Add generic ambient life
      break;
  }
  
  // Add ambient sounds
  // Add environmental effects (smoke, lights, etc.)
  
  return true;
}
