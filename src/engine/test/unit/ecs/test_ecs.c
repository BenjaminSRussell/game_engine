#include <test_framework.h>
#include <ecs/ecs.h>
#include <string.h>

// Test components
typedef struct {
    float x, y, z;
} Position;

typedef struct {
    float x, y, z;
} Velocity;

// Global IDs for tests
static ECSComponentID pos_id = 0;
static ECSComponentID vel_id = 0;

TestResult test_ecs_entity_creation(void) {
    World *world = ecs_world_create(NULL);
    TEST_ASSERT_NOT_NULL(world, "World should be created");
    
    Entity e1 = ecs_create_entity(world);
    TEST_ASSERT_EQ(1, e1.id, "First entity ID should be 1");
    TEST_ASSERT_TRUE(ecs_is_valid(world, e1), "Entity should be valid");
    
    Entity e2 = ecs_create_entity(world);
    TEST_ASSERT_EQ(2, e2.id, "Second entity ID should be 2");
    
    ecs_destroy_entity(world, e1);
    TEST_ASSERT_FALSE(ecs_is_valid(world, e1), "Entity should be invalid after destroy");
    TEST_ASSERT_TRUE(ecs_is_valid(world, e2), "Other entity should remain valid");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

TestResult test_ecs_components(void) {
    World *world = ecs_world_create(NULL);
    
    // Register components
    pos_id = ecs_register_component_simple(world, "Position", sizeof(Position));
    vel_id = ecs_register_component_simple(world, "Velocity", sizeof(Velocity));
    
    Entity e = ecs_create_entity(world);
    
    Position p = {1.0f, 2.0f, 3.0f};
    bool added = ecs_add_component(world, e, pos_id, &p);
    TEST_ASSERT_TRUE(added, "Component should be added");
    
    Position *get_p = (Position*)ecs_get_component(world, e, pos_id);
    TEST_ASSERT_NOT_NULL(get_p, "Component should be retrievable");
    TEST_ASSERT_FLOAT_EQ(1.0f, get_p->x, 0.001f, "X match");
    TEST_ASSERT_FLOAT_EQ(2.0f, get_p->y, 0.001f, "Y match");
    TEST_ASSERT_FLOAT_EQ(3.0f, get_p->z, 0.001f, "Z match");
    
    // Add second component
    Velocity v = {10.0f, 0.0f, 0.0f};
    ecs_add_component(world, e, vel_id, &v);
    
    // Verify both exist
    TEST_ASSERT_NOT_NULL(ecs_get_component(world, e, pos_id), "Pos should exist");
    TEST_ASSERT_NOT_NULL(ecs_get_component(world, e, vel_id), "Vel should exist");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

TestResult test_ecs_queries(void) {
    World *world = ecs_world_create(NULL);
    pos_id = ecs_register_component_simple(world, "Position", sizeof(Position));
    vel_id = ecs_register_component_simple(world, "Velocity", sizeof(Velocity));
    
    // Create entities: 2 with Pos+Vel, 1 with Pos only, 1 with Vel only
    Entity e1 = ecs_create_entity(world);
    ecs_add_component(world, e1, pos_id, &(Position){1,1,1});
    ecs_add_component(world, e1, vel_id, &(Velocity){1,1,1});
    
    Entity e2 = ecs_create_entity(world);
    ecs_add_component(world, e2, pos_id, &(Position){2,2,2});
    ecs_add_component(world, e2, vel_id, &(Velocity){2,2,2});
    
    Entity e3 = ecs_create_entity(world);
    ecs_add_component(world, e3, pos_id, &(Position){3,3,3});
    
    Entity e4 = ecs_create_entity(world);
    ecs_add_component(world, e4, vel_id, &(Velocity){4,4,4});
    
    // Query for Pos+Vel
    QueryDesc desc = {
        .all_components = (ComponentType[]){pos_id, vel_id},
        .all_count = 2
    };
    
    Query *q = ecs_query_create(world, &desc);
    TEST_ASSERT_NOT_NULL(q, "Query creation failed");
    
    // Count matches
    int count = 0;
    Entity e;
    while(ecs_query_next(q, &e, NULL)) {
        count++;
        // Should be e1 or e2
        bool valid_id = (e.id == e1.id || e.id == e2.id);
        if (!valid_id) {
            printf("Unexpected Entity ID: %u (Expected %u or %u)\n", e.id, e1.id, e2.id);
        }
        TEST_ASSERT_TRUE(valid_id, "Query returned unexpected entity");
    }
    
    TEST_ASSERT_EQ(2, count, "Query should match 2 entities");
    
    ecs_query_destroy(world, q);
    ecs_world_destroy(world);
    return TEST_PASS;
}

void register_ecs_tests(void) {
    TEST_REGISTER("ECS", "Entity Creation", test_ecs_entity_creation);
    TEST_REGISTER("ECS", "Component Operations", test_ecs_components);
    TEST_REGISTER("ECS", "Query System", test_ecs_queries);
}
