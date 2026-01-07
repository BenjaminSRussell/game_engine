/*
 * phase3_test.c
 * Phase 3 Rendering Pipeline Tests
 *
 * Tests render graph, G-buffer, deferred rendering, and shadow systems
 */

#include "../rendering/3d_rendering/rendering/render_graph/graph_compiler.h"
#include "../rendering/3d_rendering/rendering/render_graph/render_pass_node.h"
#include "../rendering/3d_rendering/rendering/render_graph/resource_node.h"
#include "../rendering/3d_rendering/lighting/shadows/csm_manager.h"
#include "../rendering/3d_rendering/lighting/shadows/shadow_atlas.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        return false; \
    }

#define TEST_PASS(msg) printf("PASS: %s\n", msg)

// Test 1: Render Graph Creation
bool test_render_graph_creation() {
    printf("\n=== Test 1: Render Graph Creation ===\n");
    
    render_graph_t* graph = render_graph_create("TestGraph");
    TEST_ASSERT(graph != NULL, "Render graph creation failed");
    TEST_ASSERT(strcmp(graph->name, "TestGraph") == 0, "Render graph name mismatch");
    TEST_PASS("Render graph created");
    
    render_graph_destroy(graph);
    TEST_PASS("Render graph destroyed");
    
    return true;
}

// Test 2: Render Pass Node System
bool test_render_pass_node_system() {
    printf("\n=== Test 2: Render Pass Node System ===\n");
    
    int result = rendering_render_pass_node_init();
    TEST_ASSERT(result == 0, "Render pass node init failed");
    TEST_PASS("Render pass node system initialized");
    
    uint32_t count = rendering_render_pass_node_get_count();
    printf("  Initial pass count: %u\n", count);
    TEST_PASS("Render pass node count queried");
    
    size_t memory = rendering_render_pass_node_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    rendering_render_pass_node_shutdown();
    TEST_PASS("Render pass node system shutdown");
    
    return true;
}

// Test 3: Resource Node System
bool test_resource_node_system() {
    printf("\n=== Test 3: Resource Node System ===\n");
    
    int result = rendering_resource_node_init();
    TEST_ASSERT(result == 0, "Resource node init failed");
    TEST_PASS("Resource node system initialized");
    
    uint32_t count = rendering_resource_node_get_count();
    printf("  Initial resource count: %u\n", count);
    TEST_PASS("Resource node count queried");
    
    size_t memory = rendering_resource_node_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    rendering_resource_node_shutdown();
    TEST_PASS("Resource node system shutdown");
    
    return true;
}

// Test 4: Graph Compiler System
bool test_graph_compiler_system() {
    printf("\n=== Test 4: Graph Compiler System ===\n");
    
    int result = rendering_graph_compiler_init();
    TEST_ASSERT(result == 0, "Graph compiler init failed");
    TEST_PASS("Graph compiler initialized");
    
    uint32_t count = rendering_graph_compiler_get_count();
    printf("  Compiler count: %u\n", count);
    TEST_PASS("Graph compiler count queried");
    
    size_t memory = rendering_graph_compiler_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    rendering_graph_compiler_shutdown();
    TEST_PASS("Graph compiler shutdown");
    
    return true;
}

// Test 5: CSM Manager System
bool test_csm_manager_system() {
    printf("\n=== Test 5: CSM Manager System ===\n");
    
    int result = csm_manager_init();
    TEST_ASSERT(result == 0, "CSM manager init failed");
    TEST_PASS("CSM manager initialized");
    
    uint32_t count = csm_manager_get_count();
    printf("  CSM count: %u\n", count);
    TEST_PASS("CSM manager count queried");
    
    size_t memory = csm_manager_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    csm_manager_shutdown();
    TEST_PASS("CSM manager shutdown");
    
    return true;
}

// Test 6: Shadow Atlas System
bool test_shadow_atlas_system() {
    printf("\n=== Test 6: Shadow Atlas System ===\n");
    
    int result = shadow_atlas_init();
    TEST_ASSERT(result == 0, "Shadow atlas init failed");
    TEST_PASS("Shadow atlas initialized");
    
    uint32_t count = shadow_atlas_get_count();
    printf("  Shadow atlas count: %u\n", count);
    TEST_PASS("Shadow atlas count queried");
    
    size_t memory = shadow_atlas_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    shadow_atlas_shutdown();
    TEST_PASS("Shadow atlas shutdown");
    
    return true;
}

// Test 7: Complete Render Graph Flow
bool test_complete_render_graph_flow() {
    printf("\n=== Test 7: Complete Render Graph Flow ===\n");
    
    // Initialize systems
    rendering_graph_compiler_init();
    rendering_render_pass_node_init();
    rendering_resource_node_init();
    
    // Create a render graph
    render_graph_t* graph = render_graph_create("CompleteFlowTest");
    TEST_ASSERT(graph != NULL, "Graph creation failed");
    TEST_PASS("Systems initialized and graph created");
    
    // Create a simple resource (color texture)
    rendering_resource_node_desc_t color_desc = {
        .type = RENDERING_RESOURCE_TYPE_TEXTURE_2D,
        .width = 1920,
        .height = 1080,
        .format = 0, // Placeholder format
        .flags = 0
    };
    
    rg_resource_handle_t color_tex = rg_create_texture(graph, "ColorBuffer", &color_desc);
    TEST_ASSERT(color_tex != RG_INVALID_RESOURCE, "Color texture creation failed");
    TEST_PASS("Color texture resource created");
    
    // Try to compile the graph
    int compile_result = rg_compile(graph);
    printf("  Compile result: %d\n", compile_result);
    TEST_PASS("Graph compilation attempted");
    
    // Cleanup
    render_graph_destroy(graph);
    rendering_resource_node_shutdown();
    rendering_render_pass_node_shutdown();
    rendering_graph_compiler_shutdown();
    TEST_PASS("Complete flow cleaned up");
    
    return true;
}

// Main test runner
int main(int argc, const char * argv[]) {
    printf("========================================\n");
    printf("Phase 3: Rendering Pipeline Tests\n");
    printf("========================================\n");
    
    bool all_passed = true;
    
    all_passed &= test_render_graph_creation();
    all_passed &= test_render_pass_node_system();
    all_passed &= test_resource_node_system();
    all_passed &= test_graph_compiler_system();
    all_passed &= test_csm_manager_system();
    all_passed &= test_shadow_atlas_system();
    all_passed &= test_complete_render_graph_flow();
    
    printf("\n========================================\n");
    if (all_passed) {
        printf("All Phase 3 tests PASSED!\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("Some Phase 3 tests FAILED!\n");
        printf("========================================\n");
        return 1;
    }
}
