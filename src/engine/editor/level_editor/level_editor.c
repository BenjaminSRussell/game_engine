#include "level_editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock dependencies if real ones are not easily accessible/compilable yet
// In a real scenario, we would include headers from core, ecs, scene, etc.
// #include "core/logger.h"
// #include "ecs/ecs.h"
// #include "scene/scene_manager.h"

static bool g_LevelEditorActive = false;
static char g_CurrentLevelName[256] = "default_level";

void LevelEditor_Init(void) {
    printf("[LevelEditor] Initialized\n");
    g_LevelEditorActive = true;
}

void LevelEditor_Update(float delta_time) {
    if (!g_LevelEditorActive) return;

    // TODO: Handle input, render editor UI, interact with scene
    // printf("[LevelEditor] Update %.2f\n", delta_time);
}

void LevelEditor_Shutdown(void) {
    printf("[LevelEditor] Shutdown\n");
    g_LevelEditorActive = false;
}

void LevelEditor_LoadLevel(const char* level_name) {
    if (!level_name) return;
    strncpy(g_CurrentLevelName, level_name, sizeof(g_CurrentLevelName) - 1);
    printf("[LevelEditor] Loading level: %s\n", level_name);

    // Logic to load level from file
    // SceneManager_LoadScene(level_name);
}

void LevelEditor_SaveLevel(const char* level_name) {
    if (!level_name) level_name = g_CurrentLevelName;
    printf("[LevelEditor] Saving level: %s\n", level_name);

    // Logic to save entities to file
    // FILE* f = fopen(level_name, "w"); ...
}

bool LevelEditor_IsActive(void) {
    return g_LevelEditorActive;
}

void LevelEditor_ToggleActive(void) {
    g_LevelEditorActive = !g_LevelEditorActive;
    printf("[LevelEditor] Active: %s\n", g_LevelEditorActive ? "true" : "false");
}
