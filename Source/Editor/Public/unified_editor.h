#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct Editor Editor;
typedef struct World World;
typedef uint64_t EntityID;
typedef uint64_t AssetHandle;

// Editor mode
typedef enum {
  EDITOR_MODE_EDIT,
  EDITOR_MODE_PLAY,
  EDITOR_MODE_SIMULATE
} EditorMode;

// Transform (simplified)
typedef struct {
  float position[3];
  float rotation[4]; // quaternion
  float scale[3];
} EditorTransform;

// Editor system
Editor *editor_create(World *world);
void editor_update(Editor *editor, float delta_time);
void editor_destroy(Editor *editor);

// Mode control
void editor_set_mode(Editor *editor, EditorMode mode);
EditorMode editor_get_mode(Editor *editor);

// Entity manipulation
void editor_select_entity(Editor *editor, EntityID entity);
EntityID editor_get_selected_entity(Editor *editor);
void editor_delete_entity(Editor *editor, EntityID entity);
EntityID editor_create_entity(Editor *editor, const char *name);

// Transform editing
void editor_set_entity_transform(Editor *editor, EntityID entity,
                                 const EditorTransform *transform);
void editor_get_entity_transform(Editor *editor, EntityID entity,
                                 EditorTransform *out_transform);

// Asset browser
AssetHandle editor_browse_asset(Editor *editor, uint32_t asset_type_filter);

// Scene management
bool editor_save_scene(Editor *editor, const char *path);
bool editor_load_scene(Editor *editor, const char *path);
