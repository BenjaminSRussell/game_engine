#include "material_defaults.h"
#include "shader_permutations.h"
#include <stddef.h>

MaterialMaster *material_create_default_pbr(void) {
    MaterialMaster *master = material_master_create("M_PBR_Standard", 
                                                      "assets/shaders/pbr_standard");
    if (!master) return NULL;
    
    // Default PBR Parameters
    f32 white[3] = {1.0f, 1.0f, 1.0f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_COLOR, white);
    
    f32 metallic = 0.0f;
    material_master_add_param(master, "Metallic", MATERIAL_PARAM_FLOAT, &metallic);
    
    f32 roughness = 0.5f;
    material_master_add_param(master, "Roughness", MATERIAL_PARAM_FLOAT, &roughness);
    
    f32 ao = 1.0f;
    material_master_add_param(master, "AO", MATERIAL_PARAM_FLOAT, &ao);
    
    // Texture Slots
    TextureID null_tex = {0};
    material_master_add_texture(master, "BaseColorMap", null_tex);
    material_master_add_texture(master, "NormalMap", null_tex);
    material_master_add_texture(master, "RoughnessMap", null_tex);
    material_master_add_texture(master, "MetallicMap", null_tex);
    material_master_add_texture(master, "AOMap", null_tex);
    
    // Supported Features
    material_master_set_features(master, 
        SHADER_FEATURE_NORMAL_MAP |
        SHADER_FEATURE_ROUGHNESS_MAP |
        SHADER_FEATURE_METALLIC_MAP |
        SHADER_FEATURE_AO_MAP
    );
    
    return master;
}

MaterialMaster *material_create_transparent(void) {
    // Re-use PBR shader but with alpha blending enabled
    MaterialMaster *master = material_master_create("M_Transparent", 
                                                      "assets/shaders/pbr_standard");
    if (!master) return NULL;
    
    // Parameters
    f32 white_alpha[4] = {1.0f, 1.0f, 1.0f, 0.5f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_COLOR, white_alpha);
    
    f32 roughness = 0.1f; // Glass-like
    material_master_add_param(master, "Roughness", MATERIAL_PARAM_FLOAT, &roughness);
    
    f32 metallic = 0.0f;
    material_master_add_param(master, "Metallic", MATERIAL_PARAM_FLOAT, &metallic);
    
    // Textures
    TextureID null_tex = {0};
    material_master_add_texture(master, "BaseColorMap", null_tex);
    material_master_add_texture(master, "NormalMap", null_tex);
    
    // Enable Alpha Blend
    master->alpha_blend = true;
    
    // Features (include Alpha Blend feature for shader #defines if needed)
    material_master_set_features(master, 
        SHADER_FEATURE_NORMAL_MAP |
        SHADER_FEATURE_ALPHA_BLEND
    );
    
    return master;
}

MaterialMaster *material_create_emissive(void) {
    MaterialMaster *master = material_master_create("M_Emissive", 
                                                      "assets/shaders/pbr_standard");
    if (!master) return NULL;
    
    f32 black[3] = {0.0f, 0.0f, 0.0f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_COLOR, black);
    
    f32 emissive_color[3] = {1.0f, 1.0f, 1.0f};
    material_master_add_param(master, "EmissiveColor", MATERIAL_PARAM_COLOR, emissive_color);
    
    f32 intensity = 1.0f;
    material_master_add_param(master, "EmissiveIntensity", MATERIAL_PARAM_FLOAT, &intensity);
    
    TextureID null_tex = {0};
    material_master_add_texture(master, "EmissiveMap", null_tex);
    
    material_master_set_features(master, 
        SHADER_FEATURE_EMISSIVE
    );
    
    return master;
}

MaterialMaster *material_create_foliage(void) {
    MaterialMaster *master = material_master_create("M_Foliage", 
                                                      "assets/shaders/pbr_standard");
    if (!master) return NULL;
    
    f32 green[3] = {0.1f, 0.8f, 0.1f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_COLOR, green);
    
    TextureID null_tex = {0};
    material_master_add_texture(master, "BaseColorMap", null_tex);
    
    // Double sided + Alpha Test
    master->two_sided = true;
    master->alpha_test = true;
    
    material_master_set_features(master, 
        SHADER_FEATURE_TWO_SIDED | 
        SHADER_FEATURE_ALPHA_TEST |
        SHADER_FEATURE_NORMAL_MAP
    );
    
    return master;
}
