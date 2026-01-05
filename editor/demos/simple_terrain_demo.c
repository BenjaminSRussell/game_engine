#include "../editor_common.h"
#include <malloc.h> // For calloc

/**
 * =================================================================================================
 *                           SIMPLE TERRAIN DEMO
 * =================================================================================================
 * 
 * A functional demo that integrates:
 * 1. Editor Context
 * 2. Terrain Sculpting System
 * 3. Material Node Graph
 * 
 * It runs a simulation loop and prints status, verifying the architecture works.
 */

// Renamed to prevent multiple main() definitions
int run_simple_terrain_demo() {
    LOG_INFO("Starting Simple Terrain Demo...");
    
    // 1. Initialize Context
    EditorContext* ctx = editor_context_create();
    if (!editor_context_init(ctx)) {
        LOG_ERROR("Failed to init context");
        return 1;
    }
    
    // 2. Setup Terrain System
    TerrainHeightmap heightmap;
    heightmap.width = 100;
    heightmap.height = 100;
    heightmap.heights = (f32*)calloc(100 * 100, sizeof(f32));
    heightmap.scale_x = 1.0f;
    heightmap.scale_z = 1.0f;
    heightmap.height_scale = 10.0f;
    
    TerrainSculptingSystem terrain_sys = {0};
    terrain_sys.heightmap = &heightmap;
    terrain_sys.current_brush.radius = 5.0f;
    terrain_sys.current_brush.strength = 0.5f;
    terrain_sys.current_brush.operation = SCULPT_RAISE;
    terrain_sys.current_brush.falloff = FALLOFF_SMOOTH;
    terrain_sys.history_capacity = 10;
    
    ctx->terrain_system = &terrain_sys;
    
    // Test Sculpting
    LOG_INFO("Applying Raise Brush at (50, 50)...");
    terrain_sculpting_apply_brush(&terrain_sys, (Vec3){50.0f, 0.0f, 50.0f}); // Adjusted to match signature taking Vec3
    
    // Verify changes
    f32 center_height = heightmap.heights[50 * 100 + 50];
    LOG_INFO("Height at (50,50) after sculpt: %.4f", center_height);
    
    if (center_height > 0.0f) {
        LOG_INFO("✅ Sculpting Verification: SUCCESS");
    } else {
        LOG_ERROR("❌ Sculpting Verification: FAILURE");
    }
    
    // Test Undo
    LOG_INFO("Testing Undo...");
    terrain_sculpting_undo(&terrain_sys);
    center_height = heightmap.heights[50 * 100 + 50];
    LOG_INFO("Height after undo: %.4f", center_height);
    
    if (center_height == 0.0f) {
        LOG_INFO("✅ Undo Verification: SUCCESS");
    } else {
        LOG_ERROR("❌ Undo Verification: FAILURE (Expected 0.0)");
    }
    
    // 3. Setup Material System
    LOG_INFO("Creating Material Graph...");
    MaterialGraph* graph = material_graph_create("Demo Material");
    ctx->material_graph = graph;
    
    MaterialNode* node_tex = material_graph_add_node(graph, NODE_TYPE_TEXTURE_SAMPLE);
    MaterialNode* node_mult = material_graph_add_node(graph, NODE_TYPE_MULTIPLY);
    MaterialNode* node_master = material_graph_add_node(graph, NODE_TYPE_PBR_MASTER);
    
    // Connect: Texture -> Multiply -> Master Albedo
    LOG_INFO("Connecting Nodes...");
    bool linked = true;
    linked &= material_graph_connect_nodes(graph, node_tex->node_id, 0, node_mult->node_id, 0);
    linked &= material_graph_connect_nodes(graph, node_mult->node_id, 0, node_master->node_id, 0);
    
    if (linked) {
        LOG_INFO("✅ Connection Verification: SUCCESS");
    } else {
        LOG_ERROR("❌ Connection Verification: FAILURE");
    }
    
    // Test Compilation
    LOG_INFO("Compiling Shader...");
    if (material_graph_compile(graph)) {
        LOG_INFO("✅ Compiler Verification: SUCCESS");
        LOG_INFO("Generated Shader Fragment:\n%s", graph->generated_fragment_shader);
    } else {
        LOG_ERROR("❌ Compiler Verification: FAILURE");
    }
    
    // Cleanup
    free(heightmap.heights);
    material_graph_destroy(graph);
    editor_context_destroy(ctx);
    
    LOG_INFO("Demo Completed.");
    return 0;
}
