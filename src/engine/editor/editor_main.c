#include <core/engine.h>
#include <core/logger.h>
#include <editor/editor_main.h>

static bool g_editor_active = false;

void editor_init(Engine *engine) {
    if (!engine) return;
    
    // Initialize editor subsystems
    // - Gizmo system
    // - Selection system
    // - Command history
    
    g_editor_active = true;
    LOG_INFO("Editor initialized");
}

void editor_shutdown(Engine *engine) {
    g_editor_active = false;
    LOG_INFO("Editor shutdown");
}

void editor_update(Engine *engine, f32 delta_time) {
    if (!g_editor_active) return;
    
    // Update gizmos, handle input overrides
}

void editor_render(Engine *engine) {
    if (!g_editor_active) return;
    
    // Render editor overlays (gizmos, grid, outlines)
}

// SwiftUI Bridge Hooks
void editor_set_mode(bool enabled) {
    g_editor_active = enabled;
}

void editor_on_selection_changed(void) {
    // Notify SwiftUI
}

void editor_on_object_transform_changed(void) {
    // Notify SwiftUI
}
