#include <core/logger.h>
#include <core/unified_engine.h>
#include <editor/asset_browser.h>
#include <editor/editor_main.h>
#include <editor/tools/node_editor.h>
#include <editor/ui/canvas/ui_canvas.h>
#include <editor/level_editor/level_editor.h>

// Global editor state definition
EditorState g_editor = {0};

static bool g_editor_active = false;

void editor_init(Engine *engine) {
  if (!engine)
    return;

  LOG_INFO("Initializing Editor...");

  // Initialize editor subsystems
  AssetBrowser_Init(engine->vfs, engine->assets);
  ui_rendering_ui_canvas_init();
  NodeEditor_Init();

  // Initialize Level Editor
  LevelEditor_Init(engine);

  // Initialize Editor State Systems (from editor_enhancements.c)
  viewport_system_init(&g_editor.viewport_system);
  undo_redo_system_init(&g_editor.undo_redo);
  play_in_editor_init(&g_editor.play_system);
  transform_gizmo_init(&g_editor.transform_gizmo);
  selection_outline_init(&g_editor.selection_outline);
  performance_profiler_init(&g_editor.performance_profiler);
  editor_memory_profiler_init(&g_editor.memory_profiler);
  layout_system_init(&g_editor.layout_system);

  g_editor.is_initialized = true;
  g_editor_active = true;

  LOG_INFO("Editor initialized");
}

void editor_shutdown(Engine *engine) {
  g_editor_active = false;
  g_editor.is_initialized = false;

  LevelEditor_Shutdown(engine);
  NodeEditor_Shutdown();
  ui_rendering_ui_canvas_shutdown();

  // Shutdown Editor State Systems
  // (Assuming shutdown functions exist matching init)
  viewport_system_shutdown(&g_editor.viewport_system);
  undo_redo_system_shutdown(&g_editor.undo_redo);
  play_in_editor_shutdown(&g_editor.play_system);
  transform_gizmo_shutdown(&g_editor.transform_gizmo);
  selection_outline_shutdown(&g_editor.selection_outline);
  performance_profiler_shutdown(&g_editor.performance_profiler);
  editor_memory_profiler_shutdown(&g_editor.memory_profiler);
  layout_system_shutdown(&g_editor.layout_system);

  LOG_INFO("Editor shutdown");
}

void editor_update(Engine *engine, f32 delta_time) {
  if (!g_editor_active)
    return;

  // Update subsystems
  AssetBrowser_Update(delta_time);
  NodeEditor_Update(delta_time);
  LevelEditor_Update(engine, delta_time);

  // Update Editor State Systems
  viewport_system_update(&g_editor.viewport_system, delta_time);
  play_in_editor_update(&g_editor.play_system, delta_time);

  // Update gizmo (needs target position from selection)
  // For now, pass zero vector or current selection pos
  Vec3 target_pos = {0, 0, 0};
  // TODO: Get actual selection position
  transform_gizmo_update(&g_editor.transform_gizmo, &target_pos);

  selection_outline_update(&g_editor.selection_outline);
  editor_memory_profiler_update(&g_editor.memory_profiler);
}

void editor_render(Engine *engine) {
  if (!g_editor_active)
    return;

  // Render editor overlays (gizmos, grid, outlines)
  Editor_DrawAssetBrowser(); // Draws IMGUI or console output
  NodeEditor_Render();
  LevelEditor_Render(engine);

  // Render Editor State Systems
  viewport_system_render(&g_editor.viewport_system);

  if (engine->subsystems.renderer) {
      transform_gizmo_render(&g_editor.transform_gizmo, engine->subsystems.renderer);
      selection_outline_render(&g_editor.selection_outline, engine->subsystems.renderer);
  }

  // Performance/Memory profilers usually draw to IMGUI windows
  // performance_profiler_render_window(&g_editor.performance_profiler);
  // editor_memory_profiler_render_window(&g_editor.memory_profiler);
}

// SwiftUI Bridge Hooks
void editor_set_mode(bool enabled) { g_editor_active = enabled; }

void editor_on_selection_changed(void) {
  // Notify SwiftUI
}

void editor_on_object_transform_changed(void) {
  // Notify SwiftUI
}
