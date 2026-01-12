#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include <core/types.h>
#include <core/engine.h>

typedef struct {
    bool is_initialized;
    char current_scene_path[256];
    bool is_scene_dirty;

    // Selection state (could mirror or reference global selection)
    u32 selected_entity_id;

    // Placement state
    bool is_placing_entity;
    char placement_asset_path[256];

} LevelEditorState;

void LevelEditor_Init(Engine* engine);
void LevelEditor_Shutdown(Engine* engine);
void LevelEditor_Update(Engine* engine, f32 delta_time);
void LevelEditor_Render(Engine* engine);

// Scene Actions
void LevelEditor_NewScene(Engine* engine);
bool LevelEditor_LoadScene(Engine* engine, const char* path);
bool LevelEditor_SaveScene(Engine* engine, const char* path);
bool LevelEditor_SaveSceneAs(Engine* engine, const char* path);

// Editing Actions
void LevelEditor_SpawnEntity(Engine* engine, const char* asset_path);
void LevelEditor_DeleteSelected(Engine* engine);

// Accessors
bool LevelEditor_IsSceneDirty(void);
const char* LevelEditor_GetCurrentScenePath(void);

#endif // LEVEL_EDITOR_H
