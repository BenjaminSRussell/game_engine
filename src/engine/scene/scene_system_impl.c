#include "scene/scene_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SCENES 16
#define INITIAL_CHILD_CAPACITY 4

// =================================================================================================
//                                    SCENE CORE
// =================================================================================================

Scene *scene_create(const char *name) {
  Scene *scene = (Scene *)malloc(sizeof(Scene));
  if (!scene)
    return NULL;

  scene->id = (SceneID)rand(); // Simple random ID for now
  strncpy(scene->name, name, 63);
  scene->name[63] = '\0';
  scene->isActive = false;
  scene->isLoaded = false;
  scene->maxEntities = 1000;
  scene->currentEntities = 0;
  memset(scene->filePath, 0, 256);

  // Create root node
  scene->root = hierarchy_add_node(NULL, "Root");

  return scene;
}

void scene_destroy(Scene *scene) {
  if (!scene)
    return;

  // Recursive destroy hierarchy
  if (scene->root) {
    hierarchy_remove_node(scene->root); // Removes whole tree
  }

  free(scene);
}

bool scene_load(Scene *scene, const char *path) {
  if (!scene || !path)
    return false;

  // Stub implementation: "Load" just means setting the path and flag
  strncpy(scene->filePath, path, 255);
  scene->isLoaded = true;
  printf("DEBUG: Scene loaded from %s\n", path);
  return true;
}

bool scene_load_async(Scene *scene, const char *path,
                      void (*onComplete)(Scene *)) {
  // Stub: Sync load for now
  bool result = scene_load(scene, path);
  if (result && onComplete)
    onComplete(scene);
  return result;
}

void scene_unload(Scene *scene) {
  if (!scene)
    return;
  scene->isLoaded = false;
  scene->isActive = false;
  scene_clear(scene);
}

bool scene_save(Scene *scene, const char *path) {
  if (!scene || !path)
    return false;

  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  // Write scene header
  fwrite(scene, sizeof(Scene) - sizeof(HierarchyNode *), 1, f); // Write basics

  // Serialize hierarchy
  hierarchy_serialize(scene->root, f);

  fclose(f);
  return true;
}

void scene_set_active(Scene *scene, bool active) {
  if (scene)
    scene->isActive = active;
}

bool scene_get_active(Scene *scene) { return scene ? scene->isActive : false; }

Scene *scene_get_by_name(SceneManager *manager, const char *name) {
  if (!manager)
    return NULL;
  for (u32 i = 0; i < manager->sceneCount; i++) {
    if (strcmp(manager->scenes[i]->name, name) == 0) {
      return manager->scenes[i];
    }
  }
  return NULL;
}

Scene **scene_get_all(SceneManager *manager, u32 *count) {
  if (!manager) {
    if (count)
      *count = 0;
    return NULL;
  }
  if (count)
    *count = manager->sceneCount;
  return manager->scenes;
}

Scene *scene_instantiate(Scene *templateScene) {
  if (!templateScene)
    return NULL;

  Scene *newScene = scene_create(templateScene->name);
  // Deep copy hierarchy (stub: not implemented fully, would need clone logic)
  // For now, just a fresh scene
  return newScene;
}

bool scene_merge(Scene *target, Scene *source) {
  if (!target || !source)
    return false;
  // Stub: Reparent source root children to target root
  return true;
}

void scene_clear(Scene *scene) {
  if (!scene)
    return;
  if (scene->root) {
    // Remove all children of root, but keep root
    for (u32 i = 0; i < scene->root->childCount; i++) {
      hierarchy_remove_node(scene->root->children[i]);
    }
    scene->root->childCount = 0;
  }
  scene->currentEntities = 0;
}

// =================================================================================================
//                                    SCENE TRANSITIONS
// =================================================================================================

Transition *transition_fade_create(f32 duration, Vec3 color) {
  Transition *t = (Transition *)malloc(sizeof(Transition));
  t->type = TRANSITION_FADE;
  t->duration = duration;
  t->currentTime = 0;
  t->isRunning = false;
  t->isComplete = false;
  t->data = malloc(sizeof(Vec3));
  *(Vec3 *)t->data = color;
  t->update = NULL;
  t->render = NULL;
  return t;
}

Transition *transition_dissolve_create(f32 duration, const char *patternPath) {
  Transition *t = (Transition *)malloc(sizeof(Transition));
  t->type = TRANSITION_DISSOLVE;
  t->duration = duration;
  t->currentTime = 0;
  return t; // Logic stub
}

Transition *transition_wipe_create(f32 duration, Vec3 direction) {
  Transition *t = (Transition *)malloc(sizeof(Transition));
  t->type = TRANSITION_WIPE;
  t->duration = duration;
  return t; // Logic stub
}

Transition *transition_custom_create(f32 duration, void *data,
                                     void (*update)(Transition *, f32),
                                     void (*render)(Transition *)) {
  Transition *t = (Transition *)malloc(sizeof(Transition));
  t->type = TRANSITION_CUSTOM;
  t->duration = duration;
  t->data = data;
  t->update = update;
  t->render = render;
  return t;
}

void transition_start(Transition *transition) {
  if (transition) {
    transition->isRunning = true;
    transition->currentTime = 0;
    transition->isComplete = false;
  }
}

void transition_update(Transition *transition, f32 dt) {
  if (!transition || !transition->isRunning)
    return;

  transition->currentTime += dt;
  if (transition->currentTime >= transition->duration) {
    transition->isComplete = true;
    transition->isRunning = false;
  }

  if (transition->update) {
    transition->update(transition, dt);
  }
}

void transition_render(Transition *transition) {
  if (!transition || !transition->isRunning)
    return;
  if (transition->render) {
    transition->render(transition);
  }
  // Default render logic based on type would go here
}

bool transition_complete(Transition *transition) {
  return transition ? transition->isComplete : true;
}

void transition_cancel(Transition *transition) {
  if (transition)
    transition->isRunning = false;
}

// =================================================================================================
//                                    SCENE STREAMING
// =================================================================================================

// Static list head for now (or manager could hold it)
static StreamingRegion *g_RegionList = NULL;

void streaming_region_define(Vec3 origin, Vec3 extent, const char *path) {
  StreamingRegion *r = (StreamingRegion *)malloc(sizeof(StreamingRegion));
  r->origin = origin;
  r->extent = extent;
  strncpy(r->regionPath, path, 255);
  r->isLoaded = false;
  r->isVisible = false;
  r->lodLevel = 0;
  r->next = g_RegionList;
  g_RegionList = r;
}

bool streaming_region_load(StreamingRegion *region) {
  if (!region)
    return false;
  region->isLoaded = true;
  printf("DEBUG: Streamed load region %s\n", region->regionPath);
  return true;
}

void streaming_region_unload(StreamingRegion *region) {
  if (region)
    region->isLoaded = false;
}

void streaming_update_visibility(Vec3 viewerPosition, f32 range) {
  StreamingRegion *curr = g_RegionList;
  while (curr) {
    f32 dist = vec3_distance(curr->origin, viewerPosition);
    if (dist < range && !curr->isLoaded) {
      streaming_region_load(curr);
    } else if (dist > range * 1.5f && curr->isLoaded) {
      streaming_region_unload(curr);
    }
    curr = curr->next;
  }
}

void streaming_prefetch(Vec3 predictedPosition) {
  // Stub
}

void streaming_priority_update(Vec3 viewerPosition) {
  // Stub
}

void streaming_budget_manage(u32 maxMemoryMB) {
  // Stub
}

bool streaming_async_load(StreamingRegion *region) {
  return streaming_region_load(region);
}

// =================================================================================================
//                                    SCENE HIERARCHY
// =================================================================================================

void hierarchy_build(Scene *scene) {
  // Rebuild logic if transforms are dirty
  if (scene && scene->root) {
    hierarchy_update(scene->root);
  }
}

void hierarchy_update(HierarchyNode *node) {
  if (!node)
    return;

  // Update local transform if needed
  // Update world transform based on parent

  Mat4 local = hierarchy_local_transform(node);
  if (node->parent) {
    node->worldTransform = mat4_mul(node->parent->worldTransform, local);
  } else {
    node->worldTransform = local;
  }

  for (u32 i = 0; i < node->childCount; i++) {
    hierarchy_update(node->children[i]);
  }

  node->isDirty = false;
}

HierarchyNode *hierarchy_add_node(HierarchyNode *parent, const char *name) {
  HierarchyNode *node = (HierarchyNode *)malloc(sizeof(HierarchyNode));
  strncpy(node->name, name, 63);
  node->parent = parent;
  node->children = (HierarchyNode **)malloc(sizeof(HierarchyNode *) *
                                            INITIAL_CHILD_CAPACITY);
  node->childCount = 0;
  node->childCapacity = INITIAL_CHILD_CAPACITY;
  node->localPosition = vec3_zero();
  node->localRotation = vec3_zero();
  node->localScale = vec3_one();
  node->isDirty = true;
  node->entity = NULL;

  if (parent) {
    if (parent->childCount >= parent->childCapacity) {
      parent->childCapacity *= 2;
      parent->children = (HierarchyNode **)realloc(
          parent->children, sizeof(HierarchyNode *) * parent->childCapacity);
    }
    parent->children[parent->childCount++] = node;
  }

  hierarchy_update(node);
  return node;
}

void hierarchy_remove_node(HierarchyNode *node) {
  if (!node)
    return;

  // Remove children first
  for (u32 i = 0; i < node->childCount; i++) {
    hierarchy_remove_node(node->children[i]);
  }
  free(node->children);

  // Remove from parent
  if (node->parent) {
    HierarchyNode *p = node->parent;
    for (u32 i = 0; i < p->childCount; i++) {
      if (p->children[i] == node) {
        // Swap with last
        p->children[i] = p->children[--p->childCount];
        break;
      }
    }
  }

  free(node);
}

void hierarchy_reparent(HierarchyNode *node, HierarchyNode *newParent) {
  if (!node || !newParent)
    return;

  // Remove from old parent (without freeing)
  if (node->parent) {
    HierarchyNode *p = node->parent;
    for (u32 i = 0; i < p->childCount; i++) {
      if (p->children[i] == node) {
        p->children[i] = p->children[--p->childCount];
        break;
      }
    }
  }

  // Add to new parent
  if (newParent->childCount >= newParent->childCapacity) {
    newParent->childCapacity *= 2;
    newParent->children = (HierarchyNode **)realloc(
        newParent->children,
        sizeof(HierarchyNode *) * newParent->childCapacity);
  }
  newParent->children[newParent->childCount++] = node;
  node->parent = newParent;
  node->isDirty = true;
}

void hierarchy_traverse(HierarchyNode *root,
                        void (*callback)(HierarchyNode *)) {
  if (!root || !callback)
    return;
  callback(root);
  for (u32 i = 0; i < root->childCount; i++) {
    hierarchy_traverse(root->children[i], callback);
  }
}

HierarchyNode *hierarchy_find_by_path(HierarchyNode *root, const char *path) {
  // Stub: simplified search by name
  if (!root || !path)
    return NULL;
  if (strcmp(root->name, path) == 0)
    return root; // Should be recursive path check

  for (u32 i = 0; i < root->childCount; i++) {
    HierarchyNode *res = hierarchy_find_by_path(root->children[i], path);
    if (res)
      return res;
  }
  return NULL;
}

void hierarchy_dirty_propagate(HierarchyNode *node) {
  if (!node)
    return;
  node->isDirty = true;
  for (u32 i = 0; i < node->childCount; i++) {
    hierarchy_dirty_propagate(node->children[i]);
  }
}

Mat4 hierarchy_world_transform(HierarchyNode *node) {
  if (!node)
    return mat4_identity();
  if (node->isDirty)
    hierarchy_update(node);
  return node->worldTransform;
}

Mat4 hierarchy_local_transform(HierarchyNode *node) {
  if (!node)
    return mat4_identity();
  Mat4 S = mat4_scale(node->localScale);
  Mat4 R = mat4_rotate(vec3(1, 0, 0),
                       node->localRotation.x); // Simplified rotation combo
  // R = mat4_mul(R, mat4_rotate(vec3(0,1,0), node->localRotation.y));
  // R = mat4_mul(R, mat4_rotate(vec3(0,0,1), node->localRotation.z));
  Mat4 T = mat4_translate(node->localPosition);

  return mat4_mul(T, mat4_mul(R, S));
}

void hierarchy_serialize(HierarchyNode *node, FILE *file) {
  if (!node || !file)
    return;
  fwrite(node, sizeof(HierarchyNode) - sizeof(void *) * 2, 1, file); // Stub
  for (u32 i = 0; i < node->childCount; i++) {
    hierarchy_serialize(node->children[i], file);
  }
}

HierarchyNode *hierarchy_deserialize(FILE *file) {
  // Stub
  return NULL;
}

// =================================================================================================
//                                    PREFABS
// =================================================================================================

ScenePrefab *prefab_create(const char *name, HierarchyNode *sourceNode) {
  ScenePrefab *p = (ScenePrefab *)malloc(sizeof(ScenePrefab));
  strncpy(p->name, name, 63);
  p->rootNode = sourceNode; // Should copy
  return p;
}

void prefab_destroy(ScenePrefab *prefab) {
  if (prefab)
    free(prefab);
}

HierarchyNode *prefab_instantiate(ScenePrefab *prefab, HierarchyNode *parent) {
  if (!prefab || !parent)
    return NULL;
  return hierarchy_add_node(parent, prefab->name); // Stub: should verify copy
}

void prefab_apply(HierarchyNode *instance, ScenePrefab *prefab) {}
void prefab_revert(HierarchyNode *instance, ScenePrefab *prefab) {}
void prefab_break_link(HierarchyNode *instance) {}
void prefab_get_overrides(HierarchyNode *instance, void **overrides,
                          u32 *count) {}
HierarchyNode *prefab_nested_instantiate(ScenePrefab *parentScenePrefab,
                                         ScenePrefab *childScenePrefab) {
  return NULL;
}
ScenePrefab *prefab_variant_create(ScenePrefab *base, const char *variantName) {
  return NULL;
}
void prefab_serialize(ScenePrefab *prefab, const char *path) {}
ScenePrefab *prefab_deserialize(const char *path) { return NULL; }

// =================================================================================================
//                                    SCENE MANAGER
// =================================================================================================

bool scene_manager_init(SceneManager *manager) {
  if (!manager)
    return false;
  manager->sceneCapacity = MAX_SCENES;
  manager->scenes = (Scene **)malloc(sizeof(Scene *) * MAX_SCENES);
  manager->sceneCount = 0;
  manager->activeScene = NULL;
  manager->currentTransition = NULL;
  manager->isTransitioning = false;
  manager->streamingRegions = NULL;
  return true;
}

void scene_manager_shutdown(SceneManager *manager) {
  if (!manager)
    return;
  for (u32 i = 0; i < manager->sceneCount; i++) {
    scene_destroy(manager->scenes[i]);
  }
  free(manager->scenes);
}

void scene_manager_update(SceneManager *manager, f32 dt) {
  if (!manager)
    return;

  if (manager->isTransitioning && manager->currentTransition) {
    transition_update(manager->currentTransition, dt);
    if (transition_complete(manager->currentTransition)) {
      manager->isTransitioning = false;
      // Finish switch
      if (manager->nextScene) {
        if (manager->activeScene)
          scene_set_active(manager->activeScene, false);
        manager->activeScene = manager->nextScene;
        scene_set_active(manager->activeScene, true);
        manager->nextScene = NULL;
      }
    }
  }

  // Update active scene logic
}

bool scene_manager_load_scene(SceneManager *manager, const char *path) {
  Scene *s = scene_create("NewScene");
  scene_load(s, path);
  if (manager->sceneCount < manager->sceneCapacity) {
    manager->scenes[manager->sceneCount++] = s;
  }
  return true;
}

void scene_manager_unload_scene(SceneManager *manager, Scene *scene) {
  scene_unload(scene);
}

bool scene_manager_additive_load(SceneManager *manager, const char *path) {
  return scene_manager_load_scene(manager, path);
}

void scene_manager_transition(SceneManager *manager, const char *nextScenePath,
                              Transition *transition) {
  if (!manager)
    return;
  manager->currentTransition = transition;
  manager->isTransitioning = true;
  transition_start(transition);
  // Stub: load next scene
}

void scene_manager_reload_current(SceneManager *manager) {}
f32 scene_manager_get_loading_progress(SceneManager *manager) { return 1.0f; }
void scene_manager_on_scene_loaded(SceneManager *manager, Scene *scene) {}
void scene_manager_on_scene_unloaded(SceneManager *manager, Scene *scene) {}
