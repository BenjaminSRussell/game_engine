// PBR Material Template Library - Implementation
#include "rendering/materials/material_templates.h"
#include "core/logger.h"

// === Master Material Templates ===

MaterialMaster *material_template_pbr_standard(void) {
    MaterialMaster *master = material_master_create("M_PBR_Standard", 
                                                      "assets/shaders/pbr_standard");
    
    // Default parameters
    f32 white[3] = {1.0f, 1.0f, 1.0f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_VEC3, white);
    
    f32 metallic = 0.0f;
    material_master_add_param(master, "Metallic", MATERIAL_PARAM_FLOAT, &metallic);
    
    f32 roughness = 0.5f;
    material_master_add_param(master, "Roughness", MATERIAL_PARAM_FLOAT, &roughness);
    
    f32 ao = 1.0f;
    material_master_add_param(master, "AO", MATERIAL_PARAM_FLOAT, &ao);
    
    // Texture slots
    TextureID null_tex = {0};
    material_master_add_texture(master, "BaseColorMap", null_tex);
    material_master_add_texture(master, "NormalMap", null_tex);
    material_master_add_texture(master, "RoughnessMap", null_tex);
    material_master_add_texture(master, "MetallicMap", null_tex);
    material_master_add_texture(master, "AOMap", null_tex);
    material_master_add_texture(master, "EmissiveMap", null_tex);
    
    // Supported features
    material_master_set_features(master,
        SHADER_FEATURE_NORMAL_MAP |
        SHADER_FEATURE_ROUGHNESS_MAP |
        SHADER_FEATURE_METALLIC_MAP |
        SHADER_FEATURE_AO_MAP |
        SHADER_FEATURE_EMISSIVE
    );
    
    LOG_INFO("Created PBR Standard master material");
    return master;
}

MaterialMaster *material_template_unlit(void) {
    MaterialMaster *master = material_master_create("M_Unlit", 
                                                      "assets/shaders/unlit");
    
    f32 white[3] = {1.0f, 1.0f, 1.0f};
    material_master_add_param(master, "Color", MATERIAL_PARAM_VEC3, white);
    
    TextureID null_tex = {0};
    material_master_add_texture(master, "ColorMap", null_tex);
    
    LOG_INFO("Created Unlit master material");
    return master;
}

MaterialMaster *material_template_transparent(void) {
    MaterialMaster *master = material_master_create("M_Transparent",
                                                      "assets/shaders/pbr_standard");
    
    // Same as PBR but with alpha blending
    f32 white[3] = {1.0f, 1.0f, 1.0f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_VEC3, white);
    
    f32 alpha = 0.5f;
    material_master_add_param(master, "Alpha", MATERIAL_PARAM_FLOAT, &alpha);
    
    f32 roughness = 0.5f;
    material_master_add_param(master, "Roughness", MATERIAL_PARAM_FLOAT, &roughness);
    
    TextureID null_tex = {0};
    material_master_add_texture(master, "BaseColorMap", null_tex);
    material_master_add_texture(master, "NormalMap", null_tex);
    
    master->alpha_blend = true;
    
    material_master_set_features(master,
        SHADER_FEATURE_ALPHA_BLEND |
        SHADER_FEATURE_NORMAL_MAP
    );
    
    LOG_INFO("Created Transparent master material");
    return master;
}

MaterialMaster *material_template_two_sided(void) {
    MaterialMaster *master = material_template_pbr_standard();
    master->two_sided = true;
    strncpy(master->name, "M_TwoSided", sizeof(master->name));
    
    LOG_INFO("Created TwoSided master material");
    return master;
}

MaterialMaster *material_template_foliage(void) {
    MaterialMaster *master = material_master_create("M_Foliage",
                                                      "assets/shaders/foliage");
    
    f32 green[3] = {0.2f, 0.8f, 0.3f};
    material_master_add_param(master, "BaseColor", MATERIAL_PARAM_VEC3, green);
    
    f32 roughness = 0.8f;
    material_master_add_param(master, "Roughness", MATERIAL_PARAM_FLOAT, &roughness);
    
    f32 wind_strength = 1.0f;
    material_master_add_param(master, "WindStrength", MATERIAL_PARAM_FLOAT, &wind_strength);
    
    f32 subsurface = 0.3f;
    material_master_add_param(master, "SubsurfaceStrength", MATERIAL_PARAM_FLOAT, &subsurface);
    
    TextureID null_tex = {0};
    material_master_add_texture(master, "BaseColorMap", null_tex);
    material_master_add_texture(master, "NormalMap", null_tex);
    
    master->two_sided = true;
    master->alpha_test = true;
    
    material_master_set_features(master,
        SHADER_FEATURE_NORMAL_MAP |
        SHADER_FEATURE_WIND_ANIMATION |
        SHADER_FEATURE_ALPHA_TEST |
        SHADER_FEATURE_TWO_SIDED
    );
    
    LOG_INFO("Created Foliage master material");
    return master;
}

MaterialMaster *material_template_water(void) {
    MaterialMaster *master = material_master_create("M_Water",
                                                      "assets/shaders/water");
    
    f32 blue[3] = {0.1f, 0.3f, 0.6f};
    material_master_add_param(master, "WaterColor", MATERIAL_PARAM_VEC3, blue);
    
    f32 roughness = 0.1f;
    material_master_add_param(master, "Roughness", MATERIAL_PARAM_FLOAT, &roughness);
    
    f32 refraction = 1.33f;
    material_master_add_param(master, "RefractionIndex", MATERIAL_PARAM_FLOAT, &refraction);
    
    f32 flow_speed = 0.5f;
    material_master_add_param(master, "FlowSpeed", MATERIAL_PARAM_FLOAT, &flow_speed);
    
    TextureID null_tex = {0};
    material_master_add_texture(master, "NormalMap", null_tex);
    material_master_add_texture(master, "FlowMap", null_tex);
    
    master->alpha_blend = true;
    
    material_master_set_features(master,
        SHADER_FEATURE_NORMAL_MAP |
        SHADER_FEATURE_ALPHA_BLEND
    );
    
    LOG_INFO("Created Water master material");
    return master;
}

// === Instance Creation Helpers ===

MaterialInstance *material_create_grass(MaterialMaster *pbr) {
    MaterialInstance *grass = material_instance_create(pbr, "M_Grass");
    
    f32 green[3] = {0.2f, 0.8f, 0.3f};
    material_instance_set_vec3(grass, "BaseColor", green);
    material_instance_set_float(grass, "Roughness", 0.9f);
    material_instance_set_float(grass, "Metallic", 0.0f);
    
    material_instance_compile(grass);
    return grass;
}

MaterialInstance *material_create_metal(MaterialMaster *pbr) {
    MaterialInstance *metal = material_instance_create(pbr, "M_Steel");
    
    f32 gray[3] = {0.7f, 0.7f, 0.7f};
    material_instance_set_vec3(metal, "BaseColor", gray);
    material_instance_set_float(metal, "Roughness", 0.2f);
    material_instance_set_float(metal, "Metallic", 1.0f);
    
    material_instance_compile(metal);
    return metal;
}

MaterialInstance *material_create_wood(MaterialMaster *pbr) {
    MaterialInstance *wood = material_instance_create(pbr, "M_Wood");
    
    f32 brown[3] = {0.4f, 0.25f, 0.15f};
    material_instance_set_vec3(wood, "BaseColor", brown);
    material_instance_set_float(wood, "Roughness", 0.7f);
    material_instance_set_float(wood, "Metallic", 0.0f);
    
    material_instance_compile(wood);
    return wood;
}

MaterialInstance *material_create_stone(MaterialMaster *pbr) {
    MaterialInstance *stone = material_instance_create(pbr, "M_Stone");
    
    f32 gray[3] = {0.5f, 0.5f, 0.5f};
    material_instance_set_vec3(stone, "BaseColor", gray);
    material_instance_set_float(stone, "Roughness", 0.8f);
    material_instance_set_float(stone, "Metallic", 0.0f);
    
    material_instance_compile(stone);
    return stone;
}
