/**
 * =================================================================================================
 *                          SCI-FI SPACE STATION MODULAR PIECES
 *                          Agent: AGENT_TEMPLATE_2
 * =================================================================================================
 *
 * Comprehensive sci-fi space station asset templates with modular construction,
 * connection systems, and realistic space engineering principles.
 *
 * =================================================================================================
 */
#include "../../include/asset_templates/scifi_station.h"
#include "../../include/core/memory.h"
#include "../../include/renderer/renderer.h"
#include "../../include/physics/physics.h"
#include "../../include/math/math.h"

/* =================================================================================================
 *                                  MATERIAL PROPERTIES DATABASE
 * =================================================================================================
 */

static const SciFiMaterialProperties g_material_properties[SCIFI_MATERIAL_COUNT] = {
    // SCIFI_MATERIAL_TITANIUM_ALLOY
    {
        .type = SCIFI_MATERIAL_TITANIUM_ALLOY,
        .texture_name = "textures/metal/titanium_alloy_diffuse.png",
        .normal_map_name = "textures/metal/titanium_alloy_normal.png",
        .emissive_map_name = "",
        .base_color = {0.7f, 0.7f, 0.75f},
        .emissive_color = {0.0f, 0.0f, 0.0f},
        .roughness = 0.3f,
        .metallic = 0.9f,
        .emissive_strength = 0.0f,
        .density = 4500.0f,
        .tensile_strength = 900.0f,
        .thermal_resistance = 0.8f,
        .radiation_absorption = 0.3f,
        .cost_modifier = 1.2f,
        .is_transparent = false,
        .transparency = 0.0f,
        .is_conductive = true,
        .conductivity = 2.38e6f
    },
    // SCIFI_MATERIAL_CERAMIC_COMPOSITE
    {
        .type = SCIFI_MATERIAL_CERAMIC_COMPOSITE,
        .texture_name = "textures/ceramic/ceramic_composite_diffuse.png",
        .normal_map_name = "textures/ceramic/ceramic_composite_normal.png",
        .emissive_map_name = "",
        .base_color = {0.9f, 0.9f, 0.85f},
        .emissive_color = {0.0f, 0.0f, 0.0f},
        .roughness = 0.6f,
        .metallic = 0.1f,
        .emissive_strength = 0.0f,
        .density = 3800.0f,
        .tensile_strength = 350.0f,
        .thermal_resistance = 0.95f,
        .radiation_absorption = 0.8f,
        .cost_modifier = 1.8f,
        .is_transparent = false,
        .transparency = 0.0f,
        .is_conductive = false,
        .conductivity = 0.01f
    },
    // SCIFI_MATERIAL_CARBON_FIBER
    {
        .type = SCIFI_MATERIAL_CARBON_FIBER,
        .texture_name = "textures/composite/carbon_fiber_diffuse.png",
        .normal_map_name = "textures/composite/carbon_fiber_normal.png",
        .emissive_map_name = "",
        .base_color = {0.1f, 0.1f, 0.12f},
        .emissive_color = {0.0f, 0.0f, 0.0f},
        .roughness = 0.2f,
        .metallic = 0.4f,
        .emissive_strength = 0.0f,
        .density = 1600.0f,
        .tensile_strength = 1600.0f,
        .thermal_resistance = 0.4f,
        .radiation_absorption = 0.2f,
        .cost_modifier = 2.5f,
        .is_transparent = false,
        .transparency = 0.0f,
        .is_conductive = false,
        .conductivity = 1000.0f
    },
    // SCIFI_MATERIAL_TRANSPARENT_ALUMINUM
    {
        .type = SCIFI_MATERIAL_TRANSPARENT_ALUMINUM,
        .texture_name = "textures/glass/transparent_aluminum_diffuse.png",
        .normal_map_name = "textures/glass/transparent_aluminum_normal.png",
        .emissive_map_name = "",
        .base_color = {0.95f, 0.95f, 1.0f},
        .emissive_color = {0.0f, 0.0f, 0.0f},
        .roughness = 0.05f,
        .metallic = 0.8f,
        .emissive_strength = 0.0f,
        .density = 2700.0f,
        .tensile_strength = 400.0f,
        .thermal_resistance = 0.6f,
        .radiation_absorption = 0.4f,
        .cost_modifier = 3.2f,
        .is_transparent = true,
        .transparency = 0.85f,
        .is_conductive = true,
        .conductivity = 3.5e7f
    },
    // Additional materials would continue here...
};

/* =================================================================================================
 *                                  MODULE TEMPLATES DATABASE
 * =================================================================================================
 */

static const SciFiModuleTemplate g_module_templates[SCIFI_MODULE_COUNT] = {
    // SCIFI_MODULE_CORRIDOR_STRAIGHT
    {
        .type = SCIFI_MODULE_CORRIDOR_STRAIGHT,
        .name = "Straight Corridor",
        .size = SCIFI_SIZE_MEDIUM,
        .dimensions = {4.0f, 4.0f, 6.0f},
        .primary_material = SCIFI_MATERIAL_TITANIUM_ALLOY,
        .secondary_material = SCIFI_MATERIAL_CERAMIC_COMPOSITE,
        .transparent_material = SCIFI_MATERIAL_TRANSPARENT_ALUMINUM,
        .mass = 1200.0f,
        .power_consumption = 50.0f,
        .power_generation = 0.0f,
        .crew_capacity = 0.0f,
        .radiation_shielding = 0.3f,
        .has_gravity_plating = true,
        .has_life_support = true,
        .has_atmosphere = true,
        .has_windows = true,
        .window_positions = {{2.0f, 2.0f, 0.0f}, {-2.0f, 2.0f, 0.0f}},
        .window_count = 2,
        .has_doors = true,
        .door_positions = {{0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, -3.0f}},
        .door_count = 2,
        .connections = {SCIFI_CONNECTION_AIRLOCK, SCIFI_CONNECTION_AIRLOCK, SCIFI_CONNECTION_NONE, 
                      SCIFI_CONNECTION_NONE, SCIFI_CONNECTION_NONE, SCIFI_CONNECTION_NONE},
        .has_external_ports = false,
        .external_port_count = 0,
        .structural_integrity = 0.8f,
        .is_pressurized = true,
        .operating_temperature = 293.0f,
        .requires_maintenance = false,
        .maintenance_interval = 0.0f
    },
    // SCIFI_MODULE_CORRIDOR_L_SHAPE
    {
        .type = SCIFI_MODULE_CORRIDOR_L_SHAPE,
        .name = "L-Shaped Corridor",
        .size = SCIFI_SIZE_MEDIUM,
        .dimensions = {6.0f, 6.0f, 4.0f},
        .primary_material = SCIFI_MATERIAL_TITANIUM_ALLOY,
        .secondary_material = SCIFI_MATERIAL_CERAMIC_COMPOSITE,
        .transparent_material = SCIFI_MATERIAL_TRANSPARENT_ALUMINUM,
        .mass = 1800.0f,
        .power_consumption = 75.0f,
        .power_generation = 0.0f,
        .crew_capacity = 0.0f,
        .radiation_shielding = 0.3f,
        .has_gravity_plating = true,
        .has_life_support = true,
        .has_atmosphere = true,
        .has_windows = true,
        .window_positions = {{3.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}},
        .window_count = 2,
        .has_doors = true,
        .door_positions = {{3.0f, 0.0f, 0.0f}, {0.0f, 3.0f, 0.0f}, {-3.0f, 0.0f, 0.0f}},
        .door_count = 3,
        .connections = {SCIFI_CONNECTION_AIRLOCK, SCIFI_CONNECTION_AIRLOCK, SCIFI_CONNECTION_AIRLOCK,
                      SCIFI_CONNECTION_NONE, SCIFI_CONNECTION_NONE, SCIFI_CONNECTION_NONE},
        .has_external_ports = false,
        .external_port_count = 0,
        .structural_integrity = 0.75f,
        .is_pressurized = true,
        .operating_temperature = 293.0f,
        .requires_maintenance = false,
        .maintenance_interval = 0.0f
    },
    // Additional module templates would continue here...
};

/* =================================================================================================
 *                                  UTILITY FUNCTIONS
 * =================================================================================================
 */

const SciFiMaterialProperties* scifi_get_material_properties(SciFiMaterial material) {
    if (material < 0 || material >= SCIFI_MATERIAL_COUNT) {
        return NULL;
    }
    return &g_material_properties[material];
}

SciFiModuleType scifi_get_random_module_type(int station_type) {
    // Weighted distribution based on station type
    float weights[SCIFI_MODULE_COUNT] = {0};
    
    // Set base weights
    for (int i = 0; i < SCIFI_MODULE_COUNT; i++) {
        weights[i] = 1.0f;
    }
    
    // Adjust weights based on station type
    switch (station_type) {
        case 0: // Military
            weights[SCIFI_MODULE_DEFENSE_TURRET] *= 3.0f;
            weights[SCIFI_MODULE_DEFENSE_SHIELD_GENERATOR] *= 2.5f;
            weights[SCIFI_MODULE_ARMORY] *= 2.0f;
            weights[SCIFI_MODULE_BRIG] *= 1.5f;
            break;
        case 1: // Research
            weights[SCIFI_MODULE_HABITATION_LABORATORY] *= 3.0f;
            weights[SCIFI_MODULE_RESEARCH_STATION] *= 2.5f;
            weights[SCIFI_MODULE_OBSERVATORY_DOME] *= 2.0f;
            break;
        case 2: // Colony
            weights[SCIFI_MODULE_HABITATION_QUARTERS] *= 2.5f;
            weights[SCIFI_MODULE_HABITATION_MESS_HALL] *= 2.0f;
            weights[SCIFI_MODULE_HYDROPONICS_BAY] *= 2.0f;
            weights[SCIFI_MODULE_GYM_RECREATION] *= 1.5f;
            break;
    }
    
    // Calculate total weight
    float total_weight = 0.0f;
    for (int i = 0; i < SCIFI_MODULE_COUNT; i++) {
        total_weight += weights[i];
    }
    
    // Generate random number and select module
    float random = ((float)rand() / RAND_MAX) * total_weight;
    float cumulative = 0.0f;
    
    for (int i = 0; i < SCIFI_MODULE_COUNT; i++) {
        cumulative += weights[i];
        if (random <= cumulative) {
            return (SciFiModuleType)i;
        }
    }
    
    return SCIFI_MODULE_CORRIDOR_STRAIGHT; // Fallback
}

SciFiMaterial scifi_get_random_material(void) {
    return (SciFiMaterial)(rand() % SCIFI_MATERIAL_COUNT);
}

bool scifi_is_module_suitable(SciFiModuleType type, int station_type) {
    // Basic suitability check
    switch (station_type) {
        case 0: // Military
            return type != SCIFI_MODULE_HYDROPONICS_BAY && 
                   type != SCIFI_MODULE_GYM_RECREATION;
        case 1: // Research
            return type != SCIFI_MODULE_ARMORY && 
                   type != SCIFI_MODULE_BRIG;
        case 2: // Colony
            return type != SCIFI_MODULE_DEFENSE_TURRET && 
                   type != SCIFI_MODULE_BRIG;
        default:
            return true;
    }
}

float scifi_calculate_module_cost(const SciFiModuleTemplate* template) {
    if (!template) return 0.0f;
    
    const SciFiMaterialProperties* primary = scifi_get_material_properties(template->primary_material);
    const SciFiMaterialProperties* secondary = scifi_get_material_properties(template->secondary_material);
    
    float base_cost = template->mass * 0.01f; // Base cost per kg
    float material_cost = 0.0f;
    
    if (primary) material_cost += base_cost * primary->cost_modifier * 0.7f;
    if (secondary) material_cost += base_cost * secondary->cost_modifier * 0.3f;
    
    // Add complexity modifiers
    float complexity_modifier = 1.0f;
    if (template->has_life_support) complexity_modifier += 0.2f;
    if (template->has_gravity_plating) complexity_modifier += 0.3f;
    if (template->power_generation > 0) complexity_modifier += 0.4f;
    if (template->radiation_shielding > 0.5f) complexity_modifier += 0.25f;
    
    return material_cost * complexity_modifier;
}

int scifi_get_module_crew_requirements(SciFiModuleType type) {
    switch (type) {
        case SCIFI_MODULE_COMMAND_BRIDGE: return 4;
        case SCIFI_MODULE_ENGINEERING_REACTOR: return 3;
        case SCIFI_MODULE_HABITATION_MEDBAY: return 2;
        case SCIFI_MODULE_HABITATION_LABORATORY: return 3;
        case SCIFI_MODULE_DEFENSE_TURRET: return 1;
        case SCIFI_MODULE_OBSERVATORY_DOME: return 2;
        case SCIFI_MODULE_COMMUNICATIONS_ARRAY: return 2;
        default: return 0;
    }
}

/* =================================================================================================
 *                                  MAIN API FUNCTIONS
 * =================================================================================================
 */

SciFiStationLayout* scifi_create_station(vec3 center, float radius, int module_count, int station_type) {
    SciFiStationLayout* layout = malloc(sizeof(SciFiStationLayout));
    if (!layout) return NULL;
    
    memset(layout, 0, sizeof(SciFiStationLayout));
    layout->center_position = center;
    layout->station_radius = radius;
    layout->module_count = module_count;
    layout->is_military_grade = (station_type == 0);
    layout->is_research_station = (station_type == 1);
    layout->is_colony_ship = (station_type == 2);
    
    // Allocate arrays
    layout->modules = malloc(sizeof(SciFiModuleTemplate) * module_count);
    layout->module_positions = malloc(sizeof(vec3) * module_count);
    layout->module_rotations = malloc(sizeof(float) * module_count);
    
    if (!layout->modules || !layout->module_positions || !layout->module_rotations) {
        scifi_destroy_station(layout);
        return NULL;
    }
    
    // Generate modules
    for (int i = 0; i < module_count; i++) {
        SciFiModuleType type;
        do {
            type = scifi_get_random_module_type(station_type);
        } while (!scifi_is_module_suitable(type, station_type));
        
        layout->modules[i] = g_module_templates[type];
        scifi_set_module_variation(&layout->modules[i], rand() % 8);
        
        // Position modules in a circular pattern with some randomness
        float angle = (float)i / module_count * 2.0f * M_PI + (rand() % 100 - 50) * 0.01f;
        float distance = radius * (0.5f + (rand() % 100) * 0.003f);
        
        layout->module_positions[i].x = center.x + cosf(angle) * distance;
        layout->module_positions[i].y = center.y + (rand() % 100 - 50) * 0.1f;
        layout->module_positions[i].z = center.z + sinf(angle) * distance;
        
        layout->module_rotations[i] = angle + (rand() % 100 - 50) * 0.01f;
    }
    
    // Calculate totals
    layout->total_power_output = 0.0f;
    layout->total_power_consumption = 0.0f;
    layout->total_crew_capacity = 0.0f;
    
    for (int i = 0; i < module_count; i++) {
        layout->total_power_output += layout->modules[i].power_generation;
        layout->total_power_consumption += layout->modules[i].power_consumption;
        layout->total_crew_capacity += layout->modules[i].crew_capacity;
    }
    
    return layout;
}

void scifi_destroy_station(SciFiStationLayout* layout) {
    if (!layout) return;
    
    if (layout->modules) free(layout->modules);
    if (layout->module_positions) free(layout->module_positions);
    if (layout->module_rotations) free(layout->module_rotations);
    
    free(layout);
}

void scifi_set_module_variation(SciFiModuleTemplate* template, int variation) {
    if (!template || variation < 0 || variation > 7) return;
    
    // Apply variations based on variation index
    switch (variation % 4) {
        case 0: // Default
            break;
        case 1: // Slightly larger
            template->dimensions.x *= 1.1f;
            template->dimensions.y *= 1.1f;
            template->mass *= 1.2f;
            break;
        case 2: // Different material
            template->primary_material = scifi_get_random_material();
            break;
        case 3: // Extra windows
            if (template->window_count < 12) {
                template->window_positions[template->window_count++] = (vec3){0, template->dimensions.y * 0.8f, 0};
            }
            break;
    }
    
    // Add some wear and tear variations
    template->structural_integrity *= (0.9f + (rand() % 100) * 0.002f);
    template->maintenance_interval *= (0.8f + (rand() % 100) * 0.004f);
}

float scifi_calculate_power_balance(const SciFiStationLayout* layout) {
    if (!layout) return 0.0f;
    return layout->total_power_output - layout->total_power_consumption;
}

float scifi_validate_station_layout(const SciFiStationLayout* layout) {
    if (!layout) return 0.0f;
    
    float score = 1.0f;
    
    // Check power balance
    float power_balance = scifi_calculate_power_balance(layout);
    if (power_balance < 0) {
        score *= 0.5f; // Penalty for power deficit
    }
    
    // Check module density
    float density = (float)layout->module_count / (layout->station_radius * layout->station_radius);
    if (density > 0.01f) {
        score *= 0.8f; // Penalty for overcrowding
    }
    
    // Check connectivity (simplified)
    int connected_modules = 0;
    for (int i = 0; i < layout->module_count; i++) {
        for (int j = i + 1; j < layout->module_count; j++) {
            vec3 diff = vec3_sub(layout->module_positions[i], layout->module_positions[j]);
            float distance = vec3_length(diff);
            if (distance < 10.0f) { // Within connection range
                connected_modules++;
                break;
            }
        }
    }
    
    float connectivity = (float)connected_modules / layout->module_count;
    score *= (0.5f + connectivity * 0.5f);
    
    return fmaxf(0.0f, fminf(1.0f, score));
}

void scifi_optimize_station_layout(SciFiStationLayout* layout) {
    if (!layout) return;
    
    // Simple optimization: spread out modules that are too close
    for (int i = 0; i < layout->module_count; i++) {
        for (int j = i + 1; j < layout->module_count; j++) {
            vec3 diff = vec3_sub(layout->module_positions[i], layout->module_positions[j]);
            float distance = vec3_length(diff);
            
            if (distance < 5.0f) { // Too close
                vec3 push = vec3_normalize(diff);
                float push_distance = (5.0f - distance) * 0.5f;
                
                layout->module_positions[i] = vec3_add(layout->module_positions[i], 
                                                     vec3_scale(push, push_distance));
                layout->module_positions[j] = vec3_sub(layout->module_positions[j], 
                                                     vec3_scale(push, push_distance));
            }
        }
    }
}

Entity* scifi_spawn_module(const SciFiModuleTemplate* template, vec3 position, vec4 rotation) {
    if (!template) return NULL;
    
    Entity* entity = entity_create();
    if (!entity) return NULL;
    
    // Set transform
    entity->position = position;
    entity->rotation = rotation;
    entity->scale = (vec3){1.0f, 1.0f, 1.0f};
    
    // Create mesh based on module type
    Mesh* mesh = mesh_create();
    if (!mesh) {
        entity_destroy(entity);
        return NULL;
    }
    
    // Generate basic box geometry for module
    mesh_create_box(mesh, template->dimensions);
    
    // Apply materials
    const SciFiMaterialProperties* primary_mat = scifi_get_material_properties(template->primary_material);
    const SciFiMaterialProperties* secondary_mat = scifi_get_material_properties(template->secondary_material);
    
    if (primary_mat) {
        mesh_set_texture(mesh, primary_mat->texture_name);
        mesh_set_material_properties(mesh, primary_mat->base_color, 
                                   primary_mat->roughness, primary_mat->metallic);
    }
    
    // Add mesh component
    entity_add_mesh_component(entity, mesh);
    
    // Add physics component
    PhysicsBody* body = physics_create_body(PHYSICS_BODY_STATIC);
    if (body) {
        physics_set_box_shape(body, template->dimensions);
        physics_set_mass(body, template->mass);
        entity_add_physics_component(entity, body);
    }
    
    // Add module data component
    entity_add_custom_data(entity, "module_template", (void*)template, sizeof(SciFiModuleTemplate));
    
    return entity;
}

Entity* scifi_spawn_station(const SciFiStationLayout* layout) {
    if (!layout) return NULL;
    
    Entity* root = entity_create();
    if (!root) return NULL;
    
    root->position = layout->center_position;
    
    // Spawn all modules
    for (int i = 0; i < layout->module_count; i++) {
        vec4 rotation = quat_from_axis_angle((vec3){0, 1, 0}, layout->module_rotations[i]);
        Entity* module = scifi_spawn_module(&layout->modules[i], 
                                          layout->module_positions[i], rotation);
        if (module) {
            entity_add_child(root, module);
        }
    }
    
    return root;
}

bool scifi_connect_modules(Entity* module_a, Entity* module_b, SciFiConnectionType type, 
                         int connection_point_a, int connection_point_b) {
    if (!module_a || !module_b) return false;
    
    // Get connection points
    SciFiConnectionPoint points_a[16], points_b[16];
    int count_a = scifi_get_module_connection_points(
        (SciFiModuleTemplate*)entity_get_custom_data(module_a, "module_template"), 
        points_a, 16);
    int count_b = scifi_get_module_connection_points(
        (SciFiModuleTemplate*)entity_get_custom_data(module_b, "module_template"), 
        points_b, 16);
    
    if (connection_point_a >= count_a || connection_point_b >= count_b) {
        return false;
    }
    
    // Check if connection points are compatible
    if (points_a[connection_point_a].type != points_b[connection_point_b].type ||
        points_a[connection_point_a].type != type) {
        return false;
    }
    
    // Calculate relative position and rotation for connection
    vec3 connection_offset = vec3_sub(points_b[connection_point_b].position, 
                                     points_a[connection_point_a].position);
    
    // Align modules
    vec4 target_rotation = quat_from_vectors(points_a[connection_point_a].normal, 
                                           vec3_negate(points_b[connection_point_b].normal));
    
    module_b->position = vec3_add(module_a->position, connection_offset);
    module_b->rotation = quat_mul(module_a->rotation, target_rotation);
    
    // Mark connection points as connected
    ((SciFiModuleTemplate*)entity_get_custom_data(module_a, "module_template"))
        ->connections[connection_point_a] = type;
    ((SciFiModuleTemplate*)entity_get_custom_data(module_b, "module_template"))
        ->connections[connection_point_b] = type;
    
    return true;
}

int scifi_get_module_connection_points(const SciFiModuleTemplate* template, 
                                     SciFiConnectionPoint* connection_points, 
                                     int max_connections) {
    if (!template || !connection_points || max_connections <= 0) return 0;
    
    int count = 0;
    
    // Add door positions as connection points
    for (int i = 0; i < template->door_count && count < max_connections; i++) {
        connection_points[count].position = template->door_positions[i];
        connection_points[count].normal = (vec3){0, 0, 1}; // Simplified
        connection_points[count].up = (vec3){0, 1, 0};
        connection_points[count].type = SCIFI_CONNECTION_AIRLOCK;
        connection_points[count].radius = 1.0f;
        connection_points[count].is_connected = false;
        connection_points[count].connected_module_index = -1;
        connection_points[count].connection_index = i;
        count++;
    }
    
    // Add external ports as connection points
    for (int i = 0; i < template->external_port_count && count < max_connections; i++) {
        connection_points[count].position = template->external_ports[i];
        connection_points[count].normal = (vec3){1, 0, 0}; // Simplified
        connection_points[count].up = (vec3){0, 1, 0};
        connection_points[count].type = SCIFI_CONNECTION_POWER_COUPLING;
        connection_points[count].radius = 0.5f;
        connection_points[count].is_connected = false;
        connection_points[count].connected_module_index = -1;
        connection_points[count].connection_index = i;
        count++;
    }
    
    return count;
}

float scifi_calculate_radiation_exposure(const SciFiStationLayout* layout) {
    if (!layout) return 0.0f;
    
    float total_shielding = 0.0f;
    float total_mass = 0.0f;
    
    for (int i = 0; i < layout->module_count; i++) {
        const SciFiModuleTemplate* module = &layout->modules[i];
        total_shielding += module->radiation_shielding * module->mass;
        total_mass += module->mass;
    }
    
    float average_shielding = total_mass > 0 ? total_shielding / total_mass : 0.0f;
    
    // Base radiation in deep space: ~1 mSv/day
    float base_radiation = 365.0f; // mSv/year
    
    // Reduce based on shielding
    return base_radiation * (1.0f - average_shielding);
}

float scifi_simulate_life_support(const SciFiStationLayout* layout, int crew_count, float dt) {
    if (!layout) return 0.0f;
    
    // Calculate life support capacity
    float total_oxygen_generation = 0.0f;
    float total_co2_scrubbing = 0.0f;
    float total_water_recycling = 0.0f;
    
    for (int i = 0; i < layout->module_count; i++) {
        const SciFiModuleTemplate* module = &layout->modules[i];
        
        if (module->has_life_support) {
            // Simplified life support calculations
            total_oxygen_generation += module->crew_capacity * 0.84f; // kg/day per person
            total_co2_scrubbing += module->crew_capacity * 1.0f; // kg/day per person
            total_water_recycling += module->crew_capacity * 50.0f; // liters/day per person
        }
    }
    
    // Calculate requirements
    float oxygen_required = crew_count * 0.84f; // kg/day per person
    float co2_required = crew_count * 1.0f; // kg/day per person
    float water_required = crew_count * 50.0f; // liters/day per person
    
    // Calculate efficiency
    float oxygen_efficiency = total_oxygen_generation > 0 ? 
                             fminf(1.0f, oxygen_required / total_oxygen_generation) : 0.0f;
    float co2_efficiency = total_co2_scrubbing > 0 ? 
                          fminf(1.0f, co2_required / total_co2_scrubbing) : 0.0f;
    float water_efficiency = total_water_recycling > 0 ? 
                           fminf(1.0f, water_required / total_water_recycling) : 0.0f;
    
    // Return overall life support status
    return (oxygen_efficiency + co2_efficiency + water_efficiency) / 3.0f;
}

bool scifi_generate_station_interior(const SciFiStationLayout* layout, Mesh* mesh) {
    if (!layout || !mesh) return false;
    
    // Clear existing mesh
    mesh_clear(mesh);
    
    // Generate interior geometry for each module
    for (int i = 0; i < layout->module_count; i++) {
        const SciFiModuleTemplate* module = &layout->modules[i];
        vec3 position = layout->module_positions[i];
        
        // Generate interior walls
        mesh_create_box(mesh, module->dimensions);
        
        // Add doorways
        for (int j = 0; j < module->door_count; j++) {
            vec3 door_pos = vec3_add(position, module->door_positions[j]);
            mesh_create_doorway(mesh, door_pos, (vec3){2.0f, 2.5f, 0.2f});
        }
        
        // Add windows
        for (int j = 0; j < module->window_count; j++) {
            vec3 window_pos = vec3_add(position, module->window_positions[j]);
            mesh_create_window(mesh, window_pos, (vec3){1.5f, 1.5f, 0.1f});
        }
    }
    
    // Optimize mesh
    mesh_optimize(mesh);
    
    return true;
}
