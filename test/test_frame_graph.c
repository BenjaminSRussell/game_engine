// Frame Graph - Simple Validation Test
// Minimal test to verify Week 1 API works

#include "renderer/frame_graph/frame_graph.h"
#include "core/logger.h"
#include <stdio.h>

// Dummy pass execute functions
static void test_pass_a_execute(RGPassContext *ctx, void *user_data) {
    printf("[TestPassA] Executing with user data: %s\n", (const char *)user_data);
}

static void test_pass_b_execute(RGPassContext *ctx, void *user_data) {
    printf("[TestPassB] Executing\n");
    
    // Try to get a resource
    RGResourceHandle *res_handle = (RGResourceHandle *)user_data;
    if (res_handle) {
        TextureID tex = rg_ctx_get_texture(ctx, *res_handle);
        printf("[TestPassB] Got texture ID: %u\n", tex.id);
    }
}

int test_frame_graph_basic(void) {
    printf("\n=== Frame Graph Basic Test ===\n\n");
    
    // Create graph
    RenderGraph *rg = rg_create();
    if (!rg) {
        LOG_ERROR("Failed to create render graph");
        return 1;
    }
    
    // Create some resources
    RGTextureDesc color_desc = {
        .width = 1920,
        .height = 1080,
        .depth = 1,
        .format = FORMAT_RGBA8_UNORM,
        .usage = TEXTURE_USAGE_COLOR_ATTACHMENT | TEXTURE_USAGE_SAMPLED,
        .name = "ColorBuffer"
    };
    RGResourceHandle color = rg_create_texture(rg, &color_desc);
    
    RGTextureDesc depth_desc = {
        .width = 1920,
        .height = 1080,
        .depth = 1,
        .format = FORMAT_DEPTH32F,
        .usage = TEXTURE_USAGE_DEPTH_STENCIL,
        .name = "DepthBuffer"
    };
    RGResourceHandle depth = rg_create_texture(rg, &depth_desc);
    
    // Import a "swapchain" texture (use dummy ID for test)
    TextureID swapchain_tex = {.id = 999};
    RGResourceHandle swapchain = rg_import_texture(rg, swapchain_tex, "Swapchain");
    
    // Add passes
    const char *user_str = "Hello from PassA";
    RGPassDesc pass_a_desc = {
        .name = "PassA_WriteDepth",
        .execute = test_pass_a_execute,
        .user_data = (void *)user_str
    };
    RGPassHandle pass_a = rg_add_pass(rg, &pass_a_desc);
    rg_pass_write(rg, pass_a, depth);
    
    RGPassDesc pass_b_desc = {
        .name = "PassB_ReadDepthWriteColor",
        .execute = test_pass_b_execute,
        .user_data = &depth  // Pass resource handle as user data
    };
    RGPassHandle pass_b = rg_add_pass(rg, &pass_b_desc);
    rg_pass_read(rg, pass_b, depth);
    rg_pass_write(rg, pass_b, color);
    
    RGPassDesc pass_c_desc = {
        .name = "PassC_BlitToSwapchain",
        .execute = test_pass_a_execute,
        .user_data = (void *)"PassC"
    };
    RGPassHandle pass_c = rg_add_pass(rg, &pass_c_desc);
    rg_pass_read(rg, pass_c, color);
    rg_pass_write(rg, pass_c, swapchain);
    
    // Export DOT before compile
    rg_export_dot(rg, "test_graph.dot");
    printf("Exported graph to test_graph.dot\n");
    printf("Run: dot -Tpng test_graph.dot -o test_graph.png\n\n");
    
    // Compile
    if (!rg_compile(rg)) {
        LOG_ERROR("Failed to compile render graph");
        rg_destroy(rg);
        return 1;
    }
    
    // Get stats
    RGStats stats;
    rg_get_stats(rg, &stats);
    printf("Graph Stats:\n");
    printf("  Total passes: %u\n", stats.total_passes);
    printf("  Executed passes: %u\n", stats.executed_passes);
    printf("  Culled passes: %u\n", stats.culled_passes);
    printf("  Total resources: %u\n", stats.total_resources);
    printf("  Transient: %u\n", stats.transient_resources);
    printf("  Imported: %u\n\n", stats.imported_resources);
    
    // Execute (with dummy command buffer)
    // Note: This will fail to get physical resources since we don't have a real pool yet
    // But it should call the execute callbacks
    printf("Executing graph:\n");
    CommandBuffer dummy_cmd = {0};  // Placeholder
    rg_execute(rg, &dummy_cmd);
    
    printf("\n");
    
    // Cleanup
    rg_destroy(rg);
    
    printf("Test complete!\n\n");
    return 0;
}

int main(void) {
    return test_frame_graph_basic();
}
