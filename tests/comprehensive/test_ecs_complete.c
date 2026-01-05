/**
 * COMPREHENSIVE ECS TESTS
 * Tests for all 15+ Entity Component System features
 * 
 * Coverage:
 * - Entity lifecycle
 * - Component CRUD
 * - System execution
 * - Query system
 * - Archetype storage
 * - Prefabs
 * - Serialization
 */

#include "../test_framework_unified.h"
#include <ecs/ecs.h>
#include <ecs/entity.h>
#include <ecs/component.h>
#include <ecs/system.h>
#include <ecs/query.h>
#include <ecs/prefab.h>
#include <ecs/serialization.h>

// =============================================================================
// TEST COMPONENTS
// =============================================================================

typedef struct {
    Vec3 position;
    Quaternion rotation;
    Vec3 scale;
} TransformComponent;

typedef struct {
    Vec3 velocity;
    Vec3 acceleration;
} VelocityComponent;

typedef struct {
    float health;
    float max_health;
} HealthComponent;

typedef struct {
    char name[64];
    int level;
} TagComponent;

// =============================================================================
// ENTITY LIFECYCLE TESTS
// =============================================================================

static TestResult test_entity_creation(void) {
    World *world = ecs_world_create();
    TEST_ASSERT_NOT_NULL(world, "World should be created");
    
    Entity e = ecs_create_entity(world);
    TEST_ASSERT_ENTITY_VALID(e, "Entity should be valid");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_entity_destruction(void) {
    World *world = ecs_world_create();
    
    Entity e = ecs_create_entity(world);
    TEST_ASSERT_TRUE(ecs_entity_exists(world, e), "Entity should exist");
    
    ecs_destroy_entity(world, e);
    TEST_ASSERT_FALSE(ecs_entity_exists(world, e), "Entity should not exist");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_entity_batch_creation(void) {
    World *world = ecs_world_create();
    
    Entity entities[1000];
    ecs_create_entities(world, entities, 1000);
    
    for (int i = 0; i < 1000; i++) {
        TEST_ASSERT_ENTITY_VALID(entities[i], "Batch entity should be valid");
    }
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_entity_recycling(void) {
    World *world = ecs_world_create();
    
    Entity e1 = ecs_create_entity(world);
    uint32_t id1 = ecs_entity_id(e1);
    ecs_destroy_entity(world, e1);
    
    Entity e2 = ecs_create_entity(world);
    uint32_t id2 = ecs_entity_id(e2);
    
    // ID should be reused but generation differs
    TEST_ASSERT_EQ(id1, id2, "ID should be recycled");
    TEST_ASSERT_NEQ(e1, e2, "Entity handles should differ (generation)");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// =============================================================================
// COMPONENT TESTS
// =============================================================================

static TestResult test_component_add(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    
    Entity e = ecs_create_entity(world);
    
    TransformComponent transform = {
        .position = {1.0f, 2.0f, 3.0f},
        .rotation = {0.0f, 0.0f, 0.0f, 1.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    };
    
    ecs_add_component(world, e, TransformComponent, &transform);
    
    TEST_ASSERT_TRUE(ecs_has_component(world, e, TransformComponent), 
                    "Entity should have transform");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_component_get(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    
    Entity e = ecs_create_entity(world);
    
    TransformComponent transform = {
        .position = {10.0f, 20.0f, 30.0f},
        .rotation = {0.0f, 0.0f, 0.0f, 1.0f},
        .scale = {2.0f, 2.0f, 2.0f}
    };
    
    ecs_add_component(world, e, TransformComponent, &transform);
    
    TransformComponent *retrieved = ecs_get_component(world, e, TransformComponent);
    TEST_ASSERT_NOT_NULL(retrieved, "Should get component");
    TEST_ASSERT_FLOAT_EQ(retrieved->position.x, 10.0f, 0.01f, "X position match");
    TEST_ASSERT_FLOAT_EQ(retrieved->scale.y, 2.0f, 0.01f, "Y scale match");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_component_remove(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    
    Entity e = ecs_create_entity(world);
    TransformComponent transform = {0};
    
    ecs_add_component(world, e, TransformComponent, &transform);
    TEST_ASSERT_TRUE(ecs_has_component(world, e, TransformComponent), "Should have");
    
    ecs_remove_component(world, e, TransformComponent);
    TEST_ASSERT_FALSE(ecs_has_component(world, e, TransformComponent), "Should not have");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_multiple_components(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    ecs_register_component(world, VelocityComponent);
    ecs_register_component(world, HealthComponent);
    
    Entity e = ecs_create_entity(world);
    
    TransformComponent t = {.position = {1, 2, 3}};
    VelocityComponent v = {.velocity = {4, 5, 6}};
    HealthComponent h = {.health = 100.0f, .max_health = 100.0f};
    
    ecs_add_component(world, e, TransformComponent, &t);
    ecs_add_component(world, e, VelocityComponent, &v);
    ecs_add_component(world, e, HealthComponent, &h);
    
    TEST_ASSERT_TRUE(ecs_has_component(world, e, TransformComponent), "Has transform");
    TEST_ASSERT_TRUE(ecs_has_component(world, e, VelocityComponent), "Has velocity");
    TEST_ASSERT_TRUE(ecs_has_component(world, e, HealthComponent), "Has health");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// =============================================================================
// SYSTEM TESTS
// =============================================================================

static int g_movement_system_called = 0;

static void movement_system(World *world, float dt) {
    Query *query = ecs_query_create(world);
    ecs_query_with(query, TransformComponent);
    ecs_query_with(query, VelocityComponent);
    
    QueryIterator iter = ecs_query_iter(query);
    while (ecs_query_next(&iter)) {
        TransformComponent *t = ecs_query_get(&iter, TransformComponent);
        VelocityComponent *v = ecs_query_get(&iter, VelocityComponent);
        
        t->position.x += v->velocity.x * dt;
        t->position.y += v->velocity.y * dt;
        t->position.z += v->velocity.z * dt;
    }
    
    ecs_query_destroy(query);
    g_movement_system_called++;
}

static TestResult test_system_registration(void) {
    World *world = ecs_world_create();
    
    SystemHandle sys = ecs_register_system(world, "MovementSystem", movement_system);
    TEST_ASSERT_TRUE(sys != 0, "System should be registered");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_system_execution(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    ecs_register_component(world, VelocityComponent);
    
    g_movement_system_called = 0;
    ecs_register_system(world, "MovementSystem", movement_system);
    
    // Create entity with components
    Entity e = ecs_create_entity(world);
    TransformComponent t = {.position = {0, 0, 0}};
    VelocityComponent v = {.velocity = {10, 0, 0}};
    ecs_add_component(world, e, TransformComponent, &t);
    ecs_add_component(world, e, VelocityComponent, &v);
    
    // Run systems
    ecs_update(world, 1.0f);
    
    TEST_ASSERT_EQ(g_movement_system_called, 1, "System should run once");
    
    TransformComponent *result = ecs_get_component(world, e, TransformComponent);
    TEST_ASSERT_FLOAT_EQ(result->position.x, 10.0f, 0.01f, "X should move");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_system_ordering(void) {
    World *world = ecs_world_create();
    
    // Register systems with dependencies
    SystemHandle sys_a = ecs_register_system(world, "SystemA", NULL);
    SystemHandle sys_b = ecs_register_system(world, "SystemB", NULL);
    SystemHandle sys_c = ecs_register_system(world, "SystemC", NULL);
    
    // B runs after A, C runs after B
    ecs_system_add_dependency(world, sys_b, sys_a);
    ecs_system_add_dependency(world, sys_c, sys_b);
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// =============================================================================
// QUERY TESTS
// =============================================================================

static TestResult test_query_single_component(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    
    // Create 10 entities with transforms
    for (int i = 0; i < 10; i++) {
        Entity e = ecs_create_entity(world);
        TransformComponent t = {.position = {(float)i, 0, 0}};
        ecs_add_component(world, e, TransformComponent, &t);
    }
    
    // Query all transforms
    Query *query = ecs_query_create(world);
    ecs_query_with(query, TransformComponent);
    
    int count = 0;
    QueryIterator iter = ecs_query_iter(query);
    while (ecs_query_next(&iter)) {
        count++;
    }
    
    TEST_ASSERT_EQ(count, 10, "Should find 10 entities");
    
    ecs_query_destroy(query);
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_query_multiple_components(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    ecs_register_component(world, VelocityComponent);
    
    // Create entities: 10 with both, 5 with only transform
    for (int i = 0; i < 10; i++) {
        Entity e = ecs_create_entity(world);
        TransformComponent t = {0};
        VelocityComponent v = {0};
        ecs_add_component(world, e, TransformComponent, &t);
        ecs_add_component(world, e, VelocityComponent, &v);
    }
    for (int i = 0; i < 5; i++) {
        Entity e = ecs_create_entity(world);
        TransformComponent t = {0};
        ecs_add_component(world, e, TransformComponent, &t);
    }
    
    // Query entities with both
    Query *query = ecs_query_create(world);
    ecs_query_with(query, TransformComponent);
    ecs_query_with(query, VelocityComponent);
    
    int count = 0;
    QueryIterator iter = ecs_query_iter(query);
    while (ecs_query_next(&iter)) {
        count++;
    }
    
    TEST_ASSERT_EQ(count, 10, "Should find 10 entities with both");
    
    ecs_query_destroy(query);
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_query_without(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    ecs_register_component(world, TagComponent);
    
    // Create 10 with transform, 3 also have tag
    for (int i = 0; i < 10; i++) {
        Entity e = ecs_create_entity(world);
        TransformComponent t = {0};
        ecs_add_component(world, e, TransformComponent, &t);
        
        if (i < 3) {
            TagComponent tag = {0};
            ecs_add_component(world, e, TagComponent, &tag);
        }
    }
    
    // Query transforms WITHOUT tag
    Query *query = ecs_query_create(world);
    ecs_query_with(query, TransformComponent);
    ecs_query_without(query, TagComponent);
    
    int count = 0;
    QueryIterator iter = ecs_query_iter(query);
    while (ecs_query_next(&iter)) {
        count++;
    }
    
    TEST_ASSERT_EQ(count, 7, "Should find 7 entities without tag");
    
    ecs_query_destroy(query);
    ecs_world_destroy(world);
    return TEST_PASS;
}

// =============================================================================
// ARCHETYPE TESTS
// =============================================================================

static TestResult test_archetype_creation(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    ecs_register_component(world, VelocityComponent);
    
    // Creating entities with same components should share archetype
    Entity e1 = ecs_create_entity(world);
    Entity e2 = ecs_create_entity(world);
    
    TransformComponent t = {0};
    VelocityComponent v = {0};
    
    ecs_add_component(world, e1, TransformComponent, &t);
    ecs_add_component(world, e1, VelocityComponent, &v);
    
    ecs_add_component(world, e2, TransformComponent, &t);
    ecs_add_component(world, e2, VelocityComponent, &v);
    
    Archetype *a1 = ecs_get_archetype(world, e1);
    Archetype *a2 = ecs_get_archetype(world, e2);
    
    TEST_ASSERT_EQ(a1, a2, "Should share same archetype");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// =============================================================================
// PREFAB TESTS
// =============================================================================

static TestResult test_prefab_creation(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    ecs_register_component(world, HealthComponent);
    
    // Create prefab
    Prefab *prefab = ecs_prefab_create(world, "Enemy");
    
    TransformComponent t = {.position = {0, 0, 0}, .scale = {1, 1, 1}};
    HealthComponent h = {.health = 100.0f, .max_health = 100.0f};
    
    ecs_prefab_add_component(prefab, TransformComponent, &t);
    ecs_prefab_add_component(prefab, HealthComponent, &h);
    
    TEST_ASSERT_NOT_NULL(prefab, "Prefab should be created");
    
    ecs_prefab_destroy(prefab);
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_prefab_instantiation(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    ecs_register_component(world, HealthComponent);
    
    Prefab *prefab = ecs_prefab_create(world, "Enemy");
    TransformComponent t = {.position = {0, 0, 0}};
    HealthComponent h = {.health = 100.0f, .max_health = 100.0f};
    ecs_prefab_add_component(prefab, TransformComponent, &t);
    ecs_prefab_add_component(prefab, HealthComponent, &h);
    
    // Instantiate
    Entity instance = ecs_prefab_instantiate(world, prefab);
    
    TEST_ASSERT_ENTITY_VALID(instance, "Instance should be valid");
    TEST_ASSERT_TRUE(ecs_has_component(world, instance, TransformComponent), "Has transform");
    TEST_ASSERT_TRUE(ecs_has_component(world, instance, HealthComponent), "Has health");
    
    HealthComponent *inst_health = ecs_get_component(world, instance, HealthComponent);
    TEST_ASSERT_FLOAT_EQ(inst_health->health, 100.0f, 0.01f, "Health copied");
    
    ecs_prefab_destroy(prefab);
    ecs_world_destroy(world);
    return TEST_PASS;
}

// =============================================================================
// SERIALIZATION TESTS
// =============================================================================

static TestResult test_world_serialization(void) {
    World *world = ecs_world_create();
    ecs_register_component(world, TransformComponent);
    
    // Create some entities
    for (int i = 0; i < 5; i++) {
        Entity e = ecs_create_entity(world);
        TransformComponent t = {.position = {(float)i, 0, 0}};
        ecs_add_component(world, e, TransformComponent, &t);
    }
    
    // Serialize
    uint8_t buffer[4096];
    size_t size = ecs_serialize_world(world, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(size > 0, "Should serialize");
    
    // Deserialize into new world
    World *world2 = ecs_world_create();
    ecs_register_component(world2, TransformComponent);
    
    bool success = ecs_deserialize_world(world2, buffer, size);
    TEST_ASSERT_TRUE(success, "Should deserialize");
    
    ecs_world_destroy(world);
    ecs_world_destroy(world2);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ecs_tests(void) {
    // Entity lifecycle tests
    TEST_REGISTER("ECS", "Entity creation", test_entity_creation);
    TEST_REGISTER("ECS", "Entity destruction", test_entity_destruction);
    TEST_REGISTER("ECS", "Batch entity creation", test_entity_batch_creation);
    TEST_REGISTER("ECS", "Entity recycling", test_entity_recycling);
    
    // Component tests
    TEST_REGISTER("ECS", "Component add", test_component_add);
    TEST_REGISTER("ECS", "Component get", test_component_get);
    TEST_REGISTER("ECS", "Component remove", test_component_remove);
    TEST_REGISTER("ECS", "Multiple components", test_multiple_components);
    
    // System tests
    TEST_REGISTER("ECS", "System registration", test_system_registration);
    TEST_REGISTER("ECS", "System execution", test_system_execution);
    TEST_REGISTER("ECS", "System ordering", test_system_ordering);
    
    // Query tests
    TEST_REGISTER("ECS", "Query single component", test_query_single_component);
    TEST_REGISTER("ECS", "Query multiple components", test_query_multiple_components);
    TEST_REGISTER("ECS", "Query without component", test_query_without);
    
    // Archetype tests
    TEST_REGISTER("ECS", "Archetype creation", test_archetype_creation);
    
    // Prefab tests
    TEST_REGISTER("ECS", "Prefab creation", test_prefab_creation);
    TEST_REGISTER("ECS", "Prefab instantiation", test_prefab_instantiation);
    
    // Serialization tests
    TEST_REGISTER("ECS", "World serialization", test_world_serialization);
}
