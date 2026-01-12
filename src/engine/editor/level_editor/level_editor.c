#include "level_editor.h"
#include <core/logger.h>
#include <core/input.h>
#include <scene/scene_system.h>
#include <editor/editor_main.h> // For g_editor if needed
#include <string.h>
#include <stdio.h>

static LevelEditorState g_level_editor = {0};

void LevelEditor_Init(Engine* engine) {
    if (g_level_editor.is_initialized) return;

    memset(&g_level_editor, 0, sizeof(LevelEditorState));
    g_level_editor.is_initialized = true;

    // Set default empty scene path
    strcpy(g_level_editor.current_scene_path, "Untitled");

    LOG_INFO("Level Editor initialized");
}

void LevelEditor_Shutdown(Engine* engine) {
    g_level_editor.is_initialized = false;
    LOG_INFO("Level Editor shutdown");
}

void LevelEditor_Update(Engine* engine, f32 delta_time) {
    if (!g_level_editor.is_initialized) return;

    // Handle shortcuts
    InputSystem* input = engine->subsystems.input;
    if (input) {
        // Ctrl+S for Save
        if (input->is_key_down(input, KEY_LCTRL) && input->is_key_pressed(input, KEY_S)) {
            if (strcmp(g_level_editor.current_scene_path, "Untitled") != 0) {
                LevelEditor_SaveScene(engine, g_level_editor.current_scene_path);
            } else {
                LOG_WARN("Cannot save 'Untitled' scene without a path. Use Save As.");
            }
        }

        // Ctrl+O for Open (Mockup)
        if (input->is_key_down(input, KEY_LCTRL) && input->is_key_pressed(input, KEY_O)) {
            LOG_INFO("Open Scene shortcut detected (UI should handle file dialog)");
        }

        // Delete
        if (input->is_key_pressed(input, KEY_DELETE)) {
            LevelEditor_DeleteSelected(engine);
        }
    }
}

void LevelEditor_Render(Engine* engine) {
    // Draw Level Editor UI overlays
    // e.g., File menu, Toolbar (if not drawn by editor_main), Scene Stats
}

void LevelEditor_NewScene(Engine* engine) {
    SceneManager* sm = engine->subsystems.scene_manager;
    if (!sm) return;

    // Create new scene logic here
    // For now, we might reload an empty scene or clear current
    // scene_create("New Scene");

    // Since SceneManager handles active scene, we assume it has a way to clear or set new.
    // scene_manager_load_scene(sm, "assets/scenes/empty_scene.json"); // Example

    strcpy(g_level_editor.current_scene_path, "Untitled");
    g_level_editor.is_scene_dirty = false;

    LOG_INFO("New Scene created");
}

bool LevelEditor_LoadScene(Engine* engine, const char* path) {
    SceneManager* sm = engine->subsystems.scene_manager;
    if (!sm) {
        LOG_ERROR("SceneManager not available");
        return false;
    }

    if (scene_manager_load_scene(sm, path)) {
        strncpy(g_level_editor.current_scene_path, path, sizeof(g_level_editor.current_scene_path) - 1);
        g_level_editor.is_scene_dirty = false;
        LOG_INFO("Scene loaded: %s", path);
        return true;
    }

    LOG_ERROR("Failed to load scene: %s", path);
    return false;
}

bool LevelEditor_SaveScene(Engine* engine, const char* path) {
    SceneManager* sm = engine->subsystems.scene_manager;
    if (!sm || !sm->activeScene) {
        LOG_ERROR("No active scene to save");
        return false;
    }

    if (scene_save(sm->activeScene, path)) {
        strncpy(g_level_editor.current_scene_path, path, sizeof(g_level_editor.current_scene_path) - 1);
        g_level_editor.is_scene_dirty = false;
        LOG_INFO("Scene saved: %s", path);
        return true;
    }

    LOG_ERROR("Failed to save scene: %s", path);
    return false;
}

bool LevelEditor_SaveSceneAs(Engine* engine, const char* path) {
    return LevelEditor_SaveScene(engine, path);
}

void LevelEditor_SpawnEntity(Engine* engine, const char* asset_path) {
    SceneManager* sm = engine->subsystems.scene_manager;
    if (!sm || !sm->activeScene) return;

    // Logic to spawn entity from asset
    // 1. Create HierarchyNode
    // 2. Load Asset
    // 3. Attach component

    HierarchyNode* node = hierarchy_add_node(sm->activeScene->root, "New Entity");
    if (node) {
        // Position in front of camera or at origin
        // node->localPosition = ...

        LOG_INFO("Spawned entity from %s", asset_path);
        g_level_editor.is_scene_dirty = true;
    }
}

void LevelEditor_DeleteSelected(Engine* engine) {
    // Need access to selection system
    // For now, mockup
    LOG_INFO("Delete selected entity requested");
    g_level_editor.is_scene_dirty = true;
}

bool LevelEditor_IsSceneDirty(void) {
    return g_level_editor.is_scene_dirty;
}

const char* LevelEditor_GetCurrentScenePath(void) {
    return g_level_editor.current_scene_path;
}
