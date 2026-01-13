/**
 * =================================================================================================
 *                          MEDIEVAL VILLAGE BUILDING KIT
 *                          Agent: AGENT_TEMPLATE_2
 * =================================================================================================
 */

#ifndef MEDIEVAL_VILLAGE_H
#define MEDIEVAL_VILLAGE_H

#include "math/math.h"
#include "../../include/core/entity.h"
#include "include/rendering/mesh.h"
#include "include/physics/physics.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =================================================================================================
 *                                  ENUMS AND CONSTANTS
 * =================================================================================================
 */

typedef enum MedievalBuildingType {
    MEDIEVAL_HOUSE_SMALL,
    MEDIEVAL_HOUSE_MEDIUM,
    MEDIEVAL_HOUSE_LARGE,
    MEDIEVAL_SHOP_BLACKSMITH,
    MEDIEVAL_SHOP_BAKER,
    MEDIEVAL_SHOP_TAVERN,
    MEDIEVAL_SHOP_GENERAL,
    MEDIEVAL_CHAPEL_SMALL,
    MEDIEVAL_CHAPEL_LARGE,
    MEDIEVAL_CASTLE_TOWER,
    MEDIEVAL_CASTLE_WALL,
    MEDIEVAL_CASTLE_GATE,
    MEDIEVAL_WINDMILL,
    MEDIEVAL_WELL,
    MEDIEVAL_STABLE,
    MEDIEVAL_FARMHOUSE,
    MEDIEVAL_BARN,
    MEDIEVAL_WATCHTOWER,
    MEDIEVAL_BRIDGE,
    MEDIEVAL_COUNT
} MedievalBuildingType;

typedef enum MedievalMaterial {
    MEDIEVAL_MATERIAL_STONE,
    MEDIEVAL_MATERIAL_WOOD_OAK,
    MEDIEVAL_MATERIAL_WOOD_PINE,
    MEDIEVAL_MATERIAL_PLASTER,
    MEDIEVAL_MATERIAL_THATCH,
    MEDIEVAL_MATERIAL_SLATE,
    MEDIEVAL_MATERIAL_CLAY,
    MEDIEVAL_MATERIAL_WROUGHT_IRON,
    MEDIEVAL_MATERIAL_COUNT
} MedievalMaterial;

typedef enum MedievalDecorationType {
    MEDIEVAL_DECORATION_BARREL,
    MEDIEVAL_DECORATION_CRATE,
    MEDIEVAL_DECORATION_SIGN,
    MEDIEVAL_DECORATION_LANTERN,
    MEDIEVAL_DECORATION_FLOWER_POT,
    MEDIEVAL_DECORATION_WEAPON_RACK,
    MEDIEVAL_DECORATION_ARMOR_STAND,
    MEDIEVAL_DECORATION_TAVERN_TABLE,
    MEDIEVAL_DECORATION_ANVIL,
    MEDIEVAL_DECORATION_COUNT
} MedievalDecorationType;

/* =================================================================================================
 *                                  STRUCTURES
 * =================================================================================================
 */

typedef struct MedievalBuildingTemplate {
    MedievalBuildingType type;
    char name[64];
    vec3 dimensions;
    vec3 door_position;
    vec3 window_positions[8];
    int window_count;
    MedievalMaterial primary_material;
    MedievalMaterial secondary_material;
    MedievalMaterial roof_material;
    bool has_chimney;
    vec3 chimney_position;
    bool has_balcony;
    vec3 balcony_position;
    int stories;
    float foundation_height;
    bool has_basement;
    bool has_garden;
    vec3 garden_size;
} MedievalBuildingTemplate;

typedef struct MedievalVillageLayout {
    vec3 center_position;
    float village_radius;
    int building_count;
    MedievalBuildingTemplate* buildings;
    vec3* building_positions;
    float* building_rotations;
    vec3 road_network[64];
    int road_count;
    vec3 well_position;
    vec3 chapel_position;
    bool has_castle;
    vec3 castle_position;
    float castle_radius;
    bool has_wall;
    vec3 wall_points[32];
    int wall_point_count;
} MedievalVillageLayout;

typedef struct MedievalMaterialProperties {
    MedievalMaterial type;
    char texture_name[64];
    char normal_map_name[64];
    vec3 base_color;
    float roughness;
    float metallic;
    float emissive;
    float durability;
    float cost_modifier;
    bool is_flammable;
    float thermal_conductivity;
} MedievalMaterialProperties;

/* =================================================================================================
 *                                  MAIN API FUNCTIONS
 * =================================================================================================
 */

/**
 * Create a medieval village layout with procedural generation
 * @param center Village center position
 * @param radius Village radius
 * @param building_density Number of buildings to generate
 * @return Generated village layout
 */
MedievalVillageLayout* medieval_create_village(vec3 center, float radius, int building_density);

/**
 * Destroy a medieval village layout and free memory
 * @param layout Village layout to destroy
 */
void medieval_destroy_village(MedievalVillageLayout* layout);

/**
 * Spawn a single medieval building in the world
 * @param template Building template to use
 * @param position World position
 * @param rotation Y-axis rotation in radians
 * @return Created building entity
 */
Entity* medieval_spawn_building(const MedievalBuildingTemplate* template, vec3 position, float rotation);

/**
 * Spawn an entire medieval village in the world
 * @param layout Village layout to spawn
 * @return Root entity containing all village components
 */
Entity* medieval_spawn_village(const MedievalVillageLayout* layout);

/**
 * Add decorative elements to a building
 * @param building Building entity to decorate
 * @param type Decoration type
 * @param position Local position relative to building
 */
void medieval_add_decoration(Entity* building, MedievalDecorationType type, vec3 position);

/**
 * Get material properties for a medieval material type
 * @param material Material type
 * @return Material properties or NULL if invalid
 */
const MedievalMaterialProperties* medieval_get_material_properties(MedievalMaterial material);

/**
 * Apply procedural variation to a building template
 * @param template Building template to modify
 * @param variation Variation index (0-3)
 */
void medieval_set_building_variation(MedievalBuildingTemplate* template, int variation);

/**
 * Calculate a quality score for a village layout
 * @param layout Village layout to evaluate
 * @return Quality score (higher is better)
 */
float medieval_calculate_village_score(const MedievalVillageLayout* layout);

/**
 * Optimize a village layout for better spacing and road access
 * @param layout Village layout to optimize
 */
void medieval_optimize_layout(MedievalVillageLayout* layout);

/* =================================================================================================
 *                                  UTILITY FUNCTIONS
 * =================================================================================================
 */

/**
 * Get a random medieval building type with weighted distribution
 * @return Random building type
 */
MedievalBuildingType medieval_get_random_building_type(void);

/**
 * Get a random medieval material type
 * @return Random material type
 */
MedievalMaterial medieval_get_random_material(void);

/**
 * Check if a building type is suitable for a given village size
 * @param type Building type
 * @param village_size Village radius
 * @return True if suitable
 */
bool medieval_is_building_suitable(MedievalBuildingType type, float village_size);

/**
 * Calculate the cost of building a structure
 * @param template Building template
 * @return Construction cost
 */
float medieval_calculate_building_cost(const MedievalBuildingTemplate* template);

/**
 * Get the defensive value of a building type
 * @param type Building type
 * @return Defensive value (0.0-1.0)
 */
float medieval_get_building_defense_value(MedievalBuildingType type);

#ifdef __cplusplus
}
#endif

#endif // MEDIEVAL_VILLAGE_H
