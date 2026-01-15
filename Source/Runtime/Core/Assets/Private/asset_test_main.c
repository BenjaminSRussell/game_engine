#include "unified_assets.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include "unified_renderer.h"
#include <stdio.h>
#include <string.h>

// Simple file loader for testing
static bool simple_file_loader(const char *path, void **out_data,
                               uint64_t *out_size) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return false;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size <= 0) {
    fclose(file);
    return false;
  }

  // Read file
  void *data = UNIFIED_ALLOC(size);
  if (!data) {
    fclose(file);
    return false;
  }

  size_t read = fread(data, 1, size, file);
  fclose(file);

  if (read != (size_t)size) {
    UNIFIED_FREE(data);
    return false;
  }

  *out_data = data;
  *out_size = (uint64_t)size;
  return true;
}

static bool simple_file_unloader(void *data) {
  if (data) {
    UNIFIED_FREE(data);
  }
  return true;
}

static const char *simple_file_extensions() { return ".txt,.dat,.bin"; }

int main(void) {
  printf("Starting Asset System Verification...\\n");

  // Initialize logger
  if (!logger_init(NULL)) {
    printf("FAILED: Logger init\\n");
    return 1;
  }

  // Test 1: Create asset registry
  AssetRegistryConfig config = {.asset_root_path = "assets/",
                                .max_assets = 100,
                                .cache_size_mb = 64,
                                .enable_hot_reload = true,
                                .enable_compression = false,
                                .enable_streaming = false};

  AssetRegistry *registry = asset_registry_create(&config);
  if (!registry) {
    printf("FAILED: Asset registry creation\\n");
    return 1;
  }
  printf("PASSED: Asset registry created\\n");

  // Test 2: Register custom loader
  AssetLoaderCallbacks loader = {.load = simple_file_loader,
                                 .unload = simple_file_unloader,
                                 .validate = NULL,
                                 .get_supported_extensions =
                                     simple_file_extensions};

  if (!asset_register_loader(registry, ASSET_TYPE_TEXTURE, &loader)) {
    printf("FAILED: Loader registration\\n");
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Loader registered\\n");

  // Test 3: Create test asset file
  const char *test_file = "/tmp/test_asset.txt";
  FILE *f = fopen(test_file, "w");
  if (f) {
    fprintf(f, "Test asset data");
    fclose(f);
  }

  // Test 4: Load asset
  AssetHandle handle = asset_load(registry, test_file, ASSET_TYPE_TEXTURE,
                                  ASSET_PRIORITY_NORMAL);
  if (handle == INVALID_ASSET_HANDLE) {
    printf("FAILED: Asset loading\\n");
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Asset loaded\\n");

  // Test 5: Check loaded status
  if (!asset_is_loaded(registry, handle)) {
    printf("FAILED: Asset not marked as loaded\\n");
    asset_unload(registry, handle);
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Asset status check\\n");

  // Test 6: Get metadata
  const AssetMetadata *metadata = asset_get_metadata(registry, handle);
  if (!metadata || metadata->type != ASSET_TYPE_TEXTURE) {
    printf("FAILED: Asset metadata\\n");
    asset_unload(registry, handle);
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Asset metadata retrieved\\n");

  // Test 7: Get data
  void *data = asset_get_data(registry, handle);
  if (!data) {
    printf("FAILED: Asset data retrieval\\n");
    asset_unload(registry, handle);
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Asset data retrieved\\n");

  // Test 8: Reference counting
  asset_add_ref(registry, handle);
  if (metadata->ref_count != 2) {
    printf("FAILED: Reference counting (expected 2, got %u)\\n",
           metadata->ref_count);
    asset_unload(registry, handle);
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Reference counting\\n");

  // Test 9: Asset statistics
  uint32_t loaded_count = asset_get_loaded_count(registry);
  uint64_t memory_usage = asset_get_memory_usage(registry);
  if (loaded_count != 1 || memory_usage == 0) {
    printf("FAILED: Statistics (count=%u, memory=%llu)\\n", loaded_count,
           memory_usage);
    asset_release(registry, handle);
    asset_unload(registry, handle);
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Asset statistics\\n");

  // Test 10: Utility functions
  const char *type_str = asset_type_to_string(ASSET_TYPE_TEXTURE);
  AssetType type_from_ext = asset_type_from_extension(".png");
  bool path_valid = asset_validate_path("assets/test.png");
  bool path_invalid = asset_validate_path("../../../etc/passwd");

  if (strcmp(type_str, "TEXTURE") != 0 || type_from_ext != ASSET_TYPE_TEXTURE ||
      !path_valid || path_invalid) {
    printf("FAILED: Utility functions\\n");
    asset_release(registry, handle);
    asset_unload(registry, handle);
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Utility functions\\n");

  // Test 11: Hot reload toggle
  asset_enable_hot_reload(registry, false);
  asset_enable_hot_reload(registry, true);
  printf("PASSED: Hot reload toggle\\n");

  // Test 12: Unload asset
  asset_release(registry, handle);
  asset_unload(registry, handle);
  if (asset_get_loaded_count(registry) != 0) {
    printf("FAILED: Asset unload (count=%u)\\n",
           asset_get_loaded_count(registry));
    asset_registry_destroy(registry);
    return 1;
  }
  printf("PASSED: Asset unload\\n");

  // Test 13: Cleanup
  asset_registry_destroy(registry);
  printf("PASSED: Asset registry cleanup\\n");

  logger_shutdown();

  // Cleanup test file
  remove(test_file);

  printf("\\nAsset System Verification Successful!\\n");
  return 0;
}
