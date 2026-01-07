/**
 * =================================================================================================
 *                              PROCEDURAL CITY GENERATOR
 *                                  Agent: AGENT_WORLD_2
 * =================================================================================================
 *
 * Complete procedural city/settlement generation with roads, buildings, and
 * props.
 *
 * =================================================================================================
 */

#ifndef PROCEDURAL_CITY_H
#define PROCEDURAL_CITY_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    CITY CONFIGURATION
 * =================================================================================================
 */

typedef enum CityStyle {
  CITY_STYLE_MEDIEVAL,
  CITY_STYLE_FANTASY,
  CITY_STYLE_MODERN,
  CITY_STYLE_FUTURISTIC,
  CITY_STYLE_POST_APOCALYPTIC,
  CITY_STYLE_STEAMPUNK,
  CITY_STYLE_ASIAN,
  CITY_STYLE_MIDDLE_EASTERN,
} CityStyle;

typedef enum ZoneType {
  ZONE_RESIDENTIAL,
  ZONE_COMMERCIAL,
  ZONE_INDUSTRIAL,
  ZONE_MILITARY,
  ZONE_RELIGIOUS,
  ZONE_GOVERNMENT,
  ZONE_ENTERTAINMENT,
  ZONE_PARK,
  ZONE_SLUM,
  ZONE_NOBLE,
} ZoneType;

typedef struct CityConfig {
  CityStyle style;
  float size_x;
  float size_z;
  uint32_t population_target;
  float density;
  uint32_t random_seed;

  // Layout
  bool has_walls;
  bool has_castle;
  bool has_harbor;
  bool has_river;
  float river_width;

  // Road network
  float main_road_width;
  float side_road_width;
  float alley_width;
  bool grid_based;
  float road_randomness;

  // Building heights
  uint32_t min_floors;
  uint32_t max_floors;
  float floor_height;

  // Zone distribution
  float zone_percentages[10];
} CityConfig;

// TODO(AGENT_WORLD_2): Implement city_config_validate [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement city_config_from_preset [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement city_config_serialize [Difficulty: 4]

/* =================================================================================================
 *                                    ROAD NETWORK
 * =================================================================================================
 */

typedef enum RoadType {
  ROAD_MAIN,
  ROAD_SECONDARY,
  ROAD_RESIDENTIAL,
  ROAD_ALLEY,
  ROAD_PATH,
  ROAD_BRIDGE,
} RoadType;

typedef struct RoadNode {
  uint32_t id;
  float position[3];
  uint32_t connections[6];
  uint32_t connection_count;
  RoadType type;
  bool is_intersection;
} RoadNode;

typedef struct RoadSegment {
  uint32_t id;
  uint32_t start_node;
  uint32_t end_node;
  RoadType type;
  float width;
  float length;
  bool is_curved;
  float curve_points[8][3];
  uint32_t curve_point_count;
} RoadSegment;

typedef struct RoadNetwork {
  RoadNode *nodes;
  uint32_t node_count;
  uint32_t node_capacity;
  RoadSegment *segments;
  uint32_t segment_count;
  uint32_t segment_capacity;
} RoadNetwork;

// TODO(AGENT_WORLD_2): Implement road_network_init [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement road_network_generate_grid [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement road_network_generate_organic [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement road_network_generate_radial [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement road_add_main_roads [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement road_add_secondary_roads [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement road_add_alleys [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement road_add_bridge [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement road_generate_mesh [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement road_generate_navmesh [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement road_find_path [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement intersection_generate [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement road_decorate [Difficulty: 5]

/* =================================================================================================
 *                                    CITY BLOCKS
 * =================================================================================================
 */

typedef struct CityBlock {
  uint32_t id;
  float vertices[16][2];
  uint32_t vertex_count;
  float center[2];
  float area;
  ZoneType zone;
  uint32_t building_ids[32];
  uint32_t building_count;
  uint32_t prop_ids[64];
  uint32_t prop_count;
} CityBlock;

// TODO(AGENT_WORLD_2): Implement block_from_road_polygon [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement block_subdivide [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement block_assign_zone [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement block_fill_with_buildings [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement block_add_courtyard [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement block_add_plaza [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement block_optimize_layout [Difficulty: 6]

/* =================================================================================================
 *                                    BUILDING GENERATION
 * =================================================================================================
 */

typedef enum BuildingType {
  BUILDING_HOUSE,
  BUILDING_SHOP,
  BUILDING_TAVERN,
  BUILDING_TEMPLE,
  BUILDING_WAREHOUSE,
  BUILDING_FACTORY,
  BUILDING_OFFICE,
  BUILDING_APARTMENT,
  BUILDING_MANSION,
  BUILDING_CASTLE,
  BUILDING_TOWER,
} BuildingType;

typedef struct BuildingFootprint {
  float vertices[8][2];
  uint32_t vertex_count;
  float min_height;
  float max_height;
  bool allow_irregular;
} BuildingFootprint;

typedef struct ProceduralBuilding {
  uint32_t id;
  BuildingType type;
  float position[3];
  float rotation;
  uint32_t floors;
  float floor_height;
  BuildingFootprint footprint;

  // Facade
  uint32_t wall_material;
  uint32_t roof_material;
  uint32_t door_style;
  uint32_t window_style;
  uint32_t balcony_style;

  // Generated
  uint32_t mesh_id;
  uint32_t interior_id;
  bool has_interior;
} ProceduralBuilding;

// TODO(AGENT_WORLD_2): Implement building_generate_footprint [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement building_extrude_floors [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement building_generate_roof [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement building_place_windows [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement building_place_doors [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement building_place_balconies [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement building_add_details [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement building_generate_interior [Difficulty: 8]
// TODO(AGENT_WORLD_2): Implement building_generate_furniture [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement building_generate_lod [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement building_generate_collision [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement building_bake_ao [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement building_assign_npcs [Difficulty: 5]

/* =================================================================================================
 *                                    SPECIAL STRUCTURES
 * =================================================================================================
 */

// TODO(AGENT_WORLD_2): Implement generate_city_walls [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement generate_city_gates [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement generate_castle [Difficulty: 8]
// TODO(AGENT_WORLD_2): Implement generate_temple [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement generate_market_plaza [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement generate_harbor [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement generate_bridge [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement generate_fountain [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement generate_monument [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement generate_graveyard [Difficulty: 5]

/* =================================================================================================
 *                                    CITY GENERATOR API
 * =================================================================================================
 */

typedef struct CityGenerator {
  CityConfig config;
  RoadNetwork roads;
  CityBlock *blocks;
  uint32_t block_count;
  ProceduralBuilding *buildings;
  uint32_t building_count;

  float generation_progress;
  bool is_generating;
  uint32_t generation_stage;
} CityGenerator;

// TODO(AGENT_WORLD_2): Implement city_generator_init [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement city_generator_shutdown [Difficulty: 3]
// TODO(AGENT_WORLD_2): Implement city_generator_generate [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement city_generator_generate_async [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement city_generator_get_progress [Difficulty: 2]
// TODO(AGENT_WORLD_2): Implement city_generator_cancel [Difficulty: 3]
// TODO(AGENT_WORLD_2): Implement city_generator_preview [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement city_generator_regenerate_block [Difficulty:
// 6]
// TODO(AGENT_WORLD_2): Implement city_generator_save [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement city_generator_load [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement city_generator_export_mesh [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement city_populate_npcs [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement city_add_ambient_life [Difficulty: 5]

#endif // PROCEDURAL_CITY_H
