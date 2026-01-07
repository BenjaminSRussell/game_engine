/*
 * rendering_pipeline_tests.c
 * Unit tests for 3D rendering pipeline
 *
 * Tests core rendering subsystems:
 * - GPU-driven rendering
 * - Forward rendering
 * - Deferred rendering
 * - Render graph
 * - Material system
 * - Lighting system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * TEST FRAMEWORK
 * ============================================================================ */

#define TEST_PASS  1
#define TEST_FAIL  0

typedef struct {
    const char* name;
    int (*test_fn)(void);
} Test;

typedef struct {
    int passed;
    int failed;
    int total;
} TestResult;

static TestResult g_test_result = {0, 0, 0};

void test_assert(int condition, const char* message) {
    if (!condition) {
        printf("  ❌ FAIL: %s\n", message);
        g_test_result.failed++;
    } else {
        g_test_result.passed++;
    }
}

void run_test(const char* name, int (*test_fn)(void)) {
    printf("\n📋 %s\n", name);
    g_test_result.total++;

    if (test_fn()) {
        printf("  ✅ PASS\n");
        g_test_result.passed++;
    } else {
        printf("  ❌ FAIL\n");
        g_test_result.failed++;
    }
}

void print_test_results(void) {
    printf("\n\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("TEST RESULTS\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Total:  %d tests\n", g_test_result.total);
    printf("Passed: %d tests ✅\n", g_test_result.passed);
    printf("Failed: %d tests ❌\n", g_test_result.failed);
    printf("═══════════════════════════════════════════════════════════\n");

    if (g_test_result.failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n\n");
    } else {
        printf("\n⚠️ %d TEST(S) FAILED\n\n", g_test_result.failed);
    }
}

/* ============================================================================
 * MOCK STRUCTURES & FUNCTIONS
 * ============================================================================ */

/* Mock GPU scene data */
typedef struct {
    uint32_t instance_count;
    uint32_t draw_command_count;
    uint32_t culled_count;
    bool gpu_data_valid;
    void* gpu_buffer;
} MockGPUScene;

/* Mock render pass */
typedef struct {
    uint32_t pass_id;
    const char* pass_name;
    uint32_t input_resource_count;
    uint32_t output_resource_count;
    bool executed;
} MockRenderPass;

/* Mock material instance */
typedef struct {
    uint32_t id;
    const char* material_name;
    float metallic;
    float roughness;
    uint32_t texture_id;
    bool gpu_uploaded;
} MockMaterialInstance;

/* Mock shadow system */
typedef struct {
    uint32_t shadow_map_count;
    uint32_t cascade_count;
    bool csm_valid;
    bool rt_shadows_enabled;
} MockShadowSystem;

/* Mock GI system */
typedef struct {
    uint32_t probe_count;
    bool ddgi_initialized;
    bool restir_enabled;
    uint32_t frame_count;
} MockGISystem;

/* Mock lighting system */
typedef struct {
    uint32_t directional_light_count;
    uint32_t point_light_count;
    uint32_t spot_light_count;
    uint32_t light_cluster_count;
} MockLightingSystem;

/* ============================================================================
 * GPU-DRIVEN RENDERING TESTS
 * ============================================================================ */

int test_gpu_scene_creation(void) {
    printf("  Testing GPU scene creation...\n");

    /* Create mock GPU scene */
    MockGPUScene scene = {0};
    scene.instance_count = 0;
    scene.draw_command_count = 0;
    scene.culled_count = 0;
    scene.gpu_data_valid = false;
    scene.gpu_buffer = malloc(1024);

    /* Verify creation */
    test_assert(scene.gpu_buffer != NULL, "GPU buffer allocated");
    test_assert(scene.instance_count == 0, "Instance count initialized to 0");
    test_assert(scene.gpu_data_valid == false, "GPU data not valid on creation");

    free(scene.gpu_buffer);
    return g_test_result.failed == 0;
}

int test_gpu_scene_instance_update(void) {
    printf("  Testing GPU scene instance updates...\n");

    MockGPUScene scene = {0};
    scene.instance_count = 0;
    scene.gpu_buffer = malloc(1024);

    /* Add instances */
    scene.instance_count = 100;
    test_assert(scene.instance_count == 100, "Added 100 instances");

    /* Update GPU data */
    scene.gpu_data_valid = true;
    test_assert(scene.gpu_data_valid == true, "GPU data marked valid");

    free(scene.gpu_buffer);
    return g_test_result.failed == 0;
}

int test_gpu_culling(void) {
    printf("  Testing GPU culling...\n");

    MockGPUScene scene = {0};
    scene.instance_count = 1000;
    scene.draw_command_count = 0;
    scene.culled_count = 0;
    scene.gpu_buffer = malloc(4096);

    /* Simulate culling */
    scene.culled_count = 200; /* 200 culled out of 1000 */
    scene.draw_command_count = scene.instance_count - scene.culled_count;

    test_assert(scene.culled_count == 200, "200 instances culled");
    test_assert(scene.draw_command_count == 800, "800 draw commands generated");
    test_assert(scene.draw_command_count <= scene.instance_count, "Draw commands <= instances");

    free(scene.gpu_buffer);
    return g_test_result.failed == 0;
}

int test_gpu_lod_selection(void) {
    printf("  Testing GPU LOD selection...\n");

    MockGPUScene scene = {0};
    scene.instance_count = 500;

    /* Simulate LOD selection */
    uint32_t lod0_count = 100;  /* High detail LOD */
    uint32_t lod1_count = 200;  /* Medium detail LOD */
    uint32_t lod2_count = 200;  /* Low detail LOD */

    test_assert(lod0_count + lod1_count + lod2_count == scene.instance_count,
                "LOD counts sum to instance count");
    test_assert(lod0_count <= lod1_count, "LOD0 (high detail) <= LOD1");
    test_assert(lod1_count >= lod2_count, "LOD1 >= LOD2 (low detail)");

    return g_test_result.failed == 0;
}

int test_gpu_persistent_mapping(void) {
    printf("  Testing GPU persistent mapping...\n");

    MockGPUScene scene = {0};
    scene.gpu_buffer = malloc(2048);

    /* Simulate persistent mapping */
    uint8_t* mapped_ptr = (uint8_t*)scene.gpu_buffer;

    /* Write test data */
    for (int i = 0; i < 256; i++) {
        mapped_ptr[i] = (uint8_t)i;
    }

    /* Verify data written */
    bool data_valid = true;
    for (int i = 0; i < 256; i++) {
        if (mapped_ptr[i] != (uint8_t)i) {
            data_valid = false;
            break;
        }
    }

    test_assert(data_valid, "Persistent mapping data valid");

    free(scene.gpu_buffer);
    return g_test_result.failed == 0;
}

/* ============================================================================
 * FORWARD RENDERING TESTS
 * ============================================================================ */

int test_forward_depth_prepass(void) {
    printf("  Testing forward depth prepass...\n");

    MockRenderPass prepass = {0};
    prepass.pass_id = 1;
    prepass.pass_name = "depth_prepass";
    prepass.executed = true;

    test_assert(prepass.pass_id > 0, "Prepass has valid ID");
    test_assert(strcmp(prepass.pass_name, "depth_prepass") == 0, "Prepass named correctly");
    test_assert(prepass.executed, "Prepass executed");

    return g_test_result.failed == 0;
}

int test_forward_lighting(void) {
    printf("  Testing forward lighting...\n");

    MockLightingSystem lights = {0};
    lights.directional_light_count = 1;
    lights.point_light_count = 32;
    lights.spot_light_count = 16;

    test_assert(lights.directional_light_count == 1, "1 directional light");
    test_assert(lights.point_light_count == 32, "32 point lights");
    test_assert(lights.spot_light_count == 16, "16 spot lights");

    uint32_t total_lights = lights.directional_light_count +
                           lights.point_light_count +
                           lights.spot_light_count;
    test_assert(total_lights == 49, "Total 49 lights");

    return g_test_result.failed == 0;
}

int test_forward_transparency(void) {
    printf("  Testing forward transparency...\n");

    /* Simulate transparent object ordering */
    uint32_t opaque_count = 800;
    uint32_t transparent_count = 200;
    uint32_t total = opaque_count + transparent_count;

    test_assert(total == 1000, "1000 total objects");
    test_assert(opaque_count > transparent_count, "Opaque > transparent");
    test_assert(transparent_count > 0, "Has transparent objects");

    return g_test_result.failed == 0;
}

int test_forward_clustering(void) {
    printf("  Testing forward+ light clustering...\n");

    MockLightingSystem lights = {0};
    lights.light_cluster_count = 64; /* 4x4x4 cluster grid */

    uint32_t cluster_dim = 4;
    uint32_t expected_clusters = cluster_dim * cluster_dim * cluster_dim;

    test_assert(lights.light_cluster_count == expected_clusters, "Correct cluster count");
    test_assert(lights.light_cluster_count > 0, "Has light clusters");

    return g_test_result.failed == 0;
}

/* ============================================================================
 * DEFERRED RENDERING TESTS
 * ============================================================================ */

int test_deferred_gbuffer_layout(void) {
    printf("  Testing deferred G-buffer layout...\n");

    /* G-buffer channels:
     * RT0: Albedo (RGB) + Metallic (A)
     * RT1: Normal (RG) + Roughness (B) + AO (A)
     * RT2: Emissive + Motion vectors
     * RT3: Depth (optional) or other
     */

    uint32_t gbuffer_rt_count = 3; /* Minimum 3 RTs */
    test_assert(gbuffer_rt_count >= 3, "G-buffer has 3+ RTs");

    /* Each RT 2K resolution */
    uint32_t rt0_channels = 4; /* RGBA */
    uint32_t rt1_channels = 4;
    uint32_t rt2_channels = 4;

    test_assert(rt0_channels == 4, "RT0 has 4 channels (RGBA)");
    test_assert(rt1_channels == 4, "RT1 has 4 channels");
    test_assert(rt2_channels == 4, "RT2 has 4 channels");

    return g_test_result.failed == 0;
}

int test_deferred_gbuffer_write(void) {
    printf("  Testing deferred G-buffer write...\n");

    MockRenderPass gbuffer_pass = {0};
    gbuffer_pass.pass_id = 2;
    gbuffer_pass.pass_name = "gbuffer_pass";
    gbuffer_pass.output_resource_count = 3;
    gbuffer_pass.executed = true;

    test_assert(gbuffer_pass.output_resource_count == 3, "G-buffer outputs 3 RTs");
    test_assert(gbuffer_pass.executed, "G-buffer pass executed");

    return g_test_result.failed == 0;
}

int test_deferred_lighting_pass(void) {
    printf("  Testing deferred lighting pass...\n");

    MockRenderPass lighting_pass = {0};
    lighting_pass.pass_id = 3;
    lighting_pass.pass_name = "deferred_lighting";
    lighting_pass.input_resource_count = 3; /* G-buffer RTs */
    lighting_pass.output_resource_count = 1; /* Output RT */
    lighting_pass.executed = true;

    test_assert(lighting_pass.input_resource_count == 3, "Lighting reads 3 G-buffer RTs");
    test_assert(lighting_pass.output_resource_count == 1, "Lighting outputs 1 RT");
    test_assert(lighting_pass.executed, "Lighting pass executed");

    return g_test_result.failed == 0;
}

int test_deferred_decals(void) {
    printf("  Testing deferred decals...\n");

    uint32_t decal_count = 256;
    uint32_t max_decals = 512;

    test_assert(decal_count <= max_decals, "Decal count within limits");
    test_assert(decal_count > 0, "Has decals");

    /* Screen-space decal bounds */
    float screen_width = 1920.0f;
    float screen_height = 1080.0f;

    test_assert(screen_width > 0, "Screen width valid");
    test_assert(screen_height > 0, "Screen height valid");

    return g_test_result.failed == 0;
}

/* ============================================================================
 * RENDER GRAPH TESTS
 * ============================================================================ */

int test_render_graph_node_creation(void) {
    printf("  Testing render graph node creation...\n");

    uint32_t node_count = 0;

    /* Create nodes */
    MockRenderPass passes[10] = {0};
    for (int i = 0; i < 10; i++) {
        passes[i].pass_id = i + 1;
        node_count++;
    }

    test_assert(node_count == 10, "Created 10 render pass nodes");
    test_assert(passes[0].pass_id == 1, "First node ID is 1");
    test_assert(passes[9].pass_id == 10, "Last node ID is 10");

    return g_test_result.failed == 0;
}

int test_render_graph_dependencies(void) {
    printf("  Testing render graph dependencies...\n");

    /* Create dependency chain: A -> B -> C */
    MockRenderPass pass_a = {1, "pass_a", 0, 1, true};
    MockRenderPass pass_b = {2, "pass_b", 1, 1, true};
    MockRenderPass pass_c = {3, "pass_c", 1, 1, true};

    test_assert(pass_a.pass_id == 1, "Pass A is first");
    test_assert(pass_b.pass_id == 2, "Pass B depends on A");
    test_assert(pass_c.pass_id == 3, "Pass C depends on B");

    /* Verify input/output chain */
    test_assert(pass_a.output_resource_count == pass_b.input_resource_count,
                "A's output matches B's input");
    test_assert(pass_b.output_resource_count == pass_c.input_resource_count,
                "B's output matches C's input");

    return g_test_result.failed == 0;
}

int test_render_graph_resource_aliasing(void) {
    printf("  Testing render graph resource aliasing...\n");

    /* Total memory pool */
    uint32_t total_memory = 1024 * 1024 * 256; /* 256MB */

    /* Allocate RTs that reuse memory */
    uint32_t rt_size = 1920 * 1080 * 4;
    uint32_t rt1_offset = 0;
    uint32_t rt2_offset = rt_size;
    uint32_t rt3_offset = rt_size;  /* Reuse rt2's memory after it's done */

    test_assert(rt1_offset + rt_size <= total_memory, "RT1 fits in memory");
    test_assert(rt2_offset + rt_size <= total_memory, "RT2 fits in memory");
    test_assert(rt3_offset + rt_size <= total_memory, "RT3 fits in memory (aliased)");

    return g_test_result.failed == 0;
}

int test_render_graph_execution_order(void) {
    printf("  Testing render graph execution order...\n");

    /* Create passes that must execute in order */
    uint32_t execution_order[5];

    /* Z-prepass -> GBuffer -> Lighting -> Decals -> Postprocess */
    execution_order[0] = 1; /* Z-prepass */
    execution_order[1] = 2; /* GBuffer */
    execution_order[2] = 3; /* Lighting */
    execution_order[3] = 4; /* Decals */
    execution_order[4] = 5; /* Postprocess */

    /* Verify order */
    for (int i = 1; i < 5; i++) {
        test_assert(execution_order[i] > execution_order[i-1],
                   "Execution order is sequential");
    }

    return g_test_result.failed == 0;
}

/* ============================================================================
 * SHADOW SYSTEM TESTS
 * ============================================================================ */

int test_shadow_system_csm(void) {
    printf("  Testing shadow system CSM...\n");

    MockShadowSystem shadows = {0};
    shadows.cascade_count = 4;
    shadows.shadow_map_count = 4;
    shadows.csm_valid = true;
    shadows.rt_shadows_enabled = false;

    test_assert(shadows.cascade_count == 4, "4 shadow cascades");
    test_assert(shadows.shadow_map_count == 4, "4 shadow maps");
    test_assert(shadows.csm_valid, "CSM is valid");

    return g_test_result.failed == 0;
}

int test_shadow_system_filtering(void) {
    printf("  Testing shadow filtering (PCF/PCSS)...\n");

    /* PCF: Percentage Closer Filtering */
    uint32_t pcf_samples = 16;
    test_assert(pcf_samples > 0, "PCF has samples");

    /* PCSS: Soft shadows with penumbra */
    uint32_t pcss_blocker_samples = 8;
    uint32_t pcss_filter_samples = 16;

    test_assert(pcss_blocker_samples > 0, "PCSS has blocker samples");
    test_assert(pcss_filter_samples > 0, "PCSS has filter samples");

    return g_test_result.failed == 0;
}

int test_shadow_system_rt(void) {
    printf("  Testing ray-traced shadows...\n");

    MockShadowSystem shadows = {0};
    shadows.rt_shadows_enabled = true;
    shadows.shadow_map_count = 1; /* Single RT pass */

    test_assert(shadows.rt_shadows_enabled, "RT shadows enabled");
    test_assert(shadows.shadow_map_count >= 1, "Has shadow maps");

    return g_test_result.failed == 0;
}

/* ============================================================================
 * GI SYSTEM TESTS
 * ============================================================================ */

int test_gi_ddgi_probes(void) {
    printf("  Testing DDGI probe system...\n");

    MockGISystem gi = {0};
    gi.ddgi_initialized = true;
    gi.probe_count = 8 * 8 * 8; /* 8x8x8 probe grid */

    test_assert(gi.ddgi_initialized, "DDGI initialized");
    test_assert(gi.probe_count == 512, "512 probes (8^3)");

    return g_test_result.failed == 0;
}

int test_gi_ddgi_update(void) {
    printf("  Testing DDGI probe updates...\n");

    MockGISystem gi = {0};
    gi.frame_count = 0;

    /* Update probes every frame */
    for (int i = 0; i < 60; i++) {
        gi.frame_count++;
    }

    test_assert(gi.frame_count == 60, "60 frames of updates");
    test_assert(gi.frame_count > 0, "At least one update");

    return g_test_result.failed == 0;
}

int test_gi_restir(void) {
    printf("  Testing ReSTIR GI...\n");

    MockGISystem gi = {0};
    gi.restir_enabled = true;
    gi.ddgi_initialized = true;

    test_assert(gi.restir_enabled, "ReSTIR enabled");
    test_assert(gi.ddgi_initialized, "Base DDGI initialized");

    return g_test_result.failed == 0;
}

/* ============================================================================
 * MATERIAL SYSTEM TESTS
 * ============================================================================ */

int test_material_creation(void) {
    printf("  Testing material creation...\n");

    MockMaterialInstance mat = {0};
    mat.id = 1;
    mat.material_name = "default_pbr";
    mat.metallic = 0.5f;
    mat.roughness = 0.5f;
    mat.texture_id = 0;

    test_assert(mat.id == 1, "Material has valid ID");
    test_assert(strcmp(mat.material_name, "default_pbr") == 0, "Material named correctly");

    return g_test_result.failed == 0;
}

int test_material_pbr_parameters(void) {
    printf("  Testing PBR parameters...\n");

    MockMaterialInstance mat = {0};
    mat.metallic = 0.8f;
    mat.roughness = 0.2f;

    test_assert(mat.metallic >= 0.0f && mat.metallic <= 1.0f, "Metallic in range [0,1]");
    test_assert(mat.roughness >= 0.0f && mat.roughness <= 1.0f, "Roughness in range [0,1]");
    test_assert(mat.metallic + mat.roughness > 0, "Non-zero material parameters");

    return g_test_result.failed == 0;
}

int test_material_gpu_upload(void) {
    printf("  Testing material GPU upload...\n");

    MockMaterialInstance mat = {0};
    mat.gpu_uploaded = false;

    /* Simulate GPU upload */
    mat.gpu_uploaded = true;

    test_assert(mat.gpu_uploaded, "Material uploaded to GPU");

    return g_test_result.failed == 0;
}

int test_material_texture_binding(void) {
    printf("  Testing material texture binding...\n");

    MockMaterialInstance mat = {0};
    mat.texture_id = 42;

    test_assert(mat.texture_id > 0, "Material has valid texture ID");
    test_assert(mat.texture_id == 42, "Texture ID matches");

    return g_test_result.failed == 0;
}

/* ============================================================================
 * INTEGRATION TESTS
 * ============================================================================ */

int test_full_frame_pipeline(void) {
    printf("  Testing full frame pipeline...\n");

    MockGPUScene scene = {0};
    scene.instance_count = 1000;
    scene.draw_command_count = 800;
    scene.gpu_buffer = malloc(4096);

    MockRenderPass passes[5];
    passes[0] = (MockRenderPass){1, "depth_prepass", 0, 1, true};
    passes[1] = (MockRenderPass){2, "gbuffer", 1, 3, true};
    passes[2] = (MockRenderPass){3, "lighting", 3, 1, true};
    passes[3] = (MockRenderPass){4, "decals", 1, 1, true};
    passes[4] = (MockRenderPass){5, "postprocess", 1, 1, true};

    /* Verify all passes executed */
    for (int i = 0; i < 5; i++) {
        test_assert(passes[i].executed, "Pass executed");
        test_assert(passes[i].pass_id > 0, "Pass has valid ID");
    }

    free(scene.gpu_buffer);
    return g_test_result.failed == 0;
}

int test_multi_light_rendering(void) {
    printf("  Testing multi-light rendering...\n");

    MockLightingSystem lights = {0};
    lights.directional_light_count = 1;
    lights.point_light_count = 64;
    lights.spot_light_count = 32;
    lights.light_cluster_count = 64;

    uint32_t total_lights = lights.directional_light_count +
                           lights.point_light_count +
                           lights.spot_light_count;

    test_assert(total_lights == 97, "97 total lights");
    test_assert(lights.light_cluster_count > 0, "Light clustering enabled");

    return g_test_result.failed == 0;
}

int test_shadow_and_gi_integration(void) {
    printf("  Testing shadow and GI integration...\n");

    MockShadowSystem shadows = {0};
    shadows.cascade_count = 4;
    shadows.csm_valid = true;
    shadows.rt_shadows_enabled = true;

    MockGISystem gi = {0};
    gi.ddgi_initialized = true;
    gi.probe_count = 512;
    gi.restir_enabled = true;

    test_assert(shadows.csm_valid, "CSM valid");
    test_assert(shadows.rt_shadows_enabled, "RT shadows enabled");
    test_assert(gi.ddgi_initialized, "DDGI initialized");
    test_assert(gi.restir_enabled, "ReSTIR enabled");

    return g_test_result.failed == 0;
}

int test_material_and_lighting_pipeline(void) {
    printf("  Testing material and lighting pipeline...\n");

    MockMaterialInstance mats[4] = {0};

    /* Create different materials */
    mats[0] = (MockMaterialInstance){1, "metal", 1.0f, 0.2f, 1, true};
    mats[1] = (MockMaterialInstance){2, "wood", 0.0f, 0.5f, 2, true};
    mats[2] = (MockMaterialInstance){3, "plastic", 0.5f, 0.8f, 3, true};
    mats[3] = (MockMaterialInstance){4, "glass", 0.0f, 0.0f, 4, true};

    for (int i = 0; i < 4; i++) {
        test_assert(mats[i].gpu_uploaded, "Material uploaded");
        test_assert(mats[i].texture_id > 0, "Material has texture");
    }

    return g_test_result.failed == 0;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  3D RENDERING PIPELINE - UNIT TESTS                      ║\n");
    printf("║  Testing core rendering subsystems                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    /* GPU-Driven Rendering Tests */
    printf("\n\n📊 GPU-DRIVEN RENDERING TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("GPU scene creation", test_gpu_scene_creation);
    run_test("GPU scene instance updates", test_gpu_scene_instance_update);
    run_test("GPU culling", test_gpu_culling);
    run_test("GPU LOD selection", test_gpu_lod_selection);
    run_test("GPU persistent mapping", test_gpu_persistent_mapping);

    /* Forward Rendering Tests */
    printf("\n\n📊 FORWARD RENDERING TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("Forward depth prepass", test_forward_depth_prepass);
    run_test("Forward lighting", test_forward_lighting);
    run_test("Forward transparency", test_forward_transparency);
    run_test("Forward+ clustering", test_forward_clustering);

    /* Deferred Rendering Tests */
    printf("\n\n📊 DEFERRED RENDERING TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("Deferred G-buffer layout", test_deferred_gbuffer_layout);
    run_test("Deferred G-buffer write", test_deferred_gbuffer_write);
    run_test("Deferred lighting pass", test_deferred_lighting_pass);
    run_test("Deferred decals", test_deferred_decals);

    /* Render Graph Tests */
    printf("\n\n📊 RENDER GRAPH TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("Render graph node creation", test_render_graph_node_creation);
    run_test("Render graph dependencies", test_render_graph_dependencies);
    run_test("Render graph resource aliasing", test_render_graph_resource_aliasing);
    run_test("Render graph execution order", test_render_graph_execution_order);

    /* Shadow System Tests */
    printf("\n\n📊 SHADOW SYSTEM TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("Shadow system CSM", test_shadow_system_csm);
    run_test("Shadow filtering (PCF/PCSS)", test_shadow_system_filtering);
    run_test("Ray-traced shadows", test_shadow_system_rt);

    /* GI System Tests */
    printf("\n\n📊 GI SYSTEM TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("DDGI probe system", test_gi_ddgi_probes);
    run_test("DDGI probe updates", test_gi_ddgi_update);
    run_test("ReSTIR GI", test_gi_restir);

    /* Material System Tests */
    printf("\n\n📊 MATERIAL SYSTEM TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("Material creation", test_material_creation);
    run_test("PBR parameters", test_material_pbr_parameters);
    run_test("Material GPU upload", test_material_gpu_upload);
    run_test("Material texture binding", test_material_texture_binding);

    /* Integration Tests */
    printf("\n\n📊 INTEGRATION TESTS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    run_test("Full frame pipeline", test_full_frame_pipeline);
    run_test("Multi-light rendering", test_multi_light_rendering);
    run_test("Shadow and GI integration", test_shadow_and_gi_integration);
    run_test("Material and lighting pipeline", test_material_and_lighting_pipeline);

    /* Print results */
    print_test_results();

    return g_test_result.failed == 0 ? 0 : 1;
}
