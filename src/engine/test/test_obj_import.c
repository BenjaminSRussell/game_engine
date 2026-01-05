#include "../../include/core/logger.h"
#include "../../include/engine/asset_importers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Minimal OBJ for a single triangle
const char MINIMAL_OBJ[] = "# Minimal OBJ - Single Triangle\n"
                           "v 0.0 0.0 0.0\n"
                           "v 1.0 0.0 0.0\n"
                           "v 0.5 1.0 0.0\n"
                           "vt 0.0 0.0\n"
                           "vt 1.0 0.0\n"
                           "vt 0.5 1.0\n"
                           "vn 0.0 0.0 1.0\n"
                           "vn 0.0 0.0 1.0\n"
                           "vn 0.0 0.0 1.0\n"
                           "f 1/1/1 2/2/2 3/3/3\n";

// Cube OBJ with quads (tests triangulation)
const char CUBE_OBJ[] = "# Cube with quads\n"
                        "v -1.0 -1.0 -1.0\n"
                        "v  1.0 -1.0 -1.0\n"
                        "v  1.0  1.0 -1.0\n"
                        "v -1.0  1.0 -1.0\n"
                        "v -1.0 -1.0  1.0\n"
                        "v  1.0 -1.0  1.0\n"
                        "v  1.0  1.0  1.0\n"
                        "v -1.0  1.0  1.0\n"
                        "f 1 2 3 4\n"  // Back face (quad -> 2 triangles)
                        "f 5 6 7 8\n"  // Front face
                        "f 1 2 6 5\n"  // Bottom face
                        "f 3 4 8 7\n"  // Top face
                        "f 1 4 8 5\n"  // Left face
                        "f 2 3 7 6\n"; // Right face

int main() {
  logger_init(LOG_LEVEL_INFO, LOG_TARGET_CONSOLE, NULL);
  LOG_INFO("Starting OBJ Importer Test...");

  ModelImportOptions options = {0};
  options.scale = 1.0f;

  // Test 1: Load minimal triangle from memory
  LOG_INFO("\n=== Test 1: Triangle ===");
  LOG_INFO("OBJ Data Size: %zu bytes", sizeof(MINIMAL_OBJ) - 1);
  ImportedModel *triangle_model = asset_importer_load_obj_from_memory(
      MINIMAL_OBJ, sizeof(MINIMAL_OBJ) - 1, &options);

  if (triangle_model) {
    LOG_INFO("✓ Successfully loaded triangle model!");
    LOG_INFO("  Mesh count: %u", triangle_model->mesh_count);

    if (triangle_model->mesh_count > 0) {
      LOG_INFO("  Vertices: %zu", triangle_model->meshes[0].vertex_count);
      LOG_INFO("  Indices: %zu", triangle_model->meshes[0].index_count);
      LOG_INFO("  Expected: 3 vertices, 3 indices");

      if (triangle_model->meshes[0].vertex_count == 3 &&
          triangle_model->meshes[0].index_count == 3) {
        LOG_INFO("✓ Triangle vertex/index count correct!");
      } else {
        LOG_ERROR("✗ Triangle vertex/index count mismatch!");
      }
    }

    asset_importer_free_model(triangle_model);
  } else {
    LOG_ERROR("✗ Failed to load triangle model!");
    return 1;
  }

  // Test 2: Load cube with quads (tests triangulation)
  LOG_INFO("\n=== Test 2: Cube (Quads -> Triangles) ===");
  LOG_INFO("OBJ Data Size: %zu bytes", sizeof(CUBE_OBJ) - 1);
  ImportedModel *cube_model = asset_importer_load_obj_from_memory(
      CUBE_OBJ, sizeof(CUBE_OBJ) - 1, &options);

  if (cube_model) {
    LOG_INFO("✓ Successfully loaded cube model!");
    LOG_INFO("  Mesh count: %u", cube_model->mesh_count);

    if (cube_model->mesh_count > 0) {
      LOG_INFO("  Vertices: %zu", cube_model->meshes[0].vertex_count);
      LOG_INFO("  Indices: %zu", cube_model->meshes[0].index_count);
      LOG_INFO(
          "  Expected: 8 unique vertices, 36 indices (6 quads = 12 triangles)");

      if (cube_model->meshes[0].vertex_count == 8 &&
          cube_model->meshes[0].index_count == 36) {
        LOG_INFO("✓ Cube triangulation correct!");
      } else {
        LOG_ERROR("✗ Cube triangulation mismatch!");
      }
    }

    asset_importer_free_model(cube_model);
  } else {
    LOG_ERROR("✗ Failed to load cube model!");
    return 1;
  }

  LOG_INFO("\n=== All Tests Complete ===");
  return 0;
}
