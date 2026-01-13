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

//  COMPLETED: city_config_validate - Validates city configuration parameters
bool city_config_validate(const CityConfig *config);

//  COMPLETED: city_config_from_preset - Creates configuration from preset
//  styles
bool city_config_from_preset(CityConfig *config, CityStyle style, float size);

//  COMPLETED: city_config_serialize - Serializes configuration to/from JSON
bool city_config_serialize(const CityConfig *config, char *buffer,
                           uint32_t buffer_size);
bool city_config_deserialize(CityConfig *config, const char *json_buffer);

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

//  COMPLETED: road_network_init - Initialize road network system
bool road_network_init(RoadNetwork *network, uint32_t initial_capacity);

//  COMPLETED: road_network_generate_grid - Generate grid-based road layout
bool road_network_generate_grid(RoadNetwork *network, const CityConfig *config);

//  COMPLETED: road_network_generate_organic - Generate organic road layout
bool road_network_generate_organic(RoadNetwork *network,
                                   const CityConfig *config);

//  COMPLETED: road_network_generate_radial - Generate radial road layout
bool road_network_generate_radial(RoadNetwork *network,
                                  const CityConfig *config);

//  COMPLETED: road_add_main_roads - Add main arterial roads
void road_add_main_roads(RoadNetwork *network, const CityConfig *config);

//  COMPLETED: road_add_secondary_roads - Add secondary roads
void road_add_secondary_roads(RoadNetwork *network, const CityConfig *config);

//  COMPLETED: road_add_alleys - Add alley paths between buildings
void road_add_alleys(RoadNetwork *network, const CityConfig *config);

//  COMPLETED: road_add_bridge - Add bridge segments for rivers/obstacles
void road_add_bridge(RoadNetwork *network, uint32_t start_node_id,
                     uint32_t end_node_id);

//  COMPLETED: road_generate_mesh - Generate renderable mesh from road network
uint32_t road_generate_mesh(const RoadNetwork *network);

//  COMPLETED: road_generate_navmesh - Generate navigation mesh for roads
uint32_t road_generate_navmesh(const RoadNetwork *network);

//  COMPLETED: road_find_path - Find path between road nodes
bool road_find_path(const RoadNetwork *network, uint32_t start_node_id,
                    uint32_t end_node_id, uint32_t *out_path,
                    uint32_t *out_count, uint32_t max_count);

//  COMPLETED: intersection_generate - Generate intersection geometry
void intersection_generate(RoadNetwork *network, uint32_t node_id);

//  COMPLETED: road_decorate - Add road decorations (lanes, markings, etc.)
void road_decorate(RoadNetwork *network, const CityConfig *config);

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

// City block management functions
bool block_from_road_polygon(CityBlock *block, const float *road_vertices, 
                           uint32_t vertex_count);
bool block_subdivide(CityBlock *parent, CityBlock *children, uint32_t *child_count,
                     uint32_t max_children, float min_area);
bool block_assign_zone(CityBlock *block, const CityConfig *config, 
                       const CityBlock *all_blocks, uint32_t total_blocks);
bool block_fill_with_buildings(CityBlock *block, ProceduralBuilding *buildings,
                                uint32_t *building_count, uint32_t max_buildings,
                                const CityConfig *config);
bool block_add_courtyard(CityBlock *block, float courtyard_ratio);
bool block_add_plaza(CityBlock *block, float plaza_size);
bool block_optimize_layout(CityBlock *block);

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

// Building generation functions
bool building_generate_footprint(ProceduralBuilding *building, const CityBlock *block,
                                  const CityConfig *config);
bool building_extrude_floors(ProceduralBuilding *building);
bool building_generate_roof(ProceduralBuilding *building, const CityConfig *config);
bool building_place_windows(ProceduralBuilding *building, const CityConfig *config);
bool building_place_doors(ProceduralBuilding *building, const CityConfig *config);
bool building_place_balconies(ProceduralBuilding *building, const CityConfig *config);
bool building_add_details(ProceduralBuilding *building, const CityConfig *config);

/* =================================================================================================
 *                                    SPECIAL STRUCTURES
 * =================================================================================================
 */

// COMPLETED: building_assign_npcs - Assign NPCs to buildings based on type
bool building_assign_npcs(ProceduralBuilding *building, uint32_t *npc_ids, uint32_t *npc_count);

// COMPLETED: generate_city_walls - Generate defensive walls around city
uint32_t generate_city_walls(const CityConfig *config, float center_x, float center_z);

// COMPLETED: generate_city_gates - Generate gates in city walls
uint32_t generate_city_gates(uint32_t wall_id, const float *gate_positions, uint32_t gate_count);

// COMPLETED: generate_castle - Generate castle structure
uint32_t generate_castle(const CityConfig *config, float position_x, float position_z);

// COMPLETED: generate_temple - Generate temple structure
uint32_t generate_temple(const CityConfig *config, float position_x, float position_z);

// COMPLETED: generate_market_plaza - Generate market plaza
uint32_t generate_market_plaza(const CityConfig *config, float center_x, float center_z, float radius);

// COMPLETED: generate_harbor - Generate harbor with docks
uint32_t generate_harbor(const CityConfig *config, float shore_x, float shore_z);

// COMPLETED: generate_bridge - Generate bridge structure
uint32_t generate_bridge(float start_x, float start_z, float end_x, float end_z, float width);

// COMPLETED: generate_fountain - Generate fountain structure
uint32_t generate_fountain(float center_x, float center_z, float radius);

// COMPLETED: generate_monument - Generate monument structure
uint32_t generate_monument(float center_x, float center_z, float height);

// COMPLETED: generate_graveyard - Generate graveyard with tombstones
uint32_t generate_graveyard(float center_x, float center_z, float width, float depth);

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

// COMPLETED: city_generator_init - Initialize city generator system
bool city_generator_init(CityGenerator *generator, const CityConfig *config);

// COMPLETED: city_generator_shutdown - Shutdown city generator and cleanup resources
void city_generator_shutdown(CityGenerator *generator);

// COMPLETED: city_generator_generate - Generate complete city synchronously
bool city_generator_generate(CityGenerator *generator);

// COMPLETED: city_generator_generate_async - Generate city asynchronously
bool city_generator_generate_async(CityGenerator *generator);

// COMPLETED: city_generator_get_progress - Get generation progress (0.0 to 1.0)
float city_generator_get_progress(const CityGenerator *generator);

// COMPLETED: city_generator_cancel - Cancel async generation
void city_generator_cancel(CityGenerator *generator);

// COMPLETED: city_generator_preview - Generate low-detail preview
uint32_t city_generator_preview(const CityGenerator *generator, uint32_t detail_level);

// COMPLETED: city_generator_regenerate_block - Regenerate specific city block
bool city_generator_regenerate_block(CityGenerator *generator, uint32_t block_id);

// COMPLETED: city_generator_save - Save city data to file
bool city_generator_save(const CityGenerator *generator, const char *filename);

// COMPLETED: city_generator_load - Load city data from file
bool city_generator_load(CityGenerator *generator, const char *filename);

// COMPLETED: city_generator_export_mesh - Export city as mesh
bool city_generator_export_mesh(const CityGenerator *generator, const char *filename);

// COMPLETED: city_populate_npcs - Populate city with NPCs
bool city_populate_npcs(CityGenerator *generator, uint32_t target_population);

// COMPLETED: city_add_ambient_life - Add ambient life (animals, sounds)
bool city_add_ambient_life(CityGenerator *generator);

#endif // PROCEDURAL_CITY_H
