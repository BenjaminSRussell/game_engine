// Example: Material serialization usage

#include "materials/material_serialization.h"
#include "materials/material_instance.h"
#include "core/logger.h"

void example_save_load_materials() {
    // Create master
    MaterialMaster *pbr = material_master_create("M_PBR_Standard", "assets/shaders/pbr_standard");
    
    // Add parameters
    f32 white[3] = {1, 1, 1};
    material_master_add_param(pbr, "BaseColor", MATERIAL_PARAM_VEC3, white);
    material_master_add_param(pbr, "Roughness", MATERIAL_PARAM_FLOAT, &(f32){0.5f});
    material_master_add_param(pbr, "Metallic", MATERIAL_PARAM_FLOAT, &(f32){0.0f});
    
    // Create grass material
    MaterialInstance *grass = material_instance_create(pbr, "M_Grass");
    f32 green[3] = {0.2f, 0.8f, 0.3f};
    material_instance_set_vec3(grass, "BaseColor", green);
    material_instance_set_float(grass, "Roughness", 0.9f);
    material_instance_compile(grass);
    
    // Save to file
    material_save_to_file(grass, "assets/materials/M_Grass.mat");
    
    // Load from file
    MaterialInstance *loaded = material_load_from_file("assets/materials/M_Grass.mat", pbr);
    if (loaded) {
        LOG_INFO("Loaded material: %s", loaded->name);
        
        // Verify parameters
        MaterialParameter *color = material_instance_get_param(loaded, "BaseColor");
        if (color) {
            LOG_INFO("BaseColor: [%.2f, %.2f, %.2f]",
                     color->value.vec3_val[0],
                     color->value.vec3_val[1],
                     color->value.vec3_val[2]);
        }
    }
    
    // Cleanup
    material_instance_destroy(grass);
    material_instance_destroy(loaded);
    material_master_destroy(pbr);
}

void example_property_metadata() {
    MaterialMaster *pbr = material_master_create("M_PBR_Standard", "assets/shaders/pbr_standard");
    
    // Get metadata for editor UI
    u32 count;
    MaterialPropertyMetadata *metadata = material_get_metadata(pbr, &count);
    
    LOG_INFO("Material %s has %u properties:", pbr->name, count);
    for (u32 i = 0; i < count; i++) {
        MaterialPropertyMetadata *prop = &metadata[i];
        LOG_INFO("  - %s (%s)", prop->display_name, prop->tooltip);
        LOG_INFO("    Type: %d, Range: [%.1f, %.1f]",
                 prop->type, prop->min_value, prop->max_value);
        LOG_INFO("    Category: %s, Is Slider: %s",
                 prop->category, prop->is_slider ? "yes" : "no");
    }
    
    material_master_destroy(pbr);
}

// Example: Material editor UI (pseudocode)
void render_material_editor_ui(MaterialInstance *instance) {
    u32 count;
    MaterialPropertyMetadata *metadata = material_get_metadata(instance->parent, &count);
    
    for (u32 i = 0; i < count; i++) {
        MaterialPropertyMetadata *prop = &metadata[i];
        MaterialParameter *param = material_instance_get_param(instance, prop->name);
        
        if (!param) continue;
        
        // Render UI based on metadata
        switch (prop->type) {
            case MATERIAL_PARAM_FLOAT:
                if (prop->is_slider) {
                    // UI_Slider(prop->display_name, &param->value.float_val, 
                    //           prop->min_value, prop->max_value);
                } else {
                    // UI_FloatInput(prop->display_name, &param->value.float_val);
                }
                break;
                
            case MATERIAL_PARAM_VEC3:
                if (prop->is_color) {
                    // UI_ColorPicker(prop->display_name, param->value.vec3_val);
                } else {
                    // UI_Vec3Input(prop->display_name, param->value.vec3_val);
                }
                break;
                
            // ... other types
        }
    }
}
