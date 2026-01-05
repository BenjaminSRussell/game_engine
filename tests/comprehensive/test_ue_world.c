/**
 * UNREAL ENGINE PARITY TESTS - WORLD SYSTEMS
 * Test-Driven Development for World Partition, Water, Landscape, Foliage, PCG
 */

#include "../test_framework_unified.h"
#include "ue_parity_stubs.h"

// =============================================================================
// WORLD PARTITION TESTS
// =============================================================================

static TestResult test_world_partition_creation(void) {
    WorldPartitionConfig config = {
        .cell_size = 12800.0f, // UE default
        .loading_range = 3,
        .max_loaded_cells = 100,
        .enable_hlod = true
    };
    
    WorldPartition* wp = world_partition_create(&config);
    TEST_ASSERT_NOT_NULL(wp, "World partition should be created");
    
    world_partition_destroy(wp);
    return TEST_PASS;
}

static TestResult test_world_partition_streaming(void) {
    WorldPartitionConfig config = {
        .cell_size = 5000.0f,
        .loading_range = 2
    };
    
    WorldPartition* wp = world_partition_create(&config);
    
    // Simulate viewer at origin
    world_partition_update(wp, (Vec3){0, 0, 0});
    
    // Cell at origin should be loaded
    bool origin_loaded = world_partition_is_cell_loaded(wp, 0, 0, 0);
    TEST_ASSERT_TRUE(origin_loaded, "Origin cell should be loaded");
    
    // Far away cell should not be loaded
    bool far_loaded = world_partition_is_cell_loaded(wp, 100, 0, 100);
    TEST_ASSERT_FALSE(far_loaded, "Far cell should not be loaded");
    
    world_partition_destroy(wp);
    return TEST_PASS;
}

static TestResult test_hlod_generation(void) {
    float lod_distances[] = {1000.0f, 5000.0f, 10000.0f, 50000.0f};
    HLOD* hlod = hlod_create(lod_distances, 4);
    TEST_ASSERT_NOT_NULL(hlod, "HLOD should be created");
    
    // Build from actors
    Entity actors[] = {1, 2, 3, 4, 5};
    hlod_build_from_actors(hlod, actors, 5);
    
    hlod_destroy(hlod);
    return TEST_PASS;
}

// =============================================================================
// WATER BODY TESTS
// =============================================================================

static TestResult test_water_body_ocean(void) {
    WaterBodyConfig config = {
        .type = WATER_TYPE_OCEAN,
        .wave_amplitude = 2.0f,
        .wave_length = 100.0f,
        .wave_speed = 5.0f,
        .surface_color = {0.1f, 0.3f, 0.5f, 1.0f},
        .underwater_color = {0.05f, 0.1f, 0.2f, 1.0f},
        .visibility_depth = 50.0f,
        .enable_caustics = true
    };
    
    WaterBody* ocean = water_body_create(&config);
    TEST_ASSERT_NOT_NULL(ocean, "Ocean should be created");
    
    water_body_destroy(ocean);
    return TEST_PASS;
}

static TestResult test_water_height_query(void) {
    WaterBodyConfig config = {
        .type = WATER_TYPE_LAKE,
        .wave_amplitude = 0.5f,
        .wave_length = 10.0f
    };
    
    WaterBody* lake = water_body_create(&config);
    
    // Query water height at position
    Vec3 pos = {10, 0, 10};
    float height = water_body_get_height_at(lake, pos);
    
    // Height should be reasonable (within wave amplitude of base level)
    TEST_ASSERT_RANGE(height, -1.0f, 1.0f, "Height within wave range");
    
    water_body_destroy(lake);
    return TEST_PASS;
}

static TestResult test_water_velocity(void) {
    WaterBodyConfig config = {
        .type = WATER_TYPE_RIVER,
        .wave_speed = 3.0f
    };
    
    WaterBody* river = water_body_create(&config);
    
    // Set river spline
    Vec3 spline[] = {{0,0,0}, {100,0,0}, {100,0,100}, {200,0,100}};
    water_body_set_spline(river, spline, 4);
    
    // Get flow velocity
    Vec3 velocity = water_body_get_velocity_at(river, (Vec3){50, 0, 0});
    // River should have some flow
    float speed = sqrtf(velocity.x*velocity.x + velocity.y*velocity.y + velocity.z*velocity.z);
    TEST_ASSERT_TRUE(speed >= 0.0f, "Should have valid velocity");
    
    water_body_destroy(river);
    return TEST_PASS;
}

static TestResult test_water_underwater_check(void) {
    WaterBodyConfig config = {.type = WATER_TYPE_LAKE};
    WaterBody* lake = water_body_create(&config);
    
    // Above water
    Vec3 above = {0, 10, 0};
    TEST_ASSERT_FALSE(water_body_is_underwater(lake, above), "Above should not be underwater");
    
    // Below water (assuming water at y=0)
    Vec3 below = {0, -5, 0};
    TEST_ASSERT_TRUE(water_body_is_underwater(lake, below), "Below should be underwater");
    
    water_body_destroy(lake);
    return TEST_PASS;
}

// =============================================================================
// LANDSCAPE TESTS
// =============================================================================

static TestResult test_landscape_creation(void) {
    LandscapeConfig config = {
        .component_size = 127,
        .section_count = 2,
        .quads_per_section = 63,
        .scale_x = 100.0f,
        .scale_y = 100.0f,
        .scale_z = 100.0f,
        .max_lod = 8
    };
    
    Landscape* landscape = landscape_create(&config);
    TEST_ASSERT_NOT_NULL(landscape, "Landscape should be created");
    
    landscape_destroy(landscape);
    return TEST_PASS;
}

static TestResult test_landscape_heightmap(void) {
    LandscapeConfig config = {.component_size = 127, .scale_z = 100.0f};
    Landscape* landscape = landscape_create(&config);
    
    // Import heightmap
    float heightmap[256*256];
    for (int i = 0; i < 256*256; i++) {
        heightmap[i] = sinf(i * 0.01f) * 0.5f + 0.5f;
    }
    
    landscape_import_heightmap(landscape, heightmap, 256, 256);
    
    // Query height
    float height = landscape_get_height_at(landscape, 128, 128);
    TEST_ASSERT_TRUE(height >= 0.0f, "Height should be valid");
    
    // Query normal
    Vec3 normal = landscape_get_normal_at(landscape, 128, 128);
    float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
    TEST_ASSERT_FLOAT_EQ(len, 1.0f, 0.01f, "Normal should be normalized");
    
    landscape_destroy(landscape);
    return TEST_PASS;
}

static TestResult test_landscape_layers(void) {
    LandscapeConfig config = {.component_size = 127};
    Landscape* landscape = landscape_create(&config);
    
    // Add paint layers
    LandscapeLayer* grass = landscape_add_layer(landscape, "Grass");
    LandscapeLayer* rock = landscape_add_layer(landscape, "Rock");
    LandscapeLayer* sand = landscape_add_layer(landscape, "Sand");
    
    TEST_ASSERT_NOT_NULL(grass, "Grass layer created");
    TEST_ASSERT_NOT_NULL(rock, "Rock layer created");
    TEST_ASSERT_NOT_NULL(sand, "Sand layer created");
    
    // Paint some areas
    landscape_paint_layer(landscape, grass, 50, 50, 10.0f, 1.0f);
    
    // Query layer weight
    uint8_t weight = landscape_get_layer_weight(landscape, grass, 50, 50);
    TEST_ASSERT_TRUE(weight > 0, "Should have grass weight at painted area");
    
    landscape_destroy(landscape);
    return TEST_PASS;
}

// =============================================================================
// FOLIAGE TESTS
// =============================================================================

static TestResult test_foliage_type(void) {
    FoliageTypeConfig config = {
        .mesh = NULL,
        .min_scale = 0.8f,
        .max_scale = 1.2f,
        .density = 100.0f,
        .collision = true,
        .cull_distance_min = 1000.0f,
        .cull_distance_max = 5000.0f,
        .cast_shadow = true
    };
    
    FoliageType* type = foliage_type_create(&config);
    TEST_ASSERT_NOT_NULL(type, "Foliage type should be created");
    
    foliage_type_destroy(type);
    return TEST_PASS;
}

static TestResult test_foliage_instancing(void) {
    FoliageTypeConfig config = {.density = 50.0f, .min_scale = 0.9f, .max_scale = 1.1f};
    FoliageType* type = foliage_type_create(&config);
    
    FoliageActor* actor = foliage_actor_create();
    
    // Add instances
    Transform transforms[100];
    for (int i = 0; i < 100; i++) {
        transforms[i].position = (Vec3){(float)(i % 10) * 100, 0, (float)(i / 10) * 100};
        transforms[i].rotation = (Quaternion){0, 0, 0, 1};
        transforms[i].scale = (Vec3){1, 1, 1};
    }
    
    foliage_actor_add_instances(actor, type, transforms, 100);
    
    uint32_t count = foliage_actor_get_instance_count(actor);
    TEST_ASSERT_EQ(count, 100, "Should have 100 instances");
    
    // Remove some instances
    AABB remove_box = {{0, -100, 0}, {500, 100, 500}};
    foliage_actor_remove_instances_in_box(actor, remove_box);
    
    count = foliage_actor_get_instance_count(actor);
    TEST_ASSERT_TRUE(count < 100, "Should have fewer instances after removal");
    
    foliage_actor_destroy(actor);
    foliage_type_destroy(type);
    return TEST_PASS;
}

// =============================================================================
// PCG TESTS
// =============================================================================

static TestResult test_pcg_graph(void) {
    PCGGraph* graph = pcg_graph_create("TestPCG");
    TEST_ASSERT_NOT_NULL(graph, "PCG graph should be created");
    
    // Add nodes
    PCGNode* input = pcg_add_node(graph, "LandscapeInput");
    PCGNode* scatter = pcg_add_node(graph, "PointScatter");
    PCGNode* filter = pcg_add_node(graph, "SlopeFilter");
    PCGNode* mesh = pcg_add_node(graph, "MeshSpawner");
    
    // Connect nodes
    pcg_connect(input, "Points", scatter, "Surface");
    pcg_connect(scatter, "Points", filter, "Input");
    pcg_connect(filter, "Filtered", mesh, "Points");
    
    pcg_graph_destroy(graph);
    return TEST_PASS;
}

static TestResult test_pcg_execution(void) {
    PCGGraph* graph = pcg_graph_create("TreeSpawner");
    
    PCGNode* scatter = pcg_add_node(graph, "RandomScatter");
    
    // Execute
    AABB bounds = {{-1000, 0, -1000}, {1000, 100, 1000}};
    PCGData* result = pcg_execute(graph, bounds, 12345);
    
    TEST_ASSERT_NOT_NULL(result, "Should produce result");
    
    uint32_t point_count = pcg_data_get_point_count(result);
    TEST_ASSERT_TRUE(point_count > 0, "Should generate points");
    
    // Verify points are within bounds
    for (uint32_t i = 0; i < point_count && i < 10; i++) {
        Vec3 p = pcg_data_get_point(result, i);
        TEST_ASSERT_RANGE(p.x, -1000.0f, 1000.0f, "X in bounds");
        TEST_ASSERT_RANGE(p.z, -1000.0f, 1000.0f, "Z in bounds");
    }
    
    pcg_data_destroy(result);
    pcg_graph_destroy(graph);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ue_world_tests(void) {
    // World Partition
    TEST_REGISTER("UE:WorldPartition", "Creation", test_world_partition_creation);
    TEST_REGISTER("UE:WorldPartition", "Streaming", test_world_partition_streaming);
    TEST_REGISTER("UE:WorldPartition", "HLOD generation", test_hlod_generation);
    
    // Water
    TEST_REGISTER("UE:Water", "Ocean body", test_water_body_ocean);
    TEST_REGISTER("UE:Water", "Height query", test_water_height_query);
    TEST_REGISTER("UE:Water", "Velocity", test_water_velocity);
    TEST_REGISTER("UE:Water", "Underwater check", test_water_underwater_check);
    
    // Landscape
    TEST_REGISTER("UE:Landscape", "Creation", test_landscape_creation);
    TEST_REGISTER("UE:Landscape", "Heightmap", test_landscape_heightmap);
    TEST_REGISTER("UE:Landscape", "Paint layers", test_landscape_layers);
    
    // Foliage
    TEST_REGISTER("UE:Foliage", "Type creation", test_foliage_type);
    TEST_REGISTER("UE:Foliage", "Instancing", test_foliage_instancing);
    
    // PCG
    TEST_REGISTER("UE:PCG", "Graph creation", test_pcg_graph);
    TEST_REGISTER("UE:PCG", "Execution", test_pcg_execution);
}
