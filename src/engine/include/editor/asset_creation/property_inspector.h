#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PROP_TYPE_FLOAT,
    PROP_TYPE_INT,
    PROP_TYPE_BOOL,
    PROP_TYPE_STRING,
    PROP_TYPE_COLOR,
    PROP_TYPE_VECTOR3,
    PROP_TYPE_ASSET_REF
} PropertyType;

typedef struct Property {
    const char *name;
    PropertyType type;
    void *data_ptr;
    
    // Metadata
    float min_val, max_val; // For numeric types
    bool is_valid;
    const char *error_msg;
} Property;

typedef struct PropertyGroup {
    const char *group_name;
    bool collapsed;
    Property *properties;
    int prop_count;
} PropertyGroup;

typedef struct PropertyInspector {
    PropertyGroup *groups;
    int group_count;
    
    // Multi-selection
    int selection_count;
    bool values_differ;
} PropertyInspector;

void property_inspector_init(PropertyInspector *inspector);
void property_inspector_add_group(PropertyInspector *inspector, const char *name);
void property_inspector_add_property(PropertyInspector *inspector, int group_idx, Property prop);

// Rendering (stub - would integrate with UI system)
void property_inspector_render(PropertyInspector *inspector);

// Validation
bool property_validate(Property *prop);
