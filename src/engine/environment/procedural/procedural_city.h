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
#include <stddef.h>
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

// ✅ COMPLETED: city_config_validate - Validates city configuration parameters
bool city_config_validate(const CityConfig *config);

// ✅ COMPLETED: city_config_from_preset - Creates configuration from preset
// styles
bool city_config_from_preset(CityConfig *config, CityStyle style, float size);

// ✅ COMPLETED: city_config_serialize - Serializes configuration to/from JSON
bool city_config_serialize(const CityConfig *config, char *buffer,
                           size_t buffer_size);
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

// ✅ COMPLETED: road_network_init - Initialize road network system
bool road_network_init(RoadNetwork *network, uint32_t initial_capacity);

// ✅ COMPLETED: road_network_generate_grid - Generate grid-based road layout
bool road_network_generate_grid(RoadNetwork *network, const CityConfig *config);

// ✅ COMPLETED: road_network_generate_organic - Generate organic road layout
bool road_network_generate_organic(RoadNetwork *network,
                                   const CityConfig *config);

// ✅ COMPLETED: road_network_generate_radial - Generate radial road layout
bool road_network_generate_radial(RoadNetwork *network,
                                  const CityConfig *config);

// ✅ COMPLETED: road_add_main_roads - Add main arterial roads
bool road_add_main_roads(RoadNetwork *network, const CityConfig *config);

// ✅ COMPLETED: road_add_secondary_roads - Add secondary roads
bool road_add_secondary_roads(RoadNetwork *network, const CityConfig *config);

// ✅ COMPLETED: road_add_alleys - Add alley paths between buildings
bool road_add_alleys(RoadNetwork *network, const CityConfig *config);

// ✅ COMPLETED: road_add_bridge - Add bridge segments for rivers/obstacles
bool road_add_bridge(RoadNetwork *network, uint32_t start_id, uint32_t end_id);

// ✅ COMPLETED: road_generate_mesh - Generate renderable mesh from road network
uint32_t road_generate_mesh(const RoadNetwork *network);

// ✅ COMPLETED: road_generate_navmesh - Generate navigation mesh for roads
uint32_t road_generate_navmesh(const RoadNetwork *network);

// ✅ COMPLETED: road_find_path - Find path between road nodes
bool road_find_path(const RoadNetwork *network, uint32_t start_id,
                    uint32_t end_id, uint32_t *path_nodes, size_t *path_length);

// ✅ COMPLETED: intersection_generate - Generate intersection geometry
uint32_t intersection_generate(const RoadNode *node,
                               const RoadSegment *connected_segments,
                               uint32_t segment_count);

// ✅ COMPLETED: road_decorate - Add road decorations (lanes, markings, etc.)
bool road_decorate(RoadNetwork *network, const CityConfig *config);

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

// ✅ COMPLETED: block_from_road_polygon - Create block from road-enclosed
// polygon
bool block_from_road_polygon(CityBlock *block, const float vertices[][2],
                             uint32_t vertex_count);

// ✅ COMPLETED: block_subdivide - Subdivide large blocks into smaller parcels
bool block_subdivide(CityBlock *block, uint32_t max_subdivisions);

// ✅ COMPLETED: block_assign_zone - Assign zone type based on location and city
// layout
ZoneType block_assign_zone(const CityBlock *block, const CityConfig *config,
                           const CityBlock *all_blocks, uint32_t block_count);

// ✅ COMPLETED: block_fill_with_buildings - Fill block with appropriate
// buildings
bool block_fill_with_buildings(CityBlock *block, const CityConfig *config);

// ✅ COMPLETED: block_add_courtyard - Add courtyard to residential blocks
bool block_add_courtyard(CityBlock *block);

// ✅ COMPLETED: block_add_plaza - Convert block to plaza for commercial zones
bool block_add_plaza(CityBlock *block, const CityConfig *config);

// ✅ COMPLETED: block_optimize_layout - Optimize building placement within
// block
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

// ✅ COMPLETED: building_generate_footprint - Generate building footprint based
// on type and zone
bool building_generate_footprint(BuildingFootprint *footprint,
                                 BuildingType type, const CityBlock *block,
                                 float max_area);

// ✅ COMPLETED: building_extrude_floors - Extrude building footprint vertically
bool building_extrude_floors(ProceduralBuilding *building);

// ✅ COMPLETED: building_generate_roof - Generate roof geometry based on
// building type
bool building_generate_roof(ProceduralBuilding *building);

// ✅ COMPLETED: building_place_windows - Place windows on building facade
bool building_place_windows(ProceduralBuilding *building);

// ✅ COMPLETED: building_place_doors - Place doors on building facade
bool building_place_doors(ProceduralBuilding *building);

// ✅ COMPLETED: building_place_balconies - Place balconies on appropriate
// buildings
bool building_place_balconies(ProceduralBuilding *building);

// ✅ COMPLETED: building_add_details - Add architectural details and
// decorations
bool building_add_details(ProceduralBuilding *building);

// ✅ COMPLETED: building_generate_interior - Generate interior layout and rooms
bool building_generate_interior(ProceduralBuilding *building);

// ✅ COMPLETED: building_generate_furniture - Place furniture inside buildings
bool building_generate_furniture(ProceduralBuilding *building);

// ✅ COMPLETED: building_generate_lod - Generate level-of-detail models
bool building_generate_lod(ProceduralBuilding *building);

// ✅ COMPLETED: building_generate_collision - Generate collision mesh for
// building
bool building_generate_collision(ProceduralBuilding *building);

// ✅ COMPLETED: building_bake_ao - Bake ambient occlusion for building
bool building_bake_ao(ProceduralBuilding *building);

// ✅ COMPLETED: building_assign_npcs - Assign NPCs to buildings based on type
bool building_assign_npcs(ProceduralBuilding *building);

/* =================================================================================================
 *                                    SPECIAL STRUCTURES
 * =================================================================================================
 */

// ✅ COMPLETED: generate_city_walls - Generate defensive city walls
bool generate_city_walls(const CityConfig *config, const RoadNetwork *roads);

// ✅ COMPLETED: generate_city_gates - Generate city gates in walls
bool generate_city_gates(const CityConfig *config, uint32_t wall_mesh_id);

// ✅ COMPLETED: generate_castle - Generate castle structure for
// medieval/fantasy cities
bool generate_castle(const CityConfig *config, float center_x, float center_z);

// ✅ COMPLETED: generate_temple - Generate temple/religious structure
bool generate_temple(const CityConfig *config, ZoneType zone_type,
                     float position[3]);

// ✅ COMPLETED: generate_market_plaza - Generate central market plaza
bool generate_market_plaza(const CityConfig *config, float center[3],
                           float radius);

// ✅ COMPLETED: generate_harbor - Generate harbor structures if city has water
// access
bool generate_harbor(const CityConfig *config, float river_position[3]);

// ✅ COMPLETED: generate_bridge - Generate bridge across river
bool generate_bridge(const CityConfig *config, float start[3], float end[3]);

// ✅ COMPLETED: generate_fountain - Generate decorative fountain
bool generate_fountain(const CityConfig *config, float position[3]);

// ✅ COMPLETED: generate_monument - Generate monument/statue
bool generate_monument(const CityConfig *config, float position[3]);

// ✅ COMPLETED: generate_graveyard - Generate graveyard near temples
bool generate_graveyard(const CityConfig *config, float temple_position[3]);

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

// ✅ COMPLETED: city_generator_init - Initialize city generator
bool city_generator_init(CityGenerator *generator, const CityConfig *config);

// ✅ COMPLETED: city_generator_shutdown - Cleanup city generator resources
void city_generator_shutdown(CityGenerator *generator);

// ✅ COMPLETED: city_generator_generate - Generate complete city (synchronous)
bool city_generator_generate(CityGenerator *generator);

// ✅ COMPLETED: city_generator_generate_async - Generate complete city
// (asynchronous)
bool city_generator_generate_async(CityGenerator *generator);

// ✅ COMPLETED: city_generator_get_progress - Get generation progress (0.0
// to 1.0)
float city_generator_get_progress(const CityGenerator *generator);

// ✅ COMPLETED: city_generator_cancel - Cancel async generation
void city_generator_cancel(CityGenerator *generator);

// ✅ COMPLETED: city_generator_preview - Generate low-detail preview
bool city_generator_preview(const CityGenerator *generator,
                            uint32_t *preview_mesh_id);

// ✅ COMPLETED: city_generator_regenerate_block - Regenerate specific city
// block
bool city_generator_regenerate_block(CityGenerator *generator,
                                     uint32_t block_id);

// ✅ COMPLETED: city_generator_save - Save generated city to file
bool city_generator_save(const CityGenerator *generator, const char *filename);

// ✅ COMPLETED: city_generator_load - Load generated city from file
bool city_generator_load(CityGenerator *generator, const char *filename);

// ✅ COMPLETED: city_generator_export_mesh - Export city as single mesh
bool city_generator_export_mesh(const CityGenerator *generator,
                                const char *filename);

// ✅ COMPLETED: city_populate_npcs - Populate city with NPCs
bool city_populate_npcs(CityGenerator *generator);

// ✅ COMPLETED: city_add_ambient_life - Add ambient life (animals, sounds,
// etc.)
bool city_add_ambient_life(CityGenerator *generator);

#endif // PROCEDURAL_CITY_H
