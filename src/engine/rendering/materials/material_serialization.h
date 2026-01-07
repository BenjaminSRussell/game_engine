// Material Serialization - JSON save/load for materials
#ifndef MATERIAL_SERIALIZATION_H
#define MATERIAL_SERIALIZATION_H

#include "rendering/materials/material_instance.h"

#ifdef __cplusplus
extern "C" {
#endif

// Material asset file format (.mat)
/*
{
  "material": {
    "name": "M_Grass",
    "parent": "M_PBR_Standard",
    "parameters": {
      "BaseColor": [0.2, 0.8, 0.3],
      "Roughness": 0.9,
      "Metallic": 0.0
    },
    "textures": {
      "NormalMap": "textures/grass_normal.png",
      "RoughnessMap": null
    },
    "features": ["NORMAL_MAP"],
    "quality": "HIGH"
  }
}
*/

// Save material instance to JSON file
bool material_save_to_file(MaterialInstance *instance, const char *filepath);

// Load material instance from JSON file
MaterialInstance *material_load_from_file(const char *filepath, MaterialMaster *parent);

// Save material master to JSON
bool material_master_save_to_file(MaterialMaster *master, const char *filepath);

// Load material master from JSON
MaterialMaster *material_master_load_from_file(const char *filepath);

// Serialize to JSON string (for debugging)
char *material_instance_to_json(MaterialInstance *instance);

// Property metadata for editor UI
typedef struct MaterialPropertyMetadata {
    const char *name;
    const char *display_name;
    const char *tooltip;
    MaterialParameterType type;
    
    // Editor hints
    f32 min_value;
    f32 max_value;
    bool is_color;
    bool is_slider;
    const char *category;  // "Base", "Advanced", etc.
} MaterialPropertyMetadata;

// Get metadata for all parameters
MaterialPropertyMetadata *material_get_metadata(MaterialMaster *master, u32 *out_count);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_SERIALIZATION_H
