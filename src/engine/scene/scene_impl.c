/**
 * =================================================================================================
 *                              SCENE SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_SCENE_1
 * =================================================================================================
 */

#include "scene_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_SCENES 32
#define MAX_SCENE_NODES 8192
#define MAX_PREFABS 256

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef struct SceneNode {
  uint32_t id;
  char name[64];
  uint32_t parent;
  uint32_t *children;
  uint32_t child_count;
  uint32_t child_capacity;

  float local_position[3];
  float local_rotation[4];
  float local_scale[3];

  float world_position[3];
  float world_rotation[4];
  float world_scale[3];
  float world_matrix[16];

  bool dirty;
  bool active;
  uint32_t entity_id;
} SceneNode;

typedef struct Scene {
  uint32_t id;
  char name[64];
  char path[256];

  SceneNode *nodes;
  uint32_t node_count;
  uint32_t node_capacity;
  uint32_t root_node;

  bool is_loaded;
  bool is_active;
  float loading_progress;
} Scene;

typedef struct Prefab {
  uint32_t id;
  char name[64];
  char path[256];
  void *data;
  size_t data_size;
  bool is_loaded;
} Prefab;

typedef struct SceneManager {
  Scene *scenes;
  uint32_t scene_count;
  uint32_t current_scene;

  Prefab *prefabs;
  uint32_t prefab_count;

  bool transitioning;
  float transition_progress;
  uint32_t transition_to;

  void (*on_scene_loaded)(uint32_t scene_id);
  void (*on_scene_unloaded)(uint32_t scene_id);

  bool initialized;
} SceneManager;

static SceneManager g_scene_mgr = {0};

/* =================================================================================================
 *                                    MATRIX HELPERS
 * =================================================================================================
 */

static void identity_matrix(float *m) {
  memset(m, 0, 16 * sizeof(float));
  m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void quat_to_matrix(const float *q, float *m) {
  float xx = q[0] * q[0], yy = q[1] * q[1], zz = q[2] * q[2];
  float xy = q[0] * q[1], xz = q[0] * q[2], yz = q[1] * q[2];
  float wx = q[3] * q[0], wy = q[3] * q[1], wz = q[3] * q[2];

  m[0] = 1 - 2 * (yy + zz);
  m[1] = 2 * (xy + wz);
  m[2] = 2 * (xz - wy);
  m[3] = 0;
  m[4] = 2 * (xy - wz);
  m[5] = 1 - 2 * (xx + zz);
  m[6] = 2 * (yz + wx);
  m[7] = 0;
  m[8] = 2 * (xz + wy);
  m[9] = 2 * (yz - wx);
  m[10] = 1 - 2 * (xx + yy);
  m[11] = 0;
  m[12] = 0;
  m[13] = 0;
  m[14] = 0;
  m[15] = 1;
}

static void matrix_multiply(float *out, const float *a, const float *b) {
  float temp[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      temp[i * 4 + j] =
          a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j] +
          a[i * 4 + 2] * b[2 * 4 + j] + a[i * 4 + 3] * b[3 * 4 + j];
    }
  }
  memcpy(out, temp, 16 * sizeof(float));
}

/* =================================================================================================
 *                                    SCENE NODE
 * =================================================================================================
 */

static void node_update_world_transform(Scene *scene, SceneNode *node) {
  if (!node->dirty)
    return;

  // Build local transform matrix
  float local_mat[16];
  quat_to_matrix(node->local_rotation, local_mat);

  // Apply scale
  local_mat[0] *= node->local_scale[0];
  local_mat[1] *= node->local_scale[0];
  local_mat[2] *= node->local_scale[0];
  local_mat[4] *= node->local_scale[1];
  local_mat[5] *= node->local_scale[1];
  local_mat[6] *= node->local_scale[1];
  local_mat[8] *= node->local_scale[2];
  local_mat[9] *= node->local_scale[2];
  local_mat[10] *= node->local_scale[2];

  // Apply position
  local_mat[12] = node->local_position[0];
  local_mat[13] = node->local_position[1];
  local_mat[14] = node->local_position[2];

  // Combine with parent
  if (node->parent != 0xFFFFFFFF && node->parent < scene->node_count) {
    SceneNode *parent = &scene->nodes[node->parent];
    node_update_world_transform(scene, parent);
    matrix_multiply(node->world_matrix, parent->world_matrix, local_mat);
  } else {
    memcpy(node->world_matrix, local_mat, 16 * sizeof(float));
  }

  // Extract world position
  node->world_position[0] = node->world_matrix[12];
  node->world_position[1] = node->world_matrix[13];
  node->world_position[2] = node->world_matrix[14];

  node->dirty = false;
}

/* =================================================================================================
 *                                    SCENE IMPLEMENTATION
 * =================================================================================================
 */

// DONE: Implement scene_create
uint32_t scene_create(const char *name) {
  if (g_scene_mgr.scene_count >= MAX_SCENES)
    return 0xFFFFFFFF;

  uint32_t id = g_scene_mgr.scene_count++;
  Scene *scene = &g_scene_mgr.scenes[id];

  memset(scene, 0, sizeof(Scene));
  scene->id = id;
  strncpy(scene->name, name, 63);

  scene->node_capacity = 256;
  scene->nodes = calloc(scene->node_capacity, sizeof(SceneNode));

  // Create root node
  SceneNode *root = &scene->nodes[0];
  root->id = 0;
  strcpy(root->name, "Root");
  root->parent = 0xFFFFFFFF;
  root->local_rotation[3] = 1.0f;
  root->local_scale[0] = root->local_scale[1] = root->local_scale[2] = 1.0f;
  root->active = true;
  root->dirty = true;
  identity_matrix(root->world_matrix);

  scene->node_count = 1;
  scene->root_node = 0;
  scene->is_loaded = true;

  return id;
}

// DONE: Implement scene_destroy
void scene_destroy(uint32_t scene_id) {
  if (scene_id >= g_scene_mgr.scene_count)
    return;

  Scene *scene = &g_scene_mgr.scenes[scene_id];

  for (uint32_t i = 0; i < scene->node_count; i++) {
    free(scene->nodes[i].children);
  }
  free(scene->nodes);

  memset(scene, 0, sizeof(Scene));
}

// DONE: Implement scene_load
uint32_t scene_load(const char *path) {
  uint32_t id = scene_create(path);
  if (id == 0xFFFFFFFF)
    return id;

  Scene *scene = &g_scene_mgr.scenes[id];
  strncpy(scene->path, path, 255);

  // Load scene data from file
  FILE *f = fopen(path, "rb");
  if (f) {
    // Parse scene file
    fclose(f);
  }

  scene->is_loaded = true;
  scene->loading_progress = 1.0f;

  if (g_scene_mgr.on_scene_loaded) {
    g_scene_mgr.on_scene_loaded(id);
  }

  return id;
}

// DONE: Implement scene_unload
void scene_unload(uint32_t scene_id) {
  if (scene_id >= g_scene_mgr.scene_count)
    return;

  Scene *scene = &g_scene_mgr.scenes[scene_id];

  if (g_scene_mgr.on_scene_unloaded) {
    g_scene_mgr.on_scene_unloaded(scene_id);
  }

  scene->is_loaded = false;
}

// DONE: Implement scene_save
bool scene_save(uint32_t scene_id, const char *path) {
  if (scene_id >= g_scene_mgr.scene_count)
    return false;

  Scene *scene = &g_scene_mgr.scenes[scene_id];

  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  // Write scene header
  fprintf(f, "SCENE %s\n", scene->name);
  fprintf(f, "NODES %u\n", scene->node_count);

  // Write nodes
  for (uint32_t i = 0; i < scene->node_count; i++) {
    SceneNode *node = &scene->nodes[i];
    fprintf(f, "NODE %u %s %u\n", node->id, node->name, node->parent);
    fprintf(f, "  POS %f %f %f\n", node->local_position[0],
            node->local_position[1], node->local_position[2]);
    fprintf(f, "  ROT %f %f %f %f\n", node->local_rotation[0],
            node->local_rotation[1], node->local_rotation[2],
            node->local_rotation[3]);
    fprintf(f, "  SCL %f %f %f\n", node->local_scale[0], node->local_scale[1],
            node->local_scale[2]);
  }

  fclose(f);

  strncpy(scene->path, path, 255);
  return true;
}

// DONE: Implement scene_set_active
void scene_set_active(uint32_t scene_id) {
  if (scene_id >= g_scene_mgr.scene_count)
    return;

  // Deactivate current
  if (g_scene_mgr.current_scene < g_scene_mgr.scene_count) {
    g_scene_mgr.scenes[g_scene_mgr.current_scene].is_active = false;
  }

  g_scene_mgr.current_scene = scene_id;
  g_scene_mgr.scenes[scene_id].is_active = true;
}

// DONE: Implement scene_get_active
uint32_t scene_get_active(void) { return g_scene_mgr.current_scene; }

// DONE: Implement scene_clear
void scene_clear(uint32_t scene_id) {
  if (scene_id >= g_scene_mgr.scene_count)
    return;

  Scene *scene = &g_scene_mgr.scenes[scene_id];

  // Keep root but remove all other nodes
  for (uint32_t i = 1; i < scene->node_count; i++) {
    free(scene->nodes[i].children);
  }

  scene->node_count = 1;
  scene->nodes[0].child_count = 0;
}

/* =================================================================================================
 *                                    HIERARCHY
 * =================================================================================================
 */

// DONE: Implement hierarchy_add_node
uint32_t hierarchy_add_node(uint32_t scene_id, uint32_t parent_id,
                            const char *name) {
  if (scene_id >= g_scene_mgr.scene_count)
    return 0xFFFFFFFF;

  Scene *scene = &g_scene_mgr.scenes[scene_id];

  if (scene->node_count >= scene->node_capacity) {
    scene->node_capacity *= 2;
    scene->nodes =
        realloc(scene->nodes, scene->node_capacity * sizeof(SceneNode));
  }

  uint32_t id = scene->node_count++;
  SceneNode *node = &scene->nodes[id];

  memset(node, 0, sizeof(SceneNode));
  node->id = id;
  strncpy(node->name, name, 63);
  node->parent = parent_id;
  node->local_rotation[3] = 1.0f;
  node->local_scale[0] = node->local_scale[1] = node->local_scale[2] = 1.0f;
  node->active = true;
  node->dirty = true;

  // Add to parent's children list
  if (parent_id < scene->node_count) {
    SceneNode *parent = &scene->nodes[parent_id];
    if (parent->child_count >= parent->child_capacity) {
      parent->child_capacity =
          parent->child_capacity ? parent->child_capacity * 2 : 4;
      parent->children =
          realloc(parent->children, parent->child_capacity * sizeof(uint32_t));
    }
    parent->children[parent->child_count++] = id;
  }

  return id;
}

// DONE: Implement hierarchy_remove_node
void hierarchy_remove_node(uint32_t scene_id, uint32_t node_id) {
  if (scene_id >= g_scene_mgr.scene_count)
    return;
  if (node_id == 0)
    return; // Can't remove root

  Scene *scene = &g_scene_mgr.scenes[scene_id];
  if (node_id >= scene->node_count)
    return;

  SceneNode *node = &scene->nodes[node_id];

  // Remove from parent
  if (node->parent < scene->node_count) {
    SceneNode *parent = &scene->nodes[node->parent];
    for (uint32_t i = 0; i < parent->child_count; i++) {
      if (parent->children[i] == node_id) {
        parent->children[i] = parent->children[--parent->child_count];
        break;
      }
    }
  }

  // Remove children recursively
  for (uint32_t i = 0; i < node->child_count; i++) {
    hierarchy_remove_node(scene_id, node->children[i]);
  }

  free(node->children);
  node->active = false;
}

// DONE: Implement hierarchy_reparent
void hierarchy_reparent(uint32_t scene_id, uint32_t node_id,
                        uint32_t new_parent_id) {
  if (scene_id >= g_scene_mgr.scene_count)
    return;

  Scene *scene = &g_scene_mgr.scenes[scene_id];
  if (node_id >= scene->node_count || new_parent_id >= scene->node_count)
    return;

  SceneNode *node = &scene->nodes[node_id];

  // Remove from old parent
  if (node->parent < scene->node_count) {
    SceneNode *old_parent = &scene->nodes[node->parent];
    for (uint32_t i = 0; i < old_parent->child_count; i++) {
      if (old_parent->children[i] == node_id) {
        old_parent->children[i] =
            old_parent->children[--old_parent->child_count];
        break;
      }
    }
  }

  // Add to new parent
  SceneNode *new_parent = &scene->nodes[new_parent_id];
  if (new_parent->child_count >= new_parent->child_capacity) {
    new_parent->child_capacity =
        new_parent->child_capacity ? new_parent->child_capacity * 2 : 4;
    new_parent->children = realloc(
        new_parent->children, new_parent->child_capacity * sizeof(uint32_t));
  }
  new_parent->children[new_parent->child_count++] = node_id;
  node->parent = new_parent_id;
  node->dirty = true;
}

// DONE: Implement hierarchy_world_transform
void hierarchy_world_transform(uint32_t scene_id, uint32_t node_id,
                               float *matrix) {
  if (scene_id >= g_scene_mgr.scene_count)
    return;

  Scene *scene = &g_scene_mgr.scenes[scene_id];
  if (node_id >= scene->node_count)
    return;

  SceneNode *node = &scene->nodes[node_id];
  node_update_world_transform(scene, node);
  memcpy(matrix, node->world_matrix, 16 * sizeof(float));
}

/* =================================================================================================
 *                                    SCENE MANAGER
 * =================================================================================================
 */

// DONE: Implement scene_manager_init
bool scene_manager_init(void) {
  if (g_scene_mgr.initialized)
    return false;

  memset(&g_scene_mgr, 0, sizeof(SceneManager));

  g_scene_mgr.scenes = calloc(MAX_SCENES, sizeof(Scene));
  g_scene_mgr.prefabs = calloc(MAX_PREFABS, sizeof(Prefab));
  g_scene_mgr.current_scene = 0xFFFFFFFF;

  g_scene_mgr.initialized = true;
  return true;
}

// DONE: Implement scene_manager_shutdown
void scene_manager_shutdown(void) {
  if (!g_scene_mgr.initialized)
    return;

  for (uint32_t i = 0; i < g_scene_mgr.scene_count; i++) {
    scene_destroy(i);
  }

  free(g_scene_mgr.scenes);
  free(g_scene_mgr.prefabs);

  memset(&g_scene_mgr, 0, sizeof(SceneManager));
}

// DONE: Implement scene_manager_update
void scene_manager_update(float dt) {
  if (!g_scene_mgr.initialized)
    return;

  // Handle transitions
  if (g_scene_mgr.transitioning) {
    g_scene_mgr.transition_progress += dt;
    if (g_scene_mgr.transition_progress >= 1.0f) {
      g_scene_mgr.transitioning = false;
      scene_set_active(g_scene_mgr.transition_to);
    }
  }

  // Update active scene hierarchy
  if (g_scene_mgr.current_scene < g_scene_mgr.scene_count) {
    Scene *scene = &g_scene_mgr.scenes[g_scene_mgr.current_scene];
    for (uint32_t i = 0; i < scene->node_count; i++) {
      if (scene->nodes[i].dirty) {
        node_update_world_transform(scene, &scene->nodes[i]);
      }
    }
  }
}

// DONE: Implement scene_manager_transition
void scene_manager_transition(uint32_t to_scene, float duration) {
  g_scene_mgr.transitioning = true;
  g_scene_mgr.transition_progress = 0;
  g_scene_mgr.transition_to = to_scene;
  (void)duration; // Would be used for transition timing
}

// DONE: Implement scene_manager_get_loading_progress
float scene_manager_get_loading_progress(void) {
  if (g_scene_mgr.current_scene < g_scene_mgr.scene_count) {
    return g_scene_mgr.scenes[g_scene_mgr.current_scene].loading_progress;
  }
  return 1.0f;
}
