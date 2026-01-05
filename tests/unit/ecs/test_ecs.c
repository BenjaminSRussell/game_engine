/**
 * Unit Test for ECS (Entity Component System)
 * Tests entity creation, component management, and system execution
 */

#include "../../../src/engine/ecs/ecs.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test ECS world initialization
static TestResult test_ecs_init(void) {
    ECSWorld* world = ecs_world_create();
    TEST_ASSERT_NOT_NULL(world, "ECS world created");
    ecs_world_destroy(world);
    return TEST_PASS;
}

// Test entity creation
static TestResult test_entity_creation(void) {
    ECSWorld* world = ecs_world_create();
    
    Entity e1 = ecs_create_entity(world);
    Entity e2 = ecs_create_entity(world);
    
    TEST_ASSERT(e1 != 0, "Entity 1 created");
    TEST_ASSERT(e2 != 0, "Entity 2 created");
    TEST_ASSERT(e1 != e2, "Entities have unique IDs");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// Test component addition
static TestResult test_component_add(void) {
    ECSWorld* world = ecs_world_create();
    Entity entity = ecs_create_entity(world);
    
    typedef struct {
        float x, y, z;
    } Position;
    
    Position pos = {10.0f, 20.0f, 30.0f};
    
    bool added = ecs_add_component(world, entity, COMPONENT_POSITION, &pos, sizeof(Position));
    TEST_ASSERT(added, "Component added");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// Test component retrieval
static TestResult test_component_get(void) {
    ECSWorld* world = ecs_world_create();
    Entity entity = ecs_create_entity(world);
    
    typedef struct {
        float x, y, z;
    } Position;
    
    Position pos = {10.0f, 20.0f, 30.0f};
    ecs_add_component(world, entity, COMPONENT_POSITION, &pos, sizeof(Position));
    
    Position* retrieved = (Position*)ecs_get_component(world, entity, COMPONENT_POSITION);
    TEST_ASSERT_NOT_NULL(retrieved, "Component retrieved");
    TEST_ASSERT_FLOAT_EQ(retrieved->x, 10.0f, 0.001f, "X component matches");
    TEST_ASSERT_FLOAT_EQ(retrieved->y, 20.0f, 0.001f, "Y component matches");
    TEST_ASSERT_FLOAT_EQ(retrieved->z, 30.0f, 0.001f, "Z component matches");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// Test component removal
static TestResult test_component_remove(void) {
    ECSWorld* world = ecs_world_create();
    Entity entity = ecs_create_entity(world);
    
    typedef struct { float x, y, z; } Position;
    Position pos = {10.0f, 20.0f, 30.0f};
    
    ecs_add_component(world, entity, COMPONENT_POSITION, &pos, sizeof(Position));
    TEST_ASSERT(ecs_has_component(world, entity, COMPONENT_POSITION), "Has component");
    
    bool removed = ecs_remove_component(world, entity, COMPONENT_POSITION);
    TEST_ASSERT(removed, "Component removed");
    TEST_ASSERT(!ecs_has_component(world, entity, COMPONENT_POSITION), "Component no longer exists");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// Test entity destruction
static TestResult test_entity_destroy(void) {
    ECSWorld* world = ecs_world_create();
    
    Entity entity = ecs_create_entity(world);
    TEST_ASSERT(ecs_entity_is_alive(world, entity), "Entity is alive");
    
    ecs_destroy_entity(world, entity);
    TEST_ASSERT(!ecs_entity_is_alive(world, entity), "Entity destroyed");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// Test querying entities with specific components
static TestResult test_entity_query(void) {
    ECSWorld* world = ecs_world_create();
    
    typedef struct { float x, y, z; } Position;
    typedef struct { float vx, vy, vz; } Velocity;
    
    // Create entities with different component combinations
    Entity e1 = ecs_create_entity(world);
    Position pos1 = {1, 2, 3};
    ecs_add_component(world, e1, COMPONENT_POSITION, &pos1, sizeof(Position));
    
    Entity e2 = ecs_create_entity(world);
    Position pos2 = {4, 5, 6};
    Velocity vel2 = {1, 0, 0};
    ecs_add_component(world, e2, COMPONENT_POSITION, &pos2, sizeof(Position));
    ecs_add_component(world, e2, COMPONENT_VELOCITY, &vel2, sizeof(Velocity));
    
    // Query entities with Position component
    EntityQuery query;
    query.required_components[0] = COMPONENT_POSITION;
    query.required_count = 1;
    
    EntityList* results = ecs_query_entities(world, &query);
    TEST_ASSERT_NOT_NULL(results, "Query results received");
    TEST_ASSERT(results->count >= 2, "At least 2 entities with Position");
    
    ecs_free_entity_list(results);
    ecs_world_destroy(world);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("ECS", "Initialization", test_ecs_init, NULL, NULL);
    test_register("ECS", "Entity Creation", test_entity_creation, NULL, NULL);
    test_register("ECS", "Component Add", test_component_add, NULL, NULL);
    test_register("ECS", "Component Get", test_component_get, NULL, NULL);
    test_register("ECS", "Component Remove", test_component_remove, NULL, NULL);
    test_register("ECS", "Entity Destroy", test_entity_destroy, NULL, NULL);
    test_register("ECS", "Entity Query", test_entity_query, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  ECS Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
