// renderer/model_loader.c - Model loading implementation
#include <core/logger.h>
#include <core/memory.h>
#include <rendering/mesh.h>
#include <rendering/model_loader.h>
#include <stdio.h>
#include <string.h>

// Simple OBJ loader implementation
static bool load_obj_file(Model *model, const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    LOG_ERROR("Failed to open OBJ file: %s", path);
    return false;
  }

  // Temporary storage
  float *positions = NULL;
  float *normals = NULL;
  float *uvs = NULL;
  uint32_t *indices = NULL;

  uint32_t pos_count = 0, norm_count = 0, uv_count = 0, idx_count = 0;
  uint32_t pos_cap = 1024, norm_cap = 1024, uv_cap = 1024, idx_cap = 1024;

  positions = MALLOC(pos_cap * 3 * sizeof(float));
  normals = MALLOC(norm_cap * 3 * sizeof(float));
  uvs = MALLOC(uv_cap * 2 * sizeof(float));
  indices = MALLOC(idx_cap * sizeof(uint32_t));

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == 'v' && line[1] == ' ') {
      // Vertex position
      if (pos_count >= pos_cap) {
        pos_cap *= 2;
        positions = REALLOC(positions, pos_cap * 3 * sizeof(float));
      }
      sscanf(line + 2, "%f %f %f", &positions[pos_count * 3],
             &positions[pos_count * 3 + 1], &positions[pos_count * 3 + 2]);
      pos_count++;
    } else if (line[0] == 'v' && line[1] == 'n') {
      // Vertex normal
      if (norm_count >= norm_cap) {
        norm_cap *= 2;
        normals = REALLOC(normals, norm_cap * 3 * sizeof(float));
      }
      sscanf(line + 3, "%f %f %f", &normals[norm_count * 3],
             &normals[norm_count * 3 + 1], &normals[norm_count * 3 + 2]);
      norm_count++;
    } else if (line[0] == 'v' && line[1] == 't') {
      // Texture coordinate
      if (uv_count >= uv_cap) {
        uv_cap *= 2;
        uvs = REALLOC(uvs, uv_cap * 2 * sizeof(float));
      }
      sscanf(line + 3, "%f %f", &uvs[uv_count * 2], &uvs[uv_count * 2 + 1]);
      uv_count++;
    } else if (line[0] == 'f' && line[1] == ' ') {
      // Face (simplified - assumes triangulated)
      uint32_t v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
      int matches = sscanf(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u", &v1, &vt1,
                           &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);

      if (matches == 9) {
        if (idx_count + 3 >= idx_cap) {
          idx_cap *= 2;
          indices = REALLOC(indices, idx_cap * sizeof(uint32_t));
        }
        indices[idx_count++] = v1 - 1;
        indices[idx_count++] = v2 - 1;
        indices[idx_count++] = v3 - 1;
      }
    }
  }

  fclose(file);

  // Create mesh from loaded data
  model->mesh_count = 1;
  model->meshes = MALLOC(sizeof(Mesh *));
  model->meshes[0] = MALLOC(sizeof(Mesh));

  Mesh *mesh = model->meshes[0];
  mesh->vertex_count = pos_count;
  mesh->index_count = idx_count;

  // Allocate and copy vertex data
  mesh->vertices = MALLOC(pos_count * sizeof(Vertex));
  for (uint32_t i = 0; i < pos_count; i++) {
    mesh->vertices[i].position[0] = positions[i * 3];
    mesh->vertices[i].position[1] = positions[i * 3 + 1];
    mesh->vertices[i].position[2] = positions[i * 3 + 2];

    if (i < norm_count) {
      mesh->vertices[i].normal[0] = normals[i * 3];
      mesh->vertices[i].normal[1] = normals[i * 3 + 1];
      mesh->vertices[i].normal[2] = normals[i * 3 + 2];
    }

    if (i < uv_count) {
      mesh->vertices[i].uv[0] = uvs[i * 2];
      mesh->vertices[i].uv[1] = uvs[i * 2 + 1];
    }
  }

  // Copy indices
  mesh->indices = MALLOC(idx_count * sizeof(uint32_t));
  memcpy(mesh->indices, indices, idx_count * sizeof(uint32_t));

  // Cleanup temporary storage
  FREE(positions);
  FREE(normals);
  FREE(uvs);
  FREE(indices);

  model->vertex_count = pos_count;
  model->triangle_count = idx_count / 3;
  model->status = MODEL_STATUS_READY;

  LOG_INFO("Loaded OBJ model: %s (%u vertices, %u triangles)", path,
           model->vertex_count, model->triangle_count);

  return true;
}

// Detect format from file extension
ModelFormat model_detect_format(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext)
    return MODEL_FORMAT_AUTO;

  ext++; // Skip the dot

  if (strcmp(ext, "obj") == 0)
    return MODEL_FORMAT_OBJ;
  if (strcmp(ext, "fbx") == 0)
    return MODEL_FORMAT_FBX;
  if (strcmp(ext, "gltf") == 0)
    return MODEL_FORMAT_GLTF;
  if (strcmp(ext, "glb") == 0)
    return MODEL_FORMAT_GLB;
  if (strcmp(ext, "stl") == 0)
    return MODEL_FORMAT_STL;
  if (strcmp(ext, "ply") == 0)
    return MODEL_FORMAT_PLY;
  if (strcmp(ext, "dae") == 0)
    return MODEL_FORMAT_DAE;
  if (strcmp(ext, "mmdl") == 0)
    return MODEL_FORMAT_MMDL;

  return MODEL_FORMAT_AUTO;
}

// Get default loading options
ModelLoadOptions model_get_default_options(void) {
  ModelLoadOptions options = {0};
  options.format = MODEL_FORMAT_AUTO;
  options.flags = MODEL_LOAD_DEFAULT;
  options.scale = 1.0f;
  options.progress_callback = NULL;
  options.user_data = NULL;
  return options;
}

// Load model from file
Model *model_load(const char *path, const ModelLoadOptions *options) {
  if (!path) {
    LOG_ERROR("model_load: path is NULL");
    return NULL;
  }

  // Use default options if not provided
  ModelLoadOptions default_opts = model_get_default_options();
  if (!options) {
    options = &default_opts;
  }

  // Detect format if AUTO
  ModelFormat format = options->format;
  if (format == MODEL_FORMAT_AUTO) {
    format = model_detect_format(path);
  }

  // Allocate model
  Model *model = CALLOC(1, sizeof(Model));
  model->name = strdup(path);
  model->path = strdup(path);
  model->status = MODEL_STATUS_LOADING;

  // Load based on format
  bool success = false;
  switch (format) {
  case MODEL_FORMAT_OBJ:
    success = load_obj_file(model, path);
    break;

  case MODEL_FORMAT_GLTF:
  case MODEL_FORMAT_GLB:
  case MODEL_FORMAT_FBX:
  case MODEL_FORMAT_STL:
  case MODEL_FORMAT_PLY:
  case MODEL_FORMAT_DAE:
    LOG_WARN("Format not yet implemented: %d", format);
    model->status = MODEL_STATUS_ERROR;
    break;

  default:
    LOG_ERROR("Unknown model format");
    model->status = MODEL_STATUS_ERROR;
    break;
  }

  if (!success) {
    model_free(model);
    return NULL;
  }

  // Calculate bounds
  model_calculate_bounds(model);

  return model;
}

// Free model
void model_free(Model *model) {
  if (!model)
    return;

  if (model->name)
    FREE(model->name);
  if (model->path)
    FREE(model->path);

  // Free meshes
  for (uint32_t i = 0; i < model->mesh_count; i++) {
    if (model->meshes[i]) {
      if (model->meshes[i]->vertices)
        FREE(model->meshes[i]->vertices);
      if (model->meshes[i]->indices)
        FREE(model->meshes[i]->indices);
      FREE(model->meshes[i]);
    }
  }
  if (model->meshes)
    FREE(model->meshes);

  // Free materials (TODO)
  // Free skeleton (TODO)
  // Free animations (TODO)

  FREE(model);
}

// Calculate bounding box
void model_calculate_bounds(Model *model) {
  if (!model || model->mesh_count == 0)
    return;

  model->bounds_min[0] = model->bounds_min[1] = model->bounds_min[2] = 1e10f;
  model->bounds_max[0] = model->bounds_max[1] = model->bounds_max[2] = -1e10f;

  for (uint32_t m = 0; m < model->mesh_count; m++) {
    Mesh *mesh = model->meshes[m];
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
      for (int j = 0; j < 3; j++) {
        if (mesh->vertices[i].position[j] < model->bounds_min[j])
          model->bounds_min[j] = mesh->vertices[i].position[j];
        if (mesh->vertices[i].position[j] > model->bounds_max[j])
          model->bounds_max[j] = mesh->vertices[i].position[j];
      }
    }
  }

  // Calculate center and radius
  for (int i = 0; i < 3; i++) {
    model->bounds_center[i] =
        (model->bounds_min[i] + model->bounds_max[i]) * 0.5f;
  }

  float dx = model->bounds_max[0] - model->bounds_min[0];
  float dy = model->bounds_max[1] - model->bounds_min[1];
  float dz = model->bounds_max[2] - model->bounds_min[2];
  model->bounds_radius = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f;
}

// Format utilities
const char *model_format_get_extension(ModelFormat format) {
  switch (format) {
  case MODEL_FORMAT_OBJ:
    return "obj";
  case MODEL_FORMAT_FBX:
    return "fbx";
  case MODEL_FORMAT_GLTF:
    return "gltf";
  case MODEL_FORMAT_GLB:
    return "glb";
  case MODEL_FORMAT_STL:
    return "stl";
  case MODEL_FORMAT_PLY:
    return "ply";
  case MODEL_FORMAT_DAE:
    return "dae";
  case MODEL_FORMAT_MMDL:
    return "mmdl";
  default:
    return "unknown";
  }
}

const char *model_format_get_name(ModelFormat format) {
  switch (format) {
  case MODEL_FORMAT_OBJ:
    return "Wavefront OBJ";
  case MODEL_FORMAT_FBX:
    return "Autodesk FBX";
  case MODEL_FORMAT_GLTF:
    return "glTF 2.0";
  case MODEL_FORMAT_GLB:
    return "glTF Binary";
  case MODEL_FORMAT_STL:
    return "STL";
  case MODEL_FORMAT_PLY:
    return "PLY";
  case MODEL_FORMAT_DAE:
    return "Collada DAE";
  case MODEL_FORMAT_MMDL:
    return "Minecraft Model";
  default:
    return "Unknown";
  }
}
