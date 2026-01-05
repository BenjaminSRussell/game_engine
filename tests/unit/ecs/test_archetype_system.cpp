#include <catch2/catch_test_macros.hpp>
#include <ecs/ecs.h>
#include <core/logger.h>

// Test archetype-based ECS implementation
TEST_CASE("ECS Archetype System", "[ecs]") {
    
    SECTION("World initialization") {
        World world = {0};
        ecs_world_init(&world, 1000, 50, 10);
        
        REQUIRE(world.entity_count == 0);
        REQUIRE(world.config.max_entities == 1000);
        
        ecs_world_free(&world);
    }
    
    SECTION("Entity creation and validation") {
        World world = {0};
        ecs_world_init(&world, 1000, 50, 10);
        
        Entity e1 = ecs_create_entity(&world);
        REQUIRE(e1.id != 0);
        REQUIRE(ecs_is_valid(&world, e1));
        REQUIRE(world.entity_count == 1);
        
        Entity e2 = ecs_create_entity(&world);
        REQUIRE(e2.id != e1.id);
        REQUIRE(ecs_is_valid(&world, e2));
        REQUIRE(world.entity_count == 2);
        
        ecs_world_free(&world);
    }
    
    SECTION("Component registration") {
        World world = {0};
        ecs_world_init(&world, 1000, 50, 10);
        
        ComponentInfo transform_info = {0};
        transform_info.name = "Transform";
        transform_info.size = sizeof(float) * 16; // 4x4 matrix
        transform_info.alignment = 16;
        
        ECSComponentID transform_id = ecs_register_component(&world, &transform_info);
        REQUIRE(transform_id != 0);
        REQUIRE(world.component_count == 1);
        
        const ComponentInfo* retrieved = ecs_get_component_info(&world, transform_id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->size == sizeof(float) * 16);
        
        ecs_world_free(&world);
    }
    
    SECTION("Archetype migration on component add") {
        World world = {0};
        ecs_world_init(&world, 1000, 50, 10);
        
        // Register components
        ComponentInfo position_info = {0};
        position_info.name = "Position";
        position_info.size = sizeof(float) * 3;
        position_info.alignment = 4;
        ECSComponentID position_id = ecs_register_component(&world, &position_info);
        
        ComponentInfo velocity_info = {0};
        velocity_info.name = "Velocity";
        velocity_info.size = sizeof(float) * 3;
        velocity_info.alignment = 4;
        ECSComponentID velocity_id = ecs_register_component(&world, &velocity_info);
        
        // Create entity
        Entity entity = ecs_create_entity(&world);
        REQUIRE(ecs_is_valid(&world, entity));
        
        // Add first component (triggers archetype migration from empty to [Position])
        float position[3] = {1.0f, 2.0f, 3.0f};
        bool added = ecs_add_component(&world, entity, position_id, position);
        REQUIRE(added);
        REQUIRE(ecs_has_component(&world, entity, position_id));
        
        // Add second component (triggers migration from [Position] to [Position, Velocity])
        float velocity[3] = {0.5f, 0.0f, 0.0f};
        added = ecs_add_component(&world, entity, velocity_id, velocity);
        REQUIRE(added);
        REQUIRE(ecs_has_component(&world, entity, position_id));
        REQUIRE(ecs_has_component(&world, entity, velocity_id));
        
        // Verify data integrity after migrations
        float* pos_data = (float*)ecs_get_component(&world, entity, position_id);
        REQUIRE(pos_data != nullptr);
        REQUIRE(pos_data[0] == 1.0f);
        REQUIRE(pos_data[1] == 2.0f);
        REQUIRE(pos_data[2] == 3.0f);
        
        ecs_world_free(&world);
    }
    
    SECTION("Query matching archetypes") {
        World world = {0};
        ecs_world_init(&world, 1000, 50, 10);
        
        // Register components
        ComponentInfo position_info = {0};
        position_info.name = "Position";
        position_info.size = sizeof(float) * 3;
        ECSComponentID position_id = ecs_register_component(&world, &position_info);
        
        ComponentInfo velocity_info = {0};
        velocity_info.name = "Velocity";
        velocity_info.size = sizeof(float) * 3;
        ECSComponentID velocity_id = ecs_register_component(&world, &velocity_info);
        
        // Create entities with different component combinations
        Entity e1 = ecs_create_entity(&world);
        float pos1[3] = {0, 0, 0};
        ecs_add_component(&world, e1, position_id, pos1);
        
        Entity e2 = ecs_create_entity(&world);
        float pos2[3] = {1, 1, 1};
        float vel2[3] = {0.1f, 0, 0};
        ecs_add_component(&world, e2, position_id, pos2);
        ecs_add_component(&world, e2, velocity_id, vel2);
        
        // Query for entities with Position (should match both e1 and e2)
        QueryDesc desc = {0};
        ECSComponentID all_comps[1] = {position_id};
        desc.all_components = all_comps;
        desc.all_count = 1;
        
        Query* query = ecs_query_create(&world, &desc);
        REQUIRE(query != nullptr);
        
        int entity_count = 0;
        Entity entity;
        void* components[1];
        
        while (ecs_query_next(query, &entity, components)) {
            entity_count++;
            REQUIRE(ecs_has_component(&world, entity, position_id));
        }
        
        REQUIRE(entity_count == 2); // Both e1 and e2 have Position
        
        ecs_query_destroy(&world, query);
        ecs_world_free(&world);
    }
    
    SECTION("Cache-friendly iteration") {
        World world = {0};
        ecs_world_init(&world, 10000, 50, 10);
        
        ComponentInfo position_info = {0};
        position_info.name = "Position";
        position_info.size = sizeof(float) * 3;
        ECSComponentID position_id = ecs_register_component(&world, &position_info);
        
        // Create 1000 entities with Position component
        const int entity_count = 1000;
        for (int i = 0; i < entity_count; i++) {
            Entity e = ecs_create_entity(&world);
            float pos[3] = {(float)i, (float)i, (float)i};
            ecs_add_component(&world, e, position_id, pos);
        }
        
        // Verify contiguous memory and iteration
        QueryDesc desc = {0};
        ECSComponentID all_comps[1] = {position_id};
        desc.all_components = all_comps;
        desc.all_count = 1;
        
        Query* query = ecs_query_create(&world, &desc);
        REQUIRE(query != nullptr);
        
        int iterated = 0;
        Entity entity;
        void* components[1];
        
        while (ecs_query_next(query, &entity, components)) {
            float* pos = (float*)components[0];
            REQUIRE(pos != nullptr);
            iterated++;
        }
        
        REQUIRE(iterated == entity_count);
        
        ecs_query_destroy(&world, query);
        ecs_world_free(&world);
    }
}
