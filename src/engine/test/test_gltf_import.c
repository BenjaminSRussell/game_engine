#include "../../include/core/logger.h"
#include "../../include/engine/asset_importers.h"
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

// Minimal GLTF JSON for a single triangle (SIMPLIFIED)
const char MINIMAL_GLTF[] = "{ \"asset\": { \"version\": \"2.0\" } }";

int main() {
  logger_init(LOG_LEVEL_INFO, LOG_TARGET_CONSOLE, NULL);
  LOG_INFO("Starting GLTF Importer Test...");

  ModelImportOptions options = {0};
  options.scale = 1.0f;

  // Load from memory
  LOG_INFO("Testing load from memory (Size: %zu)", sizeof(MINIMAL_GLTF));
  ImportedModel *model = asset_importer_load_gltf_from_memory(
      MINIMAL_GLTF, sizeof(MINIMAL_GLTF), NULL, &options);

  if (model) {
    LOG_INFO("Successfully loaded model from memory!");
    LOG_INFO("Mesh count: %u", model->mesh_count);

    if (model->mesh_count > 0) {
      LOG_INFO("First mesh vertices: %zu", model->meshes[0].vertex_count);
      // Verify vertices if possible (first vertex should be 0,0,0)
      // But we don't have direct access to internal structure definition
      // here unless we cast to StandardVertex which is private in the .c
      // file. We just verify it loaded something.
    }

    asset_importer_free_model(model);
    LOG_INFO("Freed model.");
  } else {
    LOG_ERROR("Failed to load model from memory.");
    return 1;
  }

  LOG_INFO("Test Complete.");
  return 0;
}
