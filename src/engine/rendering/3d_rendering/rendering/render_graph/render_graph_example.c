#include "render_pass_node.h"
#include "resource_node.h"
#include "graph_compiler.h"
#include "graph_executor.h"
#include "../../postprocess/postprocess_graph.h"
#include <stdio.h>

// Mock Metal Types for C compilation without Obj-C compiler
typedef void* id;
typedef void* MTLCommandBuffer;

// Pass execution callbacks
void execute_gbuffer(void* cmd, void* user_data) {
    printf("Executing G-Buffer Pass\n");
}

void execute_lighting(void* cmd, void* user_data) {
    printf("Executing Lighting Pass (HDR Output)\n");
}

int main() {
    // 1. Initialize Render Graph System
    rendering_graph_compiler_init();
    
    // 2. Create Graph
    render_graph_t* graph = render_graph_create("MainPipeline");
    
    // 3. Create Resources
    // Transient G-Buffer Albedo
    rendering_resource_node_desc_t albedo_desc = {
        .name = "GBuffer_Albedo",
        .type = RENDERING_RESOURCE_TYPE_TEXTURE,
        .width = 1920,
        .height = 1080,
        .depth = 1,
        .format = 80, // MTLPixelFormatRGBA8Unorm
        .usage = 4,   // MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead
        .is_transient = true
    };
    rg_resource_handle_t albedo = rg_create_texture(graph, "GBuffer_Albedo", &albedo_desc);
    
    // Transient Depth
    rendering_resource_node_desc_t depth_desc = {
        .name = "GBuffer_Depth",
        .type = RENDERING_RESOURCE_TYPE_TEXTURE,
        .width = 1920,
        .height = 1080,
        .depth = 1,
        .format = 252, // MTLPixelFormatDepth32Float
        .usage = 4,
        .is_transient = true
    };
    rg_resource_handle_t depth = rg_create_texture(graph, "GBuffer_Depth", &depth_desc);
    
    // Intermediate HDR Buffer (for lighting result before post-processing)
    rendering_resource_node_desc_t scene_hdr_desc = {
        .name = "Scene_HDR",
        .type = RENDERING_RESOURCE_TYPE_TEXTURE,
        .width = 1920,
        .height = 1080,
        .depth = 1,
        .format = 115, // MTLPixelFormatRGBA16Float
        .usage = 4,
        .is_transient = true
    };
    rg_resource_handle_t scene_hdr = rg_create_texture(graph, "Scene_HDR", &scene_hdr_desc);

    // 4. Create Passes
    
    // Pass 1: G-Buffer
    rendering_render_pass_node_desc_t gbuffer_pass_desc = {
        .name = "GBuffer",
        .type = RENDERING_PASS_TYPE_GRAPHICS,
        .execute = execute_gbuffer,
        .color_outputs = {albedo},
        .color_output_count = 1,
        .depth_output = depth
    };
    rg_add_pass(graph, "GBuffer", RENDERING_PASS_TYPE_GRAPHICS, &gbuffer_pass_desc);
    
    // Pass 2: Lighting (Writes to Scene_HDR)
    rendering_render_pass_node_desc_t lighting_pass_desc = {
        .name = "Lighting",
        .type = RENDERING_PASS_TYPE_GRAPHICS,
        .execute = execute_lighting,
        .texture_inputs = {albedo, depth},
        .texture_input_count = 2,
        .color_outputs = {scene_hdr},
        .color_output_count = 1
    };
    rg_add_pass(graph, "Lighting", RENDERING_PASS_TYPE_GRAPHICS, &lighting_pass_desc);
    
    // 5. Add Post-Processing Chain
    postprocess_graph_config_t pp_config = postprocess_graph_default_config(1920, 1080);
    rg_resource_handle_t final_ldr = postprocess_graph_add_to_graph(graph, scene_hdr, &pp_config);

    // 6. Compile Graph
    printf("Compiling Graph with Post-Processing...\n");
    if (rg_compile(graph) == 0) {
        printf("Graph Compiled Successfully!\n");
        printf("Resource Count: %d\n", graph->resource_count);
        printf("Pass Count: %d\n", graph->pass_count);
        
        printf("Execution Order:\n");
        for(uint32_t i=0; i<graph->pass_count; i++) {
            uint32_t idx = graph->execution_order[i];
            printf("  %d: Pass Index %d\n", i, idx);
        }
    } else {
        printf("Graph Compilation Failed!\n");
    }
    
    // 7. Execute (Simulated)
    printf("Executing Graph...\n");
    rg_execute(graph, NULL);
    
    // Cleanup
    render_graph_destroy(graph);
    rendering_graph_compiler_shutdown();
    
    return 0;
}
