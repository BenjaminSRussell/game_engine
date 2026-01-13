/**
 * =================================================================================================
 *                          SCI-FI SPACE STATION MODULAR PIECES
 *                          Agent: AGENT_TEMPLATE_2
 * =================================================================================================
 */

#ifndef SCIFI_STATION_H
#define SCIFI_STATION_H

#include "engine/include/math/math.h"
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

typedef enum SciFiModuleType {
    SCIFI_MODULE_CORRIDOR_STRAIGHT,
    SCIFI_MODULE_CORRIDOR_L_SHAPE,
    SCIFI_MODULE_CORRIDOR_T_SHAPE,
    SCIFI_MODULE_CORRIDOR_CROSS,
    SCIFI_MODULE_CORRIDOR_CURVED,
    SCIFI_MODULE_HABITATION_QUARTERS,
    SCIFI_MODULE_HABITATION_MESS_HALL,
    SCIFI_MODULE_HABITATION_MEDBAY,
    SCIFI_MODULE_HABITATION_LABORATORY,
    SCIFI_MODULE_COMMAND_BRIDGE,
    SCIFI_MODULE_COMMAND_CONTROL_ROOM,
    SCIFI_MODULE_ENGINEERING_REACTOR,
    SCIFI_MODULE_ENGINEERING_ENGINE_ROOM,
    SCIFI_MODULE_ENGINEERING_LIFE_SUPPORT,
    SCIFI_MODULE_CARGO_BAY_SMALL,
    SCIFI_MODULE_CARGO_BAY_LARGE,
    SCIFI_MODULE_CARGO_STORAGE,
    SCIFI_MODULE_DOCKING_PORT_SMALL,
    SCIFI_MODULE_DOCKING_PORT_LARGE,
    SCIFI_MODULE_OBSERVATORY_DOME,
    SCIFI_MODULE_DEFENSE_TURRET,
    SCIFI_MODULE_DEFENSE_SHIELD_GENERATOR,
    SCIFI_MODULE_COMMUNICATIONS_ARRAY,
    SCIFI_MODULE_SOLAR_PANEL_ARRAY,
    SCIFI_MODULE_AIRLOCK_SMALL,
    SCIFI_MODULE_AIRLOCK_LARGE,
    SCIFI_MODULE_EVA_PREP_ROOM,
    SCIFI_MODULE_GYM_RECREATION,
    SCIFI_MODULE_HYDROPONICS_BAY,
    SCIFI_MODULE_WASTE_PROCESSING,
    SCIFI_MODULE_WATER_RECYCLING,
    SCIFI_MODULE_POWER_RELAY,
    SCIFI_MODULE_COOLING_SYSTEM,
    SCIFI_MODULE_ESCAPE_POD_BAY,
    SCIFI_MODULE_RESEARCH_STATION,
    SCIFI_MODULE_QUARTERMASTER,
    SCIFI_MODULE_BRIG,
    SCIFI_MODULE_ARMORY,
    SCIFI_MODULE_SHUTTLE_HANGAR,
    SCIFI_MODULE_COUNT
} SciFiModuleType;

typedef enum SciFiMaterial {
    SCIFI_MATERIAL_TITANIUM_ALLOY,
    SCIFI_MATERIAL_CERAMIC_COMPOSITE,
    SCIFI_MATERIAL_CARBON_FIBER,
    SCIFI_MATERIAL_TRANSPARENT_ALUMINUM,
    SCIFI_MATERIAL_PLASTEEL,
    SCIFI_MATERIAL_GRAPHENE,
    SCIFI_MATERIAL_NEUTRONIUM,
    SCIFI_MATERIAL_QUANTUM_GLASS,
    SCIFI_MATERIAL_PHOTONIC_CRYSTAL,
    SCIFI_MATERIAL_DURASTEEL,
    SCIFI_MATERIAL_COUNT
} SciFiMaterial;

typedef enum SciFiConnectionType {
    SCIFI_CONNECTION_NONE,
    SCIFI_CONNECTION_BULKHEAD,
    SCIFI_CONNECTION_AIRLOCK,
    SCIFI_CONNECTION_TUBE,
    SCIFI_CONNECTION_DIRECT,
    SCIFI_CONNECTION_POWER_COUPLING,
    SCIFI_CONNECTION_DATA_PORT,
    SCIFI_CONNECTION_COUNT
} SciFiConnectionType;

typedef enum SciFiModuleSize {
    SCIFI_SIZE_SMALL,      // 2x2x3 meters
    SCIFI_SIZE_MEDIUM,     // 4x4x3 meters
    SCIFI_SIZE_LARGE,      // 6x6x4 meters
    SCIFI_SIZE_HUGE,       // 8x8x6 meters
    SCIFI_SIZE_COUNT
} SciFiModuleSize;

/* =================================================================================================
 *                                  STRUCTURES
 * =================================================================================================
 */

typedef struct SciFiModuleTemplate {
    SciFiModuleType type;
    char name[64];
    SciFiModuleSize size;
    vec3 dimensions;
    SciFiMaterial primary_material;
    SciFiMaterial secondary_material;
    SciFiMaterial transparent_material;
    float mass;
    float power_consumption;
    float power_generation;
    float crew_capacity;
    float radiation_shielding;
    bool has_gravity_plating;
    bool has_life_support;
    bool has_atmosphere;
    bool has_windows;
    vec3 window_positions[12];
    int window_count;
    bool has_doors;
    vec3 door_positions[8];
    int door_count;
    SciFiConnectionType connections[6]; // +X, -X, +Y, -Y, +Z, -Z
    bool has_external_ports;
    vec3 external_ports[4];
    int external_port_count;
    float structural_integrity;
    bool is_pressurized;
    float operating_temperature;
    bool requires_maintenance;
    float maintenance_interval;
} SciFiModuleTemplate;

typedef struct SciFiStationLayout {
    vec3 center_position;
    float station_radius;
    int module_count;
    SciFiModuleTemplate* modules;
    vec3* module_positions;
    float* module_rotations;
    int* module_connections[6]; // Connection indices for each module
    vec3 corridor_network[128];
    int corridor_count;
    vec3 reactor_positions[8];
    int reactor_count;
    vec3 bridge_position;
    vec3 cargo_bay_positions[16];
    int cargo_bay_count;
    vec3 docking_port_positions[12];
    int docking_port_count;
    bool has_shield_generators;
    vec3 shield_generator_positions[4];
    int shield_generator_count;
    float total_power_output;
    float total_power_consumption;
    float total_crew_capacity;
    bool is_military_grade;
    bool is_research_station;
    bool is_colony_ship;
} SciFiStationLayout;

typedef struct SciFiMaterialProperties {
    SciFiMaterial type;
    char texture_name[64];
    char normal_map_name[64];
    char emissive_map_name[64];
    vec3 base_color;
    vec3 emissive_color;
    float roughness;
    float metallic;
    float emissive_strength;
    float density;
    float tensile_strength;
    float thermal_resistance;
    float radiation_absorption;
    float cost_modifier;
    bool is_transparent;
    float transparency;
    bool is_conductive;
    float conductivity;
} SciFiMaterialProperties;

typedef struct SciFiConnectionPoint {
    vec3 position;
    vec3 normal;
    vec3 up;
    SciFiConnectionType type;
    float radius;
    bool is_connected;
    int connected_module_index;
    int connection_index;
} SciFiConnectionPoint;

/* =================================================================================================
 *                                  MAIN API FUNCTIONS
 * =================================================================================================
 */

/**
 * Create a sci-fi station layout with procedural generation
 * @param center Station center position
 * @param radius Station radius
 * @param module_count Number of modules to generate
 * @param station_type Type of station (military, research, colony)
 * @return Generated station layout
 */
SciFiStationLayout* scifi_create_station(vec3 center, float radius, int module_count, int station_type);

/**
 * Destroy a sci-fi station layout and free memory
 * @param layout Station layout to destroy
 */
void scifi_destroy_station(SciFiStationLayout* layout);

/**
 * Spawn a single sci-fi module in world
 * @param template Module template to use
 * @param position World position
 * @param rotation Rotation as quaternion
 * @return Created module entity
 */
Entity* scifi_spawn_module(const SciFiModuleTemplate* template, vec3 position, vec4 rotation);

/**
 * Spawn an entire sci-fi station in world
 * @param layout Station layout to spawn
 * @return Root entity containing all station components
 */
Entity* scifi_spawn_station(const SciFiStationLayout* layout);

/**
 * Connect two modules together
 * @param module_a First module entity
 * @param module_b Second module entity
 * @param connection_type Type of connection to create
 * @param connection_point_a Connection point on first module
 * @param connection_point_b Connection point on second module
 * @return True if connection successful
 */
bool scifi_connect_modules(Entity* module_a, Entity* module_b, SciFiConnectionType type, 
                         int connection_point_a, int connection_point_b);

/**
 * Validate station layout for structural integrity and life support
 * @param layout Station layout to validate
 * @return Validation score (0.0-1.0, higher is better)
 */
float scifi_validate_station_layout(const SciFiStationLayout* layout);

/**
 * Get material properties for a sci-fi material type
 * @param material Material type
 * @return Material properties or NULL if invalid
 */
const SciFiMaterialProperties* scifi_get_material_properties(SciFiMaterial material);

/**
 * Apply procedural variation to a module template
 * @param template Module template to modify
 * @param variation Variation index (0-7)
 */
void scifi_set_module_variation(SciFiModuleTemplate* template, int variation);

/**
 * Calculate power requirements for a station
 * @param layout Station layout
 * @return Net power balance (positive = surplus, negative = deficit)
 */
float scifi_calculate_power_balance(const SciFiStationLayout* layout);

/**
 * Optimize station layout for efficiency and safety
 * @param layout Station layout to optimize
 */
void scifi_optimize_station_layout(SciFiStationLayout* layout);

/* =================================================================================================
 *                                  UTILITY FUNCTIONS
 * =================================================================================================
 */

/**
 * Get a random sci-fi module type with weighted distribution
 * @param station_type Type of station influencing weights
 * @return Random module type
 */
SciFiModuleType scifi_get_random_module_type(int station_type);

/**
 * Get a random sci-fi material type
 * @return Random material type
 */
SciFiMaterial scifi_get_random_material(void);

/**
 * Check if a module type is suitable for a given station type
 * @param type Module type
 * @param station_type Station type
 * @return True if suitable
 */
bool scifi_is_module_suitable(SciFiModuleType type, int station_type);

/**
 * Calculate construction cost of a module
 * @param template Module template
 * @return Construction cost in resources
 */
float scifi_calculate_module_cost(const SciFiModuleTemplate* template);

/**
 * Get crew requirements for a module
 * @param type Module type
 * @return Minimum crew required
 */
int scifi_get_module_crew_requirements(SciFiModuleType type);

/**
 * Calculate radiation exposure for a station layout
 * @param layout Station layout
 * @return Radiation exposure in mSv/year
 */
float scifi_calculate_radiation_exposure(const SciFiStationLayout* layout);

/**
 * Get connection points for a module template
 * @param template Module template
 * @param connection_points Output array for connection points
 * @param max_connections Maximum number of connections to return
 * @return Number of connection points found
 */
int scifi_get_module_connection_points(const SciFiModuleTemplate* template, 
                                     SciFiConnectionPoint* connection_points, 
                                     int max_connections);

/**
 * Simulate station life support systems
 * @param layout Station layout
 * @param crew_count Current crew count
 * @param dt Time delta in seconds
 * @return Life support status (0.0-1.0, higher is better)
 */
float scifi_simulate_life_support(const SciFiStationLayout* layout, int crew_count, float dt);

/**
 * Generate station interior geometry for rendering
 * @param layout Station layout
 * @param mesh Output mesh for interior geometry
 * @return True if generation successful
 */
bool scifi_generate_station_interior(const SciFiStationLayout* layout, Mesh* mesh);

#ifdef __cplusplus
}
#endif

#endif // SCIFI_STATION_H
