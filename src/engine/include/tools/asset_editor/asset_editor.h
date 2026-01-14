#ifndef ASSET_EDITOR_H
#define ASSET_EDITOR_H

#include <stdbool.h>
#include <stdint.h>
#include <common.h>
#include "math/vec3.h"
#include "math/vec2.h"
#include <include/rendering/camera.h>

#ifdef __cplusplus
extern "C" {
#endif

// Asset Editor State
typedef enum {
    EDITOR_STATE_INACTIVE,
    EDITOR_STATE_CREATE,
    EDITOR_STATE_EDIT,
    EDITOR_STATE_PREVIEW,
    EDITOR_STATE_LIBRARY
} EditorState;

// Asset Types
typedef enum {
    ASSET_TYPE_3D_MODEL,
    ASSET_TYPE_2D_SPRITE,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_PARTICLE,
    ASSET_TYPE_UI_ELEMENT,
    ASSET_TYPE_ANIMATION,
    ASSET_TYPE_SOUND
} AssetType;

// Editor Tools
typedef enum {
    TOOL_SELECT,
    TOOL_MOVE,
    TOOL_ROTATE,
    TOOL_SCALE,
    TOOL_EXTRUDE,
    TOOL_SCULPT,
    TOOL_PAINT,
    TOOL_MEASURE,
    TOOL_NONE
} EditorTool;

// Viewport Settings
typedef struct {
    Camera camera;
    vec3 position;
    vec3 rotation;
    float zoom;
    bool orthographic;
    bool wireframe;
    bool show_grid;
    bool show_gizmos;
} Viewport;

// Asset Editor Context
typedef struct {
    EditorState state;
    AssetType current_asset_type;
    EditorTool active_tool;
    Viewport viewport;
    
    // Asset Management
    char current_asset_path[256];
    bool asset_modified;
    bool auto_save;
    
    // UI State
    bool show_properties_panel;
    bool show_asset_browser;
    bool show_timeline;
    bool show_toolbar;
    
    // Input State
    vec2 mouse_position;
    vec2 mouse_delta;
    bool mouse_left_pressed;
    bool mouse_right_pressed;
    bool mouse_middle_pressed;
    
    // Keyboard Modifiers
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    
    // Grid Settings
    bool grid_visible;
    float grid_size;
    int grid_subdivisions;
    vec3 grid_color;
    
    // Gizmo Settings
    bool gizmo_visible;
    float gizmo_size;
    bool gizmo_local_space;
} AssetEditor;

// Editor Initialization
bool asset_editor_init(AssetEditor* editor);
void asset_editor_cleanup(AssetEditor* editor);
void asset_editor_update(AssetEditor* editor, float dt);
void asset_editor_render(AssetEditor* editor);

// State Management
void asset_editor_set_state(AssetEditor* editor, EditorState state);
void asset_editor_set_tool(AssetEditor* editor, EditorTool tool);
void asset_editor_set_asset_type(AssetEditor* editor, AssetType type);

// Asset Operations
bool asset_editor_new_asset(AssetEditor* editor, AssetType type, const char* name);
bool asset_editor_load_asset(AssetEditor* editor, const char* path);
bool asset_editor_save_asset(AssetEditor* editor, const char* path);
bool asset_editor_export_asset(AssetEditor* editor, const char* path, const char* format);

// Input Handling
void asset_editor_handle_mouse_input(AssetEditor* editor, int button, int action, int mods);
void asset_editor_handle_cursor_pos(AssetEditor* editor, double xpos, double ypos);
void asset_editor_handle_scroll(AssetEditor* editor, double xoffset, double yoffset);
void asset_editor_handle_key_input(AssetEditor* editor, int key, int scancode, int action, int mods);

// Viewport Operations
void asset_editor_reset_camera(AssetEditor* editor);
void asset_editor_frame_selected(AssetEditor* editor);
void asset_editor_set_viewport_mode(AssetEditor* editor, bool orthographic);

// Utility Functions
bool asset_editor_is_modified(const AssetEditor* editor);
const char* asset_editor_get_current_asset(const AssetEditor* editor);
void asset_editor_set_auto_save(AssetEditor* editor, bool enabled);

#ifdef __cplusplus
}
#endif

#endif // ASSET_EDITOR_H
