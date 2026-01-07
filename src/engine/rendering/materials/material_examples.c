// Example: Creating PBR Materials with the new system

#include "rendering/materials/material_instance.h"
#include "rendering/materials/shader_permutations.h"
#include "core/logger.h"

// Create standard PBR master material
MaterialMaster *create_pbr_standard_master() {
    MaterialMaster *master = material_master_create("M_PBR_Standard", 
                                                      "assets/shaders/pbr_standard");
    
    // Add default parameters
    f32 white[3] = {1.0f, 1.0f, 1.0f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_VEC3, white);
    
    f32 metallic = 0.0f;
    material_master_add_param(master, "Metallic", MATERIAL_PARAM_FLOAT, &metallic);
    
    f32 roughness = 0.5f;
    material_master_add_param(master, "Roughness", MATERIAL_PARAM_FLOAT, &roughness);
    
    f32 ao = 1.0f;
    material_master_add_param(master, "AO", MATERIAL_PARAM_FLOAT, &ao);
    
    // Add texture slots (NULL = no default texture)
    TextureID null_tex = {0};
    material_master_add_texture(master, "BaseColorMap", null_tex);
    material_master_add_texture(master, "NormalMap", null_tex);
    material_master_add_texture(master, "RoughnessMap", null_tex);
    material_master_add_texture(master, "MetallicMap", null_tex);
    material_master_add_texture(master, "AOMap", null_tex);
    material_master_add_texture(master, "EmissiveMap", null_tex);
    
    // Set supported features
    material_master_set_features(master, 
        SHADER_FEATURE_NORMAL_MAP |
        SHADER_FEATURE_ROUGHNESS_MAP |
        SHADER_FEATURE_METALLIC_MAP |
        SHADER_FEATURE_AO_MAP |
        SHADER_FEATURE_EMISSIVE
    );
    
    return master;
}

// Example: Create grass material instance
void example_create_grass_material(MaterialMaster *pbr_master) {
    MaterialInstance *grass = material_instance_create(pbr_master, "M_Grass");
    
    // Override base color (green)
    f32 green[3] = {0.2f, 0.8f, 0.3f};
    material_instance_set_vec3(grass, "BaseColor", green);
    
    // High roughness for grass
    material_instance_set_float(grass, "Roughness", 0.9f);
    
    // No metallic
    material_instance_set_float(grass, "Metallic", 0.0f);
    
    // Load textures (example IDs)
    TextureID grass_normal = {123};  // Would come from texture manager
    material_instance_set_texture(grass, "NormalMap", grass_normal);
    
    // Compile shader variant
    material_instance_compile(grass);
    
    // Use in rendering
    // material_instance_bind(grass);
    // material_instance_upload_params(grass);
    // draw_mesh();
    
    LOG_INFO("Created grass material with shader variant (features: 0x%X)", 
             grass->active_permutation.features);
}

// Example: Create metal material instance
void example_create_metal_material(MaterialMaster *pbr_master) {
    MaterialInstance *metal = material_instance_create(pbr_master, "M_Steel");
    
    // Gray base color
    f32 gray[3] = {0.7f, 0.7f, 0.7f};
    material_instance_set_vec3(metal, "BaseColor", gray);
    
    // Full metallic
    material_instance_set_float(metal, "Metallic", 1.0f);
    
    // Low roughness (shiny)
    material_instance_set_float(metal, "Roughness", 0.2f);
    
    // With textures
    TextureID metal_roughness = {456};
    material_instance_set_texture(metal, "RoughnessMap", metal_roughness);
    
    TextureID metal_normal = {789};
    material_instance_set_texture(metal, "NormalMap", metal_normal);
    
    // This will automatically compile with NORMAL_MAP + ROUGHNESS_MAP features
    material_instance_compile(metal);
}

// Example usage in renderer
void render_scene_with_materials() {
    // Create master material once
    MaterialMaster *pbr_master = create_pbr_standard_master();
    
    // Create instances for different objects
    MaterialInstance *grass = material_instance_create(pbr_master, "M_Grass");
    f32 green[3] = {0.2f, 0.8f, 0.3f};
    material_instance_set_vec3(grass, "BaseColor", green);
    
    MaterialInstance *rock = material_instance_create(pbr_master, "M_Rock");
    f32 gray[3] = {0.5f, 0.5f, 0.5f};
    material_instance_set_vec3(rock, "BaseColor", gray);
    material_instance_set_float(rock, "Roughness", 0.8f);
    
    // Render loop
    // for each mesh:
    //   material_instance_bind(mesh->material);
    //   material_instance_upload_params(mesh->material);
    //   set_transform_uniforms();
    //   draw_mesh();
    
    // Cleanup
    material_instance_destroy(grass);
    material_instance_destroy(rock);
    material_master_destroy(pbr_master);
}
