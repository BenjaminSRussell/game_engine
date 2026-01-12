#include "rule_based_generation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static bool evaluate_simple_condition(const char* condition, const RuleVariable* variables, uint32_t var_count) {
    // Simple condition evaluation - would use proper expression parser in production
    if (strstr(condition, "temperature >")) {
        float threshold = 20.0f; // Default
        sscanf(condition, "temperature > %f", &threshold);
        
        for (uint32_t i = 0; i < var_count; i++) {
            if (strcmp(variables[i].variable, "temperature") == 0) {
                float temp;
                sscanf(variables[i].value, "%f", &temp);
                return temp > threshold;
            }
        }
    } else if (strstr(condition, "humidity <")) {
        float threshold = 50.0f; // Default
        sscanf(condition, "humidity < %f", &threshold);
        
        for (uint32_t i = 0; i < var_count; i++) {
            if (strcmp(variables[i].variable, "humidity") == 0) {
                float humidity;
                sscanf(variables[i].value, "%f", &humidity);
                return humidity < threshold;
            }
        }
    } else if (strstr(condition, "elevation >")) {
        float threshold = 100.0f; // Default
        sscanf(condition, "elevation > %f", &threshold);
        
        for (uint32_t i = 0; i < var_count; i++) {
            if (strcmp(variables[i].variable, "elevation") == 0) {
                float elevation;
                sscanf(variables[i].value, "%f", &elevation);
                return elevation > threshold;
            }
        }
    } else if (strstr(condition, "biome ==")) {
        char target_biome[32];
        sscanf(condition, "biome == %s", target_biome);
        
        for (uint32_t i = 0; i < var_count; i++) {
            if (strcmp(variables[i].variable, "biome") == 0) {
                return strcmp(variables[i].value, target_biome) == 0;
            }
        }
    }
    
    return false;
}

static bool execute_simple_action(RuleBasedGenerator* generator, const char* action, const RuleVariable* context) {
    if (strstr(action, "place_tree")) {
        Vec3 position = {0};
        for (uint32_t i = 0; i < 10; i++) { // Try to find context position
            char var_name[64];
            sprintf(var_name, "position_%u", i);
            for (uint32_t j = 0; j < 10; j++) {
                if (strcmp(context[j].variable, var_name) == 0) {
                    sscanf(context[j].value, "%f,%f,%f", &position.x, &position.y, &position.z);
                    break;
                }
            }
        }
        return rule_place_object(generator, "tree", &position);
    } else if (strstr(action, "place_rock")) {
        Vec3 position = {0};
        for (uint32_t i = 0; i < 10; i++) {
            char var_name[64];
            sprintf(var_name, "position_%u", i);
            for (uint32_t j = 0; j < 10; j++) {
                if (strcmp(context[j].variable, var_name) == 0) {
                    sscanf(context[j].value, "%f,%f,%f", &position.x, &position.y, &position.z);
                    break;
                }
            }
        }
        return rule_place_object(generator, "rock", &position);
    } else if (strstr(action, "place_building")) {
        Vec3 position = {0};
        for (uint32_t i = 0; i < 10; i++) {
            char var_name[64];
            sprintf(var_name, "position_%u", i);
            for (uint32_t j = 0; j < 10; j++) {
                if (strcmp(context[j].variable, var_name) == 0) {
                    sscanf(context[j].value, "%f,%f,%f", &position.x, &position.y, &position.z);
                    break;
                }
            }
        }
        return rule_place_object(generator, "building", &position);
    }
    
    return false;
}

RuleBasedGenerator* rule_create_generator(uint32_t seed) {
    RuleBasedGenerator* generator = malloc(sizeof(RuleBasedGenerator));
    if (!generator) return NULL;
    
    generator->rules = NULL;
    generator->rule_count = 0;
    generator->content_types = NULL;
    generator->content_type_count = 0;
    generator->regions = NULL;
    generator->region_count = 0;
    generator->objects = NULL;
    generator->object_count = 0;
    generator->max_objects = 10000;
    generator->seed = seed;
    
    generator->objects = malloc(generator->max_objects * sizeof(GameObject));
    if (!generator->objects) {
        free(generator);
        return NULL;
    }
    
    return generator;
}

bool rule_add_rule(RuleBasedGenerator* generator, const char* condition, const char* action, float priority) {
    if (!generator || !condition || !action) return false;
    
    Rule* new_rules = realloc(generator->rules, (generator->rule_count + 1) * sizeof(Rule));
    if (!new_rules) return false;
    
    generator->rules = new_rules;
    Rule* rule = &generator->rules[generator->rule_count];
    
    strncpy(rule->condition, condition, 255);
    rule->condition[255] = '\0';
    strncpy(rule->action, action, 255);
    rule->action[255] = '\0';
    rule->priority = priority;
    rule->enabled = true;
    rule->rule_id = generator->rule_count;
    
    generator->rule_count++;
    return true;
}

bool rule_register_content_type(RuleBasedGenerator* generator, const ContentType* content_type) {
    if (!generator || !content_type) return false;
    
    ContentType* new_types = realloc(generator->content_types, (generator->content_type_count + 1) * sizeof(ContentType));
    if (!new_types) return false;
    
    generator->content_types = new_types;
    generator->content_types[generator->content_type_count] = *content_type;
    generator->content_type_count++;
    
    return true;
}

bool rule_add_region(RuleBasedGenerator* generator, const Region* region) {
    if (!generator || !region) return false;
    
    Region* new_regions = realloc(generator->regions, (generator->region_count + 1) * sizeof(Region));
    if (!new_regions) return false;
    
    generator->regions = new_regions;
    generator->regions[generator->region_count] = *region;
    generator->region_count++;
    
    return true;
}

bool rule_evaluate_condition(const char* condition, const RuleVariable* variables, uint32_t var_count) {
    if (!condition || !variables) return false;
    
    return evaluate_simple_condition(condition, variables, var_count);
}

bool rule_execute_action(RuleBasedGenerator* generator, const char* action, const RuleVariable* context) {
    if (!generator || !action || !context) return false;
    
    return execute_simple_action(generator, action, context);
}

bool rule_process_all_rules(RuleBasedGenerator* generator) {
    if (!generator) return false;
    
    for (uint32_t r = 0; r < generator->region_count; r++) {
        Region* region = &generator->regions[r];
        
        // Create context variables for this region
        RuleVariable context[10];
        uint32_t var_count = 0;
        
        strcpy(context[var_count].variable, "temperature");
        sprintf(context[var_count].value, "%.2f", region->temperature);
        var_count++;
        
        strcpy(context[var_count].variable, "humidity");
        sprintf(context[var_count].value, "%.2f", region->humidity);
        var_count++;
        
        strcpy(context[var_count].variable, "elevation");
        sprintf(context[var_count].value, "%.2f", region->elevation);
        var_count++;
        
        strcpy(context[var_count].variable, "biome");
        strcpy(context[var_count].value, region->biome_type);
        var_count++;
        
        // Evaluate all rules for this region
        for (uint32_t i = 0; i < generator->rule_count; i++) {
            Rule* rule = &generator->rules[i];
            if (!rule->enabled) continue;
            
            if (rule_evaluate_condition(rule->condition, context, var_count)) {
                // Add position context for action execution
                Vec3 test_pos = rule_random_position_in_region(generator, region);
                strcpy(context[var_count].variable, "position_0");
                sprintf(context[var_count].value, "%.2f,%.2f,%.2f", test_pos.x, test_pos.y, test_pos.z);
                
                rule_execute_action(generator, rule->action, context);
            }
        }
    }
    
    return true;
}

bool rule_place_object(RuleBasedGenerator* generator, const char* type_name, const Vec3* position) {
    if (!generator || !type_name || !position) return false;
    
    if (generator->object_count >= generator->max_objects) {
        // Expand object array
        uint32_t new_max = generator->max_objects * 2;
        GameObject* new_objects = realloc(generator->objects, new_max * sizeof(GameObject));
        if (!new_objects) return false;
        
        generator->objects = new_objects;
        generator->max_objects = new_max;
    }
    
    GameObject* obj = &generator->objects[generator->object_count];
    strcpy(obj->name, type_name);
    strcpy(obj->type, type_name);
    obj->position = *position;
    obj->size.x = 1.0f;
    obj->size.y = 1.0f;
    obj->size.z = 1.0f;
    obj->properties = NULL;
    
    generator->object_count++;
    return true;
}

bool rule_generate_objects(RuleBasedGenerator* generator) {
    if (!generator) return false;
    
    return rule_process_all_rules(generator);
}

Region* rule_get_region_at(const RuleBasedGenerator* generator, float x, float z) {
    if (!generator) return NULL;
    
    for (uint32_t i = 0; i < generator->region_count; i++) {
        const Region* region = &generator->regions[i];
        float dx = x - region->x;
        float dz = z - region->z;
        float distance = sqrtf(dx * dx + dz * dz);
        
        if (distance <= region->radius) {
            return (Region*)region;
        }
    }
    
    return NULL;
}

bool rule_check_spatial_constraints(const RuleBasedGenerator* generator, const Vec3* position, const SpatialConstraint* constraints) {
    if (!generator || !position || !constraints) return false;
    
    // Check distance from existing objects
    for (uint32_t i = 0; i < generator->object_count; i++) {
        const GameObject* obj = &generator->objects[i];
        float dx = position->x - obj->position.x;
        float dy = position->y - obj->position.y;
        float dz = position->z - obj->position.z;
        float distance = sqrtf(dx * dx + dy * dy + dz * dz);
        
        if (distance < constraints->min_distance || distance > constraints->max_distance) {
            return false;
        }
    }
    
    return true;
}

bool rule_find_valid_positions(RuleBasedGenerator* generator, const Region* region, const SpatialConstraint* constraints, Vec3* positions, uint32_t max_positions, uint32_t* found_count) {
    if (!generator || !region || !positions || !found_count) return false;
    
    *found_count = 0;
    
    for (uint32_t i = 0; i < max_positions * 10; i++) { // Try 10x more attempts than needed
        Vec3 test_pos = rule_random_position_in_region(generator, region);
        
        if (rule_check_spatial_constraints(generator, &test_pos, constraints)) {
            if (*found_count < max_positions) {
                positions[*found_count] = test_pos;
                (*found_count)++;
            }
        }
    }
    
    return (*found_count > 0);
}

float rule_random_float(RuleBasedGenerator* generator, float min, float max) {
    if (!generator) return 0.0f;
    
    generator->seed = generator->seed * 1103515245 + 12345;
    float normalized = (float)(generator->seed & 0x7fffffff) / (float)0x7fffffff;
    return min + normalized * (max - min);
}

int rule_random_int(RuleBasedGenerator* generator, int min, int max) {
    if (!generator) return 0;
    
    generator->seed = generator->seed * 1103515245 + 12345;
    return min + (generator->seed % (max - min + 1));
}

Vec3 rule_random_position_in_region(RuleBasedGenerator* generator, const Region* region) {
    Vec3 position = {0};
    
    if (!generator || !region) return position;
    
    // Random point in circle
    float angle = rule_random_float(generator, 0.0f, 2.0f * M_PI);
    float radius = sqrtf(rule_random_float(generator, 0.0f, 1.0f)) * region->radius;
    
    position.x = region->x + cosf(angle) * radius;
    position.z = region->z + sinf(angle) * radius;
    position.y = region->elevation; // Use region elevation as Y coordinate
    
    return position;
}

bool rule_position_in_region(const Vec3* position, const Region* region) {
    if (!position || !region) return false;
    
    float dx = position->x - region->x;
    float dz = position->z - region->z;
    float distance = sqrtf(dx * dx + dz * dz);
    
    return distance <= region->radius;
}

GameObject* rule_find_objects_by_type(const RuleBasedGenerator* generator, const char* type_name, uint32_t* count) {
    if (!generator || !type_name || !count) return NULL;
    
    // Count matching objects first
    *count = 0;
    for (uint32_t i = 0; i < generator->object_count; i++) {
        if (strcmp(generator->objects[i].type, type_name) == 0) {
            (*count)++;
        }
    }
    
    if (*count == 0) return NULL;
    
    // Allocate and fill result array
    GameObject* results = malloc(*count * sizeof(GameObject));
    if (!results) {
        *count = 0;
        return NULL;
    }
    
    uint32_t index = 0;
    for (uint32_t i = 0; i < generator->object_count; i++) {
        if (strcmp(generator->objects[i].type, type_name) == 0) {
            results[index++] = generator->objects[i];
        }
    }
    
    return results;
}

bool rule_export_objects(const RuleBasedGenerator* generator, const char* filename, const char* format) {
    if (!generator || !filename || !format) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) return false;
    
    if (strcmp(format, "json") == 0) {
        fprintf(file, "{\n");
        fprintf(file, "  \"objects\": [\n");
        
        for (uint32_t i = 0; i < generator->object_count; i++) {
            const GameObject* obj = &generator->objects[i];
            fprintf(file, "    {\n");
            fprintf(file, "      \"name\": \"%s\",\n", obj->name);
            fprintf(file, "      \"type\": \"%s\",\n", obj->type);
            fprintf(file, "      \"position\": [%.2f, %.2f, %.2f],\n", 
                    obj->position.x, obj->position.y, obj->position.z);
            fprintf(file, "      \"size\": [%.2f, %.2f, %.2f]\n", 
                    obj->size.x, obj->size.y, obj->size.z);
            fprintf(file, "    }%s\n", (i < generator->object_count - 1) ? "," : "");
        }
        
        fprintf(file, "  ]\n");
        fprintf(file, "}\n");
    } else if (strcmp(format, "csv") == 0) {
        fprintf(file, "name,type,x,y,z,sx,sy,sz\n");
        
        for (uint32_t i = 0; i < generator->object_count; i++) {
            const GameObject* obj = &generator->objects[i];
            fprintf(file, "%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                    obj->name, obj->type, obj->position.x, obj->position.y, obj->position.z,
                    obj->size.x, obj->size.y, obj->size.z);
        }
    }
    
    fclose(file);
    return true;
}

void rule_destroy_generator(RuleBasedGenerator* generator) {
    if (!generator) return;
    
    free(generator->rules);
    free(generator->content_types);
    free(generator->regions);
    free(generator->objects);
    free(generator);
}
