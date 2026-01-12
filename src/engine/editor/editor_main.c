#include <core/logger.h>
#include <core/unified_engine.h>
#include <editor/asset_browser.h>
#include <editor/editor_main.h>
#include <editor/tools/node_editor.h>
#include <editor/ui/canvas/ui_canvas.h>

static bool g_editor_active = false;

void editor_init(Engine *engine) {
  if (!engine)
    return;

  // Initialize editor subsystems
  // - Gizmo system
  // - Selection system
  // - Command history
  AssetBrowser_Init(engine->vfs, engine->assets);
  ui_rendering_ui_canvas_init();
  NodeEditor_Init();

  g_editor_active = true;
  LOG_INFO("Editor initialized");
}

void editor_shutdown(Engine *engine) {
  NodeEditor_Shutdown();
  ui_rendering_ui_canvas_shutdown();
  g_editor_active = false;
  LOG_INFO("Editor shutdown");
}

void editor_update(Engine *engine, f32 delta_time) {
  if (!g_editor_active)
    return;

  AssetBrowser_Update(delta_time);
  NodeEditor_Update(delta_time);
  // Update gizmos, handle input overrides
}

void editor_render(Engine *engine) {
  if (!g_editor_active)
    return;

  // Render editor overlays (gizmos, grid, outlines)
  Editor_DrawAssetBrowser();
  NodeEditor_Render();
}

// SwiftUI Bridge Hooks
void editor_set_mode(bool enabled) { g_editor_active = enabled; }

void editor_on_selection_changed(void) {
  // Notify SwiftUI
}

void editor_on_object_transform_changed(void) {
  // Notify SwiftUI
}
