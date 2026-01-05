/**
 * =================================================================================================
 *                          PROPERTY INSPECTOR
 * =================================================================================================
 */

#include "property_inspector.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void property_inspector_init(PropertyInspector *inspector) {
    memset(inspector, 0, sizeof(PropertyInspector));
}

void property_inspector_add_group(PropertyInspector *inspector, const char *name) {
    inspector->group_count++;
    inspector->groups = realloc(inspector->groups, sizeof(PropertyGroup) * inspector->group_count);
    PropertyGroup *group = &inspector->groups[inspector->group_count - 1];
    group->group_name = name;
    group->collapsed = false;
    group->properties = NULL;
    group->prop_count = 0;
}

void property_inspector_add_property(PropertyInspector *inspector, int group_idx, Property prop) {
    if (group_idx >= inspector->group_count) return;
    PropertyGroup *group = &inspector->groups[group_idx];
    group->prop_count++;
    group->properties = realloc(group->properties, sizeof(Property) * group->prop_count);
    group->properties[group->prop_count - 1] = prop;
}

bool property_validate(Property *prop) {
    if (!prop->data_ptr) {
        prop->is_valid = false;
        prop->error_msg = "Null data pointer";
        return false;
    }
    
    switch (prop->type) {
        case PROP_TYPE_FLOAT: {
            float val = *(float*)prop->data_ptr;
            if (val < prop->min_val || val > prop->max_val) {
                prop->is_valid = false;
                prop->error_msg = "Value out of range";
                return false;
            }
            break;
        }
        case PROP_TYPE_INT: {
            int val = *(int*)prop->data_ptr;
            if (val < (int)prop->min_val || val > (int)prop->max_val) {
                prop->is_valid = false;
                prop->error_msg = "Value out of range";
                return false;
            }
            break;
        }
        case PROP_TYPE_STRING: {
            const char *str = (const char*)prop->data_ptr;
            if (!str || strlen(str) == 0) {
                prop->is_valid = false;
                prop->error_msg = "Empty string";
                return false;
            }
            break;
        }
        default:
            break;
    }
    
    prop->is_valid = true;
    prop->error_msg = NULL;
    return true;
}

void property_inspector_render(PropertyInspector *inspector) {
    // This would integrate with an actual UI system
    // For each group:
    //   - Render collapsible header
    //   - If expanded, render each property with appropriate widget
    //   - Float: drag slider
    //   - Int: integer input
    //   - Bool: checkbox
    //   - Color: color picker
    //   - String: text input
    //   - AssetRef: drag-drop slot with browse button
    
    for (int g = 0; g < inspector->group_count; g++) {
        PropertyGroup *group = &inspector->groups[g];
        
        // ui_collapsible_header(group->group_name, &group->collapsed);
        
        if (!group->collapsed) {
            for (int p = 0; p < group->prop_count; p++) {
                Property *prop = &group->properties[p];
                
                // Validate before rendering
                property_validate(prop);
                
                // Render appropriate widget based on type
                switch (prop->type) {
                    case PROP_TYPE_FLOAT:
                        // ui_drag_float(prop->name, (float*)prop->data_ptr, prop->min_val, prop->max_val);
                        break;
                    case PROP_TYPE_INT:
                        // ui_drag_int(prop->name, (int*)prop->data_ptr, (int)prop->min_val, (int)prop->max_val);
                        break;
                    case PROP_TYPE_BOOL:
                        // ui_checkbox(prop->name, (bool*)prop->data_ptr);
                        break;
                    case PROP_TYPE_COLOR:
                        // ui_color_picker(prop->name, (float*)prop->data_ptr); // RGBA array
                        break;
                    case PROP_TYPE_STRING:
                        // ui_text_input(prop->name, (char*)prop->data_ptr, 256);
                        break;
                    case PROP_TYPE_ASSET_REF:
                        // ui_asset_slot(prop->name, (uint64_t*)prop->data_ptr);
                        break;
                    default:
                        break;
                }
                
                // Show validation error if invalid
                if (!prop->is_valid && prop->error_msg) {
                    // ui_text_colored(prop->error_msg, RED);
                }
            }
        }
    }
}
