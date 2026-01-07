#ifndef SCENE_H
#define SCENE_H

#include "include/common.h"
#include "include/ecs/ecs.h"
#include <math/mat4.h>
#include <math/vec3.h>

// Forward declarations
struct EngineCore;
struct Camera;

// Scene node types
typedef enum {
  SCENE_NODE_ROOT,
  SCENE_NODE_ENTITY,
  SCENE_NODE_SPRITE,
  SCENE_NODE_MESH,
  SCENE_NODE_LIGHT,
  SCENE_NODE_CAMERA,
  SCENE_NODE_GROUP
} SceneNodeType;

// Scene node (for hierarchical scene graph)
typedef struct SceneNode {
  struct SceneNode *parent;
  struct SceneNode *first_child;
  struct SceneNode *next_sibling;

  SceneNodeType type;
  Entity entity; // If this node represents an entity

  // Transform
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
  Mat4 transform_matrix;
  bool transform_dirty;

  // Rendering
  u32 layer; // For 2.5D depth sorting
  bool visible;
  bool enabled;

  // User data
  void *user_data;
} SceneNode;

// GameObject - high-level abstraction for game objects
typedef struct {
  Entity entity;
  SceneNode *node;
  const char *name;
  bool active;
  void *user_data;
} GameObject;

// Scene structure
typedef struct Scene {
  const char *name;
  struct EngineCore *engine;

  // Scene graph
  SceneNode *root_node;
  u32 node_count;
  u32 node_capacity;

  // Game objects
  GameObject *game_objects;
  u32 game_object_count;
  u32 game_object_capacity;

  // Layers (for 2.5D depth sorting)
  u32 layer_count;
  f32 *layer_depths; // Depth values for each layer

  // Systems
  bool use_physics;
  bool use_audio;

  // State
  bool initialized;
  bool active;
} Scene;

// Scene lifecycle
bool scene_init(Scene *scene, struct EngineCore *engine, const char *name);
void scene_shutdown(Scene *scene);
void scene_update(Scene *scene, f32 delta_time);
void scene_render(Scene *scene);

// Scene activation
void scene_set_active(Scene *scene, bool active);
bool scene_is_active(Scene *scene);

// Scene node management
SceneNode *scene_create_node(Scene *scene, SceneNodeType type,
                             SceneNode *parent);
void scene_destroy_node(Scene *scene, SceneNode *node);
void scene_update_transforms(Scene *scene);

// GameObject management
GameObject *scene_create_game_object(Scene *scene, const char *name);
void scene_destroy_game_object(Scene *scene, GameObject *obj);
GameObject *scene_find_game_object(Scene *scene, const char *name);
GameObject *scene_get_game_object(Scene *scene, Entity entity);

// Layer management (for 2.5D)
void scene_set_layer_count(Scene *scene, u32 layer_count);
void scene_set_layer_depth(Scene *scene, u32 layer, f32 depth);
f32 scene_get_layer_depth(Scene *scene, u32 layer);
u32 scene_get_layer_count(Scene *scene);

// Entity integration
Entity scene_create_entity(Scene *scene);
void scene_destroy_entity(Scene *scene, Entity entity);
SceneNode *scene_get_entity_node(Scene *scene, Entity entity);

// Transform helpers
void scene_node_set_position(SceneNode *node, Vec3 position);
void scene_node_set_rotation(SceneNode *node, Vec3 rotation);
void scene_node_set_scale(SceneNode *node, Vec3 scale);
Vec3 scene_node_get_position(SceneNode *node);
Vec3 scene_node_get_rotation(SceneNode *node);
Vec3 scene_node_get_scale(SceneNode *node);
Mat4 scene_node_get_world_transform(SceneNode *node);

// Camera management
void scene_set_camera(Scene *scene, struct Camera *camera);
struct Camera *scene_get_camera(Scene *scene);

#endif // SCENE_H
