#ifndef RULE_BASED_GENERATION_H
#define RULE_BASED_GENERATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    char name[64];
    char type[32];
    Vec3 position;
    Vec3 size;
    void* properties;
} GameObject;

typedef struct {
    char condition[256];
    char action[256];
    float priority;
    bool enabled;
    uint32_t rule_id;
} Rule;

typedef struct {
    char name[64];
    char type[32];
    float min_size;
    float max_size;
    float min_spacing;
    float max_spacing;
    uint32_t max_instances;
    char required_tags[8][32];
    char excluded_tags[8][32];
    uint32_t required_tag_count;
    uint32_t excluded_tag_count;
} ContentType;

typedef struct {
    char name[64];
    float x, z;
    float radius;
    char biome_type[32];
    float temperature;
    float humidity;
    float elevation;
    char tags[16][32];
    uint32_t tag_count;
} Region;

typedef struct {
    Rule* rules;
    uint32_t rule_count;
    ContentType* content_types;
    uint32_t content_type_count;
    Region* regions;
    uint32_t region_count;
    GameObject* objects;
    uint32_t object_count;
    uint32_t max_objects;
    uint32_t seed;
} RuleBasedGenerator;

// Rule system functions
RuleBasedGenerator* rule_create_generator(uint32_t seed);
bool rule_add_rule(RuleBasedGenerator* generator, const char* condition, const char* action, float priority);
bool rule_remove_rule(RuleBasedGenerator* generator, uint32_t rule_id);
bool rule_enable_rule(RuleBasedGenerator* generator, uint32_t rule_id, bool enabled);
bool rule_modify_rule(RuleBasedGenerator* generator, uint32_t rule_id, const char* new_condition, const char* new_action);

// Content type management
bool rule_register_content_type(RuleBasedGenerator* generator, const ContentType* content_type);
bool rule_unregister_content_type(RuleBasedGenerator* generator, const char* type_name);
ContentType* rule_get_content_type(const RuleBasedGenerator* generator, const char* type_name);

// Region management
bool rule_add_region(RuleBasedGenerator* generator, const Region* region);
bool rule_remove_region(RuleBasedGenerator* generator, const char* region_name);
Region* rule_get_region_at(const RuleBasedGenerator* generator, float x, float z);
bool rule_update_region_properties(RuleBasedGenerator* generator, const char* region_name, float temperature, float humidity);

// Rule evaluation and execution
typedef struct {
    char variable[64];
    char value[128];
} RuleVariable;

bool rule_evaluate_condition(const char* condition, const RuleVariable* variables, uint32_t var_count);
bool rule_execute_action(RuleBasedGenerator* generator, const char* action, const RuleVariable* context);
bool rule_process_all_rules(RuleBasedGenerator* generator);

// Object generation
bool rule_generate_objects(RuleBasedGenerator* generator);
bool rule_place_object(RuleBasedGenerator* generator, const char* type_name, const Vec3* position);
bool rule_remove_object(RuleBasedGenerator* generator, uint32_t object_id);
GameObject* rule_find_objects_by_type(const RuleBasedGenerator* generator, const char* type_name, uint32_t* count);
GameObject* rule_find_objects_in_region(const RuleBasedGenerator* generator, const Region* region, uint32_t* count);

// Spatial queries and constraints
typedef struct {
    float min_distance;
    float max_distance;
    bool require_line_of_sight;
    char required_nearby_types[8][32];
    char excluded_nearby_types[8][32];
    uint32_t required_nearby_count;
    uint32_t excluded_nearby_count;
} SpatialConstraint;

bool rule_check_spatial_constraints(const RuleBasedGenerator* generator, const Vec3* position, const SpatialConstraint* constraints);
bool rule_find_valid_positions(RuleBasedGenerator* generator, const Region* region, const SpatialConstraint* constraints, Vec3* positions, uint32_t max_positions, uint32_t* found_count);

// Rule templates and presets
typedef struct {
    char name[64];
    Rule* rules;
    uint32_t rule_count;
    ContentType* content_types;
    uint32_t content_type_count;
} RuleTemplate;

bool rule_load_template(RuleBasedGenerator* generator, const RuleTemplate* template);
bool rule_save_template(const RuleBasedGenerator* generator, RuleTemplate* template, const char* name);
bool rule_apply_preset(RuleBasedGenerator* generator, const char* preset_name);

// Advanced rule features
typedef struct {
    uint32_t condition_hash;
    uint32_t action_hash;
    uint32_t evaluation_count;
    float total_evaluation_time;
    uint32_t execution_count;
    float total_execution_time;
} RulePerformanceStats;

RulePerformanceStats rule_get_rule_stats(const RuleBasedGenerator* generator, uint32_t rule_id);
bool rule_optimize_rules(RuleBasedGenerator* generator);
bool rule_validate_rules(RuleBasedGenerator* generator);

// Rule debugging and visualization
bool rule_enable_debug_mode(RuleBasedGenerator* generator, bool enabled);
bool rule_log_rule_evaluation(RuleBasedGenerator* generator, uint32_t rule_id, bool result, float time);
bool rule_export_rule_graph(const RuleBasedGenerator* generator, const char* filename);

// Integration with other systems
bool rule_integrate_with_terrain(RuleBasedGenerator* generator, const void* terrain_data);
bool rule_integrate_with_climate(RuleBasedGenerator* generator, const void* climate_data);
bool rule_integrate_with_ecosystem(RuleBasedGenerator* generator, const void* ecosystem_data);

// Export and import
bool rule_export_generator_state(const RuleBasedGenerator* generator, const char* filename);
bool rule_import_generator_state(RuleBasedGenerator* generator, const char* filename);
bool rule_export_objects(const RuleBasedGenerator* generator, const char* filename, const char* format);

// Utility functions
float rule_random_float(RuleBasedGenerator* generator, float min, float max);
int rule_random_int(RuleBasedGenerator* generator, int min, int max);
Vec3 rule_random_position_in_region(RuleBasedGenerator* generator, const Region* region);
bool rule_position_in_region(const Vec3* position, const Region* region);

// Cleanup
void rule_destroy_generator(RuleBasedGenerator* generator);
void rule_destroy_template(RuleTemplate* template);

#ifdef __cplusplus
}
#endif

#endif // RULE_BASED_GENERATION_H
