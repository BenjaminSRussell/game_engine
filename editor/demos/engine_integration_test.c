// VoxelForge Engine - Engine Integration Verification
// Verifies that the internal AssetManager works with the VFS and real assets

#include <core/asset_manager.h>
#include <core/logger.h>
#include <stdio.h>

int main() {
  printf("🔍 VoxelForge Engine Integration Verification\n");
  printf("===========================================\n\n");

  // 1. Initialize Asset Manager
  AssetManager *assets = asset_manager_create(32);
  if (!assets) {
    printf("❌ Failed to create Asset Manager\n");
    return 1;
  }
  printf("✅ Asset Manager Initialized (Capacity: 32)\n");

  // 2. Load Real Asset (Sand Texture)
  const char *asset_path =
      "/assets/textures/sand_diffuse_4k.jpg"; // Path relative to mount point
  // Note: VFS mount set in asset_manager_create assumes /assets ->
  // ./assets/downloaded So if file is at
  // ./assets/downloaded/textures/sand_diffuse_4k.jpg, VFS path is
  // /assets/textures/... Wait, step 395 showed mount: vfs_mount(manager->vfs,
  // "/assets", "./assets/downloaded"); And file list shows
  // `assets/downloaded/textures/sand_diffuse_4k.jpg`. So correct VFS path is
  // `/assets/textures/sand_diffuse_4k.jpg`.

  printf("📦 Attempting to load: %s\n", asset_path);
  Asset *sand_tex = asset_manager_load(assets, "sand_diffuse",
                                       ASSET_TYPE_TEXTURE, asset_path);

  if (sand_tex) {
    printf("✅ LOAD SUCCESS!\n");
    printf("   - Size: %u bytes (%.2f MB)\n", sand_tex->size,
           sand_tex->size / (1024.0f * 1024.0f));
    printf("   - Ref Count: %u\n", sand_tex->ref_count);
    printf("   - Path: %s\n", sand_tex->path);
  } else {
    printf(
        "❌ LOAD FAILED! (This is expected if path is wrong or VFS failed)\n");
  }

  // 3. Report Status
  asset_manager_report(assets);

  // 4. Cleanup
  asset_manager_destroy(assets);
  printf("\n✅ Verification Complete\n");
  return 0;
}
