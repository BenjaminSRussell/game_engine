/**
 * =================================================================================================
 *                           ASSET TEMPLATES - ENVIRONMENT SYSTEM
 *                               Agent: AGENT_TEMPLATE_2
 * =================================================================================================
 *
 * Pre-built environment kits for rapid world construction.
 * Modular pieces that snap together for seamless level design.
 *
 * =================================================================================================
 */

#ifndef ASSET_TEMPLATES_ENVIRONMENT_H
#define ASSET_TEMPLATES_ENVIRONMENT_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    ENVIRONMENT KIT TYPES
 * =================================================================================================
 */

typedef enum EnvironmentKitType {
  ENV_KIT_MEDIEVAL_VILLAGE,
  ENV_KIT_MEDIEVAL_CASTLE,
  ENV_KIT_SCIFI_CORRIDOR,
  ENV_KIT_SCIFI_STATION,
  ENV_KIT_FANTASY_DUNGEON,
  ENV_KIT_FANTASY_FOREST,
  ENV_KIT_MODERN_CITY,
  ENV_KIT_MODERN_INTERIOR,
  ENV_KIT_NATURAL_CAVE,
  ENV_KIT_NATURAL_CLIFFS,
  ENV_KIT_INDUSTRIAL,
  ENV_KIT_POST_APOCALYPTIC,
} EnvironmentKitType;

/* =================================================================================================
 *                                    MODULAR PIECE
 * =================================================================================================
 */

typedef struct SocketDefinition {
  char name[32];
  float position[3];
  float rotation[4];
  char compatible_tags[8][16];
  uint32_t compatible_tag_count;
} SocketDefinition;

typedef struct ModularPiece {
  uint32_t id;
  char name[64];
  char category[32];
  uint32_t mesh_id;
  uint32_t material_id;
  float bounds_min[3];
  float bounds_max[3];
  SocketDefinition sockets[8];
  uint32_t socket_count;
  uint32_t lod_mesh_ids[4];
  bool has_collision;
  uint32_t collision_mesh_id;
  float grid_size[3];
  bool snap_to_grid;
} ModularPiece;

// TODO(AGENT_TEMPLATE_2): Implement modular piece loading [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement socket snapping system [Difficulty: 7]
// TODO(AGENT_TEMPLATE_2): Implement auto-socket detection [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement piece variation system [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement piece randomization [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement navmesh generation for pieces [Difficulty:
// 7]

/* =================================================================================================
 *                                    ENVIRONMENT KIT
 * =================================================================================================
 */

typedef struct EnvironmentKit {
  EnvironmentKitType type;
  char name[64];
  char description[256];
  ModularPiece *pieces;
  uint32_t piece_count;
  // Categories
  struct {
    char name[32];
    uint32_t start, count;
  } categories[16];
  uint32_t category_count;
  // Default materials
  uint32_t *default_materials;
  uint32_t material_count;
  // Props included
  uint32_t *prop_ids;
  uint32_t prop_count;
  // Thumbnail
  char thumbnail_path[256];
} EnvironmentKit;

// TODO(AGENT_TEMPLATE_2): Implement kit loading [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement kit browser UI [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement kit search and filter [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement kit favoriting [Difficulty: 3]
// TODO(AGENT_TEMPLATE_2): Implement kit export/import [Difficulty: 5]

/* =================================================================================================
 *                                    PROCEDURAL PLACEMENT
 * =================================================================================================
 */

typedef struct PlacementRule {
  char piece_category[32];
  float min_spacing;
  float max_spacing;
  float random_rotation;
  float random_scale_min;
  float random_scale_max;
  float slope_min;
  float slope_max;
  float altitude_min;
  float altitude_max;
  bool align_to_surface;
  bool cluster_mode;
  float cluster_radius;
  uint32_t cluster_min;
  uint32_t cluster_max;
} PlacementRule;

// TODO(AGENT_TEMPLATE_2): Implement procedural scatter [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement path-based placement [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement area fill [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement collision-aware placement [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement undo for batch placement [Difficulty: 5]

/* =================================================================================================
 *                                    BUILDING GENERATION
 * =================================================================================================
 */

typedef struct BuildingConfig {
  uint32_t floors;
  float floor_height;
  float width;
  float depth;
  EnvironmentKitType exterior_kit;
  EnvironmentKitType interior_kit;
  bool generate_interior;
  bool generate_roof;
  uint32_t door_count;
  uint32_t window_per_floor;
} BuildingConfig;

// TODO(AGENT_TEMPLATE_2): Implement procedural building exterior [Difficulty:
// 8]
// TODO(AGENT_TEMPLATE_2): Implement procedural building interior [Difficulty:
// 8]
// TODO(AGENT_TEMPLATE_2): Implement procedural roof generation [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement building variation [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement furniture population [Difficulty: 7]
// TODO(AGENT_TEMPLATE_2): Implement building navmesh [Difficulty: 6]

/* =================================================================================================
 *                                    PROP SYSTEM
 * =================================================================================================
 */

typedef struct PropDefinition {
  uint32_t id;
  char name[64];
  char category[32];
  uint32_t mesh_id;
  uint32_t material_id;
  bool is_physics_enabled;
  float mass;
  bool is_destructible;
  float health;
  bool is_interactive;
  char interact_script[128];
  char loot_table[64];
} PropDefinition;

// TODO(AGENT_TEMPLATE_2): Implement prop database [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement prop instantiation [Difficulty: 4]
// TODO(AGENT_TEMPLATE_2): Implement prop destruction system [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement prop interaction system [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement prop pooling [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement prop LOD system [Difficulty: 5]

/* =================================================================================================
 *                                    ENVIRONMENT API
 * =================================================================================================
 */

// TODO(AGENT_TEMPLATE_2): Implement environment_kit_load [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement environment_place_piece [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement environment_snap_piece [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement environment_generate_building [Difficulty:
// 7]
// TODO(AGENT_TEMPLATE_2): Implement environment_scatter_props [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement environment_save_prefab [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement environment_load_prefab [Difficulty: 5]
// TODO(AGENT_TEMPLATE_2): Implement environment_optimize_scene [Difficulty: 6]
// TODO(AGENT_TEMPLATE_2): Implement environment_bake_lighting [Difficulty: 7]

#endif // ASSET_TEMPLATES_ENVIRONMENT_H
