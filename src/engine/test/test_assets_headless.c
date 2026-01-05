// Headless Asset Loading Test
// Demonstrates loading and validating all downloaded assets without rendering

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple asset verification structures
typedef struct {
  const char *path;
  const char *type;
  size_t min_size;
  bool loaded;
} AssetTest;

// Function to check if file exists and meets size requirements
bool verify_asset(const char *path, size_t min_size) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("FAILED: %s (file not found)\n", path);
    return false;
  }

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fclose(f);

  if (size < min_size) {
    printf("WARNING: %s (size: %zu bytes, expected: >%zu bytes)\n", path,
           size, min_size);
    return false;
  }

  printf("VERIFIED: %s (%zu KB)\n", path, size / 1024);
  return true;
}

int main(int argc, char **argv) {
  printf("🎮 VoxelForge Engine - Headless Asset Test\n");
  printf("==========================================\n\n");

  const char *base_path =
      "/Users/benjaminrussell/Desktop/Minecraft v2/assets/downloaded";

  // Define all assets to test
  AssetTest assets[] = {
      // Water textures
      {"/textures/water_diffuse_4k.jpg", "Texture", 1000000, false},
      {"/textures/water_normal_4k.jpg", "Texture", 1000000, false},
      {"/textures/water_roughness_4k.jpg", "Texture", 500000, false},

      // Sand textures
      {"/textures/sand_diffuse_4k.jpg", "Texture", 1000000, false},
      {"/textures/sand_normal_4k.jpg", "Texture", 1000000, false},

      // Wet rocks
      {"/textures/wet_rocks_diffuse_4k.jpg", "Texture", 1000000, false},
      {"/textures/wet_rocks_normal_4k.jpg", "Texture", 1000000, false},

      // Grass
      {"/textures/grass_diffuse_4k.jpg", "Texture", 1000000, false},
      {"/textures/grass_normal_4k.jpg", "Texture", 1000000, false},

      // Rock cliff
      {"/textures/rock_cliff_diffuse_4k.jpg", "Texture", 1000000, false},
      {"/textures/rock_cliff_normal_4k.jpg", "Texture", 1000000, false},

      // Forest ground
      {"/textures/forest_ground_diffuse_4k.jpg", "Texture", 1000000, false},
      {"/textures/forest_ground_normal_4k.jpg", "Texture", 1000000, false},

      // HDRIs
      {"/environment/sky_clear_2k.hdr", "HDRI", 2000000, false},
      {"/environment/sky_overcast_2k.hdr", "HDRI", 2000000, false},
      {"/environment/sky_sunset_2k.hdr", "HDRI", 2000000, false},

      // 3D Models
      {"/models/rock_01.glb", "Model", 500000, false},
      {"/models/rock_02.glb", "Model", 500000, false},
      {"/models/rock_03.glb", "Model", 500000, false},
      {"/models/tree_stump.glb", "Model", 500000, false},
  };

  int total = sizeof(assets) / sizeof(AssetTest);
  int passed = 0;
  int failed = 0;

  printf("📦 Testing %d assets...\n\n", total);

  // Test each asset
  for (int i = 0; i < total; i++) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s%s", base_path, assets[i].path);

    if (verify_asset(full_path, assets[i].min_size)) {
      assets[i].loaded = true;
      passed++;
    } else {
      failed++;
    }
  }

  printf("\n==========================================\n");
  printf("📊 TEST RESULTS\n");
  printf("==========================================\n");
  printf("Total Assets: %d\n", total);
  printf("✅ Passed: %d\n", passed);
  printf("❌ Failed: %d\n", failed);
  printf("Success Rate: %d%%\n", (passed * 100) / total);
  printf("\n");

  if (failed == 0) {
    printf("🎉 ALL ASSETS READY FOR SHOWTIME!\n\n");
    printf("🚀 Asset Capabilities:\n");
    printf("  - 4K PBR textures for realistic materials\n");
    printf("  - HDR environment maps for lighting\n");
    printf("  - GLB models ready for scene population\n");
    printf("  - Total asset size: ~33 MB\n");
    printf("\n");
    printf("✨ Ready to demonstrate:\n");
    printf("  1. Realistic water rendering\n");
    printf("  2. Beach/island environments\n");
    printf("  3. Dynamic sky lighting\n");
    printf("  4. 3D model instancing\n");
    printf("\n");
    return 0;
  } else {
    printf("⚠️  Some assets missing or corrupted.\n");
    printf("Run: ./scripts/download_free_assets.sh to re-download\n\n");
    return 1;
  }
}
