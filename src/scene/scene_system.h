/**
 * =================================================================================================
 *                              SCENE MANAGEMENT SYSTEM
 *                              Agent: AGENT_SCENE_1
 * =================================================================================================
 */

#ifndef SCENE_SYSTEM_H
#define SCENE_SYSTEM_H

#include "engine/include/common.h"
#include "engine/include/math/math.h"
#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct Scene Scene;
typedef struct SceneManager SceneManager;
typedef struct HierarchyNode HierarchyNode;
typedef struct ScenePrefab ScenePrefab;

// Scene ID type
typedef u32 SceneID;
typedef u32 NodeID;
typedef u32 ScenePrefabID;

/* =================================================================================================
 *                                    SCENE CORE
 * =================================================================================================
 */

typedef struct Scene {
  SceneID id;
  char name[64];
  bool isActive;
  bool isLoaded;

  // Root of the scene hierarchy
  HierarchyNode *root;

  // Limits
  u32 maxEntities;
  u32 currentEntities;

  // File path for serialization
  char filePath[256];
} Scene;

Scene *scene_create(const char *name);
void scene_destroy(Scene *scene);
bool scene_load(Scene *scene, const char *path);
bool scene_load_async(Scene *scene, const char *path,
                      void (*onComplete)(Scene *));
void scene_unload(Scene *scene);
bool scene_save(Scene *scene, const char *path);
void scene_set_active(Scene *scene, bool active);
bool scene_get_active(Scene *scene);
Scene *scene_get_by_name(SceneManager *manager, const char *name);
Scene **scene_get_all(SceneManager *manager, u32 *count);
Scene *scene_instantiate(Scene *templateScene);
bool scene_merge(Scene *target, Scene *source);
void scene_clear(Scene *scene);

/* =================================================================================================
 *                                    SCENE TRANSITIONS
 * =================================================================================================
 */

typedef enum {
  TRANSITION_NONE,
  TRANSITION_FADE,
  TRANSITION_DISSOLVE,
  TRANSITION_WIPE,
  TRANSITION_CUSTOM
} TransitionType;

typedef struct Transition {
  TransitionType type;
  f32 duration;
  f32 currentTime;
  bool isRunning;
  bool isComplete;

  // Custom data
  void *data;
  void (*update)(struct Transition *, f32);
  void (*render)(struct Transition *);
} Transition;

Transition *transition_fade_create(f32 duration, Vec3 color);
Transition *transition_dissolve_create(f32 duration, const char *patternPath);
Transition *transition_wipe_create(f32 duration, Vec3 direction);
Transition *transition_custom_create(f32 duration, void *data,
                                     void (*update)(Transition *, f32),
                                     void (*render)(Transition *));
void transition_start(Transition *transition);
void transition_update(Transition *transition, f32 dt);
void transition_render(Transition *transition);
bool transition_complete(Transition *transition);
void transition_cancel(Transition *transition);

/* =================================================================================================
 *                                    SCENE STREAMING
 * =================================================================================================
 */

typedef struct StreamingRegion {
  Vec3 origin;
  Vec3 extent;
  bool isLoaded;
  bool isVisible;
  u32 lodLevel;
  char regionPath[256];
  struct StreamingRegion *next; // Simple linked list for now
} StreamingRegion;

void streaming_region_define(Vec3 origin, Vec3 extent, const char *path);
bool streaming_region_load(StreamingRegion *region);
void streaming_region_unload(StreamingRegion *region);
void streaming_update_visibility(Vec3 viewerPosition, f32 range);
void streaming_prefetch(Vec3 predictedPosition);
void streaming_priority_update(Vec3 viewerPosition);
void streaming_budget_manage(u32 maxMemoryMB);
bool streaming_async_load(StreamingRegion *region);

/* =================================================================================================
 *                                    SCENE HIERARCHY
 * =================================================================================================
 */

struct HierarchyNode {
  NodeID id;
  char name[64];

  // Transform
  Vec3 localPosition;
  Vec3 localRotation; // Euler angles
  Vec3 localScale;
  Mat4 worldTransform;
  bool isDirty;

  // Hierarchy
  struct HierarchyNode *parent;
  struct HierarchyNode **children;
  u32 childCount;
  u32 childCapacity;

  // Entity/Data attachment (void* to generic Entity for now)
  void *entity;
};

void hierarchy_build(Scene *scene);
void hierarchy_update(HierarchyNode *node);
HierarchyNode *hierarchy_add_node(HierarchyNode *parent, const char *name);
void hierarchy_remove_node(HierarchyNode *node);
void hierarchy_reparent(HierarchyNode *node, HierarchyNode *newParent);
void hierarchy_traverse(HierarchyNode *root, void (*callback)(HierarchyNode *));
HierarchyNode *hierarchy_find_by_path(HierarchyNode *root, const char *path);
void hierarchy_dirty_propagate(HierarchyNode *node);
Mat4 hierarchy_world_transform(HierarchyNode *node);
Mat4 hierarchy_local_transform(HierarchyNode *node);
void hierarchy_serialize(HierarchyNode *node, FILE *file);
HierarchyNode *hierarchy_deserialize(FILE *file);

/* =================================================================================================
 *                                    PREFABS
 * =================================================================================================
 */

struct ScenePrefab {
  ScenePrefabID id;
  char name[64];
  HierarchyNode *rootNode; // The template hierarchy
  char sourcePath[256];
};

ScenePrefab *prefab_create(const char *name, HierarchyNode *sourceNode);
void prefab_destroy(ScenePrefab *prefab);
HierarchyNode *prefab_instantiate(ScenePrefab *prefab, HierarchyNode *parent);
void prefab_apply(HierarchyNode *instance, ScenePrefab *prefab);
void prefab_revert(HierarchyNode *instance, ScenePrefab *prefab);
void prefab_break_link(HierarchyNode *instance);
void prefab_get_overrides(HierarchyNode *instance, void **overrides,
                          u32 *count);
HierarchyNode *prefab_nested_instantiate(ScenePrefab *parentScenePrefab,
                                         ScenePrefab *childScenePrefab);
ScenePrefab *prefab_variant_create(ScenePrefab *base, const char *variantName);
void prefab_serialize(ScenePrefab *prefab, const char *path);
ScenePrefab *prefab_deserialize(const char *path);

/* =================================================================================================
 *                                    SCENE MANAGER
 * =================================================================================================
 */

struct SceneManager {
  Scene **scenes;
  u32 sceneCount;
  u32 sceneCapacity;

  Scene *activeScene;
  Scene *nextScene;

  Transition *currentTransition;
  bool isTransitioning;

  StreamingRegion *streamingRegions; // Linked list head
};

bool scene_manager_init(SceneManager *manager);
void scene_manager_shutdown(SceneManager *manager);
void scene_manager_update(SceneManager *manager, f32 dt);
bool scene_manager_load_scene(SceneManager *manager, const char *path);
void scene_manager_unload_scene(SceneManager *manager, Scene *scene);
bool scene_manager_additive_load(SceneManager *manager, const char *path);
void scene_manager_transition(SceneManager *manager, const char *nextScenePath,
                              Transition *transition);
void scene_manager_reload_current(SceneManager *manager);
f32 scene_manager_get_loading_progress(SceneManager *manager);
void scene_manager_on_scene_loaded(SceneManager *manager, Scene *scene);
void scene_manager_on_scene_unloaded(SceneManager *manager, Scene *scene);

#endif // SCENE_SYSTEM_H
