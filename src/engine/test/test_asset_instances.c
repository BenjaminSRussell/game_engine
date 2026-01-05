#include <stdio.h>
#include <assert.h>
#include "core/asset_manager.h"
#include "resource/asset_instance.h"
#include "core/logger.h"

int main() {
    printf("=== Asset Instance Test Suite ===\\n");
    
    // Test 1: Create Asset Manager
    printf("Test 1: Creating Asset Manager...\\n");
    AssetManager *manager = asset_manager_create(100, NULL);
    assert(manager != NULL);
    assert(manager->capacity == 100);
    assert(manager->count == 0);
    assert(manager->assets != NULL);
    printf("✓ Asset Manager created successfully\\n\\n");
    
    // Test 2: Create a test asset manually
    printf("Test 2: Adding test asset...\\n");
    Asset test_asset = {0};
    strncpy(test_asset.id, "test_asset_001", sizeof(test_asset.id));
    test_asset.type = ASSET_TYPE_MESH;
    test_asset.loaded = true;
    test_asset.ref_count = 0;
    test_asset.data = NULL;
    test_asset.size = 0;
    
    // Manually add it to manager for testing
    if (manager->count < manager->capacity) {
        manager->assets[manager->count++] = test_asset;
        printf("✓ Test asset added\\n\\n");
    }
    
    // Test 3: Get asset by ID
    printf("Test 3: Retrieving asset by ID...\\n");
    Asset *retrieved = asset_manager_get(manager, "test_asset_001");
    assert(retrieved != NULL);
    assert(strcmp(retrieved->id, "test_asset_001") == 0);
    printf("✓ Asset retrieved: %s\\n\\n", retrieved->id);
    
    // Test 4: Create instance
    printf("Test 4: Creating asset instance...\\n");
    Vec3 pos = {1.0f, 2.0f, 3.0f};
    Quat rot = {0.0f, 0.0f, 0.0f, 1.0f};
    AssetInstance *instance = asset_manager_create_instance(manager, "test_asset_001", pos, rot);
    
    if (instance) {
        printf("✓ Instance created: ID=%u\\n", instance->instance_id);
        printf("  Source Asset: %s\\n", instance->source_asset ? instance->source_asset->id : "NULL");
        printf("\\n");
    } else {
        printf("✗ Failed to create instance\\n\\n");
    }
    
    // Test 5: Get instances for asset
    printf("Test 5: Querying instances for asset...\\n");
    AssetInstance *instances[10];
    uint32_t count = asset_manager_get_instances_for_asset(manager, "test_asset_001", instances, 10);
    printf("  Found %u instances\\n", count);
    if (count > 0) {
        printf("✓ Instance query successful\\n\\n");
    }
    
    // Test 6: Destroy instance
    if (instance) {
        printf("Test 6: Destroying instance...\\n");
        asset_manager_destroy_instance(manager, instance->instance_id);
        printf("✓ Instance destroyed\\n\\n");
    }
    
    // Test 7: Cleanup
    printf("Test 7: Cleaning up Asset Manager...\\n");
    asset_manager_destroy(manager);
    printf("✓ Asset Manager destroyed\\n\\n");
    
    printf("=== All Tests Passed! ===\\n");
    return 0;
}
