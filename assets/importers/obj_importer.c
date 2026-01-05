#include "../../../../include/containers/hashmap.h"
#include "../../../../include/core/logger.h"
#include "../../../../include/core/string_utils.h"
#include "../../../../include/engine/asset_importers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Standard vertex layout for the engine
typedef struct {
  float x, y, z;    // Position
  float nx, ny, nz; // Normal
  float u, v;       // UV0 (primary texture coordinates)
  float u1, v1;     // UV1 (secondary texture coordinates - lightmaps, detail)
  float tx, ty, tz, tw; // Tangent
} StandardVertex;

// Internal allocation tracking list
struct AllocationList {
  void *ptr;
  struct AllocationList *next;
};

typedef struct {
  int v, vt, vn;
} VertexKey;

static u32 vertex_key_hash(const void *key) {
  const VertexKey *k = (const VertexKey *)key;
  u32 h = 0x811c9dc5;
  h ^= (u32)k->v;
  h *= 0x01000193;
  h ^= (u32)k->vt;
  h *= 0x01000193;
  h ^= (u32)k->vn;
  h *= 0x01000193;
  return h;
}

static bool vertex_key_equals(const void *a, const void *b) {
  const VertexKey *ka = (const VertexKey *)a;
  const VertexKey *kb = (const VertexKey *)b;
  return ka->v == kb->v && ka->vt == kb->vt && ka->vn == kb->vn;
}

typedef struct {
  float *positions;
  u32 pos_count;
  float *normals;
  u32 norm_count;
  float *uvs;
  u32 uv_count;
} RawMeshData;

// Temporary representation for building meshes
typedef struct {
  u32 material_index;
  StandardVertex *vertices;
  u32 *indices;
  u32 vertex_count;
  u32 index_count;
  u32 vertex_capacity;
  u32 index_capacity;
  HashMap *vertex_to_index;
} MeshBuilder;

static void parse_mtl(const char *path, ImportedModel *model,
                      struct AllocationList **alloc_list_head) {
  FILE *file = fopen(path, "r");
  if (!file)
    return;

  // First pass: count materials
  u32 mat_count = 0;
  char line[512];
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "newmtl ", 7) == 0)
      mat_count++;
  }

  if (mat_count == 0) {
    fclose(file);
    return;
  }

  model->materials = malloc(sizeof(ImportedMaterial) * mat_count);
  memset(model->materials, 0, sizeof(ImportedMaterial) * mat_count);
  model->material_count = mat_count;

  fseek(file, 0, SEEK_SET);

  int current_mat = -1;
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "newmtl ", 7) == 0) {
      current_mat++;
      char *name = line + 7;
      name[strcspn(name, "\r\n")] = 0;
      strncpy(model->materials[current_mat].name, name, 127);

      // Set defaults
      model->materials[current_mat].properties.albedo = (Vec3){1, 1, 1};
      model->materials[current_mat].properties.roughness = 1;
      model->materials[current_mat].properties.alpha = 1;
    } else if (current_mat >= 0) {
      if (strncmp(line, "Kd ", 3) == 0) {
        sscanf(line + 3, "%f %f %f",
               &model->materials[current_mat].properties.albedo.x,
               &model->materials[current_mat].properties.albedo.y,
               &model->materials[current_mat].properties.albedo.z);
      } else if (strncmp(line, "map_Kd ", 7) == 0) {
        char *path_ptr = line + 7;
        path_ptr[strcspn(path_ptr, "\r\n")] = 0;

        // Construct relative path
        const char *last_slash = strrchr(path, '/');
        if (last_slash) {
          size_t len = last_slash - path + 1;
          strncpy(model->materials[current_mat].albedo_path, path, len);
          strcat(model->materials[current_mat].albedo_path, path_ptr);
        } else {
          strcpy(model->materials[current_mat].albedo_path, path_ptr);
        }
      }
      // Add more MTL parameters if needed (Ks, map_Bump, etc)
    }
  }
  fclose(file);
}

static int count_chars(const char *s, char c) {
  int count = 0;
  while (*s) {
    if (*s == c)
      count++;
    s++;
  }
  return count;
}

ImportedModel *asset_importer_load_obj(const char *path,
                                       const ModelImportOptions *options) {
  FILE *file = fopen(path, "r");
  if (!file) {
    LOG_ERROR("Failed to open OBJ file: %s", path);
    return NULL;
  }

  struct AllocationList *alloc_list_head = NULL;
  ImportedModel *model = calloc(1, sizeof(ImportedModel));

  // Temporary storage for raw data
  u32 pos_cap = 1024, norm_cap = 1024, uv_cap = 1024;
  float *pos = malloc(sizeof(float) * 3 * pos_cap);
  float *norm = malloc(sizeof(float) * 3 * norm_cap);
  float *uv = malloc(sizeof(float) * 2 * uv_cap);
  u32 pos_idx = 0, norm_idx = 0, uv_idx = 0;

  // Track current material
  int current_material_idx = -1;

  // Mesh builders per material
  MeshBuilder *builders = NULL;
  u32 builder_count = 0;

  char line[512];
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "v ", 2) == 0) {
      if (pos_idx >= pos_cap) {
        pos_cap *= 2;
        pos = realloc(pos, sizeof(float) * 3 * pos_cap);
      }
      sscanf(line + 2, "%f %f %f", &pos[pos_idx * 3], &pos[pos_idx * 3 + 1],
             &pos[pos_idx * 3 + 2]);
      pos_idx++;
    } else if (strncmp(line, "vn ", 3) == 0) {
      if (norm_idx >= norm_cap) {
        norm_cap *= 2;
        norm = realloc(norm, sizeof(float) * 3 * norm_cap);
      }
      sscanf(line + 3, "%f %f %f", &norm[norm_idx * 3], &norm[norm_idx * 3 + 1],
             &norm[norm_idx * 3 + 2]);
      norm_idx++;
    } else if (strncmp(line, "vt ", 3) == 0) {
      if (uv_idx >= uv_cap) {
        uv_cap *= 2;
        uv = realloc(uv, sizeof(float) * 2 * uv_cap);
      }
      sscanf(line + 3, "%f %f", &uv[uv_idx * 2], &uv[uv_idx * 2 + 1]);
      uv_idx++;
    } else if (strncmp(line, "mtllib ", 7) == 0) {
      char mtl_filename[256];
      sscanf(line + 7, "%255s", mtl_filename);

      char mtl_path[512];
      const char *last_slash = strrchr(path, '/');
      if (last_slash) {
        size_t len = last_slash - path + 1;
        strncpy(mtl_path, path, len);
        mtl_path[len] = '\0';
        strcat(mtl_path, mtl_filename);
      } else {
        strcpy(mtl_path, mtl_filename);
      }
      parse_mtl(mtl_path, model, &alloc_list_head);
    } else if (strncmp(line, "usemtl ", 7) == 0) {
      char mat_name[128];
      sscanf(line + 7, "%127s", mat_name);
      current_material_idx = -1;
      for (u32 i = 0; i < model->material_count; i++) {
        if (strcmp(model->materials[i].name, mat_name) == 0) {
          current_material_idx = (int)i;
          break;
        }
      }
    } else if (strncmp(line, "f ", 2) == 0) {
      // Ensure we have a builder for current material
      u32 b_idx = 0;
      bool found = false;
      for (u32 i = 0; i < builder_count; i++) {
        if ((int)builders[i].material_index == current_material_idx) {
          b_idx = i;
          found = true;
          break;
        }
      }
      if (!found) {
        b_idx = builder_count++;
        builders = realloc(builders, sizeof(MeshBuilder) * builder_count);
        builders[b_idx].material_index = (u32)current_material_idx;
        builders[b_idx].vertex_count = 0;
        builders[b_idx].index_count = 0;
        builders[b_idx].vertex_capacity = 1024;
        builders[b_idx].index_capacity = 3072;
        builders[b_idx].vertices =
            malloc(sizeof(StandardVertex) * builders[b_idx].vertex_capacity);
        builders[b_idx].indices =
            malloc(sizeof(u32) * builders[b_idx].index_capacity);
        builders[b_idx].vertex_to_index =
            hashmap_create(1024, sizeof(VertexKey), sizeof(u32),
                           vertex_key_hash, vertex_key_equals);
      }

      MeshBuilder *b = &builders[b_idx];

      // Parse face indices (handle v/vt/vn)
      char *token = strtok(line + 2, " \t\r\n");
      VertexKey keys[32]; // Max 32 vertices per face
      u32 face_v_count = 0;
      while (token && face_v_count < 32) {
        VertexKey pk = {0, 0, 0};
        if (strstr(token, "//")) {
          sscanf(token, "%d//%d", &pk.v, &pk.vn);
        } else if (count_chars(token, '/') == 1) {
          sscanf(token, "%d/%d", &pk.v, &pk.vt);
        } else if (count_chars(token, '/') == 2) {
          sscanf(token, "%d/%d/%d", &pk.v, &pk.vt, &pk.vn);
        } else {
          sscanf(token, "%d", &pk.v);
        }

        // Handle relative indices
        if (pk.v < 0)
          pk.v = pos_idx + pk.v + 1;
        if (pk.vt < 0)
          pk.vt = uv_idx + pk.vt + 1;
        if (pk.vn < 0)
          pk.vn = norm_idx + pk.vn + 1;

        keys[face_v_count++] = pk;
        token = strtok(NULL, " \t\r\n");
      }

      // Triangulate
      for (u32 i = 1; i < face_v_count - 1; i++) {
        u32 tri[3] = {0, i, i + 1};
        for (int k = 0; k < 3; k++) {
          VertexKey *vk = &keys[tri[k]];
          u32 *existing_idx = hashmap_get(b->vertex_to_index, vk);
          u32 final_idx;
          if (existing_idx) {
            final_idx = *existing_idx;
          } else {
            final_idx = b->vertex_count++;
            if (b->vertex_count >= b->vertex_capacity) {
              b->vertex_capacity *= 2;
              b->vertices = realloc(b->vertices, sizeof(StandardVertex) *
                                                     b->vertex_capacity);
            }

            StandardVertex *vout = &b->vertices[final_idx];
            memset(vout, 0, sizeof(StandardVertex));

            if (vk->v > 0) {
              vout->x = pos[(vk->v - 1) * 3];
              vout->y = pos[(vk->v - 1) * 3 + 1];
              vout->z = pos[(vk->v - 1) * 3 + 2];
            }
            if (vk->vn > 0) {
              vout->nx = norm[(vk->vn - 1) * 3];
              vout->ny = norm[(vk->vn - 1) * 3 + 1];
              vout->nz = norm[(vk->vn - 1) * 3 + 2];
            }
            if (vk->vt > 0) {
              vout->u = uv[(vk->vt - 1) * 2];
              vout->v = uv[(vk->vt - 1) * 2 + 1];
              if (options && options->flip_uvs)
                vout->v = 1.0f - vout->v;
            }

            hashmap_insert(b->vertex_to_index, vk, &final_idx);
          }

          if (b->index_count >= b->index_capacity) {
            b->index_capacity *= 2;
            b->indices = realloc(b->indices, sizeof(u32) * b->index_capacity);
          }
          b->indices[b->index_count++] = final_idx;
        }
      }
    }
  }

  fclose(file);
  free(pos);
  free(norm);
  free(uv);

  // Finalize model
  model->mesh_count = builder_count;
  model->meshes = malloc(sizeof(MeshDesc) * builder_count);
  for (u32 i = 0; i < builder_count; i++) {
    MeshBuilder *b = &builders[i];
    model->meshes[i].vertices = b->vertices;
    model->meshes[i].vertex_count = b->vertex_count;
    model->meshes[i].vertex_stride = sizeof(StandardVertex);
    model->meshes[i].indices = b->indices;
    model->meshes[i].index_count = b->index_count;
    model->meshes[i].index_stride = sizeof(u32);
    model->meshes[i].topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    model->meshes[i].material_index = b->material_index;

    // Track for freeing
    struct AllocationList *vnode = malloc(sizeof(struct AllocationList));
    vnode->ptr = b->vertices;
    vnode->next = alloc_list_head;
    alloc_list_head = vnode;

    struct AllocationList *inode = malloc(sizeof(struct AllocationList));
    inode->ptr = b->indices;
    inode->next = alloc_list_head;
    alloc_list_head = inode;

    hashmap_destroy(b->vertex_to_index);
  }

  if (builders)
    free(builders);

  model->raw_data = alloc_list_head;
  LOG_INFO("Loaded OBJ model: %s (%d groups, %d materials)", path,
           builder_count, model->material_count);

  return model;
}

ImportedModel *
asset_importer_load_obj_from_memory(const void *data, size_t size,
                                    const ModelImportOptions *options) {
  // For now, write to temporary file to use the file parser, or implement
  // memory parsing. Memory parsing is cleaner but longer. Let's do a basic
  // memory parsing later if needed.
  LOG_WARN("OBJ load from memory not fully implemented. Redirecting to "
           "temporary file.");
  FILE *tmp = fopen(".tmp_model.obj", "wb");
  if (!tmp)
    return NULL;
  fwrite(data, 1, size, tmp);
  fclose(tmp);

  ImportedModel *model = asset_importer_load_obj(".tmp_model.obj", options);
  remove(".tmp_model.obj");
  return model;
}
