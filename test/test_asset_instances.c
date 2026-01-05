#include "resource/asset_instance.h"
#include "core/asset_manager.h"
#include "math/vec3.h"
#include "math/quat.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mock Asset structure if not fully available or needed
// We rely on the one in asset_manager.h

void test_registry_init() {
    printf("Testing Registry Initialization...\n");
    AssetInstanceRegistry registry;
    asset_instance_registry_init(&registry, 10);
    
    assert(registry.instances != NULL);
    assert(registry.capacity == 10);
    assert(registry.count == 0);
    assert(registry.next_id == 1);
    
    asset_instance_registry_destroy(&registry);
    assert(registry.instances == NULL);
    assert(registry.count == 0);
    printf("Passed.\n");
}

void test_instance_lifecycle() {
    printf("Testing Instance Lifecycle...\n");
    AssetInstanceRegistry registry;
    asset_instance_registry_init(&registry, 2); // Small capacity to force resize
    
    Asset mock_asset;
    strcpy(mock_asset.id, "test_asset");
    
    // Create 1
    AssetInstance *inst1 = asset_instance_registry_create(&registry, &mock_asset);
    assert(inst1 != NULL);
    assert(inst1->instance_id == 1);
    assert(inst1->source_asset == &mock_asset);
    assert(registry.count == 1);
    
    // Create 2
    AssetInstance *inst2 = asset_instance_registry_create(&registry, &mock_asset);
    assert(inst2 != NULL);
    assert(inst2->instance_id == 2);
    assert(registry.count == 2);
    
    // Create 3 (should resize)
    AssetInstance *inst3 = asset_instance_registry_create(&registry, &mock_asset);
    assert(inst3 != NULL);
    assert(inst3->instance_id == 3);
    assert(registry.count == 3);
    assert(registry.capacity >= 3);
    
    // Lookup
    AssetInstance *lookup = asset_instance_registry_get(&registry, 2);
    // assert(lookup == inst2); // Pointer inst2 is INVALID due to realloc above!
    assert(lookup != NULL);
    assert(lookup->instance_id == 2);
    // The registry_get function does linear search by ID.
    lookup = asset_instance_registry_get(&registry, 2);
    assert(lookup != NULL);
    assert(lookup->instance_id == 2);
    
    // Destroy generic instance 2
    asset_instance_registry_destroy_instance(&registry, 2);
    assert(registry.count == 2);
    assert(asset_instance_registry_get(&registry, 2) == NULL);
    
    // Validate compaction (swap-and-pop)
    // One instance should be 1, other should be 3.
    // Order might have changed.
    bool found1 = false, found3 = false;
    for(uint32_t i=0; i<registry.count; i++) {
        if (registry.instances[i].instance_id == 1) found1 = true;
        if (registry.instances[i].instance_id == 3) found3 = true;
    }
    assert(found1 && found3);

    asset_instance_registry_destroy(&registry);
    printf("Passed.\n");
}

void test_manager_integration() {
    printf("Testing Manager Integration...\n");
    AssetManager *mgr = asset_manager_create(10, NULL);
    
    // Inject a mock asset since load is stubbed
    // Manually allocate assets array because stubbed create leaves it NULL
    mgr->assets = (Asset*)calloc(10, sizeof(Asset));
    Asset *mock = &mgr->assets[0];
    strcpy(mock->id, "dummy_cube");
    mgr->count = 1;

    // Test API
    Vec3 pos = {0,0,0};
    Quat rot = {0,0,0,1};
    AssetInstance *inst = asset_manager_create_instance(mgr, "dummy_cube", pos, rot);
    
    assert(inst != NULL);
    assert(inst->source_asset == mock);
    assert(inst->instance_id > 0);
    
    // Test query
    AssetInstance *results[10];
    uint32_t count = asset_manager_get_instances_for_asset(mgr, "dummy_cube", results, 10);
    assert(count == 1);
    assert(results[0] == inst);
    
    // Test destroy
    asset_manager_destroy_instance(mgr, inst->instance_id);
    assert(mgr->instance_registry.count == 0);
    
    asset_manager_destroy(mgr);
    printf("Passed.\n");
}

int main() {
    printf("=== Running Asset Instance Tests ===\n");
    test_registry_init();
    test_instance_lifecycle();
    test_manager_integration();
    printf("=== All Tests Passed ===\n");
    return 0;
}
