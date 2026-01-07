/**
 * =================================================================================================
 *                          MEDIEVAL VILLAGE BUILDING KIT
 *                          Agent: AGENT_TEMPLATE_2
 * =================================================================================================
 *
 * Comprehensive medieval village asset templates with procedural generation,
 * modular building pieces, and authentic architectural styles.
 *
 * =================================================================================================
 */
#include "include/assets/templates/medieval_village.h"
#include "include/core/memory.h"
#include "include/rendering/renderer.h"
#include "include/physics/physics.h"
#include "math/math.h"

/* =================================================================================================
 *                                  MEDIEVAL BUILDING TYPES
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

/* =================================================================================================
 *                                  BUILDING TEMPLATE STRUCTURES
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

/* =================================================================================================
 *                                  MATERIAL PROPERTIES
 * =================================================================================================
 */

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

static MedievalMaterialProperties g_medieval_materials[MEDIEVAL_MATERIAL_COUNT] = {
    // Stone
    {
        .type = MEDIEVAL_MATERIAL_STONE,
        .texture_name = "medieval_stone_diffuse",
        .normal_map_name = "medieval_stone_normal",
        .base_color = {0.7f, 0.7f, 0.7f},
        .roughness = 0.8f,
        .metallic = 0.0f,
        .emissive = 0.0f,
        .durability = 1.0f,
        .cost_modifier = 1.2f,
        .is_flammable = false,
        .thermal_conductivity = 0.8f
    },
    // Oak Wood
    {
        .type = MEDIEVAL_MATERIAL_WOOD_OAK,
        .texture_name = "oak_wood_diffuse",
        .normal_map_name = "oak_wood_normal",
        .base_color = {0.6f, 0.4f, 0.2f},
        .roughness = 0.6f,
        .metallic = 0.0f,
        .emissive = 0.0f,
        .durability = 0.8f,
        .cost_modifier = 1.0f,
        .is_flammable = true,
        .thermal_conductivity = 0.15f
    },
    // Pine Wood
    {
        .type = MEDIEVAL_MATERIAL_WOOD_PINE,
        .texture_name = "pine_wood_diffuse",
        .normal_map_name = "pine_wood_normal",
        .base_color = {0.7f, 0.5f, 0.3f},
        .roughness = 0.7f,
        .metallic = 0.0f,
        .emissive = 0.0f,
        .durability = 0.6f,
        .cost_modifier = 0.8f,
        .is_flammable = true,
        .thermal_conductivity = 0.12f
    },
    // Plaster
    {
        .type = MEDIEVAL_MATERIAL_PLASTER,
        .texture_name = "medieval_plaster_diffuse",
        .normal_map_name = "medieval_plaster_normal",
        .base_color = {0.9f, 0.85f, 0.8f},
        .roughness = 0.4f,
        .metallic = 0.0f,
        .emissive = 0.0f,
        .durability = 0.4f,
        .cost_modifier = 0.6f,
        .is_flammable = false,
        .thermal_conductivity = 0.5f
    },
    // Thatch
    {
        .type = MEDIEVAL_MATERIAL_THATCH,
        .texture_name = "thatch_roof_diffuse",
        .normal_map_name = "thatch_roof_normal",
        .base_color = {0.8f, 0.6f, 0.3f},
        .roughness = 0.9f,
        .metallic = 0.0f,
        .emissive = 0.0f,
        .durability = 0.3f,
        .cost_modifier = 0.4f,
        .is_flammable = true,
        .thermal_conductivity = 0.08f
    },
    // Slate
    {
        .type = MEDIEVAL_MATERIAL_SLATE,
        .texture_name = "slate_roof_diffuse",
        .normal_map_name = "slate_roof_normal",
        .base_color = {0.3f, 0.3f, 0.4f},
        .roughness = 0.5f,
        .metallic = 0.0f,
        .emissive = 0.0f,
        .durability = 0.9f,
        .cost_modifier = 1.5f,
        .is_flammable = false,
        .thermal_conductivity = 0.7f
    },
    // Clay
    {
        .type = MEDIEVAL_MATERIAL_CLAY,
        .texture_name = "clay_tile_diffuse",
        .normal_map_name = "clay_tile_normal",
        .base_color = {0.6f, 0.4f, 0.2f},
        .roughness = 0.6f,
        .metallic = 0.0f,
        .emissive = 0.0f,
        .durability = 0.7f,
        .cost_modifier = 0.7f,
        .is_flammable = false,
        .thermal_conductivity = 0.6f
    },
    // Wrought Iron
    {
        .type = MEDIEVAL_MATERIAL_WROUGHT_IRON,
        .texture_name = "wrought_iron_diffuse",
        .normal_map_name = "wrought_iron_normal",
        .base_color = {0.2f, 0.2f, 0.25f},
        .roughness = 0.3f,
        .metallic = 0.8f,
        .emissive = 0.0f,
        .durability = 0.9f,
        .cost_modifier = 2.0f,
        .is_flammable = false,
        .thermal_conductivity = 0.5f
    }
};

/* =================================================================================================
 *                                  BUILDING TEMPLATES DATABASE
 * =================================================================================================
 */

static MedievalBuildingTemplate g_medieval_building_templates[MEDIEVAL_COUNT] = {
    // Small House
    {
        .type = MEDIEVAL_HOUSE_SMALL,
        .name = "Small Medieval House",
        .dimensions = {4.0f, 5.0f, 4.0f},
        .door_position = {0.0f, 0.0f, 2.0f},
        .window_positions = {{-1.5f, 1.5f, 2.0f}, {1.5f, 1.5f, 2.0f}},
        .window_count = 2,
        .primary_material = MEDIEVAL_MATERIAL_WOOD_OAK,
        .secondary_material = MEDIEVAL_MATERIAL_PLASTER,
        .roof_material = MEDIEVAL_MATERIAL_THATCH,
        .has_chimney = true,
        .chimney_position = {1.0f, 3.0f, 0.0f},
        .has_balcony = false,
        .stories = 1,
        .foundation_height = 0.3f,
        .has_basement = false,
        .has_garden = true,
        .garden_size = {6.0f, 0.0f, 6.0f}
    },
    // Medium House
    {
        .type = MEDIEVAL_HOUSE_MEDIUM,
        .name = "Medium Medieval House",
        .dimensions = {6.0f, 6.0f, 6.0f},
        .door_position = {0.0f, 0.0f, 3.0f},
        .window_positions = {{-2.0f, 1.5f, 3.0f}, {2.0f, 1.5f, 3.0f}, {-2.0f, 3.5f, 3.0f}, {2.0f, 3.5f, 3.0f}},
        .window_count = 4,
        .primary_material = MEDIEVAL_MATERIAL_STONE,
        .secondary_material = MEDIEVAL_MATERIAL_WOOD_OAK,
        .roof_material = MEDIEVAL_MATERIAL_SLATE,
        .has_chimney = true,
        .chimney_position = {1.5f, 4.0f, 0.0f},
        .has_balcony = true,
        .balcony_position = {0.0f, 3.0f, 3.0f},
        .stories = 2,
        .foundation_height = 0.5f,
        .has_basement = true,
        .has_garden = true,
        .garden_size = {8.0f, 0.0f, 8.0f}
    },
    // Blacksmith Shop
    {
        .type = MEDIEVAL_SHOP_BLACKSMITH,
        .name = "Blacksmith Shop",
        .dimensions = {8.0f, 4.0f, 6.0f},
        .door_position = {0.0f, 0.0f, 3.0f},
        .window_positions = {{-3.0f, 2.0f, 3.0f}, {3.0f, 2.0f, 3.0f}},
        .window_count = 2,
        .primary_material = MEDIEVAL_MATERIAL_STONE,
        .secondary_material = MEDIEVAL_MATERIAL_WOOD_OAK,
        .roof_material = MEDIEVAL_MATERIAL_SLATE,
        .has_chimney = true,
        .chimney_position = {0.0f, 3.0f, 0.0f},
        .has_balcony = false,
        .stories = 1,
        .foundation_height = 0.6f,
        .has_basement = false,
        .has_garden = false,
        .garden_size = {0.0f, 0.0f, 0.0f}
    },
    // Tavern
    {
        .type = MEDIEVAL_SHOP_TAVERN,
        .name = "Village Tavern",
        .dimensions = {10.0f, 5.0f, 8.0f},
        .door_position = {0.0f, 0.0f, 4.0f},
        .window_positions = {{-4.0f, 1.5f, 4.0f}, {4.0f, 1.5f, 4.0f}, {-4.0f, 1.5f, 0.0f}, {4.0f, 1.5f, 0.0f}},
        .window_count = 4,
        .primary_material = MEDIEVAL_MATERIAL_WOOD_OAK,
        .secondary_material = MEDIEVAL_MATERIAL_PLASTER,
        .roof_material = MEDIEVAL_MATERIAL_THATCH,
        .has_chimney = true,
        .chimney_position = {2.0f, 3.0f, 0.0f},
        .has_balcony = true,
        .balcony_position = {0.0f, 2.5f, 4.0f},
        .stories = 1,
        .foundation_height = 0.4f,
        .has_basement = true,
        .has_garden = true,
        .garden_size = {12.0f, 0.0f, 10.0f}
    },
    // Castle Tower
    {
        .type = MEDIEVAL_CASTLE_TOWER,
        .name = "Castle Tower",
        .dimensions = {6.0f, 12.0f, 6.0f},
        .door_position = {0.0f, 0.0f, 3.0f},
        .window_positions = {{-2.5f, 3.0f, 3.0f}, {2.5f, 3.0f, 3.0f}, {-2.5f, 6.0f, 3.0f}, {2.5f, 6.0f, 3.0f}},
        .window_count = 4,
        .primary_material = MEDIEVAL_MATERIAL_STONE,
        .secondary_material = MEDIEVAL_MATERIAL_STONE,
        .roof_material = MEDIEVAL_MATERIAL_SLATE,
        .has_chimney = false,
        .chimney_position = {0.0f, 0.0f, 0.0f},
        .has_balcony = false,
        .stories = 3,
        .foundation_height = 1.0f,
        .has_basement = true,
        .has_garden = false,
        .garden_size = {0.0f, 0.0f, 0.0f}
    }
};

/* =================================================================================================
 *                                  PROCEDURAL GENERATION FUNCTIONS
 * =================================================================================================
 */

static vec3 medieval_generate_road_path(vec3 start, vec3 end, float curvature) {
    vec3 midpoint = vec3_add(start, end);
    midpoint = vec3_scale(midpoint, 0.5f);
    
    // Add perpendicular offset for curvature
    vec3 direction = vec3_subtract(end, start);
    vec3 perpendicular = {-direction.z, 0.0f, direction.x};
    perpendicular = vec3_normalize(perpendicular);
    
    float offset = sinf(curvature * M_PI) * 2.0f;
    midpoint = vec3_add(midpoint, vec3_scale(perpendicular, offset));
    
    return midpoint;
}

static bool medieval_check_building_collision(vec3 pos1, vec3 size1, vec3 pos2, vec3 size2) {
    vec3 min1 = vec3_subtract(pos1, vec3_scale(size1, 0.5f));
    vec3 max1 = vec3_add(pos1, vec3_scale(size1, 0.5f));
    vec3 min2 = vec3_subtract(pos2, vec3_scale(size2, 0.5f));
    vec3 max2 = vec3_add(pos2, vec3_scale(size2, 0.5f));
    
    return (min1.x <= max2.x && max1.x >= min2.x &&
            min1.y <= max2.y && max1.y >= min2.y &&
            min1.z <= max2.z && max1.z >= min2.z);
}

static vec3 medieval_find_valid_position(MedievalVillageLayout* layout, vec3 building_size) {
    const int max_attempts = 100;
    
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        // Generate random position within village radius
        float angle = (float)rand() / RAND_MAX * 2.0f * M_PI;
        float distance = (float)rand() / RAND_MAX * (layout->village_radius - 5.0f);
        
        vec3 position = {
            layout->center_position.x + cosf(angle) * distance,
            layout->center_position.y,
            layout->center_position.z + sinf(angle) * distance
        };
        
        // Check collision with existing buildings
        bool collision = false;
        for (int i = 0; i < layout->building_count; i++) {
            if (medieval_check_building_collision(position, building_size, 
                                                 layout->building_positions[i], 
                                                 layout->buildings[i].dimensions)) {
                collision = true;
                break;
            }
        }
        
        if (!collision) {
            return position;
        }
    }
    
    // Fallback position at village center
    return layout->center_position;
}

/* =================================================================================================
 *                                  BUILDING MESH GENERATION
 * =================================================================================================
 */

static Mesh* medieval_generate_building_mesh(const MedievalBuildingTemplate* template) {
    Mesh* mesh = mesh_create();
    
    // Generate foundation
    vec3 foundation_size = {template->dimensions.x, template->foundation_height, template->dimensions.z};
    vec3 foundation_pos = {0.0f, -template->foundation_height * 0.5f, 0.0f};
    mesh_add_box(mesh, foundation_pos, foundation_size);
    
    // Generate main structure
    vec3 main_size = {template->dimensions.x, template->dimensions.y - template->foundation_height, template->dimensions.z};
    vec3 main_pos = {0.0f, template->dimensions.y * 0.5f, 0.0f};
    mesh_add_box(mesh, main_pos, main_size);
    
    // Generate roof (simple pitched roof)
    float roof_height = template->dimensions.x * 0.3f;
    vec3 roof_size = {template->dimensions.x * 1.1f, roof_height, template->dimensions.z * 1.1f};
    vec3 roof_pos = {0.0f, template->dimensions.y + roof_height * 0.5f, 0.0f};
    mesh_add_pyramid(mesh, roof_pos, roof_size);
    
    // Generate chimney
    if (template->has_chimney) {
        vec3 chimney_size = {0.8f, template->dimensions.y * 0.3f, 0.8f};
        vec3 chimney_top = {template->chimney_position.x, 
                           template->dimensions.y + roof_height * 0.8f, 
                           template->chimney_position.z};
        mesh_add_box(mesh, chimney_top, chimney_size);
    }
    
    // Generate balcony
    if (template->has_balcony) {
        vec3 balcony_size = {template->dimensions.x * 0.8f, 0.1f, 1.0f};
        vec3 balcony_pos = {template->balcony_position.x, 
                           template->balcony_position.y, 
                           template->balcony_position.z + 1.0f};
        mesh_add_box(mesh, balcony_pos, balcony_size);
    }
    
    mesh_build(mesh);
    return mesh;
}

static void medieval_apply_materials(Mesh* mesh, const MedievalBuildingTemplate* template) {
    // Apply materials based on template
    MedievalMaterialProperties* primary = &g_medieval_materials[template->primary_material];
    MedievalMaterialProperties* secondary = &g_medieval_materials[template->secondary_material];
    MedievalMaterialProperties* roof = &g_medieval_materials[template->roof_material];
    
    // Set material properties for different mesh parts
    mesh_set_material(mesh, 0, primary->texture_name, primary->normal_map_name, 
                     primary->base_color, primary->roughness, primary->metallic);
    mesh_set_material(mesh, 1, secondary->texture_name, secondary->normal_map_name, 
                     secondary->base_color, secondary->roughness, secondary->metallic);
    mesh_set_material(mesh, 2, roof->texture_name, roof->normal_map_name, 
                     roof->base_color, roof->roughness, roof->metallic);
}

/* =================================================================================================
 *                                  VILLAGE LAYOUT GENERATION
 * =================================================================================================
 */

MedievalVillageLayout* medieval_generate_village_layout(vec3 center, float radius, int building_density) {
    MedievalVillageLayout* layout = malloc(sizeof(MedievalVillageLayout));
    layout->center_position = center;
    layout->village_radius = radius;
    layout->building_count = building_density;
    
    // Allocate arrays
    layout->buildings = malloc(sizeof(MedievalBuildingTemplate) * building_density);
    layout->building_positions = malloc(sizeof(vec3) * building_density);
    layout->building_rotations = malloc(sizeof(float) * building_density);
    
    // Place well at center
    layout->well_position = center;
    
    // Place chapel near center
    float chapel_angle = (float)rand() / RAND_MAX * 2.0f * M_PI;
    float chapel_distance = radius * 0.2f;
    layout->chapel_position = {
        center.x + cosf(chapel_angle) * chapel_distance,
        center.y,
        center.z + sinf(chapel_angle) * chapel_distance
    };
    
    // Determine if village has castle (30% chance)
    layout->has_castle = ((float)rand() / RAND_MAX) < 0.3f;
    if (layout->has_castle) {
        layout->castle_position = {
            center.x + radius * 0.7f,
            center.y,
            center.z + radius * 0.7f
        };
        layout->castle_radius = radius * 0.15f;
    }
    
    // Generate buildings
    for (int i = 0; i < building_density; i++) {
        // Select building type with weighted distribution
        float rand_val = (float)rand() / RAND_MAX;
        MedievalBuildingType type;
        
        if (rand_val < 0.4f) {
            type = MEDIEVAL_HOUSE_SMALL;
        } else if (rand_val < 0.6f) {
            type = MEDIEVAL_HOUSE_MEDIUM;
        } else if (rand_val < 0.7f) {
            type = MEDIEVAL_SHOP_BLACKSMITH;
        } else if (rand_val < 0.8f) {
            type = MEDIEVAL_SHOP_TAVERN;
        } else if (rand_val < 0.85f) {
            type = MEDIEVAL_CHAPEL_SMALL;
        } else if (rand_val < 0.9f) {
            type = MEDIEVAL_WINDMILL;
        } else if (rand_val < 0.95f) {
            type = MEDIEVAL_STABLE;
        } else {
            type = MEDIEVAL_FARMHOUSE;
        }
        
        layout->buildings[i] = g_medieval_building_templates[type];
        layout->building_positions[i] = medieval_find_valid_position(layout, layout->buildings[i].dimensions);
        layout->building_rotations[i] = (float)rand() / RAND_MAX * 2.0f * M_PI;
    }
    
    // Generate road network
    layout->road_count = 0;
    
    // Main road from well to chapel
    layout->road_network[layout->road_count++] = layout->well_position;
    layout->road_network[layout->road_count++] = layout->chapel_position;
    
    // Roads connecting buildings
    for (int i = 0; i < building_density && layout->road_count < 63; i++) {
        // Connect to nearest building or landmark
        vec3 nearest = layout->well_position;
        float min_distance = vec3_distance(layout->building_positions[i], nearest);
        
        float dist_to_chapel = vec3_distance(layout->building_positions[i], layout->chapel_position);
        if (dist_to_chapel < min_distance) {
            nearest = layout->chapel_position;
            min_distance = dist_to_chapel;
        }
        
        // Add road segment
        if (min_distance < radius * 0.3f) {
            layout->road_network[layout->road_count++] = layout->building_positions[i];
            layout->road_network[layout->road_count++] = nearest;
        }
    }
    
    // Generate village wall if large enough
    layout->has_wall = radius > 30.0f;
    if (layout->has_wall) {
        layout->wall_point_count = 16;
        for (int i = 0; i < 16; i++) {
            float angle = (float)i / 16.0f * 2.0f * M_PI;
            layout->wall_points[i] = {
                center.x + cosf(angle) * (radius + 5.0f),
                center.y,
                center.z + sinf(angle) * (radius + 5.0f)
            };
        }
    }
    
    return layout;
}

/* =================================================================================================
 *                                  MAIN API FUNCTIONS
 * =================================================================================================
 */

MedievalVillageLayout* medieval_create_village(vec3 center, float radius, int building_density) {
    return medieval_generate_village_layout(center, radius, building_density);
}

void medieval_destroy_village(MedievalVillageLayout* layout) {
    if (layout) {
        free(layout->buildings);
        free(layout->building_positions);
        free(layout->building_rotations);
        free(layout);
    }
}

Entity* medieval_spawn_building(const MedievalBuildingTemplate* template, vec3 position, float rotation) {
    Entity* building = entity_create();
    
    // Generate and assign mesh
    Mesh* mesh = medieval_generate_building_mesh(template);
    medieval_apply_materials(mesh, template);
    
    entity_set_mesh(building, mesh);
    entity_set_position(building, position);
    entity_set_rotation(building, vec3(0.0f, rotation, 0.0f));
    
    // Add physics collider
    PhysicsBody* body = physics_create_static_body();
    vec3 collider_size = template->dimensions;
    physics_add_box_collider(body, collider_size);
    entity_set_physics_body(building, body);
    
    // Set building properties
    entity_set_name(building, template->name);
    entity_set_type(building, ENTITY_TYPE_BUILDING);
    
    return building;
}

Entity* medieval_spawn_village(const MedievalVillageLayout* layout) {
    Entity* village_root = entity_create();
    entity_set_name(village_root, "Medieval Village");
    entity_set_position(village_root, layout->center_position);
    
    // Spawn all buildings
    for (int i = 0; i < layout->building_count; i++) {
        Entity* building = medieval_spawn_building(&layout->buildings[i], 
                                                  layout->building_positions[i], 
                                                  layout->building_rotations[i]);
        entity_add_child(village_root, building);
    }
    
    // Spawn well
    Entity* well = entity_create();
    entity_set_name(well, "Village Well");
    entity_set_position(well, layout->well_position);
    // Add well mesh and physics
    entity_add_child(village_root, well);
    
    // Spawn chapel
    Entity* chapel = entity_create();
    entity_set_name(chapel, "Village Chapel");
    entity_set_position(chapel, layout->chapel_position);
    // Add chapel mesh and physics
    entity_add_child(village_root, chapel);
    
    // Spawn castle if present
    if (layout->has_castle) {
        Entity* castle = entity_create();
        entity_set_name(castle, "Castle");
        entity_set_position(castle, layout->castle_position);
        // Add castle components
        entity_add_child(village_root, castle);
    }
    
    return village_root;
}

void medieval_add_decoration(Entity* building, MedievalDecorationType type, vec3 position) {
    // Add decorative elements like barrels, crates, signs, etc.
    Entity* decoration = entity_create();
    
    switch (type) {
        case MEDIEVAL_DECORATION_BARREL:
            entity_set_name(decoration, "Barrel");
            // Add barrel mesh
            break;
        case MEDIEVAL_DECORATION_CRATE:
            entity_set_name(decoration, "Crate");
            // Add crate mesh
            break;
        case MEDIEVAL_DECORATION_SIGN:
            entity_set_name(decoration, "Sign");
            // Add sign mesh
            break;
        case MEDIEVAL_DECORATION_LANTERN:
            entity_set_name(decoration, "Lantern");
            // Add lantern mesh with light
            break;
    }
    
    entity_set_position(decoration, position);
    entity_add_child(building, decoration);
}

const MedievalMaterialProperties* medieval_get_material_properties(MedievalMaterial material) {
    if (material >= 0 && material < MEDIEVAL_MATERIAL_COUNT) {
        return &g_medieval_materials[material];
    }
    return NULL;
}

void medieval_set_building_variation(MedievalBuildingTemplate* template, int variation) {
    // Apply procedural variations to building template
    switch (variation % 4) {
        case 0:
            // Variation 1: Different window positions
            for (int i = 0; i < template->window_count; i++) {
                template->window_positions[i].x += (float)(variation % 3 - 1) * 0.5f;
            }
            break;
        case 1:
            // Variation 2: Different roof material
            template->roof_material = (template->roof_material + 1) % MEDIEVAL_MATERIAL_COUNT;
            break;
        case 2:
            // Variation 3: Add extra decorative elements
            template->has_balcony = !template->has_balcony;
            break;
        case 3:
            // Variation 4: Adjust dimensions slightly
            template->dimensions.x *= 0.9f + (float)rand() / RAND_MAX * 0.2f;
            template->dimensions.z *= 0.9f + (float)rand() / RAND_MAX * 0.2f;
            break;
    }
}

/* =================================================================================================
 *                                  UTILITY FUNCTIONS
 * =================================================================================================
 */

float medieval_calculate_village_score(const MedievalVillageLayout* layout) {
    float score = 0.0f;
    
    // Score based on building variety
    int building_types[MEDIEVAL_COUNT] = {0};
    for (int i = 0; i < layout->building_count; i++) {
        building_types[layout->buildings[i].type]++;
    }
    
    int variety_count = 0;
    for (int i = 0; i < MEDIEVAL_COUNT; i++) {
        if (building_types[i] > 0) variety_count++;
    }
    score += variety_count * 10.0f;
    
    // Score based on road connectivity
    score += layout->road_count * 2.0f;
    
    // Score based on defensive structures
    if (layout->has_wall) score += 20.0f;
    if (layout->has_castle) score += 30.0f;
    
    // Score based on village size
    score += layout->building_count * 5.0f;
    
    return score;
}

void medieval_optimize_layout(MedievalVillageLayout* layout) {
    // Optimize building positions for better spacing and road access
    for (int iteration = 0; iteration < 10; iteration++) {
        for (int i = 0; i < layout->building_count; i++) {
            vec3 current_pos = layout->building_positions[i];
            vec3 ideal_pos = current_pos;
            
            // Move buildings closer to roads
            float min_road_distance = FLT_MAX;
            for (int j = 0; j < layout->road_count - 1; j += 2) {
                vec3 road_start = layout->road_network[j];
                vec3 road_end = layout->road_network[j + 1];
                
                // Calculate distance to road segment
                vec3 road_dir = vec3_subtract(road_end, road_start);
                float road_length = vec3_length(road_dir);
                if (road_length > 0.0f) {
                    road_dir = vec3_normalize(road_dir);
                    vec3 to_building = vec3_subtract(current_pos, road_start);
                    float projection = vec3_dot(to_building, road_dir);
                    projection = fmaxf(0.0f, fminf(road_length, projection));
                    
                    vec3 closest_point = vec3_add(road_start, vec3_scale(road_dir, projection));
                    float distance = vec3_distance(current_pos, closest_point);
                    
                    if (distance < min_road_distance) {
                        min_road_distance = distance;
                        if (distance > 8.0f) {
                            // Move closer to road
                            vec3 move_dir = vec3_subtract(closest_point, current_pos);
                            move_dir = vec3_normalize(move_dir);
                            ideal_pos = vec3_add(current_pos, vec3_scale(move_dir, 1.0f));
                        }
                    }
                }
            }
            
            // Check if new position is valid
            bool valid_position = true;
            for (int j = 0; j < layout->building_count; j++) {
                if (i != j && medieval_check_building_collision(ideal_pos, layout->buildings[i].dimensions,
                                                              layout->building_positions[j], layout->buildings[j].dimensions)) {
                    valid_position = false;
                    break;
                }
            }
            
            if (valid_position) {
                layout->building_positions[i] = ideal_pos;
            }
        }
    }
}
