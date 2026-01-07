// Complete Material System Example - Hot-Reload + Templates

#include "rendering/materials/material_templates.h"
#include "rendering/materials/material_hot_reload.h"
#include "rendering/materials/material_serialization.h"
#include "core/logger.h"

void example_complete_material_system() {
    LOG_INFO("=== Complete Material System Demo ===");
    
    // 1. Create template library
    MaterialMaster *pbr = material_template_pbr_standard();
    MaterialMaster *unlit = material_template_unlit();
    MaterialMaster *transparent = material_template_transparent();
    MaterialMaster *foliage = material_template_foliage();
    MaterialMaster *water = material_template_water();
    
    // 2. Create material instances using helpers
    MaterialInstance *grass = material_create_grass(pbr);
    MaterialInstance *metal = material_create_metal(pbr);
    MaterialInstance *wood = material_create_wood(pbr);
    MaterialInstance *stone = material_create_stone(pbr);
    
    // 3. Save materials to files
    material_save_to_file(grass, "assets/materials/M_Grass.mat");
    material_save_to_file(metal, "assets/materials/M_Steel.mat");
    material_save_to_file(wood, "assets/materials/M_Wood.mat");
    material_save_to_file(stone, "assets/materials/M_Stone.mat");
    
    // 4. Setup hot-reload system
    MaterialHotReloader *reloader = material_hot_reload_create();
    material_hot_reload_start(reloader, "assets/materials");
    
    // Register materials for hot-reload
    material_hot_reload_register(reloader, grass);
    material_hot_reload_register(reloader, metal);
    material_hot_reload_register(reloader, wood);
    material_hot_reload_register(reloader, stone);
    
    LOG_INFO("Material system initialized with hot-reload");
    
    // 5. Game loop (simplified)
    for (int frame = 0; frame < 10; frame++) {
        // Update hot-reload (checks for file changes)
        material_hot_reload_update(reloader);
        
        // Render using materials
        // material_instance_bind(grass);
        // material_instance_upload_params(grass);
        // draw_mesh(grass_mesh);
        
        // If artist edits M_Grass.mat while game is running,
        // it will automatically reload!
    }
    
    // 6. Cleanup
    material_hot_reload_unregister(reloader, grass);
    material_hot_reload_stop(reloader);
    material_hot_reload_destroy(reloader);
    
    material_instance_destroy(grass);
    material_instance_destroy(metal);
    material_instance_destroy(wood);
    material_instance_destroy(stone);
    
    material_master_destroy(pbr);
    material_master_destroy(unlit);
    material_master_destroy(transparent);
    material_master_destroy(foliage);
    material_master_destroy(water);
    
    LOG_INFO("=== Material System Demo Complete ===");
}

// Example: Artist workflow
void example_artist_workflow() {
    LOG_INFO("=== Artist Workflow Example ===");
    
    // Artist creates new material by editing JSON
    // assets/materials/M_CustomRock.mat:
    /*
    {
      "material": {
        "name": "M_CustomRock",
        "parent": "M_PBR_Standard",
        "parameters": {
          "BaseColor": [0.6, 0.55, 0.5],
          "Roughness": 0.85,
          "Metallic": 0.0
        },
        "textures": {
          "NormalMap": "textures/rock_normal.png",
          "RoughnessMap": "textures/rock_roughness.png"
        }
      }
    }
    */
    
    // Load in code
    MaterialMaster *pbr = material_template_pbr_standard();
    MaterialInstance *custom_rock = material_load_from_file("assets/materials/M_CustomRock.mat", pbr);
    
    if (custom_rock) {
        LOG_INFO("Loaded custom rock material!");
        
        // Artist can keep editing M_CustomRock.mat
        // Changes hot-reload automatically
        
        material_instance_destroy(custom_rock);
    }
    
    material_master_destroy(pbr);
}

// Example: Complete rendering integration
void example_render_with_materials(MaterialMaster *pbr) {
    // Create materials for scene
    MaterialInstance *terrain_grass = material_create_grass(pbr);
    MaterialInstance *rock = material_create_stone(pbr);
    MaterialInstance *tree_bark = material_create_wood(pbr);
    
    // Pseudocode render loop
    /*
    for each mesh in scene:
        // Select material
        MaterialInstance *mat = mesh->material;
        
        // Bind shader variant
        material_instance_bind(mat);
        
        // Upload material parameters
        material_instance_upload_params(mat);
        
        // Upload transform uniforms
        glUniformMatrix4fv(mat->active_variant->uniforms.model, ...);
        glUniformMatrix4fv(mat->active_variant->uniforms.view, ...);
        glUniformMatrix4fv(mat->active_variant->uniforms.projection, ...);
        
        // Draw
        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
    */
    
    material_instance_destroy(terrain_grass);
    material_instance_destroy(rock);
    material_instance_destroy(tree_bark);
}
